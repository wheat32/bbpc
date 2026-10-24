#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Questions the Qt launcher asks the port's own Python helpers, answered as JSON on stdout.

The game check, mod discovery, third-party patches, the FSR 4.1.1 DLL check and the co-op servers
are the same code run.sh uses (game_check, mods, patches, online, tools/fsr4cap/dll_info), so the
launcher and the game never disagree. Each command prints one JSON object per line: one line for
an answer, a stream of events for thd-sign-in.

    launcher_query.py game-check GAME_DIR
    launcher_query.py mods MODS_DIR
    launcher_query.py patches PATCHES_DIR
    launcher_query.py dll-info UPSCALER_DLL [LOADER_DLL]
    launcher_query.py online-check --server HOST:PORT --webapi URL --community URL
    launcher_query.py thd-check --server URL
    launcher_query.py thd-sign-in --server URL [--device NAME]
"""
import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'tools' / 'fsr4cap'))

import game_check  # noqa: E402
import mods  # noqa: E402
import online  # noqa: E402
import patches  # noqa: E402


def emit(value):
    print(json.dumps(value, ensure_ascii=False), flush=True)


def game_check_command(a):
    game = Path(a.game).expanduser()
    if not (game / 'eboot.bin').is_file():
        return {'eboot': False, 'problem': None}
    found = game_check.problem(game)
    problem = None if found is None else dict(zip(('kind', 'title', 'version'), found))
    return {'eboot': True, 'problem': problem}


def mods_command(a):
    return {'mods': mods.discover(Path(a.directory).expanduser())}


def patches_command(a):
    found = []
    for key, _path, meta in patches.external_patches(Path(a.directory).expanduser()):
        found.append({
            'key': key,
            'name': meta.get('Name') or key,
            'author': meta.get('Author') or '',
            'note': (meta.get('Note') or '').replace('\\n', '\n'),
            'default': (meta.get('isEnabled') or 'false').lower() == 'true',
        })
    return {'patches': found}


def dll_info_command(a):
    import dll_info
    status, problem, details = dll_info.inspect(a.upscaler, a.loader)
    return {'status': status, 'problem': problem, 'details': details,
            'unsupported': dll_info.UNSUPPORTED, 'no_loader': dll_info.NO_LOADER}


def results(rows):
    return {'results': [{'name': name, 'address': address, 'ok': ok, 'detail': detail}
                        for name, address, ok, detail in rows]}


def online_check_command(a):
    return results(online.check(a.server, a.webapi, a.community))


def thd_check_command(a):
    return results(online.check_thd(a.server))


def thd_sign_in_command(a):
    """Events: {"event": "code", "page", "code"}, then {"event": "signed-in", "online_id",
    "token"} or {"event": "error", "message"}. The launcher cancels by ending the process."""
    try:
        name, token = online.thd_sign_in(
            a.server, lambda page, code: emit({'event': 'code', 'page': page, 'code': code}),
            device=a.device)
    except ValueError as error:
        return {'event': 'error', 'message': str(error)}
    return {'event': 'signed-in', 'online_id': name, 'token': token}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    sub = parser.add_subparsers(dest='command', required=True)
    p = sub.add_parser('game-check')
    p.add_argument('game')
    p.set_defaults(run=game_check_command)
    p = sub.add_parser('mods')
    p.add_argument('directory')
    p.set_defaults(run=mods_command)
    p = sub.add_parser('patches')
    p.add_argument('directory')
    p.set_defaults(run=patches_command)
    p = sub.add_parser('dll-info')
    p.add_argument('upscaler')
    p.add_argument('loader', nargs='?')
    p.set_defaults(run=dll_info_command)
    p = sub.add_parser('online-check')
    p.add_argument('--server', default=online.SHADNET_SERVER)
    p.add_argument('--webapi', default=online.WEBAPI_SERVER)
    p.add_argument('--community', default=online.COMMUNITY_SERVER)
    p.set_defaults(run=online_check_command)
    p = sub.add_parser('thd-check')
    p.add_argument('--server', default='')
    p.set_defaults(run=thd_check_command)
    p = sub.add_parser('thd-sign-in')
    p.add_argument('--server', default='')
    p.add_argument('--device', default='bbpc on Linux')
    p.set_defaults(run=thd_sign_in_command)
    a = parser.parse_args(argv)
    emit(a.run(a))
    return 0


if __name__ == '__main__':
    sys.exit(main())
