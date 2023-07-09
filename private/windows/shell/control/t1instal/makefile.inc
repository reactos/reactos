#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#

!IFNDEF BUILDMSG
BUILDMSG=
!ENDIF

!IFNDEF NO_STRICT
STRICT=1
!ENDIF

SDKINC = $(_NTROOT)\public\sdk\inc
OAKINC = $(_NTROOT)\public\oak\inc

TARGETSRCS=     msg00001.bin \
                t1msg.h      \
                t1msg.rc

all: $(TARGETSRCS)
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

clean: cleansrc all

cleansrc:
    -erase $(TARGETSRCS)

msg00001.bin t1msg.rc t1msg.h: t1msg.mc
    mc -v t1msg.mc


