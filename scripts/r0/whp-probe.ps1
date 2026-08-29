param(
  [string]$Output = $(if ($env:KODA_R0_OUT) { Join-Path $env:KODA_R0_OUT 'whp-probe.json' } else { 'C:\koda-r0\evidence\whp-probe.json' })
)
$ErrorActionPreference = 'Stop'

# R0-A is deliberately a capability/partition probe only. It creates no guest,
# touches no disk, and does not enable Hyper-V beyond the caller's bootstrap.
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class KodaWhp {
  [DllImport("WinHvPlatform.dll")] public static extern int WHvGetCapability(uint cap, out uint result, uint size, out uint written);
  [DllImport("WinHvPlatform.dll")] public static extern int WHvCreatePartition(out IntPtr partition);
  [DllImport("WinHvPlatform.dll")] public static extern int WHvSetupPartition(IntPtr partition);
  [DllImport("WinHvPlatform.dll")] public static extern int WHvCreateVirtualProcessor(IntPtr partition, uint index, uint flags);
  [DllImport("WinHvPlatform.dll")] public static extern int WHvDeletePartition(IntPtr partition);
}
'@

function HResult([int]$Value) { ('0x{0:X8}' -f ([uint32]$Value)) }
$rows = [ordered]@{}
$present = 0; $written = 0
$hr = [KodaWhp]::WHvGetCapability(0, [ref]$present, 4, [ref]$written)
$rows.hypervisor_present = [ordered]@{ hresult = (HResult $hr); value = $present; bytes = $written }
if ($hr -ne 0 -or $present -ne 1) { $rows.result = 'AZURE_WHP_BLOCKED'; $rows.reason = 'WHvCapabilityCodeHypervisorPresent was not true' }
else {
  $partition = [IntPtr]::Zero
  $hrCreate = [KodaWhp]::WHvCreatePartition([ref]$partition)
  $rows.create_partition = HResult $hrCreate
  if ($hrCreate -ne 0) { $rows.result = 'AZURE_WHP_BLOCKED'; $rows.reason = 'WHvCreatePartition failed' }
  else {
    try {
      $hrSetup = [KodaWhp]::WHvSetupPartition($partition)
      $rows.setup_partition = HResult $hrSetup
      if ($hrSetup -ne 0) { $rows.result = 'AZURE_WHP_BLOCKED'; $rows.reason = 'WHvSetupPartition failed' }
      else {
        $hrVp = [KodaWhp]::WHvCreateVirtualProcessor($partition, 0, 0)
        $rows.create_vcpu = HResult $hrVp
        $rows.result = if ($hrVp -eq 0) { 'WHP_CAPABILITY_PASS' } else { 'AZURE_WHP_BLOCKED' }
      }
    } finally { [void][KodaWhp]::WHvDeletePartition($partition) }
  }
}
$parent = Split-Path -Parent $Output
New-Item -ItemType Directory -Force -Path $parent | Out-Null
[ordered]@{ schema = 'koda.r0.whp-probe.v1'; timestamp_utc = [DateTime]::UtcNow.ToString('o'); probe = $rows } |
  ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 -Path $Output
Get-Content -Raw $Output
