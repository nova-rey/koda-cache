#!/bin/sh
# Locate the virtio disk by stable serial; never assume /dev/vdX order.
set -eu
serial_required=${KODA_R0_SERIAL:-KODA-R0-DISK}
device=''
for sysdev in /sys/block/*; do
  [ -f "$sysdev/device/serial" ] || continue
  serial=$(cat "$sysdev/device/serial")
  if [ "$serial" = "$serial_required" ]; then
    [ -z "$device" ] || { echo 'duplicate KODA-R0-DISK serial' >&2; exit 2; }
    device="/dev/$(basename "$sysdev")"
  fi
done
[ -n "$device" ] || { echo "guest disk serial not found: $serial_required" >&2; exit 3; }
if awk -v d="$device" '$2 == d { found = 1 } END { exit found ? 0 : 1 }' /proc/mounts; then
  echo 'refusing to use a mounted guest block device' >&2; exit 4
fi
: "${KODA_R0_OFFSET:?KODA_R0_OFFSET is required}"
: "${KODA_R0_LENGTH:?KODA_R0_LENGTH is required}"
exec /usr/local/bin/koda-r0-io-helper --device "$device" --offset "$KODA_R0_OFFSET" --length "$KODA_R0_LENGTH"
