#!/bin/sh
set -eu
cd "$(dirname "$0")"
if ! command -v cl65 >/dev/null 2>&1; then
    echo 'Install cc65 first (Debian/Ubuntu: apt install cc65; macOS: brew install cc65).' >&2
    exit 1
fi
make all "$@"
