#!/bin/bash
# do_code_format.sh

function version
{
  echo "$(basename ${0}) version 0.0.2"
}

function usage
{
  cat <<EOF
$(basename ${0}) does code formatting.

Usage:
    $(basename ${0}) [<options>] <files>

Options:
    --help       print this message
    --version    print $(basename ${0}) version
EOF
}

case ${1} in
  --help)
    usage
    exit 0
  ;;

  --version)
    version
    exit 0
  ;;
esac

clang-format -style=file -i $@
