# R0 result

## Terminal status: `R0_REQUIRES_BARE_METAL`

Azure R0-A was retried with IPv4 forced inside the Azure CLI process. ARM DNS
and HTTPS work over IPv4, and authenticated catalog calls succeeded. The
subscription allowed `Standard_D2s_v5` in `westcentralus` and exposed Windows 11
24H2 Pro. A temporary Windows VM was created with `SecurityType=Standard` and no
secondary disk. Direct RDP authentication and an independent Custom Script
Extension guest path worked.

The existing Run Command path remained stuck, but the independent extension path
ran the existing WHP probe twice. Before and after one clean deallocate/start,
`WHvGetCapability(HypervisorPresent)` returned HRESULT `0x00000000` with value
`0`. Azure therefore does not expose a usable WHP hypervisor to this VM. Stock
smolvm boot was correctly not attempted after the capability gate failed.
This is an Azure nested-virtualization limitation, not evidence that libkrun or
KODA is unsuitable.

The temporary resource group was deleted and `group exists` returned `false`
after the deletion operation settled. No Azure VM, OS disk, network resource,
or secondary data disk remains from this attempt.

The independent final audit found no evidence contradiction, but required richer
machine-readable Azure artifacts. Those are now recorded in `vm-profile.json`,
`run-command-attempts.json`, and `resource-groups-post-cleanup.json`; the earlier
`resource-groups.json` is explicitly pre-provisioning evidence.

Remaining R0 work requires a bare-metal Windows 11 host: WHP/libkrun boot,
allowlisted disposable-device handoff, 100 consecutive cycles, bounded failure
tests, integrity/reboot validation, and the final libkrun decision.
