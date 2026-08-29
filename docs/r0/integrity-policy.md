# R0 integrity cadence

Every completed ownership cycle emits a machine-readable validation record. The
validator must confirm the expected volume identity, a normal NTFS mount, all
baseline SHA-256 manifests, and `chkdsk <drive>: /scan` with no newly reported
errors. Validation is read-only: no `/f`, `/r`, repair, or dirty-bit clearing is
permitted.

Run validation at these points:

1. immediately after cycle 1;
2. after every 10 cycles (or sooner, never later than 20);
3. after cycle 100;
4. after each failure or abnormal termination; and
5. after reboot/recovery before declaring R0 complete.

Each check is classified as:

- **PASS** — identity, hashes, mount, and non-repairing NTFS scan all agree;
- **FAIL** — any mismatch, scan error, unexpected filesystem change, or
  uncertain ownership state;
- **INCONCLUSIVE** — infrastructure stopped before handoff and the device was
  positively still Windows-owned. INCONCLUSIVE never counts toward the 100-cycle
  consecutive successful handoff streak.

The harness must stop on FAIL or uncertainty. It must never auto-repair NTFS or
remount while a VMM process or descendant may still hold the device.
