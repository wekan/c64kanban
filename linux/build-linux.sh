#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
make host
printf '%s\n' 'Run ./build/kanban-host in a terminal at least 40 columns by 25 rows.'
