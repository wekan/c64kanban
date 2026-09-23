@echo off
setlocal
cd /d "%~dp0"
where cl65 >nul 2>nul
if errorlevel 1 (
  echo Install cc65 and add its bin directory to PATH first.
  exit /b 1
)
if not exist build mkdir build
cl65 -t c64 -O -I src -m build\kanban.map -Ln build\kanban.lbl -o kanban.prg kanban.c src\model.c src\storage.c src\i18n.c src\platform_c64.c src\disk_c64.c
if errorlevel 1 exit /b 1
echo Built kanban.prg
