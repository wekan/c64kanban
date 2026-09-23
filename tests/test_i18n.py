#!/usr/bin/env python3
"""Check translation coverage, printf parity and rendered C64 row widths."""
import json
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
entries = []
for line in (root/'src/strings.def').read_text().splitlines():
    if not line.startswith('KB_TEXT'): continue
    match = re.fullmatch(r'KB_TEXT\((\w+), (".*"), (".*")\)', line)
    assert match, line
    key = match[1]
    en, fi = json.loads(match[2]), json.loads(match[3])
    assert en and fi, key
    assert re.findall(r'%[csu]', en) == re.findall(r'%[csu]', fi), key
    for text in (en, fi):
        assert all(32 <= ord(c) <= 95 for c in re.sub(r'%[csu]', '', text)), (key, text)
        args = {'LANE_LABEL': ('A'*24,), 'SELECTION': (40, 16),
                'CARD_STATUS': ('PIENI', 'VALMIS'), 'PROGRESS': (80, 80),
                'CURRENT_FILE': ('A'*12, 11), 'FILE_NOTICE': ('A'*12, 11),
                'LANGUAGE_LABEL': ('ENGLISH',)}.get(key, ())
        rendered = text % args
        assert len(rendered) <= 39, (key, len(rendered), rendered)
    entries.append(key)
assert len(entries) == len(set(entries)) < 255
# User data, format punctuation and the application/file name are not UI text.
for name in ('kanban.c', 'src/storage.c', 'src/disk_c64.c'):
    source = re.sub(r'/\*.*?\*/', '', (root/name).read_text(), flags=re.S)
    for literal in re.findall(r'"([^"\n]*)"', source):
        assert not re.search(r'[A-Z]{3} [A-Z]{3}', literal), (name, literal)
print(f'i18n: {len(entries)} English/Finnish pairs, formats and 39-column rows passed')
