!IFNDEF BUILDMSG
BUILDMSG=
!ENDIF

!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF




.\prflbmsg.h .\msg00001.bin .\prflbmsg.rc : .\prflbmsg.mc
	mc -v  prflbmsg.mc

