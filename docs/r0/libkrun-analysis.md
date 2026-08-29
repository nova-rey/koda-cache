# libkrun analysis (source inspection)

Pinned snapshots examined:

- smolvm v1.13.1 (`62f5f04bcb4fc097cca55cffa5772a32093f4636`)
- bundled libkrun (`2dfbd6fb6a1acf50413b64009b90fed8c589c9c3`)
- bundled libkrunfw (`8f1307644bdcb95fe1343bf0d6ccf30aa22f1549`)
- official libkrun development source (`0d75eb4b9d7f742e9b290b7372e4be491e68b173`)

Windows uses the WHP backend. `krun_add_disk2` and `krun_add_disk3` accept path
strings; block construction opens the path twice and imago obtains Windows
capacity through ordinary metadata. `KRUN_SYNC_FULL` exposes virtio FLUSH and
routes it to imago `sync_all()`. No upstream raw-device, offline/online,
exclusive-reopen, or repeated NTFS handoff test was found.

The bounded patch tree remains: B1 Windows raw length via
`IOCTL_DISK_GET_LENGTH_INFO`; B2 HANDLE-backed path only if path reopening cannot
prove ownership; B3 device-reported alignment if 4096-byte assumptions fail.
No patch was applied because no Windows raw-device execution occurred.
