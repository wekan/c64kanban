# C64 Kanban roadmap

## Implemented

- [x] Shared C89 model and 40-column keyboard interface.
- [x] Multiple editable boards, swimlanes and lists.
- [x] Create, open, edit and delete cards.
- [x] Card descriptions, priorities, done flags and checklist progress.
- [x] Move cards between boards/swimlanes/lists; reorder within a cell.
- [x] Named checklists with editable, removable and checkable items.
- [x] Paging through all cards and entities within the memory limits.
- [x] Cascading deletion, confirmation prompts and capacity errors.
- [x] C64 sequential disk I/O, device/file selection and startup load.
- [x] Two alternating snapshots, CRC and structural validation.
- [x] Preserve RAM on failed load and previous snapshot on failed write.
- [x] Read-back verification before reporting successful save.
- [x] Shared native terminal build, reproducible C64 build scripts and CI.
- [x] Automated model, storage fault-injection and UI regression tests.
- [x] VICE 1541 disk save and cold restart smoke test.
- [x] Runnable PRG, D64 build target and usage/file-format documentation.

- [x] English/Finnish UI and a live language switch in settings.
- [x] Persist language with project data; import version-1 snapshots.
- [x] Both-language UI tests, translation bounds, and EN/FI cold-restart tests.

## Further work / explicitly not included

- [ ] Physical C64/1541 testing and SD2IEC/Ultimate compatibility checks.
- [ ] Tape, REU and cartridge-specific backends.
- [ ] Reordering swimlanes, lists and checklist items.
- [ ] Labels, due dates, WIP limits, search and archived-card filtering.
- [ ] WeKan import/export tooling.
- [ ] Larger data sets via paging or expansion RAM.
- [ ] Extended PETSCII and accented user text input.

The current release implements the core requested kanban workflow within stock
C64 memory. It is not intended to match every feature of a networked WeKan server.
