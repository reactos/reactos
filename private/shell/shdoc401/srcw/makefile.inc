..\filetypw.c: ..\filetype.c
    echo #define UNICODE > $@
    type $** >> $@
