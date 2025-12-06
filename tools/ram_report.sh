#!/usr/bin/env bash
set -euo pipefail

ELF_PATH=${1:-build/.elf}
NM_BIN=${NM_BIN:-arm-none-eabi-nm}
OBJDUMP_BIN=${OBJDUMP_BIN:-arm-none-eabi-objdump}

if ! command -v "$NM_BIN" >/dev/null 2>&1; then
  if command -v nm >/dev/null 2>&1; then
    NM_BIN=nm
  else
    echo "Error: nm tool not found" >&2
    exit 1
  fi
fi

if ! command -v "$OBJDUMP_BIN" >/dev/null 2>&1; then
  if command -v objdump >/dev/null 2>&1; then
    OBJDUMP_BIN=objdump
  else
    echo "Error: objdump tool not found" >&2
    exit 1
  fi
fi

if [ ! -f "$ELF_PATH" ]; then
  echo "Error: ELF file '$ELF_PATH' not found" >&2
  exit 1
fi

RAM_SECTIONS='^\\.(bss|data|ram_d2|nocache|eth|ram[0-9]|ram[0-9]_init)$'

echo "Using nm: $NM_BIN" >&2
echo "Using objdump: $OBJDUMP_BIN" >&2
echo "ELF: $ELF_PATH" >&2

echo "--- Top 30 RAM symbols (size, section, name) ---" >&2
$OBJDUMP_BIN -t "$ELF_PATH" |
  awk -v regex="$RAM_SECTIONS" '
    $4 ~ regex && $5 ~ /^[0-9A-Fa-f]+$/ {
      size = strtonum("0x" $5);
      if (size > 0) {
        printf "%10d %-12s %s\n", size, $4, $6;
      }
    }
  ' | sort -nr | head -n 30
