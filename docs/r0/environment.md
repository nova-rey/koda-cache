# R0 environment record

## Repository

- Repository: `nova-rey/koda-cache`
- Baseline commit: `71de59f112966e40984aee1b07aca12ca29b33b3`
- Runtime gate: smolvm v1.13.1, bundled libkrun `2dfbd6fb6a1acf50413b64009b90fed8c589c9c3`, libkrunfw `8f1307644bdcb95fe1343bf0d6ccf30aa22f1549`

## Current execution status

- Host OS: Linux development environment; Windows WHP and raw-device execution are unavailable locally.
- Azure CLI: 2.89.1.
- Azure subscription cache: enabled and authenticated.
- Azure live catalog calls initially timed out because the CLI selected an unusable IPv6 route. DNS returns both A and AAAA records; `curl -4` reaches ARM, while IPv6 times out. Authenticated ARM catalog calls now work over IPv4, and `scripts/r0/az-ipv4.sh` forces IPv4 inside the Azure CLI process without changing system networking.
- Azure catalog: `Standard_D2s_v5` is available in `westcentralus`; D2s_v4/v5 are restricted in eastus/eastus2/centralus. Windows 11 `win11-24h2-pro` versions are available there; latest observed catalog version is `26100.9168.260809`.
- Azure VM/data disk: not provisioned; resource-group listing is empty.
- WHP result: `AZURE_WHP_BLOCKED` at the Azure guest-execution layer; no
  `AZURE_WHP_PASS` evidence exists. Overall R0 result is
  `R0_REQUIRES_BARE_METAL`, not a VMM rejection.

This file must be replaced with the exact redacted Windows build, Azure region/SKU/image version, or physical-host inventory before R0 closure.
