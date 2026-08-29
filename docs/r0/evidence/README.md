# R0 evidence index

Azure catalog/network evidence is under `azure/`. `vm-profile.json` and
`run-command-attempts.json` record the temporary gate VM and the stuck guest
execution channel. `cleanup.json` and `resource-groups-post-cleanup.json` prove
the disposable resource group was removed.

No WHP probe, smolvm boot, data-disk attachment, raw-device cycle, NTFS scan,
failure test, or reboot evidence exists. Their absence is intentional and is the
reason for `R0_REQUIRES_BARE_METAL`.
