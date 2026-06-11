#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prg32_repo="${PRG32_REPO:-"$repo_dir/../PRG32"}"
firmware_elf="${1:-"${PRG32_FIRMWARE_ELF:-"$prg32_repo/build/PRG32.elf"}"}"
architecture="${PRG32_ARCHITECTURE:-esp32c6}"
portable="${PRG32_PORTABLE:-1}"
game_tool="$prg32_repo/tools/prg32_game.py"

build_args=()
if [[ "$portable" == "1" || "$portable" == "true" || "$portable" == "yes" ]]; then
  if ! python3 "$game_tool" build --help | grep -q -- '--portable'; then
    cat >&2 <<EOF
error: $game_tool does not support portable cartridge builds.

Update PRG32 to a branch/release with the portable ABI-table tooling, for example:
  git -C "$prg32_repo" fetch origin dev-relocation-1
  git -C "$prg32_repo" checkout dev-relocation-1

Or build the legacy firmware-specific format with:
  PRG32_PORTABLE=0 scripts/build.sh "$firmware_elf"
EOF
    exit 2
  fi
  build_args+=(--portable)
else
  build_args+=(--firmware-elf "$firmware_elf" --legacy-absolute-imports)
fi

mkdir -p "$repo_dir/dist"

python3 "$game_tool" build \
  "$repo_dir/src/debris.c" \
  --entry-prefix debris \
  --name debris \
  --out "$repo_dir/dist/debris-$architecture.raw.prg32" \
  "${build_args[@]}"

python3 "$game_tool" attach-metadata \
  "$repo_dir/dist/debris-$architecture.raw.prg32" \
  --out "$repo_dir/dist/debris-$architecture.prg32" \
  --metadata "$repo_dir/metadata/metadata.json" \
  --icon "$repo_dir/assets/icon.png" \
  --screenshot "$repo_dir/assets/screenshot.png" \
  --colophon "$repo_dir/metadata/colophon.json" \
  --architecture "$architecture"

echo "$repo_dir/dist/debris-$architecture.prg32"
