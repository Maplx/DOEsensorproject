#!/bin/bash
# Stop the 6TiSCH gateway on the Raspberry Pi: webapp first, then gwapp.
# Run with sudo (the processes are started by root):
#
#     sudo bin/stop_gateway.sh
#
# "quiet" as first argument suppresses the chatter (used by start_gateway.sh).

QUIET="${1:-}"
say() { [ "$QUIET" = "quiet" ] || echo "$@"; }

if [ "$(id -u)" -ne 0 ]; then
    echo "please run with sudo: sudo $0"
    exit 1
fi

# webapp (match the command line without matching this script's own shell)
PIDS="$(pgrep -f '[n]ode app.js')"
if [ -n "$PIDS" ]; then
    say "stopping webapp (pid $PIDS)"
    kill $PIDS 2>/dev/null
    sleep 1
    kill -9 $PIDS 2>/dev/null
else
    say "webapp not running"
fi

# gwapp
PIDS="$(pgrep -x gwapp.exe)"
if [ -n "$PIDS" ]; then
    say "stopping gwapp (pid $PIDS)"
    kill $PIDS 2>/dev/null
    sleep 1
    kill -9 $PIDS 2>/dev/null
else
    say "gwapp not running"
fi

sleep 1
if pgrep -x gwapp.exe > /dev/null || pgrep -f '[n]ode app.js' > /dev/null; then
    echo "some processes are still alive:"
    pgrep -af 'gwapp.exe|[n]ode app.js'
    exit 1
fi
say "gateway stopped"
