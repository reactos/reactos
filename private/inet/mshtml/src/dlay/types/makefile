!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\dlay\types=)
MODULE=dlaytype
INCLUDEDIRS=$(SRCDIR)\
        $(SRCROOT)\core\include\
        $(SRCROOT)\dlay\include\
        $(SRCROOT)\core\types\
        $(SRCROOT)\ctrl\types\
        $(TOOLSINCLUDE)\

!include $(ROOT)\src\f3\make\make1.inc


IDLTARGS=$(SDKDIR)\std.h $(BLDDIR)\std_i.c $(BLDDIR)\std_p.c $(BLDDIR)\std_d.c

all :\
        $(SDKDIR)\std.h \
        $(BLDDIR)\std_i.c \
        $(BLDDIR)\std_p.c \
        $(BLDDIR)\std_d.c \

depend:: \
        $(SDKDIR)\std.h      \

clean::
    $(DEL) $(SDKDIR)\std.h

# -------------------------------------------------------------
#  MIDL
# -------------------------------------------------------------

PRINTMSG=
WARN0=
!ifdef SILENTMODE
PRINTMSG=$(ECHO) Building $@...
WARN0=/w0
!endif

$(BLDDIR)\std.h $(BLDDIR)\std_i.c $(BLDDIR)\std_p.c $(BLDDIR)\std_d.c : std.idl
    SET INCLUDE=$(INCLUDEPATH)
    $(PRINTMSG)
    $(MIDL) /ms_ext /c_ext\
        -Zp8\
        -cpp_cmd $(CC)\
        -cpp_opt "-nologo /DMUNGETYPLIB=1 /DMIDL_COMPILER $(DEFINES) -E -Tc"\
        -no_warn\
        -char unsigned\
        -header $(BLDDIR)\std.h\
        -proxy $(BLDDIR)\std_p.c\
        -dlldata $(BLDDIR)\std_d.c\
        -iid $(BLDDIR)\std_i.c\
        std.idl

$(SDKDIR)\std.h : $(BLDDIR)\std.h std.sed
    $(SED) -f std.sed <$(BLDDIR)\std.h >$(SDKDIR)\std.h


!include $(MAKE2_INC)

# -------------------------------------------------------------
#  Extra dependencies
# -------------------------------------------------------------

depend::
    $(MKDEP) -n /I $(INCLUDEDIRS: = /I ) -s.h -P $(BLDDIR)\ std.idl >> $(BLDDIR)\depend.inc


!endif		# NTMAKEENV
