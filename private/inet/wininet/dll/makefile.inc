!IFNDEF BASEDIR
BASEDIR=$(_NTDRIVE)$(_NTROOT)
!ENDIF

!IFNDEF SDKINC
SDKINC=$(BASEDIR)\public\sdk\inc
!ENDIF

!if "$(BUILD_PRODUCT)" == "IE"
MC_INC = -A
!endif

$(SDKINC)\inetmsg.h inetmsg.rc msg00001.bin : inetmsg.mc
    mc $(MC_INC) -v -d -h $(SDKINC) inetmsg.mc

#wininet.sym: wininet.map
#    mapsym -o wininet.sym wininet.map

wininet.rc : selfreg_wininet.inf
wininet.rc : ..\inetui\inetui.rc
wininet.rc : *.ico

selfreg_wininet.inf : selfreg.inx
    $(CLEANINF) $** $@
