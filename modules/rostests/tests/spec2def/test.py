import subprocess
import os
import tempfile
import sys
import difflib

# ${_spec_file} = ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
# spec2def -n=${_dllname} -a=${ARCH} ${ARGN} --implib -d=${BIN_PATH}/${_libname}_implib.def ${_spec_file}
# spec2def -n=${_dllname} -a=${ARCH} -d=${BIN_PATH}/${_file}.def -s=${BIN_PATH}/${_file}_stubs.c ${__with_relay_arg} ${__version_arg} ${_spec_file}
# spec2def --ms -a=${_ARCH} --implib -n=${_dllname} -d=${_def_file} -l=${_asm_stubs_file} ${_spec_file}
# spec2def --ms -a=${ARCH} -n=${_dllname} -d=${BIN_PATH}/${_file}.def -s=${BIN_PATH}/${_file}_stubs.c ${__with_relay_arg} ${__version_arg} ${_spec_file}

SCRIPT_DIR = os.path.dirname(__file__)
SPEC_FILE = os.path.join(SCRIPT_DIR, 'test.spec')
DATA_DIR = os.path.join(SCRIPT_DIR, 'testdata')

class ResultFile:
    def __init__(self, datadir, filename):
        self.filename = filename
        with open(os.path.join(datadir, filename), 'r') as content:
            self.data = content.read()

    def normalize(self):
        data = self.data.splitlines()
        data = [line for line in data if line]
        return '\n'.join(data)


class TestCase:
    def __init__(self, spec_args, prefix):
        self.spec_args = spec_args
        self.prefix = prefix
        self.expect_files = []
        self.result_files = []
        self.stdout = self.stderr = None
        self.returncode = None

    def run(self, cmd, tmpdir, all_datafiles):
        datafiles = [filename for filename in all_datafiles if filename.startswith(self.prefix)]
        self.expect_files = [ResultFile(DATA_DIR, datafile) for datafile in datafiles]
        tmppath = os.path.join(tmpdir, self.prefix)
        args = [elem.replace('$tmp$', tmppath) for elem in self.spec_args]
        args = [cmd] + args + [SPEC_FILE]
        proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.stdout, self.stderr = proc.communicate()
        self.returncode = proc.returncode
        self.result_files = [ResultFile(tmpdir, tmpfile) for tmpfile in os.listdir(tmpdir)]

    def verify(self):
        if False:
            for result in self.result_files:
                with open(os.path.join(DATA_DIR, result.filename), 'w') as content:
                    content.write(result.data)
            return

        if self.returncode != 0:
            print('Failed return code', self.returncode, 'for', self.prefix)
            return
        self.expect_files.sort(key= lambda elem: elem.filename)
        self.result_files.sort(key= lambda elem: elem.filename)
        exp_len = len(self.expect_files)
        res_len = len(self.result_files)
        if exp_len != res_len:
            print('Not enough files for', self.prefix, 'got:', res_len, 'wanted:', exp_len)
            return

        for n in range(len(self.expect_files)):
            exp = self.expect_files[n]
            res = self.result_files[n]
            if exp.normalize() == res.normalize():
                # Content 100% the same, ignoring empty newlines
                continue

            exp_name = 'expected/' + exp.filename
            res_name = 'output/' + res.filename
            exp = exp.data.splitlines()
            res = res.data.splitlines()
            diff = difflib.unified_diff(exp, res, fromfile=exp_name, tofile=res_name, lineterm='')
            for line in diff:
                print(line)


TEST_CASES = [
    # GCC implib
    TestCase([ '-n=testdll.xyz', '-a=i386', '--implib', '-d=$tmp$test.def', '--no-private-warnings' ], '01-'),
    TestCase([ '-n=testdll.xyz', '-a=x86_64', '--implib', '-d=$tmp$test.def', '--no-private-warnings' ], '02-'),
    # GCC normal
    TestCase([ '-n=testdll.xyz', '-a=i386', '-d=$tmp$test.def', '-s=$tmp$stubs.c' ], '03-'),
    TestCase([ '-n=testdll.xyz', '-a=x86_64', '-d=$tmp$test.def', '-s=$tmp$stubs.c' ], '04-'),
    TestCase([ '-n=testdll.xyz', '-a=i386', '-d=$tmp$test.def', '-s=$tmp$stubs.c', '--with-tracing' ], '05-'),
    # MSVC implib
    TestCase([ '--ms', '-n=testdll.xyz', '-a=i386', '--implib', '-d=$tmp$test.def', '-l=$tmp$stubs.asm' ], '06-'),
    TestCase([ '--ms', '-n=testdll.xyz', '-a=x86_64', '--implib', '-d=$tmp$test.def', '-l=$tmp$stubs.asm' ], '07-'),
    # MSVC normal
    TestCase([ '--ms', '-n=testdll.xyz', '-a=i386', '-d=$tmp$test.def', '-s=$tmp$stubs.c' ], '08-'),
    TestCase([ '--ms', '-n=testdll.xyz', '-a=x86_64', '-d=$tmp$test.def', '-s=$tmp$stubs.c' ], '09-'),
    TestCase([ '--ms', '-n=testdll.xyz', '-a=i386', '-d=$tmp$test.def', '-s=$tmp$stubs.c', '--with-tracing' ], '10-'),
]


def run_test(testcase, cmd, all_files):
    with tempfile.TemporaryDirectory() as tmpdirname:
        testcase.run(cmd, tmpdirname, all_files)
        testcase.verify()

def main(args):
    cmd = args[0] if args else 'spec2def'
    all_files = os.listdir(DATA_DIR)
    for testcase in TEST_CASES:
        run_test(testcase, cmd, all_files)

if __name__ == '__main__':
    main(sys.argv[1:])
