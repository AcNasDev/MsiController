#!/bin/sh

# Package scripts run before DKMS's post-transaction hook. Start the service
# here so its ExecStartPre can load the finished kernel module.
[ -d /run/systemd/system ] || exit 0
systemctl is-enabled --quiet msi-ec-service.service || exit 0
if ! systemctl restart msi-ec-service.service; then
  printf '%s\n' 'msicontroller: warning: service could not start; check matching kernel headers and DKMS status' >&2
fi
