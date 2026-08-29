#!/usr/bin/env bash
set -euo pipefail

# Azure CLI itself may prefer the AAAA record in constrained environments. This
# wrapper changes address-family selection inside the CLI process only; it does
# not alter host networking or /etc/hosts.
az_python=${AZURE_CLI_PYTHON:-/opt/az/bin/python3}
if [[ ! -x "$az_python" ]]; then
  echo "Azure CLI's bundled Python is not available: $az_python" >&2
  exit 2
fi
exec "$az_python" - "$@" <<'PY'
import runpy
import socket
import sys

_getaddrinfo = socket.getaddrinfo
def ipv4_only(host, port, family=0, type=0, proto=0, flags=0):
    return _getaddrinfo(host, port, socket.AF_INET, type, proto, flags)
socket.getaddrinfo = ipv4_only
sys.argv = ["azure-cli"] + sys.argv[1:]
runpy.run_module("azure.cli", run_name="__main__")
PY
