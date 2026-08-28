# KODA R0 — Architecture Feasibility Execution Plan

## 1. Current-state findings

### VERIFIED

- KODA is a clean B0-only repository on `main` at commit `71de59f112966e40984aee1b07aca12ca29b33b3`, matching `origin/main`. It has one open issue, [R0: Architecture feasibility](https://github.com/nova-rey/koda-cache/issues/1), no milestone, no tags, and no implementation language.
- `host/`, `guest/`, `tools/`, `tests/`, and `scripts/` are placeholders. R0 tooling belongs under `tools/`, not in the future production host service.
- Every KODA commit must append an entry to `bible.md`; existing entries are immutable.
- Official libkrun stable `v1.19.4` does not contain the Windows WHP backend. Windows support is currently in the `2.0.0-dev` line inspected at [`0d75eb4`](https://github.com/libkrun/libkrun/commit/0d75eb4b9d7f742e9b290b7372e4be491e68b173).
- Stock [smolvm v1.13.1](https://github.com/smol-machines/smolvm/releases/tag/v1.13.1) ships Windows x86-64 binaries and bundles:
  - smolvm tag commit `62f5f04bcb4fc097cca55cffa5772a32093f4636`
  - smol-machines/libkrun `2dfbd6fb6a1acf50413b64009b90fed8c589c9c3`
  - smol-machines/libkrunfw `8f1307644bdcb95fe1343bf0d6ccf30aa22f1549`
- Windows libkrun uses WHP: `src/whp/src/lib.rs` calls `WHvGetCapability`, then creates and configures a WHP partition.
- The block APIs are `krun_add_disk`, `krun_add_disk2`, and `krun_add_disk3`. All accept a path string; `krun_add_disk3` adds raw format, direct-I/O, and sync-mode control.
- The path reaches `Block::new()` in `src/devices/src/virtio/block/device.rs`, which opens it using Rust `OpenOptions` and imago 0.2.3. The path is opened twice during construction.
- `KRUN_SYNC_FULL` advertises `VIRTIO_BLK_F_FLUSH`; guest flush requests reach imago `flush()` and `sync()`, ultimately using Windows `sync_all()`. No separate FUA contract was found.
- imago’s Windows backend uses ordinary metadata length and has no `IOCTL_DISK_GET_LENGTH_INFO` fallback. It also assumes 4096-byte direct-I/O alignment. Raw DASD capacity and alignment are therefore not source-proven.
- `FSCTL_LOCK_VOLUME` succeeds only when no files remain open, flushes cached filesystem data, and keeps the volume locked until its handle closes. This is positive evidence of relinquished filesystem ownership. [Microsoft FSCTL_LOCK_VOLUME](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_lock_volume)
- `FSCTL_DISMOUNT_VOLUME` alone is insufficient because subsequent access can remount the volume. [Microsoft FSCTL_DISMOUNT_VOLUME](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_dismount_volume)
- `IOCTL_VOLUME_OFFLINE` blocks volume reads and writes while still permitting physical-disk-handle I/O. It is the preferred R0 host-filesystem exclusion mechanism. Whole-disk offline is a separate attribute and is not required for the main proof.
- Azure documents `Standard_D2s_v4` as 2 vCPU, 8 GiB, x86-64, nested-virtualization capable. `Standard_D2s_v5` is the ordered fallback. [Azure Dsv4](https://learn.microsoft.com/en-us/azure/virtual-machines/sizes/general-purpose/dsv4-series), [Azure Dsv5](https://learn.microsoft.com/en-us/azure/virtual-machines/sizes/general-purpose/dsv5-series)
- Azure nested virtualization requires `SecurityType=Standard`. Microsoft does not support arbitrary non-Hyper-V virtualization applications inside a Hyper-V VM, so nested capability does not prove libkrun/WHP viability. [Nested virtualization](https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/user-guide/nested-virtualization), [configuration requirements](https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/enable-nested-virtualization)

### LIKELY

- Current path-based libkrun can pass `\\.\PhysicalDriveN` through `CreateFileW`, but raw capacity will probably report zero or fail until imago gains Windows raw-device length discovery.
- The smallest probable patch is imago/libkrun backend work, not a new public KODA architecture.
- A C++20 R0 controller is the smallest reliable host harness: it uses canonical Windows SDK structures and the libkrun C ABI directly, while remaining explicitly disposable spike code.
- `IOCTL_VOLUME_OFFLINE`, a retained volume-lock handle, and a VMM process contained in a kill-on-close Job Object form the cleanest user-mode ownership barrier.

### UNKNOWN — empirical gates

- Whether WHP can create and run a libkrun guest inside the selected Azure VM.
- Current subscription-specific Windows 11 image, region, quota, and SKU availability. The CLI account cache is present, but live catalog calls timed out during planning and must be refreshed.
- Whether stock imago reports the correct `PhysicalDrive` capacity on the selected Windows build.
- Whether libkrun’s two path opens remain valid while the NTFS volume is locked, dismounted, and offline.
- Whether default share mode is sufficient for the filesystem-ownership proof or a HANDLE-backed/exclusive-open patch is required.
- Whether raw-device `sync_all()` succeeds and provides the expected Windows flush behavior.
- Whether the selected Azure disk reports 512e or 4K alignment and whether imago’s bounce path handles it correctly.

## 2. Recommended R0 architecture

- Begin in Azure only for R0-A. Use Windows 11 x64 Gen2, `Standard_D2s_v4`, `SecurityType=Standard`, Standard SSD OS storage, and no data disk or inbound RDP during the WHP gate.
- Select the first live-compatible region in this fixed order: `eastus`, `eastus2`, `centralus`. Use `Standard_D2s_v5` only if D2s_v4 is restricted or unavailable.
- Resolve `MicrosoftWindowsDesktop:windows-11:win11-24h2-pro:latest`, then `win11-23h2-pro`, to an exact image version before provisioning. Record the exact URN.
- After `AZURE_WHP_PASS`, attach one 32-GiB `StandardSSD_LRS` managed disk at LUN 0 with host caching `None`.
- Test whole-disk passthrough first using `\\.\PhysicalDriveN`. Partition-device passthrough is attempted only if whole-disk failure is specifically namespace-related.
- Use stock smolvm v1.13.1 only for the Azure WHP boot gate.
- Use a dedicated `koda-r0.exe` controller/worker harness for raw-device work:
  - Controller owns identity checks, volume lock/offline state, recovery, validation, and JSONL evidence.
  - A child `vmm-worker` loads `krun.dll`, attaches the raw path, and runs inside a Job Object with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`.
- Use the rootfs bundled with smolvm v1.13.1, augmented at run time with a small statically linked guest helper. Do not build a production appliance.
- Do not mount NTFS in Linux. The guest accesses only an explicitly reserved unallocated raw test region.
- The 32-GiB GPT disk contains:
  - one 30-GiB NTFS partition labeled exactly `KODA-R0-DISPOSABLE`;
  - one derived 64-MiB raw-test region after the partition, aligned to 1 MiB;
  - untouched space before the backup GPT structures.
- For each cycle, allocate one 128-KiB record:
  - 64-KiB Windows challenge;
  - 64-KiB guest response;
  - 100 cycles consume 12.5 MiB of the reserved region.

## 3. Public spike interfaces

`koda-r0.exe` shall expose:

- `inventory --json <path>`: read-only device inventory.
- `enroll --volume <volume-guid> --azure-lun 0 --output <allowlist> --confirm KODA-R0-DISPOSABLE`: create the composite device allowlist.
- `baseline --allowlist <path> --manifest <path>`: create corpus hashes and NTFS baseline.
- `image-proof`: prove the harness using an ordinary raw image file before touching a device.
- `device-probe --read-only`: test raw open, length, geometry, guest-visible capacity, and flush without writes.
- `cycle --cycles N`: execute normal handoff cycles.
- `failure-test <startup|abort|terminate>`: invoke only the named bounded failure scenario.
- `recover --allowlist <path>`: idempotently reclaim only the exact allowlisted offline volume.
- `vmm-worker`: private child entry point; never called manually.

The allowlist schema `koda.r0.device-allowlist.v1` shall contain the volume GUID and serial, GPT disk GUID, storage DUID/device identifiers, model/serial/bus, exact length, Azure LUN when applicable, expected partition layout, label, reserved-region bounds, and fingerprints of the OS disk that must never match.

Evidence uses `koda.r0.event.v1` JSONL with `run_id`, sequence, UTC timestamp, state, result, Windows build, runtime provenance, redacted device fingerprint, operation, Win32/HRESULT, guest report, and artifact hashes.

## 4. Execution phases

### R0-A — Azure WHP viability gate

1. Refresh Azure authentication and perform live account, quota, region, SKU, image, and Windows-client licensing checks.
2. Provision only the temporary Windows VM.
3. Capture Windows build, CPU, optional-feature, boot configuration, Azure VM profile, and security type.
4. Enable `HypervisorPlatform`, set `hypervisorlaunchtype=auto`, and reboot.
5. Run a minimal WHP probe covering:
   - `WHvCapabilityCodeHypervisorPresent`;
   - `WHvCreatePartition`;
   - `WHvSetupPartition`;
   - `WHvCreateVirtualProcessor`.
6. Download and checksum smolvm v1.13.1. Boot an unmodified Alpine workload to a deterministic userspace marker.
7. Deallocate/start the VM and repeat the stock boot once.
8. Emit exactly `AZURE_WHP_PASS` or `AZURE_WHP_BLOCKED`.
9. Do not attach the managed data disk until the pass result exists.

`AZURE_WHP_PASS` requires both boots, including the post-deallocation boot, and all WHP API calls to succeed.

### R0-B — Raw block backend proof

1. Build the direct harness and guest helper.
2. Prove `krun_add_disk3(RAW, direct_io=false, KRUN_SYNC_FULL)` using a disposable regular raw image.
3. Attach the managed disk only now; initialize the exact LUN-0 disk, create the prescribed layout, and enroll its stable identity.
4. Run a read-only `PhysicalDrive` probe:
   - correct host capacity;
   - correct guest capacity;
   - logical/physical sector sizes;
   - guest reads of GPT and reserved-region challenge;
   - explicit guest `fsync`;
   - clean worker release.
5. If stock support fails, take only the applicable bounded libkrun branch below.
6. Permit raw writes only after capacity, bounds, alignment, identity, and flush checks pass.

### R0-C — Windows ownership harness

Implement the exact transition:

`WINDOWS_READY → WINDOWS_FLUSHED → WINDOWS_LOCKED → WINDOWS_DISMOUNTED → WINDOWS_VOLUME_OFFLINE → HOST_FILESYSTEM_INACCESSIBLE → GUEST_ATTACHED`

Steps:

1. Re-resolve the allowlisted identity and current `PhysicalDriveN`.
2. Reject any identity/layout mismatch.
3. Open the volume GUID DASD handle with `GENERIC_READ|GENERIC_WRITE`, `FILE_SHARE_READ|FILE_SHARE_WRITE`, `OPEN_EXISTING`.
4. Snapshot all mount paths.
5. Call `FlushFileBuffers`.
6. Call `FSCTL_LOCK_VOLUME`; require success.
7. Require `FSCTL_IS_VOLUME_MOUNTED == false`.
8. Call `FSCTL_DISMOUNT_VOLUME`.
9. Call `IOCTL_VOLUME_OFFLINE`.
10. Keep the locking handle open for the complete guest lifetime.
11. Prove independent volume/filesystem I/O fails with `ERROR_NOT_READY`.
12. Spawn the VMM worker in the kill-on-close Job Object.

Return sequence:

`GUEST_IO_COMPLETE → GUEST_FLUSHED → GUEST_RELEASED → WINDOWS_REOPEN → WINDOWS_VOLUME_ONLINE → WINDOWS_MOUNTED → NTFS_VERIFIED`

1. Require the guest helper’s successful readback, `fsync`, close, and completion report.
2. Wait until the VMM Job Object contains zero processes.
3. Re-resolve identity and successfully open the physical disk exclusively.
4. Close the raw probe.
5. Send `IOCTL_VOLUME_ONLINE`.
6. Unlock/close the retained volume handle.
7. Reopen the known volume and require it to mount.
8. Require mount paths to equal the snapshot; restore with `SetVolumeMountPointW` only if Windows lost them.
9. Run full per-cycle integrity validation.

Do not remove drive letters during the normal path. Offline state supplies exclusion without creating persistent Mount Manager mutations.

### R0-D — Guest raw-I/O round trip

The guest helper shall:

- locate the disk by virtio block serial `KODA-R0-DISK`, never by `/dev/vdX` order;
- verify capacity and logical block size;
- prove the NTFS partition is not mounted;
- read and hash the cycle challenge;
- write only the cycle’s precomputed response slot;
- read the response back;
- call `fsync` on the block-device handle;
- close it;
- emit one JSON completion record.

The host then verifies the guest response directly from the raw region. NTFS corpus content must remain byte-for-byte unchanged.

### R0-E — Repetition and bounded failures

1. Complete one normal proof cycle.
2. Freeze the tested binaries, DLL provenance, device layout, and corpus.
3. Complete 100 consecutive normal cycles. Any failed or inconclusive normal cycle resets the consecutive count after correction.
4. Execute each required failure test once against a clean baseline.
5. Reboot Windows after a clean return and rerun identity, corpus, dirty-bit, and `chkdsk` validation.

### R0-F — Evidence and closure

- Sanitize and commit reproducible evidence.
- Record whether libkrun was unmodified or patched and pin all source commits and binary hashes.
- Update `docs/ROADMAP.md` and issue #1 only after a terminal R0 result exists.
- Append the result to `bible.md`.
- Delete the Azure resource group after evidence export.
- Publish exactly one R0 status from Section 13.

## 5. Files to create or modify

### Documentation

- `docs/r0/README.md` — R0 index, boundaries, reproduction order, and final status.
- `docs/r0/environment.md` — Windows/Azure or physical-host inventory and pinned versions.
- `docs/r0/libkrun-analysis.md` — traced APIs, commits, functions, raw-device findings, and patch outcome.
- `docs/r0/windows-handoff.md` — documented ownership sequence, recovery rules, and evidence interpretation.
- `docs/r0/results.md` — cycle/failure totals, integrity results, limitations, and terminal decision.
- `docs/r0/evidence/README.md` — evidence schema, redaction rules, and reproduction guidance.
- `docs/ROADMAP.md` — update only during closure.
- `README.md` — update status only during closure.
- `bible.md` — append one entry in every commit.

### Host and guest spike

- `tools/r0-host/CMakeLists.txt` — isolated C++20/CTest build.
- `tools/r0-host/include/koda_r0/*.hpp` — modular identity, ownership, libkrun, evidence, and recovery interfaces.
- `tools/r0-host/src/main.cpp` — CLI dispatch only.
- `tools/r0-host/src/device_identity.cpp` — composite identity and safety rejection.
- `tools/r0-host/src/volume_ownership.cpp` — flush/lock/dismount/offline/online operations.
- `tools/r0-host/src/libkrun_worker.cpp` — pinned DLL loading and direct libkrun invocation.
- `tools/r0-host/src/evidence.cpp` — JSONL state/evidence writer.
- `tools/r0-host/src/recovery.cpp` — fail-closed, idempotent reclaim.
- `tools/r0-host/tests/*.cpp` — non-destructive identity, state, bounds, serialization, failpoint, and recovery tests.
- `guest/r0/io-helper.c` — static raw-region read/write/readback/fsync helper.
- `guest/r0/guest-io.sh` — serial-based discovery and helper orchestration.
- `tests/r0/device-allowlist.example.json` — nonfunctional schema example.
- `tests/r0/corpus-spec.json` — deterministic corpus sizes, paths, and seed.
- `scripts/r0/azure.sh` — preflight, provision, attach, deallocate, and delete commands.
- `scripts/r0/windows-bootstrap.ps1` — environment capture, WHP enablement, and gate execution.
- `scripts/r0/prepare-test-disk.ps1` — LUN-bound disk initialization and exact sacrificial layout.

### Sanitized closure evidence

- `docs/r0/evidence/azure-whp.json`
- `docs/r0/evidence/runtime-provenance.json`
- `docs/r0/evidence/device-redacted.json`
- `docs/r0/evidence/cycles.jsonl`
- `docs/r0/evidence/integrity.jsonl`
- `docs/r0/evidence/failure-tests.json`
- `docs/r0/evidence/checksums.sha256`

Keep unsanitized/raw working output under ignored `out/r0/<run-id>/`.

Conditional patch files, created only for Outcome B:

- `patches/libkrun/README.md` — base commits, build instructions, rationale, and upstream status.
- `patches/libkrun/0001-windows-raw-device-support.patch` — minimal pinned patch.

## 6. Commands and environment steps

### Azure preflight and provisioning

```bash
az login
az account show -o json

export KODA_AZ_LOCATION="eastus"
export KODA_AZ_VM_SIZE="Standard_D2s_v4"
export KODA_AZ_IMAGE_SKU="win11-24h2-pro"

az vm list-skus \
  --location "$KODA_AZ_LOCATION" \
  --resource-type virtualMachines \
  --size "$KODA_AZ_VM_SIZE" \
  --all -o json

az vm list-usage --location "$KODA_AZ_LOCATION" -o table

az vm image list \
  --location "$KODA_AZ_LOCATION" \
  --publisher MicrosoftWindowsDesktop \
  --offer windows-11 \
  --sku "$KODA_AZ_IMAGE_SKU" \
  --all -o table

az vm image terms show \
  --urn "MicrosoftWindowsDesktop:windows-11:${KODA_AZ_IMAGE_SKU}:latest"
```

Repeat with the fixed fallback order only when unavailable.

```bash
export KODA_AZ_RG="koda-r0-a-$(date +%Y%m%d)"
export KODA_AZ_VM="koda-r0-win11"
export KODA_AZ_ADMIN="kodaadmin"
export KODA_AZ_IMAGE="MicrosoftWindowsDesktop:windows-11:${KODA_AZ_IMAGE_SKU}:<EXACT_VERSION>"

az group create \
  --name "$KODA_AZ_RG" \
  --location "$KODA_AZ_LOCATION" \
  --tags project=koda phase=r0-a disposable=true deleteAfter="<YYYY-MM-DD>"

az vm create \
  --resource-group "$KODA_AZ_RG" \
  --name "$KODA_AZ_VM" \
  --location "$KODA_AZ_LOCATION" \
  --size "$KODA_AZ_VM_SIZE" \
  --image "$KODA_AZ_IMAGE" \
  --security-type Standard \
  --license-type Windows_Client \
  --storage-sku StandardSSD_LRS \
  --public-ip-address "" \
  --admin-username "$KODA_AZ_ADMIN" \
  --admin-password "$KODA_AZ_ADMIN_PASSWORD"
```

Use `--license-type Windows_Client` only after the preflight confirms entitlement.

After `AZURE_WHP_PASS`:

```bash
export KODA_AZ_DATA_DISK="koda-r0-disposable"

az disk create \
  --resource-group "$KODA_AZ_RG" \
  --name "$KODA_AZ_DATA_DISK" \
  --location "$KODA_AZ_LOCATION" \
  --size-gb 32 \
  --sku StandardSSD_LRS \
  --tags project=koda phase=r0 disposable=true

az vm disk attach \
  --resource-group "$KODA_AZ_RG" \
  --vm-name "$KODA_AZ_VM" \
  --name "$KODA_AZ_DATA_DISK" \
  --lun 0 \
  --caching None
```

### Windows prerequisites and build

```powershell
Enable-WindowsOptionalFeature `
  -Online -FeatureName HypervisorPlatform -All -NoRestart
bcdedit /set hypervisorlaunchtype auto
Restart-Computer
```

Install Visual Studio 2022 Build Tools with the C++ workload, CMake, Ninja, Git, and the Windows SDK. Install Rust MSVC only if a libkrun patch must be built.

```powershell
cmake -S tools/r0-host -B out/r0-host -G Ninja `
  -DCMAKE_BUILD_TYPE=Release
cmake --build out/r0-host
ctest --test-dir out/r0-host --output-on-failure
```

Build the guest helper on Linux/WSL and do not commit the binary:

```bash
mkdir -p out/r0
x86_64-linux-musl-gcc -O2 -Wall -Wextra -Werror -static \
  guest/r0/io-helper.c -o out/r0/koda-r0-guest-io
sha256sum out/r0/koda-r0-guest-io
```

### Harness workflow

```powershell
koda-r0.exe inventory --json out\r0\inventory.json

koda-r0.exe enroll `
  --volume "\\?\Volume{GUID}\" `
  --azure-lun 0 `
  --output out\r0\device-allowlist.json `
  --confirm KODA-R0-DISPOSABLE

koda-r0.exe baseline `
  --allowlist out\r0\device-allowlist.json `
  --manifest out\r0\baseline.json

koda-r0.exe image-proof <pinned-runtime-arguments>

koda-r0.exe device-probe `
  --allowlist out\r0\device-allowlist.json `
  --read-only

koda-r0.exe cycle `
  --allowlist out\r0\device-allowlist.json `
  --cycles 1 `
  --confirm KODA-R0-DISPOSABLE

koda-r0.exe cycle `
  --allowlist out\r0\device-allowlist.json `
  --cycles 100 `
  --confirm KODA-R0-DISPOSABLE
```

### Cost control and cleanup

```bash
az vm deallocate \
  --resource-group "$KODA_AZ_RG" \
  --name "$KODA_AZ_VM"

az resource list --resource-group "$KODA_AZ_RG" -o table

az group delete \
  --name "$KODA_AZ_RG" \
  --yes --no-wait

az group exists --name "$KODA_AZ_RG"
```

## 7. Safety guardrails

The harness must refuse destructive work unless every condition passes:

- Exact volume label is `KODA-R0-DISPOSABLE`.
- Explicit allowlist schema and destructive confirmation token are present.
- Composite identity matches; disk number alone is never accepted.
- Azure execution resolves exactly LUN 0 and matches the enrolled Azure disk.
- Disk is basic GPT with exactly one expected NTFS partition and the prescribed reserved region.
- Disk is not dynamic, Storage Spaces, BitLocker-protected, clustered, or multi-extent.
- Disk contains no system, boot, recovery, pagefile, hibernation, crashdump, or shadow-copy dependency.
- Running OS volume extents and all system-disk fingerprints are independently compared and must differ.
- Any missing/random identifier, unexpected partition, extra volume, size change, or mount-layout change aborts.
- All offsets and lengths are checked for overflow, partition overlap, and GPT overlap before opening writable.
- Default mode is inventory/read-only; write mode requires both allowlist and explicit confirmation.
- No force-dismount fallback is permitted after lock failure.
- All storage handles are non-inheritable.
- The VMM worker is confined to a kill-on-close Job Object.
- After any post-offline failure, Windows is not remounted until the job is empty and exclusive raw reopen proves guest release.
- `recover` operates only on the exact enrolled device and never on “the only offline disk.”
- Whole-disk `DISK_ATTRIBUTE_OFFLINE` is only a bounded experiment with `Persist=FALSE`; it is not the normal path.
- A failed identity, lock, flush, offline, guest-flush, release, or validation check stops further destructive cycles.

## 8. Test matrix

| Scenario | Action | Expected result | Pass condition | Evidence |
|---|---|---|---|---|
| Azure WHP gate | WHP probe plus stock smolvm boot, then cold repeat | Linux reaches marker twice | All WHP calls and both boots succeed | WHP JSON, boot logs, VM profile |
| Regular-image proof | Attach ordinary raw file | Harness and guest control path works | Correct capacity, I/O, flush, release | Image-proof JSONL |
| Read-only raw proof | Attach allowlisted PhysicalDrive read-only | Correct device visible in guest | Identity, capacity, geometry, challenge hash match | Device-probe JSON |
| Normal round trip | Windows → guest → Windows | No ownership overlap; guest response observed | All states complete, NTFS validation clean | Cycle JSONL, integrity JSONL |
| Repetition | 100 consecutive cycles | No lifetime/handle drift | All 100 cycles PASS consecutively | Cycles JSONL and summary |
| Guest startup failure | Fail after volume offline but before guest ownership | Controller reclaims disk | Job empty, raw reopen succeeds, NTFS clean | Failure-test JSON |
| VMM termination | Terminate worker after `GUEST_ATTACHED` | Handles close and disk is reclaimable | Job empty, exclusive reopen, clean NTFS | Process/job and integrity evidence |
| Aborted handoff | Inject failpoint before guest ownership | Return to direct mode | No guest attach; online/mount/validation succeed | State log |
| Clean reboot | Reboot after clean return | Ordinary NTFS disk returns normally | Identity, mount paths, hashes, dirty bit, chkdsk pass | Post-reboot report |
| Recovery command | Simulate controller exit with volume offline | Exact device recovered fail-closed | Wrong device rejected; enrolled device restored | Recovery test log |

Per completed normal cycle:

- `PASS`: exact identity, complete state sequence, expected guest record, successful guest flush/close, zero VMM processes, exclusive raw reopen, normal remount, clean dirty bit, unchanged corpus hashes, expected raw response, and `chkdsk /scan` exit 0.
- `FAIL`: any identity, ownership, flush, payload, hash, mount, dirty-bit, or NTFS validation mismatch.
- `INCONCLUSIVE`: safety remained intact but required evidence was unavailable or the attempt never reached a classifiable result. It does not count toward 100 and cannot support R0 closure.

Run after every completed cycle:

```powershell
fsutil dirty query R:
chkdsk R: /scan
```

A nonzero `chkdsk` exit is a cycle failure. Never run repair flags automatically.

## 9. libkrun decision tree

### Outcome A — existing support works

Use smolvm v1.13.1’s pinned `krun.dll` with:

- `krun_add_disk3`
- `KRUN_DISK_FORMAT_RAW`
- `direct_io=false` initially
- `KRUN_SYNC_FULL`
- `block_id="KODA-R0-DISK"`

Document the permissive Windows raw-handle sharing limitation if filesystem lock/offline still supplies sufficient positive ownership evidence.

### Outcome B1 — bounded capacity patch

Trigger: raw path opens but host or guest capacity is zero/wrong.

- Patch imago’s Windows size discovery to use `IOCTL_DISK_GET_LENGTH_INFO` for device handles while preserving regular-file metadata behavior.
- Keep the public libkrun API unchanged.
- Add regular-file and mocked/raw-device size tests.

### Outcome B2 — bounded HANDLE/exclusivity patch

Trigger: capacity works, but libkrun’s reopen/share behavior prevents positive ownership evidence.

- Add a Windows-specific libkrun API accepting a duplicated `HANDLE`.
- Duplicate at configuration time; make lifetime and ownership explicit.
- Build the block backend from imago’s existing already-open `std::fs::File` conversion.
- Do not generalize this into a new cross-platform device framework.

### Outcome B3 — bounded alignment patch

Trigger: length works but sector-aligned I/O fails.

- Query `StorageAccessAlignmentProperty` and disk geometry.
- Replace the hard-coded Windows alignment assumption with device-reported constraints.
- Retain bounce buffering for guest requests that meet virtio’s 512-byte contract but not host alignment.

For B1–B3, carry a patch file against the pinned source during R0. Create a fork only after the patch passes R0 and is suitable for upstream review.

### Outcome C — architecture unsuitable

Stop and emit `R0_BLOCKED_VMM` if safe operation requires:

- a Windows kernel driver;
- invasive redesign of libkrun/imago;
- unreliable flush or release semantics;
- unbounded changes to virtio-blk;
- inability to prevent filesystem ownership overlap.

The next investigation candidates are Hyper-V-native device attachment or another WHP-capable VMM with an explicit raw-HANDLE backend. They are not implemented in R0.

## 10. Azure fallback trigger

Declare `AZURE_WHP_BLOCKED` after:

- correct Standard security, nested-capable SKU, WHP feature, boot configuration, and reboot have been verified;
- one configuration correction has been attempted;
- one cold deallocate/start retry has been attempted;
- failure evidence includes WHP HRESULTs, Windows events, VM profile, and stock smolvm logs;
- no more than two clean attempts or approximately two engineer-hours have been spent.

Do not resize repeatedly, install alternative nested hypervisors, or pursue heroic Azure workarounds.

If a minimal WHP partition probe fails in Azure, or the same pinned runtime works on physical Windows but fails there, close the cloud portion as `R0_REQUIRES_BARE_METAL`.

Bare-metal continuation requires:

- Windows 11 x64 with WHP;
- one explicitly disposable secondary internal SATA/NVMe disk;
- USB only if it exposes stable serial identity and reliable fixed-disk/UASP semantics;
- never a partition on the Windows system device;
- the same whole-disk layout, allowlist, harness, corpus, and tests.

Azure failure is not VMM failure.

## 11. Evidence, cost, and Git strategy

- Create branch `r0/architecture-feasibility`.
- Use focused commits for analysis, harness, experiment procedure, and results.
- Append a dated `bible.md` entry in every commit.
- Keep raw logs and binaries in ignored `out/r0/`.
- Commit only sanitized, bounded evidence and source.
- Never commit subscription/tenant IDs, usernames, public IPs, credentials, access tokens, signed URLs, RDP secrets, disk images, compiled DLLs, or proprietary Windows binaries.
- Deallocate the VM whenever testing pauses; delete the entire uniquely named resource group after evidence export.
- Do not use Premium SSD, Bastion, snapshots, backup vaults, or reserved capacity.
- Do not commit speculative upstream source into KODA. Carry a pinned patch file; fork only if upstream submission becomes justified.
- Open a PR from the R0 branch after the terminal status and evidence are complete.

## 12. Assumptions

- Whole-disk passthrough is sufficient for R0. Same-NVMe partition sharing is deferred.
- The Azure managed disk proves Windows logical ownership and raw `PhysicalDrive` semantics, not physical SATA/NVMe PnP behavior.
- The guest never mounts or repairs NTFS.
- Host-visible guest modification is restricted to the reserved raw region.
- `direct_io=false` plus `KRUN_SYNC_FULL` is tested first; direct I/O is enabled only after device-reported alignment is handled.
- A cycle failure does not authorize automatic filesystem repair.
- INCONCLUSIVE is a test-run classification, not a terminal R0 milestone status.

## 13. Exact R0 exit criteria

Emit exactly one:

### `R0_PASS`

All of the following are proven on Azure or bare metal:

- WHP/libkrun boots the pinned Linux guest.
- Windows successfully flushes, locks, dismounts, and offlines the disposable volume.
- Positive evidence shows no writable Windows filesystem ownership during guest use.
- The guest sees the correct raw device, performs controlled I/O, flushes, closes, and releases it.
- Windows positively proves guest release, reclaims the device, and remounts NTFS.
- One proof cycle and 100 consecutive repeated cycles pass.
- Startup failure, aborted handoff, VMM termination, recovery, and clean-reboot tests pass.
- Every cycle preserves volume identity, corpus hashes, clean dirty bit, and clean `chkdsk /scan`.
- Any required libkrun changes are bounded to Outcome B.

### `R0_BLOCKED_VMM`

On a confirmed WHP-capable Windows host, safe raw-device operation requires disproportionate VMM/backend changes, a kernel driver, or cannot provide reliable ownership, flush, or release semantics.

### `R0_REQUIRES_BARE_METAL`

Azure cannot provide usable WHP behavior after the bounded gate, the architecture itself has not been disproven, and the remaining hardware-dependent R0 tests require a physical Windows 11 host.
