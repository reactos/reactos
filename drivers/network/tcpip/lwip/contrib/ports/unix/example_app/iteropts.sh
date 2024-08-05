#!/bin/bash

LOGFILE=iteropts.log
EXAPPDIR=../../../examples/example_app
RETVAL=0

pushd `dirname "$0"`
pwd
echo Starting Iteropts run >> $LOGFILE
for f in $EXAPPDIR/test_configs/*.h
do
    echo Cleaning...
    make clean > /dev/null
    BUILDLOG=$(basename "$f" ".h").log
    echo testing $f
    echo testing $f >> $LOGFILE
    rm -f $EXAPPDIR/lwipopts_test.h
    # cat the file to update its timestamp
    cat $f > $EXAPPDIR/lwipopts_test.h
    make TESTFLAGS="-DLWIP_OPTTEST_FILE -Wno-documentation" -j 4 1> $BUILDLOG 2>&1
    ERR=$?
    if [ $ERR != 0 ]; then
        cat $BUILDLOG
        echo file $f failed with $ERR >> $LOGFILE
        echo ++++++++ $f FAILED +++++++
        RETVAL=1
    fi
    echo test $f done >> $LOGFILE
done
echo done, cleaning
make clean > /dev/null
popd
echo Exit value: $RETVAL
exit $RETVAL
