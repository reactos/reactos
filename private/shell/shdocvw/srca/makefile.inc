# BUGBUG (scotth) 4/22/98: temporary for a week 
delfile.tmp:
    del delfile.tmp
    del ..\filetypa.c
    

..\filetypa.cpp: ..\filetype.cpp
    echo #undef UNICODE > $@
    type $** >> $@
