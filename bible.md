# KODA Project Bible

## 2026-08-28 — B0 bootstrap

Established the public KODA repository skeleton and documented the accepted direction from the KODA task brief. The governing invariant is: **NVMe is acceleration, never authority.** B0 is complete; R0 remains not started. No raw-device, microVM, cache, LIO, iSCSI, benchmark, host service, GUI, or installer implementation was added.

## 2026-08-28 — R0 spike scaffolding

Started R0 architecture-feasibility execution on branch `r0/architecture-feasibility`. Added an isolated C++20 host safety/evidence seam, a minimal guest raw-I/O helper, Azure/Windows bootstrap scripts, and initial R0 environment documentation. Azure catalog access timed out during read-only preflight; no VM, managed data disk, or destructive storage test was created. R0 remains open pending Windows WHP and disposable-device evidence.
