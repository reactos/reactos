#!/usr/bin/env bash

# Copyright 2019 Alexandre Abadie <alexandre.abadie@inria.fr>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

CODESPELL_CMD="codespell"

if tput colors &> /dev/null && [ "$(tput colors)" -ge 8 ]; then
    CERROR=$'\033[1;31m'
    CRESET=$'\033[0m'
else
    CERROR=
    CRESET=
fi

: "${LWIPBASE:=$(cd $(dirname $0)/; pwd)}"
cd $LWIPBASE

: "${LWIPTOOLS:=${LWIPBASE}}"
. "${LWIPTOOLS}"/codespell_changed_files.sh

FILEREGEX='\.([CcHh]|sh|py|md|txt)$'
EXCLUDE='^(./contrib/apps/LwipMibCompiler/Mibs)'
FILES=$(FILEREGEX=${FILEREGEX} EXCLUDE=${EXCLUDE} changed_files)

if [ -z "${FILES}" ]; then
    exit 0
fi

${CODESPELL_CMD} --version &> /dev/null || {
    printf "%s%s: cannot execute \"%s\"!%s\n" "${CERROR}" "$0" "${CODESPELL_CMD}" "${CRESET}"
    exit 1
}

CODESPELL_OPTS="-q 2"  # Disable "WARNING: Binary file"
CODESPELL_OPTS+=" --check-hidden"
# Disable false positives "nd  => and, 2nd", "ans => and", "tolen => token",
# "ofo => of", "WAN => WANT", "mut => must, mutt, moot"
CODESPELL_OPTS+=" --ignore-words-list=nd,ans,tolen,ofo,wan,mut "
# propagate all options to codespell -> pass "-w" to this script to write changes
CODESPELL_OPTS+="$@"

# Filter-out all false positive raising "disabled due to" messages.
ERRORS=$(${CODESPELL_CMD} ${CODESPELL_OPTS} ${FILES} | grep -ve "disabled due to")

if [ -n "${ERRORS}" ]
then
    printf "%sThere are typos in the following files:%s\n\n" "${CERROR}" "${CRESET}"
    printf "%s\n" "${ERRORS}"
    # TODO: return 1 when all typos are fixed
    exit 0
else
    exit 0
fi
