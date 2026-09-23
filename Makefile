CC = cc
CL65 ?= cl65
PYTHON ?= python3
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CORE = src/model.c src/storage.c src/i18n.c
HOST = src/platform_host.c src/disk_host.c
C64 = src/platform_c64.c src/disk_c64.c
HEADERS = $(wildcard src/*.h src/*.def)
.PHONY: all host test clean disk check-snapshot
all: kanban.prg
kanban.prg: kanban.c $(CORE) $(C64) $(HEADERS)
	@mkdir -p build
	$(CL65) -t c64 -O -I src -m build/kanban.map -Ln build/kanban.lbl -o $@ kanban.c $(CORE) $(C64)
host: build/kanban-host
build/kanban-host: kanban.c $(CORE) $(HOST) $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -I src -o $@ kanban.c $(CORE) $(HOST)
test: build/test-model build/test-storage build/test-ui
	./build/test-model
	./build/test-storage
	./build/test-ui
	$(PYTHON) tests/test_i18n.py
build/test-model: tests/test_model.c src/model.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -I src -o $@ tests/test_model.c src/model.c
build/test-storage: tests/test_storage.c $(CORE) $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -I src -o $@ tests/test_storage.c $(CORE)
build/test-ui: tests/test_ui.c kanban.c $(CORE) $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -I src -o $@ tests/test_ui.c $(CORE) src/disk_host.c
check-snapshot: build/check-snapshot
	cd build/vice-test && ../check-snapshot
build/check-snapshot: tests/check_snapshot.c $(CORE) src/disk_host.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -I src -o $@ tests/check_snapshot.c $(CORE) src/disk_host.c
disk: kanban.prg
	c1541 -format 'c64 kanban,kb' d64 build/c64kanban.d64 -write kanban.prg kanban
clean:
	rm -f *.o src/*.o
	rm -rf build
