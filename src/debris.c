#include "prg32.h"

#define BOARD_W 10
#define BOARD_H 20
#define CELL 8
#define BOARD_X 84
#define BOARD_Y 20
#define PREVIEW_X 204
#define HOLD_X 28
#define FALL_FRAMES_BASE 34

typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} game_state_t;

typedef struct {
    int initialized;
    uint8_t board[BOARD_H][BOARD_W];
    int piece;
    int rot;
    int x;
    int y;
    int hold;
    int hold_used;
    int queue[3];
    uint32_t rng;
    uint32_t score;
    uint16_t lines;
    uint8_t level;
    uint8_t state;
    uint8_t audio_ready;
    uint8_t clear_flash;
    uint8_t drop_timer;
    uint8_t lock_delay;
    uint32_t frame;
    uint32_t last_input;
} debris_t;

static debris_t g;
static int saved_fullscreen;
static prg32_band_mode_t saved_top;
static prg32_band_mode_t saved_bottom;

static const uint16_t colors[8] = {
    PRG32_COLOR_BLACK,
    PRG32_COLOR_CYAN,
    PRG32_COLOR_BLUE,
    PRG32_COLOR_YELLOW,
    PRG32_COLOR_GREEN,
    PRG32_COLOR_MAGENTA,
    PRG32_COLOR_RED,
    PRG32_COLOR_WHITE,
};

static const int8_t shape[7][4][4][2] = {
    {{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}},
    {{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}},
    {{{2,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,1},{0,2}},{{0,0},{1,0},{1,1},{1,2}}},
    {{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}}},
    {{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}},
    {{{1,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{0,1},{1,1},{1,2}}},
    {{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}},
};

static const uint8_t thump_sample[] = {
    128, 210, 250, 238, 190, 134, 86, 54, 42, 50, 72, 100, 126, 145, 154, 154,
    148, 138, 128, 121, 117, 118, 121, 125, 128, 128, 128, 128, 128, 128, 128, 128,
};

static const uint8_t crackle_sample[] = {
    128, 240, 24, 210, 70, 186, 52, 225, 36, 172, 91, 235, 64, 154, 104, 202,
    84, 170, 98, 190, 112, 160, 122, 145, 128, 138, 126, 132, 128, 128, 128, 128,
};

static void append_char(char *dst, int cap, int *pos, char ch) {
    if (*pos < cap - 1) {
        dst[*pos] = ch;
        *pos += 1;
        dst[*pos] = '\0';
    }
}

static void append_text(char *dst, int cap, int *pos, const char *src) {
    while (src && *src) append_char(dst, cap, pos, *src++);
}

static void append_uint(char *dst, int cap, int *pos, uint32_t value, int width) {
    char tmp[12];
    int count = 0;
    do {
        tmp[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value && count < (int)sizeof(tmp));
    while (count < width) tmp[count++] = '0';
    while (count > 0) append_char(dst, cap, pos, tmp[--count]);
}

static void make_line(char *dst, int cap, const char *label, uint32_t value) {
    int pos = 0;
    dst[0] = '\0';
    append_text(dst, cap, &pos, label);
    append_uint(dst, cap, &pos, value, 1);
}

static uint32_t rnd(void) {
    g.rng = g.rng * 1664525u + 1013904223u;
    return g.rng;
}

static void sound_beep(int freq, int ms) {
    if (!g.audio_ready) return;
    prg32_audio_beep(freq, ms);
}

static void sound_sample(const uint8_t *sample, int len, int rate) {
    if (!g.audio_ready) return;
    prg32_audio_sample_u8(sample, (uint32_t)len, rate);
}

static int fits(int piece, int rot, int x, int y) {
    for (int i = 0; i < 4; ++i) {
        int px = x + shape[piece][rot][i][0];
        int py = y + shape[piece][rot][i][1];
        if (px < 0 || px >= BOARD_W || py >= BOARD_H) return 0;
        if (py >= 0 && g.board[py][px]) return 0;
    }
    return 1;
}

static int next_piece(void) {
    return (int)(rnd() % 7u);
}

static void spawn(void) {
    g.piece = g.queue[0];
    g.queue[0] = g.queue[1];
    g.queue[1] = g.queue[2];
    g.queue[2] = next_piece();
    g.rot = 0;
    g.x = 3;
    g.y = -2;
    g.hold_used = 0;
    g.drop_timer = 0;
    g.lock_delay = 0;
    if (!fits(g.piece, g.rot, g.x, g.y)) {
        g.state = STATE_GAME_OVER;
        sound_sample(crackle_sample, sizeof(crackle_sample), 11025);
    }
}

static void reset_game(void) {
    for (int y = 0; y < BOARD_H; ++y) {
        for (int x = 0; x < BOARD_W; ++x) g.board[y][x] = 0;
    }
    g.rng ^= prg32_ticks_ms() + 0x6d2b79f5u;
    g.score = 0;
    g.lines = 0;
    g.level = 1;
    g.hold = -1;
    g.hold_used = 0;
    g.clear_flash = 0;
    g.queue[0] = next_piece();
    g.queue[1] = next_piece();
    g.queue[2] = next_piece();
    g.state = STATE_PLAYING;
    spawn();
}

static void add_score(int cleared, int hard_cells) {
    static const uint16_t line_score[5] = {0, 100, 300, 500, 800};
    if (cleared > 0) {
        g.score += (uint32_t)line_score[cleared] * (uint32_t)g.level;
        g.lines = (uint16_t)(g.lines + cleared);
        g.level = (uint8_t)(1 + g.lines / 8);
        if (g.level > 12) g.level = 12;
    }
    g.score += (uint32_t)hard_cells * 2u;
}

static void clear_lines(void) {
    int cleared = 0;
    for (int y = BOARD_H - 1; y >= 0; --y) {
        int full = 1;
        for (int x = 0; x < BOARD_W; ++x) {
            if (!g.board[y][x]) {
                full = 0;
                break;
            }
        }
        if (!full) continue;
        for (int yy = y; yy > 0; --yy) {
            for (int x = 0; x < BOARD_W; ++x) g.board[yy][x] = g.board[yy - 1][x];
        }
        for (int x = 0; x < BOARD_W; ++x) g.board[0][x] = 0;
        ++cleared;
        ++y;
    }
    if (cleared > 0) {
        add_score(cleared, 0);
        g.clear_flash = 8;
        sound_sample(crackle_sample, sizeof(crackle_sample), 15000);
        sound_beep(520 + cleared * 120, 65);
    }
}

static void lock_piece(void) {
    for (int i = 0; i < 4; ++i) {
        int px = g.x + shape[g.piece][g.rot][i][0];
        int py = g.y + shape[g.piece][g.rot][i][1];
        if (py >= 0 && py < BOARD_H && px >= 0 && px < BOARD_W) {
            g.board[py][px] = (uint8_t)(g.piece + 1);
        }
    }
    sound_sample(thump_sample, sizeof(thump_sample), 9000);
    clear_lines();
    spawn();
}

static int move_piece(int dx, int dy) {
    if (fits(g.piece, g.rot, g.x + dx, g.y + dy)) {
        g.x += dx;
        g.y += dy;
        if (dx) sound_beep(180, 12);
        return 1;
    }
    return 0;
}

static void rotate_piece(int dir) {
    int next = (g.rot + dir + 4) & 3;
    static const int8_t kicks[] = {0, -1, 1, -2, 2};
    for (int i = 0; i < 5; ++i) {
        if (fits(g.piece, next, g.x + kicks[i], g.y)) {
            g.rot = next;
            g.x += kicks[i];
            sound_beep(330, 22);
            return;
        }
    }
}

static void hold_piece(void) {
    if (g.hold_used) return;
    int old = g.piece;
    if (g.hold < 0) {
        g.hold = old;
        spawn();
    } else {
        g.piece = g.hold;
        g.hold = old;
        g.rot = 0;
        g.x = 3;
        g.y = -2;
        if (!fits(g.piece, g.rot, g.x, g.y)) g.state = STATE_GAME_OVER;
    }
    g.hold_used = 1;
    sound_beep(660, 35);
}

static void hard_drop(void) {
    int cells = 0;
    while (fits(g.piece, g.rot, g.x, g.y + 1)) {
        ++g.y;
        ++cells;
    }
    add_score(0, cells);
    lock_piece();
}

static int fall_frames(void) {
    int speed = FALL_FRAMES_BASE - (int)(g.level - 1) * 2;
    return speed < 6 ? 6 : speed;
}

static void update_play(uint32_t input, uint32_t pressed) {
    if (pressed & PRG32_BTN_LEFT) move_piece(-1, 0);
    if (pressed & PRG32_BTN_RIGHT) move_piece(1, 0);
    if (pressed & PRG32_BTN_A) rotate_piece(1);
    if (pressed & PRG32_BTN_UP) hard_drop();
    if (g.state != STATE_PLAYING) return;
    if (pressed & PRG32_BTN_B) hold_piece();
    if (pressed & PRG32_BTN_SELECT) {
        g.state = STATE_PAUSED;
        sound_beep(220, 60);
        return;
    }
    uint8_t limit = (input & PRG32_BTN_DOWN) ? 3 : (uint8_t)fall_frames();
    if (++g.drop_timer >= limit) {
        g.drop_timer = 0;
        if (!move_piece(0, 1)) {
            if (++g.lock_delay >= 3) lock_piece();
        } else {
            g.lock_delay = 0;
            if (input & PRG32_BTN_DOWN) g.score++;
        }
    }
    if (g.clear_flash) --g.clear_flash;
}

static void draw_block(int x, int y, uint16_t color, int small) {
    int size = small ? 6 : CELL;
    prg32_gfx_rect(x, y, size - 1, size - 1, color);
    prg32_gfx_rect(x + 1, y + 1, size - 3, 1, PRG32_COLOR_WHITE);
    prg32_gfx_pixel(x + size - 2, y + size - 2, PRG32_COLOR_BLACK);
}

static void draw_piece(int piece, int rot, int x, int y, int cell) {
    if (piece < 0) return;
    for (int i = 0; i < 4; ++i) {
        int px = x + shape[piece][rot][i][0] * cell;
        int py = y + shape[piece][rot][i][1] * cell;
        draw_block(px, py, colors[piece + 1], cell < CELL);
    }
}

static void draw_panel_text(int x, int y, const char *text, uint16_t fg) {
    prg32_gfx_text8(x, y, text, fg, PRG32_COLOR_BLACK);
}

static void draw_stats(void) {
    char line[32];
    draw_panel_text(8, 8, "DEBRIS", PRG32_COLOR_YELLOW);
    make_line(line, sizeof(line), "SCORE ", g.score);
    draw_panel_text(188, 16, line, PRG32_COLOR_WHITE);
    make_line(line, sizeof(line), "LEVEL ", g.level);
    draw_panel_text(188, 32, line, PRG32_COLOR_CYAN);
    make_line(line, sizeof(line), "LINES ", g.lines);
    draw_panel_text(188, 48, line, PRG32_COLOR_GREEN);
    draw_panel_text(HOLD_X, 42, "HOLD", PRG32_COLOR_CYAN);
    draw_piece(g.hold, 0, HOLD_X, 58, 6);
    draw_panel_text(PREVIEW_X, 76, "NEXT", PRG32_COLOR_CYAN);
    for (int n = 0; n < 3; ++n) draw_piece(g.queue[n], 0, PREVIEW_X, 92 + n * 30, 6);
}

static void draw_board(void) {
    uint16_t frame_color = g.clear_flash ? PRG32_COLOR_WHITE : PRG32_COLOR_BLUE;
    prg32_gfx_rect(BOARD_X - 4, BOARD_Y - 4, BOARD_W * CELL + 8, BOARD_H * CELL + 8, frame_color);
    prg32_gfx_rect(BOARD_X, BOARD_Y, BOARD_W * CELL, BOARD_H * CELL, PRG32_COLOR_BLACK);
    for (int y = 0; y < BOARD_H; ++y) {
        for (int x = 0; x < BOARD_W; ++x) {
            if (((x + y) & 1) == 0) {
                prg32_gfx_pixel(BOARD_X + x * CELL, BOARD_Y + y * CELL, 0x2104);
            }
            if (g.board[y][x]) draw_block(BOARD_X + x * CELL, BOARD_Y + y * CELL, colors[g.board[y][x]], 0);
        }
    }
    for (int i = 0; i < 4; ++i) {
        int px = g.x + shape[g.piece][g.rot][i][0];
        int py = g.y + shape[g.piece][g.rot][i][1];
        if (py >= 0) draw_block(BOARD_X + px * CELL, BOARD_Y + py * CELL, colors[g.piece + 1], 0);
    }
}

static void draw_game(void) {
    prg32_gfx_clear(PRG32_COLOR_BLACK);
    for (int i = 0; i < 26; ++i) {
        int x = (int)((g.frame * 2u + (uint32_t)i * 37u) % 320u);
        int y = 190 - (int)((g.frame + (uint32_t)i * 11u) % 90u);
        prg32_gfx_pixel(x, y, (i & 1) ? PRG32_COLOR_BLUE : PRG32_COLOR_MAGENTA);
    }
    draw_board();
    draw_stats();
    if (g.state == STATE_PAUSED) {
        prg32_gfx_rect(72, 84, 176, 34, 0x2104);
        draw_panel_text(112, 94, "PAUSED", PRG32_COLOR_YELLOW);
        draw_panel_text(88, 108, "SELECT TO RESUME", PRG32_COLOR_WHITE);
    }
}

static void draw_title(void) {
    prg32_gfx_clear(PRG32_COLOR_BLACK);
    prg32_gfx_text8(72, 42, "DEBRIS", PRG32_COLOR_YELLOW, PRG32_COLOR_BLACK);
    prg32_gfx_text8(48, 66, "A MULTI LEVEL BLOCK GAME", PRG32_COLOR_CYAN, PRG32_COLOR_BLACK);
    for (int i = 0; i < 7; ++i) {
        draw_piece(i, (g.frame / 18u + (uint32_t)i) & 3u, 46 + i * 30, 104 + (i & 1) * 10, 6);
    }
    prg32_gfx_text8(56, 154, "SELECT STARTS", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    prg32_gfx_text8(24, 174, "LEFT/RIGHT MOVE  A ROTATE", PRG32_COLOR_GREEN, PRG32_COLOR_BLACK);
    prg32_gfx_text8(24, 188, "UP DROP  DOWN SOFT  B HOLD", PRG32_COLOR_GREEN, PRG32_COLOR_BLACK);
}

static void draw_game_over(void) {
    draw_game();
    prg32_gfx_rect(52, 72, 216, 58, 0x2104);
    prg32_gfx_text8(96, 82, "GAME OVER", PRG32_COLOR_RED, 0x2104);
    char line[32];
    make_line(line, sizeof(line), "FINAL ", g.score);
    prg32_gfx_text8(88, 102, line, PRG32_COLOR_WHITE, 0x2104);
    prg32_gfx_text8(72, 118, "SELECT RESTARTS", PRG32_COLOR_YELLOW, 0x2104);
}

void debris_init(void) {
    saved_fullscreen = prg32_gfx_fullscreen_enabled();
    saved_top = prg32_band_mode(PRG32_BAND_TOP);
    saved_bottom = prg32_band_mode(PRG32_BAND_BOTTOM);
    prg32_gfx_set_fullscreen(0);
    prg32_band_set_mode(PRG32_BAND_TOP, PRG32_BAND_MODE_FPS);
    prg32_band_set_mode(PRG32_BAND_BOTTOM, PRG32_BAND_MODE_CUSTOM);
    prg32_band_set_game_info("DEBRIS");
    g.initialized = 1;
    g.rng = 0xdeb815u ^ prg32_ticks_ms();
    g.state = STATE_TITLE;
    g.audio_ready = prg32_audio_init(NULL) ? 1 : 0;
    if (g.audio_ready) prg32_audio_set_master_volume(160);
    prg32_input_wait_released(PRG32_BTN_A | PRG32_BTN_B | PRG32_BTN_SELECT);
}

void debris_update(void) {
    uint32_t input = prg32_input_read_menu();
    uint32_t pressed = input & ~g.last_input;
    g.frame++;
    if (g.state == STATE_TITLE) {
        if (pressed & (PRG32_BTN_SELECT | PRG32_BTN_A | PRG32_BTN_B)) {
            reset_game();
            sound_beep(440, 80);
        }
    } else if (g.state == STATE_PLAYING) {
        update_play(input, pressed);
    } else if (g.state == STATE_PAUSED) {
        if (pressed & PRG32_BTN_SELECT) {
            g.state = STATE_PLAYING;
            sound_beep(330, 50);
        }
    } else if (g.state == STATE_GAME_OVER) {
        if (pressed & (PRG32_BTN_SELECT | PRG32_BTN_A | PRG32_BTN_B)) {
            reset_game();
            sound_beep(520, 80);
        }
    }
    g.last_input = input;
}

void debris_draw(void) {
    if (g.state == STATE_TITLE) {
        draw_title();
    } else if (g.state == STATE_GAME_OVER) {
        draw_game_over();
    } else {
        draw_game();
    }
    char band[64];
    int pos = 0;
    band[0] = '\0';
    append_text(band, sizeof(band), &pos, "SEL PAUSE/START  A ROT  B HOLD  LV ");
    append_uint(band, sizeof(band), &pos, g.level ? g.level : 1, 1);
    prg32_band_set_text(PRG32_BAND_BOTTOM, band);
}

void debris_shutdown(void) {
    prg32_band_set_game_info("");
    prg32_band_set_mode(PRG32_BAND_TOP, saved_top);
    prg32_band_set_mode(PRG32_BAND_BOTTOM, saved_bottom);
    if (g.audio_ready) prg32_audio_stop_all();
    prg32_gfx_set_fullscreen(saved_fullscreen);
}
