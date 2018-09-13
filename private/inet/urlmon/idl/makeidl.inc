# DEST_TREE is set by the calling makefile

UMON_DEST=..\inc\$(DEST_TREE)

!ifndef MIDL
MIDL = midl.exe
!endif

#
# BUGBUG - We want to compile these IDL files /Oi but the interpretor
# cannot handle error_status_t parameters.  When it can, change /Os to /Oi
#
MIDL_FLAGS= \
 -Zp8 \
 -I$(INCLUDES) \
 -Os \
 -oldnames \
 -char unsigned \
 -error allocation \
 -ms_ext -c_ext \
 $(C_DEFINES) \
 -cpp_cmd $(TARGET_CPP)

CSWITCH=-prefix cstub _

urlmon.h:  ..\urlmon.idl
     $(MIDL) $(MIDL_FLAGS) \
        -cstub $(UMON_DEST)\umon_c.c    \
        -sstub $(UMON_DEST)\umon_s.c    \
        ..\urlmon.idl

allidl:   urlmon.h

clean:
          erase urlmon.h   >NUL 2>NUL
