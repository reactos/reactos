#!/bin/sh

PATH=/bin:/usr/bin:/usr/local/bin
SRCDIR=../source/reactos
 
cd $SRCDIR
svn -q update > /dev/null 2>&1
