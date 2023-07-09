#   THUNK Make file
#
#
# Macros defined on command line:
# DEST   - Destination of obj's.
# CFLAGS - DOS version dependent C compiler flags
# AFLAGS - DOS version dependent assembler flags

!ifdef NTMAKEENV    # Prevent NT's build.exe from executing this makefile

clean :

!else  # NTMAKEENV


DIR             = thunk
ROOT            = ..\..
IS_OEM          = 1

!ifdef VERDIR
ROOT            = ..\$(ROOT)
CLEANLIST       = *.asm
!else
VERSIONLIST     = debug retail
!endif

!ifdef DBCS
THKASM          = Shl3216.asm 
!else
THKASM          = Shl3216.asm Shl1632.asm 
!endif

THKASM_CC       = Cctl1632.asm
THKASM_DLG      = dlgthk.asm

COMMONMKFILE    = makefile

THUNKCOM        = $(ROOT)\dev\tools\binr\thunk.exe
THUNK           = $(THUNKCOM) $(THUNKOPT)

!include $(ROOT)\shell\shell.mk


!ifdef VERDIR

TARGETS         = $(TARGETS) $(THKASM) $(THKASM_CC) $(THKASM_DLG)

!ifdef  DBCS
TARGETS         = $(TARGETS) shl1632.asm
!endif

all build default: $(TARGETS)

#
#  Rules for making thunks
#
INCLUDE         =
WIN32DEV        = $(DEVROOT)

$(THKASM_CC) :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC _TEXT -o $(@B) ..\$(@B).thk

$(THKASM_DLG) : $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC THUNK16B -o $(@B) ..\$(@B).thk


!IFDEF DBCS
Shl1632.asm : ..\Shl1632.thk
    sed  -f ..\shl1632d.sed < ..\Shl1632.thk > Shl1632D.thk
    $(THUNK) -NC _TEXT -o $(@B) shl1632d.thk
    copy $(@B).asm smag.asm
    sed -f ..\shlthk.sed smag.asm > $(@B).asm
    del smag.asm
!ENDIF

$(THKASM) :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC _TEXT -o $(@B) ..\$(@B).thk
    copy $(@B).asm smag.asm
    sed -f ..\shlthk.sed smag.asm > $(@B).asm
    del smag.asm

!endif  # VERDIR

!endif # NTMAKEENV
