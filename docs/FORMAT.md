# C64 Kanban snapshot format, version 2

Files are SEQ files named BASE-A and BASE-B, without a PRG load address.
Every valid file is exactly 7,376 bytes. Multi-byte header words are unsigned,
little-endian. Generation comparison uses `(a - b) & 65535`, newer when the
result is between 1 and 32767. Equal generations prefer A.

| Offset | Length | Meaning |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `C64K` (43 36 34 4B hex) |
| 4 | 1 | Format version, 2 (legacy version 1 also readable) |
| 5 | 1 | Language: 0 = English, 1 = Finnish; must be 0 in version 1 |
| 6 | 2 | Generation modulo 65536 |
| 8 | 2 | Payload length, 7364 |
| 10 | 2 | CRC-16/CCITT-FALSE |
| 12 | 7364 | Payload below |

CRC polynomial is 0x1021, initial value 0xFFFF, no reflection, no final XOR.
It covers header bytes 0–9 followed by the complete payload, excluding CRC bytes.
The reference vector `123456789` produces 0x29B1.

Version 2 uses the former reserved byte for the language. Version-1 snapshots
restore English. Unsupported languages are rejected even when the CRC is valid.
The language and working data are committed together only for a validated
candidate; a failed load preserves both. Save scanning never changes the current
UI language, and read-back verifies it along with the payload.

The payload layout is unchanged from version 1, in this order:

| Payload offset | Count | Record size | Records |
| ---: | ---: | ---: | --- |
| 0 | 4 | 27 | Boards |
| 108 | 12 | 27 | Swimlanes |
| 432 | 16 | 27 | Lists |
| 864 | 40 | 93 | Cards |
| 4584 | 20 | 27 | Checklists |
| 5124 | 80 | 28 | Checklist items |

All record fields are bytes or arrays of bytes. Compile-time size assertions
ensure the C structures match this layout, without padding or host integer fields.
Indices are zero-based stable slot IDs, not display positions.

**Named record (27 bytes):** used flag, parent slot, title[25]. Board parent is 0;
swimlane/list parent is a board; checklist parent is a card.

**Card (93 bytes):** used, board, swimlane, list, rank, done, priority,
title[25], description[61]. Rank is contiguous starting at 0 within a
swimlane/list cell. Priority is 0 (low), 1 (medium), 2 (high).

**Item (28 bytes):** used, checklist parent, done, title[25].

Used and done flags must be 0 or 1. Text fields must have a NUL terminator in the
field, with text bytes 32–95. Titles must contain a non-space character. Descriptions
may be empty. Newly created/deleted records and text padding are zeroed.

The loader validates used flags, parent existence, board membership of card lanes
and lists, unique and contiguous card ranks, text bounds, priorities and booleans.
Every board must have a swimlane and list; there must be at least one board. No
pointer, native integer, screen state, filename or drive setting is serialized.
The language is the one persistent UI setting, stored in the header.

A valid candidate is first loaded into a staging snapshot. Invalid candidates
never replace the working state. Save scans both slots and overwrites the older
or invalid slot. It refuses to proceed when scanning either slot has an I/O error;
this avoids guessing which file is safe to replace on an unreliable drive.

For extraction using VICE c1541, specify the SEQ type:

```sh
c1541 -attach project.d64 -read 'kanban-a,s' KANBAN-A -read 'kanban-b,s' KANBAN-B
```

Both host and C64 use the same bytes. A transfer tool must preserve binary data
and mark imported files as SEQ. The format is specific to C64 Kanban and is not a
WeKan JSON import/export format.
