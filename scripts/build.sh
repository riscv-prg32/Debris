#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prg32_repo="${PRG32_REPO:-"$repo_dir/../PRG32"}"
firmware_elf="${1:-"${PRG32_FIRMWARE_ELF:-"$prg32_repo/build/PRG32.elf"}"}"
architecture="${PRG32_ARCHITECTURE:-esp32c6}"
portable="${PRG32_PORTABLE:-1}"
build_args=()
if [[ "$portable" == "1" || "$portable" == "true" || "$portable" == "yes" ]]; then
  build_args+=(--portable)
else
  build_args+=(--firmware-elf "$firmware_elf" --legacy-absolute-imports)
fi

mkdir -p "$repo_dir/dist"

(cd "$prg32_repo" && python3 -m prg32 cartridge build \
  "$repo_dir/src/debris.c" \
  --entry-prefix debris \
  --name debris \
  --architecture "$architecture" \
  --out "$repo_dir/dist/debris-$architecture.raw.prg32" \
  "${build_args[@]}")

(cd "$prg32_repo" && python3 -m prg32 store attach-metadata \
  "$repo_dir/dist/debris-$architecture.raw.prg32" \
  --out "$repo_dir/dist/debris-$architecture.prg32" \
  --metadata "$repo_dir/metadata/metadata.json" \
  --icon "$repo_dir/assets/icon.png" \
  --screenshot "$repo_dir/assets/screenshot.png" \
  --colophon "$repo_dir/metadata/colophon.json" \
  --architecture "$architecture")

echo "$repo_dir/dist/debris-$architecture.prg32"
