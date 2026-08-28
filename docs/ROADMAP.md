# KODA Roadmap to V1

KODA is currently at **B0 — Repository Bootstrap: COMPLETE**. **R0 — Architecture Feasibility: NOT STARTED**.

## B0 — Repository Bootstrap — COMPLETE

Establish the public repository, accepted design and roadmap documentation, project hygiene, and a source-tree skeleton. No storage implementation is included.

## R0 — Architecture Feasibility — NOT STARTED

Determine whether a Windows 11 host can safely and repeatedly relinquish an ordinary physical disk or partition to a WHP/libkrun Linux microVM and reclaim it without simultaneous ownership or NTFS corruption. This is the next milestone.

## R1 — Return Path and Performance Ceiling

Establish the return path and measure the ceiling imposed by the host, microVM, block path, and iSCSI transport.

## R2 — KODA Cache Proof

Demonstrate the intended cache behavior and the NVMe-acceleration/backing-disk-authority invariant under controlled, disposable test conditions.

## R3 — Safety Core

Build the ownership, transition, recovery, and fail-closed safety core required for trustworthy operation.

## R4 — Stateless Appliance

Shape the Linux appliance as a narrowly scoped, reproducible storage appliance rather than a general-purpose server environment.

## R5 — V1 Host Product

Develop the Windows host integration needed for a usable V1 product, including orchestration and recovery tooling.

## R6 — V1 Qualification

Qualify the complete system across supported hardware, transitions, failure cases, recovery, and performance criteria before any V1 readiness claim.

