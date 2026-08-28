$ErrorActionPreference = 'Stop'
$out = if ($env:KODA_R0_OUT) { $env:KODA_R0_OUT } else { 'C:\koda-r0\evidence' }
New-Item -ItemType Directory -Force -Path $out | Out-Null
Get-ComputerInfo | Out-File (Join-Path $out 'computer-info.txt')
Get-CimInstance Win32_OperatingSystem | Format-List * | Out-File (Join-Path $out 'os.txt')
Get-CimInstance Win32_Processor | Format-List * | Out-File (Join-Path $out 'cpu.txt')
Get-WindowsOptionalFeature -Online |
  Where-Object FeatureName -match 'Hyper|VirtualMachine' |
  ConvertTo-Json -Depth 4 | Out-File (Join-Path $out 'optional-features.json')
bcdedit /enum '{current}' | Out-File (Join-Path $out 'bcdedit.txt')
systeminfo.exe | Out-File (Join-Path $out 'systeminfo.txt')

Enable-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -All -NoRestart
bcdedit /set hypervisorlaunchtype auto | Out-File (Join-Path $out 'bcdedit-set.txt')
Write-Output 'REBOOT_REQUIRED'
