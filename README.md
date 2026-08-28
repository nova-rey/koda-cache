# KODA

**Kernel-Offloaded Disk Acceleration**

KODA is an architecture-feasibility project for accelerating inexpensive, high-capacity Windows storage with a smaller NVMe cache while keeping Windows bare metal. A tiny Linux microVM is intended to act only as a storage appliance; it is not a general-purpose server environment.

> **Core invariant: NVMe is acceleration, never authority.**
>
> The ordinary backing NTFS disk remains authoritative, independently meaningful, and recoverable without KODA. Any design that makes the cache the sole source of truth is out of scope.

## High-level architecture

```text
Windows application
        ↓
NTFS / C:\Games
        ↓
Windows iSCSI initiator
        ↓
Linux microVM
        ↓
dm-cache (writethrough)
      ↙         ↘
 NVMe cache    HDD origin
```

The intended direct path remains available:

```text
Windows
   ↓
ordinary NTFS backing disk
```

## Status

**Pre-prototype / architecture feasibility.** KODA does not currently accelerate disks. B0 establishes the public repository; R0 is the next engineering milestone and has not started.

## Planned prototype direction

- Windows 11 x86-64
- Windows Hypervisor Platform
- smolvm/libkrun candidate runtime (subject to R0 feasibility)
- Alpine Linux appliance
- Linux `dm-cache` in writethrough mode
- Linux LIO
- Windows native iSCSI initiator

The stack is a candidate direction, not a claim that these components have been integrated or validated.

## Roadmap

B0 — Repository Bootstrap is complete. The planned milestones are R0 — Architecture Feasibility, R1 — Return Path and Performance Ceiling, R2 — KODA Cache Proof, R3 — Safety Core, R4 — Stateless Appliance, R5 — V1 Host Product, and R6 — V1 Qualification. See [the roadmap](docs/ROADMAP.md).

The detailed architecture is in [DESIGN.md](docs/DESIGN.md).

## Developer warning

> **KODA is pre-production storage software. Do not test raw-device functionality against irreplaceable data or a system disk.**

## License

KODA is released under the [Apache License 2.0](LICENSE).

