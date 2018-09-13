#MIDL= $(DEVTOOLS)\midl.exe

PASS0_HEADERDIR =$(O)
PASS0_SOURCEDIR =$(O)
MIDL_UUIDDIR    =$(O)

$O\OptsHold.idl : $(ROOT)\src\edit\OptsHold.idl
    copy $** $@

$O\OptsHold.h $O\OptsHold.tlb libmain.cxx : OptsHold.idl

#this is not final yet
$O\optshold.tlb : $O\optshold.idl
    $(MIDL) \
!ifndef WIN16
    -Zp8 \
!else
    /Zp2 /DWIN16 /DDOS /DWIN $(MIDL_FLAGS) \
!endif
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $O\optshold_p.c \
    -dlldata $O\dlldata.c \
    -iid $O\optshold_i.c \
    -header $O\optshold.h \
    -cpp_cmd $(TARGET_CPP) \
    -tlb $(O)\optshold.tlb \
    $(C_DEFINES) \
    $(MIDL_OPTIMIZATION) \
    $O\optshold.idl







