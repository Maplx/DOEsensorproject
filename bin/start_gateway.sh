#!/bin/bash
# Start the 6TiSCH gateway on the Raspberry Pi: gwapp (root-node bridge) first,
# then the webapp. Run with sudo:
#
#     sudo bin/start_gateway.sh
#
# Order matters: gwapp sends the slot-frame configuration to the webapp exactly
# once, when gwapp itself starts up. If the webapp is started first and gwapp
# later, or only the webapp is restarted, the webapp never gets a scheduler and
# every node that tries to join is rejected. This script always stops whatever
# is running, starts gwapp, waits, then starts the webapp.
#
# Environment overrides:
#   SERIAL=/dev/ttyACM0   serial port of the root LaunchPad
#   LOG_DIR=/home/<user>  where log_gwapp.txt / log_webapp.txt are written

set -u
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERIAL="${SERIAL:-/dev/ttyACM0}"
LOG_DIR="${LOG_DIR:-/home/${SUDO_USER:-$USER}}"
GWAPP_DIR="$REPO/source/Projects/gateway/proj"
WEBAPP_DIR="$REPO/bin/webapp"
GWAPP_LOG="$LOG_DIR/log_gwapp.txt"
WEBAPP_LOG="$LOG_DIR/log_webapp.txt"

if [ "$(id -u)" -ne 0 ]; then
    echo "please run with sudo: sudo $0"
    exit 1
fi
if [ ! -e "$SERIAL" ]; then
    echo "serial port $SERIAL not found. Is the LaunchPad plugged in? (ls /dev/ttyACM*)"
    exit 1
fi

# 1. stop anything left over
"$REPO/bin/stop_gateway.sh" quiet

# 2. gwapp
echo "[1/3] starting gwapp on $SERIAL (log: $GWAPP_LOG)"
cd "$GWAPP_DIR" || exit 1
setsid nohup ./gwapp.exe -s "$SERIAL" -f 100 -b 2 -n 6 > "$GWAPP_LOG" 2>&1 < /dev/null &
sleep 5
if ! pgrep -x gwapp.exe > /dev/null; then
    echo "gwapp did not start. Last log lines:"
    strings "$GWAPP_LOG" | tail -5
    exit 1
fi

# 3. webapp
echo "[2/3] starting webapp (log: $WEBAPP_LOG)"
cd "$WEBAPP_DIR" || exit 1
setsid nohup node app.js > "$WEBAPP_LOG" 2>&1 < /dev/null &
sleep 3
if ! pgrep -f "[n]ode app.js" > /dev/null; then
    echo "webapp did not start. Last log lines:"
    tail -5 "$WEBAPP_LOG"
    exit 1
fi

# 4. wait for the root node to come up
echo "[3/3] waiting for the root node to start the network..."
for i in $(seq 1 30); do
    if strings "$GWAPP_LOG" | grep -q "root node starts"; then
        break
    fi
    if strings "$GWAPP_LOG" | grep -q "Assertion"; then
        echo "gwapp hit an assertion (usually the LaunchPad needs its reset button pressed):"
        strings "$GWAPP_LOG" | grep -m1 "Assertion"
        exit 1
    fi
    sleep 2
done
strings "$GWAPP_LOG" | grep -m1 "root node starts" || echo "root node has not reported yet, check $GWAPP_LOG"
grep -q "set_minimal_config" "$WEBAPP_LOG" && echo "webapp received the slot-frame config" || echo "warning: webapp has no slot-frame config yet"

# fix ownership so the normal user can read/delete the logs and db files
chown "${SUDO_USER:-$USER}" "$GWAPP_LOG" "$WEBAPP_LOG" 2>/dev/null
chown -R "${SUDO_USER:-$USER}" "$WEBAPP_DIR/db" 2>/dev/null

echo
echo "gateway is up. Web UI: http://$(hostname -I | awk '{print $1}'):8080/"
echo "watch sensor data:  tail -f $WEBAPP_LOG | grep -E 'soot|topology|dao_report'"
