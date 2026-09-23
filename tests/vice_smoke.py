#!/usr/bin/env python3
"""Exercise the real PRG, KERNAL IEC I/O and a 1541 D64 using VICE.

Requires VICE with legally obtained C64/1541 ROMs. No ROMs are distributed here.
Use --xvfb /path/to/Xvfb on a headless host, or supply DISPLAY normally.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build' / 'vice-test'

class Monitor:
    def __init__(self, process, port):
        self.process = process
        end = time.monotonic() + 15
        while True:
            try:
                self.sock = socket.create_connection(('127.0.0.1', port), timeout=1)
                break
            except OSError:
                if process.poll() is not None or time.monotonic() > end:
                    raise RuntimeError('VICE monitor did not start; see build/vice-test/vice.log')
                time.sleep(.1)
        self.sock.settimeout(10)
        self.sock.sendall(b'\n')
        self.receive()

    def receive(self):
        output = b''
        while not re.search(rb'\(C:\$[0-9a-fA-F]+\)\s*$', output):
            chunk = self.sock.recv(65536)
            if not chunk:
                raise RuntimeError('VICE disconnected')
            output += chunk
        # Some VICE versions emit two prompts when first interrupted.
        self.sock.settimeout(.05)
        try:
            while True: output += self.sock.recv(65536)
        except socket.timeout:
            pass
        finally:
            self.sock.settimeout(10)
        return output.decode(errors='replace')

    def command(self, text):
        self.sock.sendall((text + '\n').encode())
        output = self.receive()
        if 'ERROR' in output or 'Syntax error' in output:
            raise RuntimeError(output)
        return output

    def run(self, seconds=.25):
        self.sock.sendall(b'exit\n')
        time.sleep(seconds)
        self.sock.sendall(b'\n')
        self.receive()

    def screen(self):
        target = OUT / 'screen.bin'
        self.command(f'bsave "{target}" 0 0400 07e7')
        codes = target.read_bytes()
        def char(c):
            c &= 127
            return chr(c + 64) if c < 32 else chr(c) if c < 64 else '?'
        return '\n'.join(''.join(char(c) for c in codes[y:y+40]) for y in range(0,1000,40))

    def expect(self, text, seconds=25):
        end = time.monotonic() + seconds
        while True:
            screen = self.screen()
            if text in screen:
                return screen
            if time.monotonic() > end:
                raise AssertionError(f'Expected {text!r}\n{screen}')
            self.run(.25)

    def keys(self, text):
        values = list(text.encode('ascii')) if isinstance(text, str) else text
        for start in range(0, len(values), 8):
            batch = values[start:start+8]
            self.command('> 0277 ' + ' '.join(f'{c:02x}' for c in batch))
            self.command(f'> 00c6 {len(batch):02x}')
            self.run(.18)

    def screenshot(self, name):
        self.command(f'screenshot "{OUT / name}" 2')

    def quit(self):
        self.sock.sendall(b'quit\n')
        self.sock.close()
        self.process.wait(timeout=10)

def free_port():
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        return sock.getsockname()[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vice', default='x64sc')
    parser.add_argument('--c1541', default='c1541')
    parser.add_argument('--rom-dir', type=Path, help='VICE data directory containing C64/ and DRIVES/')
    parser.add_argument('--system-dir', type=Path)
    parser.add_argument('--xvfb')
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    xvfb = None
    proc = None
    with (OUT/'vice.log').open('w') as log:
        try:
            if args.xvfb:
                display = 97
                env['DISPLAY'] = f'127.0.0.1:{display}'
                xvfb = subprocess.Popen([args.xvfb, f':{display}', '-screen','0','1024x768x24',
                    '-ac','-nolisten','unix','-listen','tcp'], env=env, stdout=log, stderr=log)
                time.sleep(.5)
            disk = OUT/'test.d64'
            subprocess.run([args.c1541,'-format','kanban test,kb','d64',str(disk),
                '-write',str(ROOT/'kanban.prg'),'kanban'],check=True,env=env,stdout=log,stderr=log)
            port = free_port()
            cmd = [args.vice, '-default', '+sound','-warp','-drive8type','1541',
                '-drive8truedrive', '-8',str(disk),'-remotemonitor',
                '-remotemonitoraddress',f'127.0.0.1:{port}', '-autostartprgmode','1',
                '-autostart',str(ROOT/'kanban.prg')]
            if args.system_dir: cmd += ['-directory',str(args.system_dir)]
            if args.rom_dir:
                for flag, path in [('-kernal','C64/kernal-901227-03.bin'),
                    ('-basic','C64/basic-901226-01.bin'),('-chargen','C64/chargen-901225-01.bin'),
                    ('-dos1541','DRIVES/dos1541-325302-01+901229-05.bin'),
                    ('-dos1541II','DRIVES/dos1541ii-251968-03.bin')]:
                    cmd += [flag,str(args.rom_dir/path)]
            def start():
                p = subprocess.Popen(cmd, env=env, stdout=log, stderr=log)
                time.sleep(1)
                return p, Monitor(p,port)
            proc, mon = start()
            mon.expect('NO SAVED DATA')
            assert 'MY BOARD' in mon.screen() and 'GENERAL' in mon.screen()
            mon.keys('N'); mon.expect('NEW CARD'); mon.keys('SHIP C64\r')
            mon.expect('1 SHIP C64')
            mon.keys([13]); mon.expect('CHECK ITEMS: 0 / 0 DONE')
            mon.keys('D'); mon.expect('DESCRIPTION (60 CHARS)'); mon.keys('PERSIST AFTER REBOOT\r')
            mon.keys('PP'); mon.expect('PRIORITY: HIGH')
            mon.keys('K'); mon.expect('CHECKLISTS'); mon.keys('NRELEASE\r')
            mon.keys([13]); mon.expect('RELEASE'); mon.keys('NTEST DISK\r')
            mon.keys(' '); mon.expect('[X] TEST DISK')
            mon.keys([3,3]); mon.expect('CHECK ITEMS: 1 / 1 DONE')
            mon.keys([3]); mon.expect('1 SHIP C64')
            mon.keys('S'); mon.expect('SAVED AND VERIFIED', 60)
            mon.screenshot('saved-board.png')
            print('VICE: created card, description, priority, checklist; saved and verified', flush=True)
            # A second save exercises alternating snapshots and closed-file DOS status.
            mon.keys('NSECOND CARD\r'); mon.expect('2 SECOND CAR')
            mon.keys('FK'); mon.expect('KIELI: SUOMI')
            mon.screenshot('settings-fi.png')
            mon.keys([3]); mon.expect('UIMARATA: GENERAL')
            mon.keys('H'); mon.expect('C64 KANBAN - OHJE')
            mon.screenshot('help-fi.png')
            mon.keys([13]); mon.keys('S')
            mon.expect('TALLENNETTU JA TARKISTETTU', 60)
            mon.screenshot('board-fi.png')
            mon.quit(); proc = None
            # Restart emulator, keeping only disk state. Autoload restores all fields.
            proc, mon = start()
            mon.expect('LADATTU - H: OHJE', 60)
            mon.expect('1 SHIP C64'); mon.expect('2 SECOND CAR')
            mon.keys('1'); mon.expect('PERSIST AFTER REBOOT')
            mon.expect('TASO: SUURI'); mon.expect('VALMIIT KOHDAT: 1 / 1')
            mon.screenshot('restored-card.png')
            mon.keys('K'); mon.keys([13]); mon.expect('[X] TEST DISK')
            mon.screenshot('restored-checklist.png')
            mon.keys([3,3,3]); mon.keys('FK'); mon.expect('LANGUAGE: ENGLISH')
            mon.keys([3]); mon.keys('S'); mon.expect('SAVED AND VERIFIED', 60)
            mon.quit(); proc = None
            proc, mon = start()
            mon.expect('LOADED - H FOR HELP', 60)
            mon.expect('1 SHIP C64'); mon.expect('2 SECOND CAR')
            mon.quit(); proc = None
            # Preserve pristine release image separately from test data.
            subprocess.run([args.c1541,'-attach',str(disk),'-read','kanban-a,s',str(OUT/'KANBAN-A'),
                '-read','kanban-b,s',str(OUT/'KANBAN-B')],check=True,env=env,stdout=log,stderr=log)
            print('VICE: FI language and card/checklist data restored; switching back to EN persisted too', flush=True)
        finally:
            if proc is not None and proc.poll() is None: proc.terminate(); proc.wait(timeout=5)
            if xvfb is not None: xvfb.terminate(); xvfb.wait(timeout=5)

if __name__ == '__main__': main()
