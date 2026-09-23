# Validation of this implementation

Base repository: `wekan/c64kanban`, commit
`e966be8e64d5f25f6c1690736227c2488d699d00` (main, before these changes).

## Passed

- Strict C89 host compilation with GCC, `-pedantic -Wall -Wextra -Werror`.
- `make test`: model, storage, shared UI and translation-catalog tests.
- 10,000 seeded model mutations with invariant validation after each operation.
- Board/list/swimlane deletion cascades, stable slot reuse, card rank repair,
  movement across boards, title validation and all fixed-pool capacity limits.
- Checklist creation, editing, completion counts and cascade removal.
- UI manager flows, card edits, checkbox toggles, destination selection,
  deletion confirmation, cancellation, bounded text entry and paging in both English
  and Finnish. Switching language leaves user content unchanged.
- All 92 English/Finnish translation pairs: matching printf placeholders, valid
  stock-C64 characters and at most 39 displayed columns (including substitutions).
- Version-1 migration to English, version-2 language persistence, invalid-language
  rejection and restoration of data/language together after recovery.
- Snapshot round trips, checksum reference vector, malformed headers and parent
  references, trailing data, truncation and generation rollover.
- Interrupted writes at 59 byte offsets, plus open/remove/close errors and failed
  read-back verification; recovery retains the previous verified snapshot.
- AddressSanitizer and UndefinedBehaviorSanitizer on all three native test suites.
  LeakSanitizer was disabled because this runner cannot inspect process threads;
  the application itself uses static allocation.
- Bilingual PRG builds with Ubuntu's cc65 2.19 package. The original core release
  was also built with cc65 development revision `e11fb5c`.
- VICE 3.7.1 `x64sc`, PAL C64 and true 1541 drive emulation: create a card, edit its
  description and priority, create a checklist, check an item, save and read back,
  add a second card, switch to Finnish, save the alternate slot, exit and cold-start
  the emulator, then verify the Finnish UI, both cards and the first card's complete
  details and checkbox. Switch back to English, save and cold-start again to check
  persistence in both directions.
- Extract the two C64 SEQ files and read them with the native file backend;
  assertions verify that text, priorities and item completion are unchanged.
- `git diff --check`.

The PNG files in `docs/`, including the Finnish settings and help screens, are
actual VICE screenshots from the integration test, not mockups.

## C64 resource use (cc65 2.19 release binary)

- PRG: 24,858 bytes, including load address and BASIC startup stub.
- Working snapshot: 7,364 bytes; validation snapshot: another 7,364 bytes.
- Static allocation ends at `$A3C4`.
- 11,324 bytes remain below the C stack top at `$D000`.
- A snapshot is 7,376 bytes / 30 disk blocks; two slots use 60 blocks.
- The release D64 contains only KANBAN and free space, with no test project data.

## Reproduce

```sh
make test host all
python3 tools/check_memory.py
python3 tests/vice_smoke.py
make check-snapshot
```

The emulator test has optional `--rom-dir`, `--system-dir`, `--vice`, `--c1541`
and `--xvfb` arguments for nonstandard installations. It writes only under
`build/vice-test`. Read-back checks use the files it extracts there. Native tests
use an in-memory fault-injection disk; they do not modify project save files.

## Remaining validation limits

Physical Commodore hardware, SD2IEC/Ultimate devices, Windows execution of the
build script and the GitHub Actions workflow have not been exercised in this
session. Tape, REU and cartridge persistence are not implemented. The two-file
protocol preserves an existing file during an interrupted data write, but cannot
guarantee recovery from physical media failure or interrupted filesystem metadata
updates. Disk-level backup remains separate from application snapshot recovery.
