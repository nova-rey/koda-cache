# KODA Design Specification

## Status and scope

KODA (Kernel-Offloaded Disk Acceleration) is at the pre-prototype, architecture-feasibility stage. This document records the B0 repository bootstrap basis and the accepted architectural direction supplied for the project. It is a design source, not evidence that R0 has been completed.

## Defining invariant

**NVMe is acceleration, never authority.**

The backing disk remains the authoritative storage medium. It must remain independently meaningful and recoverable without KODA. The cache must not become the only copy of user data, and a failure or removal of KODA must not redefine storage ownership.

## Intended architecture

Windows remains the bare-metal host and uses its native iSCSI initiator to reach a small Linux microVM. The microVM is a narrowly scoped storage appliance. Inside it, Linux `dm-cache` is the candidate caching layer, configured for writethrough behavior, with NVMe as the cache device and an ordinary disk or partition as the origin.

```text
Windows application → NTFS volume → Windows iSCSI initiator
                                      ↓
                         Linux microVM / storage appliance
                                      ↓
                              dm-cache (writethrough)
                                ↙                 ↘
                         NVMe cache          HDD origin
```

The direct mode is a first-class safety and recovery concept: Windows can use the ordinary NTFS backing disk without the microVM or cache path.

## Candidate components

The current prototype direction is Windows 11 x86-64, Windows Hypervisor Platform, a smolvm/libkrun candidate runtime, an Alpine Linux appliance, Linux `dm-cache`, Linux LIO, and the Windows native iSCSI initiator. The runtime choice remains explicitly subject to R0 feasibility; no component is treated as validated by this repository bootstrap.

## Safety boundaries

Storage ownership transitions, raw-device access, cache assembly, iSCSI export, recovery, and return to direct mode must be designed around exclusive ownership and data-integrity preservation. Fail-open behavior must not be weakened for benchmark gains. Destructive testing requires disposable media and must never default to a real or system disk.

## Non-goals for B0

B0 does not open raw disks, relinquish or reclaim Windows devices, build an Alpine appliance, configure LIO or `dm-cache`, benchmark iSCSI, patch libkrun, or implement a Windows service, GUI, or installer.

