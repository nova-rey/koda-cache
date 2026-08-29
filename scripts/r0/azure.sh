#!/usr/bin/env bash
set -euo pipefail

: "${KODA_AZ_LOCATION:=eastus}"
: "${KODA_AZ_VM_SIZE:=Standard_D2s_v4}"
: "${KODA_AZ_IMAGE_SKU:=win11-24h2-pro}"
: "${KODA_AZ_RG:=koda-r0-a-$(date +%Y%m%d)}"
: "${KODA_AZ_VM:=koda-r0-win11}"
: "${KODA_AZ_CLI:=az}"

if [[ "${KODA_AZ_FORCE_IPV4:-0}" == 1 ]]; then
  KODA_AZ_CLI="$(dirname "$0")/az-ipv4.sh"
fi

case "${1:-preflight}" in
  preflight)
    "$KODA_AZ_CLI" account show -o json
    "$KODA_AZ_CLI" vm list-skus --location "$KODA_AZ_LOCATION" --resource-type virtualMachines \
      --size "$KODA_AZ_VM_SIZE" --all -o json
    "$KODA_AZ_CLI" vm list-usage --location "$KODA_AZ_LOCATION" -o table
    "$KODA_AZ_CLI" vm image list --location "$KODA_AZ_LOCATION" \
      --publisher MicrosoftWindowsDesktop --offer windows-11 \
      --sku "$KODA_AZ_IMAGE_SKU" --all -o table
    ;;
  create-gate)
    : "${KODA_AZ_IMAGE:?set an exact image URN after preflight}"
    : "${KODA_AZ_ADMIN:?set a non-secret admin username}"
    : "${KODA_AZ_ADMIN_PASSWORD:?set the password out-of-band}"
    "$KODA_AZ_CLI" group create --name "$KODA_AZ_RG" --location "$KODA_AZ_LOCATION" \
      --tags project=koda phase=r0-a disposable=true
    "$KODA_AZ_CLI" vm create --resource-group "$KODA_AZ_RG" --name "$KODA_AZ_VM" \
      --location "$KODA_AZ_LOCATION" --size "$KODA_AZ_VM_SIZE" \
      --image "$KODA_AZ_IMAGE" --security-type Standard \
      --license-type Windows_Client --storage-sku StandardSSD_LRS \
      --public-ip-address "" --admin-username "$KODA_AZ_ADMIN" \
      --admin-password "$KODA_AZ_ADMIN_PASSWORD"
    ;;
  deallocate)
    "$KODA_AZ_CLI" vm deallocate --resource-group "$KODA_AZ_RG" --name "$KODA_AZ_VM"
    ;;
  delete)
    "$KODA_AZ_CLI" group delete --name "$KODA_AZ_RG" --yes --no-wait
    ;;
  *)
    echo "usage: $0 {preflight|create-gate|deallocate|delete}" >&2
    exit 2
    ;;
esac
