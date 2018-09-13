!IFNDEF BASEDIR
BASEDIR=$(_NTDRIVE)$(_NTROOT)
!ENDIF

!IFNDEF SDKINC
SDKINC=$(BASEDIR)\public\sdk\inc
!ENDIF

unsup.h: unsup.w
    wcshdr < unsup.w > $@
