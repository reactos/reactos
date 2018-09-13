# BUGBUG (scotth) 4/22/98: temporary for a week 
delfile.tmp:
    del delfile.tmp
    del ..\filetypw.c
    

..\filetypw.cpp: ..\filetype.cpp
    echo #define UNICODE > $@
    type $** >> $@
