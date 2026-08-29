# KODA Project Bible

## 2026-08-28 — B0 bootstrap

Established the public KODA repository skeleton and documented the accepted direction from the KODA task brief. The governing invariant is: **NVMe is acceleration, never authority.** B0 is complete; R0 remains not started. No raw-device, microVM, cache, LIO, iSCSI, benchmark, host service, GUI, or installer implementation was added.

## 2026-08-28 — R0 spike scaffolding

Started R0 architecture-feasibility execution on branch `r0/architecture-feasibility`. Added an isolated C++20 host safety/evidence seam, a minimal guest raw-I/O helper, Azure/Windows bootstrap scripts, and initial R0 environment documentation. Azure catalog access timed out during read-only preflight; no VM, managed data disk, or destructive storage test was created. R0 remains open pending Windows WHP and disposable-device evidence.

## 2026-08-28 — R0 ownership-sequence correction

The Windows ownership seam now records `FlushFileBuffers` explicitly and verifies
that the volume is not mounted only after the required lock-and-dismount sequence.
This remains a compile-guarded feasibility spike; no Windows or destructive
storage operation has been run from this checkout.

## 2026-08-28 — R0 fail-closed reclaim gate

Reclaim now requires an exclusive reopen of the allowlisted raw namespace before
bringing the volume online, and it does not claim remount or NTFS validation
success without separate evidence. This is untested on Windows pending the WHP
gate; no storage was touched.

## 2026-08-28 — R0 integrity cadence

Added the read-only integrity cadence and explicit PASS/FAIL/INCONCLUSIVE
classification required for the 100-cycle handoff streak. No repair operation,
Windows device, or Azure resource was used.

## 2026-08-28 — R0 WHP capability probe

Added a read-only PowerShell WHP capability/partition/vCPU probe for the Azure
gate. It creates no VM or disk and records HRESULTs plus an explicit gate result;
PowerShell execution remains pending on a Windows host.

## 2026-08-29 — R0 Azure IPv4 recovery

IPv4-specific diagnostics isolated the Azure failure to the CLI's IPv6 path:
IPv4 ARM/TLS and authenticated REST calls now work. The subscription catalog
allows Standard_D2s_v5 in westcentralus and exposes Windows 11 24H2 Pro images.
Added an in-process IPv4 Azure CLI wrapper and sanitized network/catalog evidence;
no resources or disks were created.

## 2026-08-29 — R0 Azure gate result

IPv4-forced ARM access enabled subscription catalog verification and creation of
one temporary Standard_D2s_v5 Windows VM without a data disk. Azure Run Command
remained stuck across restart/deallocate-start recovery, so WHP could not be
probed. The temporary resource-group deletion was submitted; R0 is
`R0_REQUIRES_BARE_METAL`, not a VMM rejection.

## 2026-08-29 — R0 Azure cleanup confirmed

The disposable Azure resource group eventually reported `exists=false`; no
secondary data disk had been attached. The R0 result and sanitized cleanup
evidence are committed and the feature branch is published for review.
