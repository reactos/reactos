!include ..\daytona\makefile.inc
#
# Copy files from server to make local version of base functions.
#
..\regmap.c : ..\..\server\regmap.c
    copy ..\..\server\regmap.c ..\
