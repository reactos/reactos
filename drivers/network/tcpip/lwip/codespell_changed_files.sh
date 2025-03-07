# Copyright 2017 Kaspar Schleiser <kaspar@schleiser.de>
# Copyright 2014 Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
# Copyright 2014 Hinnerk van Bruinehsen <h.v.bruinehsen@fu-berlin.de>
# Copyright 2020 Jonathan Demeyer <jona.dem@gmail.com>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

changed_files() {
    : ${FILEREGEX:='\.([CcHh])$'}
    : ${EXCLUDE:=''}
    : ${DIFFFILTER:='ACMR'}

    DIFFFILTER="--diff-filter=${DIFFFILTER}"

    # select either all or only touched-in-branch files, filter through FILEREGEX
    if [ -z "${BASE_BRANCH}" ]; then
        FILES="$(git ls-tree -r --full-tree --name-only HEAD | grep -E ${FILEREGEX})"
    else
        FILES="$(git diff ${DIFFFILTER} --name-only ${BASE_BRANCH} | grep -E ${FILEREGEX})"
    fi

    # filter out negatives
    echo "${FILES}" | grep -v -E ${EXCLUDE}
}
