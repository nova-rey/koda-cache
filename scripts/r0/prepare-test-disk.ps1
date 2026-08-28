param(
  [Parameter(Mandatory = $true)][int]$Lun,
  [Parameter(Mandatory = $true)][string]$ExpectedModel,
  [string]$Label = 'KODA-R0-DISPOSABLE'
)
$ErrorActionPreference = 'Stop'

if ($Label -ne 'KODA-R0-DISPOSABLE') { throw 'refusing a non-sacrificial label' }
$disk = Get-Disk | Where-Object { $_.Location -match "LUN $Lun" -and $_.FriendlyName -eq $ExpectedModel }
if ($disk.Count -ne 1) { throw 'LUN/model did not resolve exactly one disk' }
if ($disk.IsBoot -or $disk.IsSystem -or $disk.IsReadOnly) { throw 'unsafe system/read-only disk' }
if ($disk.Number -eq 0) { throw 'PhysicalDisk0 is never accepted' }

Set-Disk -Number $disk.Number -IsOffline $false
if ($disk.PartitionStyle -eq 'RAW') { Initialize-Disk -Number $disk.Number -PartitionStyle GPT -PassThru | Out-Null }
$part = New-Partition -DiskNumber $disk.Number -Size 30GB -DriveLetter R
Format-Volume -Partition $part -FileSystem NTFS -NewFileSystemLabel $Label -AllocationUnitSize 4096 -Confirm:$false
Write-Output "Prepared disk $($disk.Number) only after LUN/model/system checks. Derive the raw reserved region from the resulting GPT layout; do not hard-code a disk number."
