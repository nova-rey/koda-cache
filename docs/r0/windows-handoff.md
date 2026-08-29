# Windows handoff analysis

The intended sequence is `FlushFileBuffers` → `FSCTL_LOCK_VOLUME` →
`FSCTL_DISMOUNT_VOLUME` → `IOCTL_VOLUME_OFFLINE`, retaining the locked volume
handle while the VMM owns only the positively allowlisted raw namespace.
`FSCTL_LOCK_VOLUME` is the filesystem ownership proof; dismount alone is not.
Before `IOCTL_VOLUME_ONLINE`, the harness must observe guest completion and an
exclusive raw reopen after the VMM job/process tree is gone. Remount, identity,
hash, and read-only `chkdsk /scan` validation are separate postconditions.

This sequence was source-reviewed and represented in the C++20 seam, but was not
executed because Azure guest command execution remained unavailable and no bare-
metal Windows host was present.
