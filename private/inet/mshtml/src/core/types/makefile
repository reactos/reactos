ROOT=$(MAKEDIR:\src\core\types=)
MODULE=coretype
INCLUDEDIRS=$(SRCDIR)\
    $(SRCROOT)\core\include\
    $(SRCROOT)\core\types\
    $(TYPEDIR)\
    $(ROOT)\help\
    $(TOOLSINCLUDE)\
    $(TOOLSMISCINCLUDE)\
    $(SRCROOT)\..

!include $(ROOT)\src\f3\make\make1.inc

TYPETARG= \
    $(TYPEDIR)\types.hdl \
    $(TYPEDIR)\dataobj.hdl 

!if [$(MKDIR) $(SDKDIR)]
!endif

!if [$(MKDIR) $(BINDIR)]
!endif

all: $(TYPETARG)

!include $(MAKE2_INC)
