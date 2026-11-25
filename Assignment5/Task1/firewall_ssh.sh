#!/usr/bin/env bash
# Firewall: allow only SSH, HTTP, HTTPS. Run as root (sudo).
set -euo pipefail
ufw default deny incoming
ufw default allow outgoing
ufw allow OpenSSH
ufw allow 80/tcp
ufw allow 443/tcp
ufw --force enable
ufw status verbose
# SSH hardening (edit /etc/ssh/sshd_config, then: systemctl reload ssh)
#   PasswordAuthentication no
#   PermitRootLogin no
#   PubkeyAuthentication yes
