!ifdef PASS0ONLY
.SUFFIXES: .c
!endif

theap.c: ..\heap.c ..\heapdbg.c ..\heapdll.c

t.c: ..\handle.c ..\atom.c

obj\$(TARGET_DIRECTORY)\generr.obj: ..\generr.c

..\error.h: obj\$(TARGET_DIRECTORY)\generr.obj
    generr $? $@
