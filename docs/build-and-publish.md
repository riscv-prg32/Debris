# Build and Publish Debris

Debris uses the DeviceDemo cartridge template structure with the `debris`
entry prefix and a single source file at `src/debris.c`.

## Build ESP32-C6

```sh
export PRG32_REPO=/path/to/PRG32
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh
```

The script writes `dist/debris-esp32c6.prg32`.

## Build QEMU

```sh
export PRG32_REPO=/path/to/PRG32
export PRG32_ARCHITECTURE=qemu
scripts/build.sh
```

The script writes `dist/debris-qemu.prg32`.

## Upload

```sh
python3 "$PRG32_REPO/tools/prg32_game.py" upload \
  dist/debris-esp32c6.prg32 \
  --url http://192.168.4.1
```

Use the board IP shown in PRG32 setup mode when the device is joined to a
network.

## Publish Bundle

Build both architectures, then run:

```sh
scripts/pack-store-bundle.sh
```

The bundle is `dist/debris-store-bundle.zip`.
