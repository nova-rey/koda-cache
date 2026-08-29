# R0 result

## Terminal status: `R0_REQUIRES_BARE_METAL`

Azure R0-A was retried with IPv4 forced inside the Azure CLI process. ARM DNS
and HTTPS work over IPv4, and authenticated catalog calls succeeded. The
subscription allowed `Standard_D2s_v5` in `westcentralus` and exposed Windows 11
24H2 Pro. A temporary Windows VM was created with `SecurityType=Standard` and no
secondary disk.

The guest execution channel could not be used: the first Run Command bootstrap
remained stuck; later calls hung or returned `Conflict: Run command extension
execution is in progress`; VM instance view did not expose a usable VM-agent
status. A restart and one deallocate/start recovery attempt did not clear it.
Consequently no WHP capability probe, stock smolvm boot, disk attachment, or
destructive storage operation occurred. This is an Azure execution-environment
block, not evidence that libkrun or KODA is unsuitable.

The temporary resource group was deleted and `group exists` returned `false`
after the deletion operation settled. No Azure VM, OS disk, network resource,
or secondary data disk remains from this attempt.

Remaining R0 work requires a bare-metal Windows 11 host: WHP/libkrun boot,
allowlisted disposable-device handoff, 100 consecutive cycles, bounded failure
tests, integrity/reboot validation, and the final libkrun decision.
