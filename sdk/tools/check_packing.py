'''
PROJECT:     ReactOS code linter
LICENSE:     MIT (https://spdx.org/licenses/MIT)
PURPOSE:     Verifies that there are no headers included where packing is modified
COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
'''
from pathlib import Path
import re

DEFAULT_SUFFIXES = [
    '.cpp', '.cxx', '.cc', '.c', '.idl',
    '.hpp', '.h', '.inc'
]

START_HEADERS = [
    'pshpack1.h',
    'pshpack2.h',
    'pshpack4.h',
    'pshpack8.h',
]

END_HEADERS = [
    'poppack.h',
]


def print_error(file, line, text):
    print(f'{file}({line}): ERROR: {text}')

def print_warning(file, line, text):
    print(f'{file}({line}): WARNING: {text}')


def check_file(filename):
    cur_packing = []
    with open (filename, 'rb') as input_file:
        for line_nr, line in enumerate(input_file):
            res = re.search(rb'#[\s]*include[\s]+[<"]([^[">]+)[">]', line)
            if res:
                header = res.group(1).decode('utf-8')
                line_nr += 1    # Line numbers start at 1
                if header in START_HEADERS:
                    if cur_packing:
                        print_warning(filename, line_nr, f'Overrides packing from {cur_packing[-1][0]} to {header}')
                    cur_packing.append([header, line_nr])
                elif header in END_HEADERS:
                    if cur_packing:
                        cur_packing.pop()
                    else:
                        print_error(filename, line_nr, f'Unexpected "{header}"')
                elif cur_packing:
                    err = f'Include "{header}" while struct packing is modified ({cur_packing[-1][0]})'
                    print_error(filename, line_nr, err)
    if cur_packing:
        print_error(filename, cur_packing[-1][1], 'Struct packing not restored!')


def check_packing(path, include_suffixes):
    global EXCLUDE_SUFFIXES
    for item in path.iterdir():
        if item.is_dir() and item.name[0] != '.':
            check_packing(item, include_suffixes)
            continue
        suffix = item.suffix
        if suffix in include_suffixes:
            check_file(item)


if __name__ == '__main__':
    # Skip filename and 'sdk/tools'
    use_path = Path(__file__).parents[2]
    check_packing(use_path, DEFAULT_SUFFIXES)
