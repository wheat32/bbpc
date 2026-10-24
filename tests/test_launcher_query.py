"""scripts/launcher_query.py: the JSON answers the Qt launcher reads (launcher/process/portQueries)."""
from paths import ROOT
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = ROOT / 'scripts' / 'launcher_query.py'

PATCH_XML = '''<?xml version="1.0"?>
<Patch>
  <TitleID><ID>CUSA03173</ID></TitleID>
  <Metadata Title="Bloodborne" Name="Test patch" Note="line one\\nline two" Author="someone"
            PatchVer="1.0" AppVer="01.09" AppElf="eboot.bin" isEnabled="true">
    <PatchList><Line Type="bytes" Address="0x400000" Value="90"/></PatchList>
  </Metadata>
  <Metadata Title="Bloodborne" Name="Other version" AppVer="01.00" AppElf="eboot.bin">
    <PatchList><Line Type="bytes" Address="0x400000" Value="90"/></PatchList>
  </Metadata>
</Patch>
'''


def query(*arguments):
    run = subprocess.run([sys.executable, str(SCRIPT), *map(str, arguments)], capture_output=True,
                         text=True, check=True, timeout=60)
    return json.loads(run.stdout.strip().splitlines()[-1])


class LauncherQueryTests(unittest.TestCase):
    def test_game_check_without_eboot_reports_it(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(query('game-check', directory), {'eboot': False, 'problem': None})

    def test_game_check_problem_has_kind_title_and_version(self):
        with tempfile.TemporaryDirectory() as directory:
            (Path(directory) / 'eboot.bin').write_bytes(b'not an executable')
            answer = query('game-check', directory)
            self.assertTrue(answer['eboot'])
            self.assertEqual(sorted(answer['problem']), ['kind', 'title', 'version'])
            self.assertEqual(answer['problem']['kind'], 'unreadable')

    def test_mods_lists_folders_with_a_mod_layout(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'Good' / 'dvdroot_ps4' / 'chr').mkdir(parents=True)
            (root / 'Bare' / 'parts').mkdir(parents=True)
            (root / 'Not a mod' / 'random').mkdir(parents=True)
            self.assertEqual(query('mods', directory), {'mods': ['Bare', 'Good']})

    def test_patches_lists_this_versions_patches_with_their_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            (Path(directory) / 'extra.xml').write_text(PATCH_XML, encoding='utf-8')
            self.assertEqual(query('patches', directory), {'patches': [{
                'key': 'extra.xml/Test patch', 'name': 'Test patch', 'author': 'someone',
                'note': 'line one\nline two', 'default': True}]})

    def test_dll_info_rejects_a_dll_without_fsr4(self):
        with tempfile.TemporaryDirectory() as directory:
            dll = Path(directory) / 'amd_fidelityfx_upscaler_dx12.dll'
            dll.write_bytes(b'MZ' + bytes(64))
            answer = query('dll-info', dll)
            self.assertEqual(answer['status'], answer['unsupported'])
            self.assertEqual(answer['details']['kind'], 'not_fsr4')


if __name__ == '__main__':
    unittest.main()
