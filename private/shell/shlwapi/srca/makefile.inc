..\patha.c: ..\path.c
    echo #undef UNICODE > $@
    type $** >> $@

..\nativea.cpp: ..\native.cpp
    echo #undef UNICODE > $@
    type $** >> $@

