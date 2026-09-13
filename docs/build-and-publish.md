# Build and Publish Debris

Debris uses the DeviceDemo cartridge template structure with the `debris`
entry prefix and a single source file at `src/debris.c`.
Builds target the portable ABI on current PRG32 `main`. Its short sound effects
use the default portable audio instrument and timed notes.

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
PYTHONPATH="$PRG32_REPO" python3 -m prg32 esp32c6 upload \
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
