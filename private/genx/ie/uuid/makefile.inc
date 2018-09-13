clean:
    -del /F /Q $(NTTARGETFILE0)
    -del *_i.c

GUIDLIB_INCLUDES = /I$(SDK_INC_PATH) /I$(CRT_INC_PATH) /I$(BASEDIR)\private\inc /I$(TYPESUUID)

#
# Inference rules.
#

.SUFFIXES: .gc

.c{$O}.gc:
    copy $? $@

!if $(AXP64)
GUID_CPL_OPTIONS=/cpl_opt:" /qap64"
!else
GUID_CPL_OPTIONS=
!endif

{$O}.gc{$O}.lib:
    guidlib /Out:$@ $(GUID_CPL_OPTIONS) /CPP_OPT:"$(GUIDLIB_INCLUDES)" $?

