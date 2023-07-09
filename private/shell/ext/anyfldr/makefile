NAME = OtherFld
#WIN32 = TRUE
BUILDDLL = TRUE
DLLENTRY = DllEntryPoint
RES_DIR = ..
ROOT = ..\..\..\..

!ifdef VERDIR
ROOT=..\$(ROOT)
WIN32=TRUE
!else
VERSIONLIST=debug retail
!endif

RCFLAGS=$(RCFLAGS) -DWIN32

MISCOBJ0=debug.obj defclsf.obj
PCHOBJ0=otherfld.obj
# STATOBJ0=

LIB0= user32.lib kernel32.lib gdi32.lib comctl32.lib advapi32.lib shell32.lib mpr.lib libcmt.lib
LIB1= uuid.lib

!include $(ROOT)\win\shell\common.mk
