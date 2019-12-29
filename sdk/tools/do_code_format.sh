#!/bin/bash
# do_code_format.sh

if [ ! -x clang-format ]; then
	echo ERROR: Program clang-format is not found.
	exit 1
fi

clang-format -style=file -i $@
