################ ROOT definitions ################

!IFNDEF ROOT
!ERROR ROOT environment variable not defined.
!ENDIF

!IFNDEF MSVCROOT
!ERROR MSVCROOT environment variable not defined.
!ENDIF

################ PATH definitions ################


!IF [set PATH=;]
!ENDIF
!IF [set INCLUDE=;]
!ENDIF
!IF [set LIB=;]
!ENDIF

PATH=$(MSVCROOT)\bin;$(ROOT)\dev\tools\common
INCLUDE=$(ROOT)\dev\inc;$(MSVCROOT)\include;$(MSVCROOT)\mfc\include;$(ROOT)\dev\inc16
LIB=$(ROOT)\dev\lib;$(MSVCROOT)\lib;$(MSVCROOT)\mfc\lib

DSTDIR=winrel

!ifndef BUILD
BUILD=debug
!endif


default: $(BUILD)


all:  debug  retail


forcebld.tgt:
   @rem bitme



debug retail: forcebld.tgt



debug:
  md debug
  cd debug
  $(MAKE) -f ..\fontreg.mk  DEBUG=1
  cd ..



retail:
  md retail
  cd retail
  $(MAKE) -f ..\fontreg.mk
  cd ..

