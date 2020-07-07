#!/bin/bash
set -euo pipefail

carbon_server=${1:?No carbon server given}; shift

systemctl is-active brubeck >/dev/null && systemctl stop brubeck

[ -d /usr/local/sbin ] || mkdir -p /usr/local/sbin
[ -f /usr/local/sbin/brubeck ] && mv /usr/local/sbin/brubeck /usr/local/sbin/brubeck.old
cp brubeck /usr/local/sbin/brubeck

mkdir -p /etc/brubeck
cat >/etc/brubeck/config.json <<EOF
{
    "sharding": false,
    "server_name": "brubeck-$HOSTNAME",
    "dumpfile": "/var/log/brubeck.dump",
    "capacity": 15,
    "backends": [
        {
            "type": "carbon",
            "address": "$carbon_server",
            "port": 2003,
            "frequency": 10,
            "pickle": false
        }
    ],
    "samplers": [
        {
            "type": "statsd",
            "address": "0.0.0.0",
            "port": 8125,
            "workers": 8,
            "multisock": false,
            "multimsg": 0
        }
    ]
}
EOF

cp brubeck.service /etc/systemd/system
systemctl daemon-reload >/dev/null
systemctl enable --now brubeck
