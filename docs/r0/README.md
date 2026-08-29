# KODA R0 evidence and execution

R0 is in progress. This directory records the reproducible architecture-feasibility experiment; it is not production KODA code.

The required order is: Azure catalog preflight, Azure WHP gate, regular-image harness proof, disposable-disk inventory and allowlist, ownership cycles, bounded failures, integrity validation, and closure decision.

No data disk may be attached before `AZURE_WHP_PASS`. Current planning execution has authenticated Azure CLI access, but live catalog requests timed out, so no Azure resources have been provisioned and no terminal R0 status is claimed.

Raw output belongs under ignored `out/r0/`. Only redacted, hashed, machine-readable evidence belongs under `docs/r0/evidence/`.

The host spike is under `tools/r0-host/`; the guest helper is under `guest/r0/`. The harness refuses non-Windows execution for destructive operations and must never be treated as a production service.

Integrity cadence and PASS/FAIL/INCONCLUSIVE rules are defined in
[`integrity-policy.md`](integrity-policy.md). The execution target is 100
consecutive successful handoff cycles; infrastructure interruptions before a
cycle begins do not reset the streak only when Windows ownership remains
positively established.

The ownership implementation fails closed if the exclusive raw reopen cannot be
proved. Bringing a volume online is not treated as a completed return: remount,
identity validation, hash checks, and non-repairing `chkdsk /scan` must supply
separate evidence before a cycle can pass.
