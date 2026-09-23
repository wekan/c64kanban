#!/usr/bin/env python3
"""Reject a cc65 C64 build that leaves less than 4 KiB below the stack top."""
import re
import sys
from pathlib import Path
path = Path(sys.argv[1] if len(sys.argv) > 1 else 'build/kanban.map')
match = re.search(r'^BSS\s+([0-9A-F]+)\s+([0-9A-F]+)\s+', path.read_text(), re.M)
if not match:
    raise SystemExit('BSS segment not found in linker map')
end = int(match[2], 16) + 1
headroom = 0xD000 - end
if headroom < 4096:
    raise SystemExit(f'Insufficient stack headroom: {headroom} bytes')
print(f'C64 static allocation ends at ${end:04X}; {headroom} bytes below $D000 stack top')
