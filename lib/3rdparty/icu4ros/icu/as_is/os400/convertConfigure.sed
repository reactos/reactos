# Copyright (C) 2006-2006, International Business Machines Corporation
# and others.  All Rights Reserved.
#
# Use "test -x" instead of "test -f" most of the time.
# due to how executables are created in a different file system.
s/as_executable_p="test -f"/as_executable_p="test -x"/g
s/test -f "$ac_file"/test -x "$ac_file"/g
s/test -f $ac_dir\/install-sh/test -x $ac_dir\/install-sh/g
s/test -f $ac_dir\/install.sh/test -x $ac_dir\/install.sh/g
s/test -f $ac_dir\/shtool/test -x $ac_dir\/shtool/g
# Use the more efficient del instead of rm command.
s/rm[ ]*-rf/del -f/g
s/rm[ ]*-fr/del -f/g
s/rm[ ]*-f/del -f/g
# Borne shell isn't always available on i5/OS
s/\/bin\/sh/\/usr\/bin\/qsh/g
# Use -c qpponly instead of -E to enable the preprocessor on the compiler
s/\$CC -E/\$CC -c -qpponly/g
# no diff in qsh the equivalent is cmp
s/ diff / cmp -s /g