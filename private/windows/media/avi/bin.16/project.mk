# Clear out any pre-existing suffixes
.SUFFIXES:

# Setup defaults for target directories
PBIN    =$(ROOT)\mciavi32\vfw16\$(DEBUG)
PLIB    =$(ROOT)\mciavi32\vfw16\$(DEBUG)\lib.16
PINC    =$(ROOT)\mciavi32\vfw16\$(DEBUG)\inc.16
PVER    =$(ROOT)\verinfo.16
!ifndef OS
OS	=i386
!endif

# Ensure that environment variables are present
!if [set PATH=;]
!endif
!if [set INCLUDE=;]
!endif
!if [set LIB=;]
!endif
PATH=;
INCLUDE=;
LIB=;

# Ensure ROOT is defined and set tools directories
!ifndef ROOT
!error  "ROOT" environment variable not defined for "$(NAME)".
!endif

DEVROOT =$(ROOT)\bin.16


# Ensure that the project has been defined
!ifndef PROJECT
PROJECT =MTN
!else
!if "$(PROJECT)" == "acm" || "$(PROJECT)" == "ACM"
PROJECT =ACM
!else
!if "$(PROJECT)" == "motown" || "$(PROJECT)" == "MOTOWN" || "$(PROJECT)" == "Motown" || "$(PROJECT)" == "mtn" || "$(PROJECT)" == "MTN" || "$(PROJECT)" == "Mtn"
PROJECT =MTN
!else
!if "$(PROJECT)" == "vfw" || "$(PROJECT)" == "VFW" || "$(PROJECT)" == "VfW"
PROJECT =VFW
!else
!error  Unknown project defined by the "PROJECT" variable: $(PROJECT)
!endif
!endif
!endif
!endif

######## Tool/include paths error checking #######

!ifdef IS_OEM

!ifdef IS_PRIVATE
!error  IS_PRIVATE with IS_OEM is redundant
!endif # IS_PRIVATE

!ifdef IS_SDK
!error  IS_SDK with IS_OEM is redundant
!endif # IS_SDK

!ifdef IS_DDK
!error  IS_DDK with IS_OEM is redundant
!endif # IS_DDK

!endif # IS_OEM

!ifdef IS_16
!ifdef IS_32
!error  Can't define IS_16 with IS_32
!endif # IS_32
!ifdef WANT_16
!error  WANT_16 with IS_16 is redundant
!endif # WANT_16
!endif # IS_16

!ifdef IS_32
!ifdef IS_16
!error  Can't define IS_32 with IS_16
!endif # IS_16
!ifdef WANT_32
!error  WANT_32 with IS_32 is redundant
!endif # WANT_32
!ifdef WANT_286
!error  Can't define WANT_286 with IS_32
!endif # WANT_286
!endif # IS_32

############### Tool/include paths ###############

!ifdef IS_OEM
IS_PRIVATE      =TRUE
IS_SDK          =TRUE
IS_DDK          =TRUE
!endif # IS_OEM

PATH    =$(DEVROOT)
INCLUDE =$(ROOT)\inc.16
LIB     =$(ROOT)\lib.16


# Get rid of the ";;" at the begining of these variables
# just in case one of our tools croaks on it.

PATH    =$(PATH:;;=)
LIB     =$(LIB:;;=)
INCLUDE =$(INCLUDE:;;=)

########## Definitions for the Assembler ##########

!ifdef IS_16
AFLAGS  =$(AFLAGS) -DIS_16
!else
AFLAGS  =$(AFLAGS) -DIS_32
!endif

!ifdef MASM6
ASM     =mlx
!ifdef NOLOGO
ASM     =$(ASM) -nologo
!endif # ifdef NOLOGO
!ifdef CODE
AFLAGS  =$(AFLAGS) -Fl$(@B)
!endif # ifdef CODE
AFLAGS  =$(AFLAGS) -W3 -WX -Zd -c -Cx -DMASM6
!else
ASM     =masm
!ifdef CODE
AFLAGS  =$(AFLAGS) -l
!endif # ifdef CODE
AFLAGS  =$(AFLAGS) -t -W2 -Zd -Mx
!endif

########## Definitions for C compiler #############

CL      =cl
!ifdef IS_16
CFLAGS  =$(CFLAGS) -W3 -WX -Zdp -Gs -c -DIS_16 -D$(PROJECT)
!ifndef WANT_C816NOT
CFLAGS  =$(CFLAGS) -Z7
!endif # WANT_C816NOT
!ifdef WANT_286
CFLAGS  =$(CFLAGS) -G2
!else
CFLAGS  =$(CFLAGS) -G3
!endif # WANT_286
!else  # ifdef IS_16
CFLAGS  =$(CFLAGS) -W3 -WX -Zp -Fd$(@B) -Gs -c -DIS_32 -D$(PROJECT)
!if "$(DEBUG)" == "internal"
CFLAGS  =$(CFLAGS) -Zi
!endif
!endif # ifdef IS_16 ... else

!ifdef CODE
CFLAGS  =$(CFLAGS) -Fc$(@B)
!endif # ifdef CODE

!ifdef NOLOGO
CL      =$(CL) -nologo
!endif # ifdef NOLOGO

########## Definitions for VxD linker #################

!ifndef DEVICEEXT
DEVICEEXT=vxd
!endif

LINK    =link386
!ifdef NOLOGO
LINK    =$(LINK) /NOLOGO
!endif
LFLAGS  =$(LFLAGS) /L /MAP /NOI /NOD /NOPACKCODE /NOE

########## Definitions for 16 bit linker #################

LINK16  =link
!ifdef NOLOGO
LINK16  =$(LINK16) /NOLOGO
!endif
L16FLAGS=$(L16FLAGS) /MAP /NOPACKCODE /NOE /NOD
!ifndef NOMAPLINES
L16FLAGS=$(L16FLAGS) /L
!endif

########## Definitions for 32 bit linker #################

#ifdef WANT_C932LINK
LINK32	=$(DEVROOT)\tools\c932\bin\link -link
#else
LINK32  =link -link
#endif

!ifdef NOLOGO
LINK32  =$(LINK32) -nologo
!endif
L32FLAGS=$(L32FLAGS) -nodefaultlib -align:0x1000

!ifdef FIXEDLINK
L32FLAGS=$(L32FLAGS) -fixed
!endif

########## Definitions for generic linker #################

#!ifdef IS_16
#LINK   =$(LINK16)
#!else
#LINK   =$(LINK32)
#!endif

########## Definitions for rc #####################

RC      =rc
RCFLAGS =$(RCFLAGS) -r -D$(PROJECT)
!ifdef IS_16
RCFLAGS =$(RCFLAGS) -dIS_16
!else
RCFLAGS =$(RCFLAGS) -dIS_32
!endif
RESFLAGS=$(RESFLAGS) -t
!if "$(PROJECT)" != "MTN"
RESFLAGS=$(RESFLAGS) -31
!endif
RESONEXE=resonexe

########## Definitions library manager ############

!ifndef LB
!if "$(IS_32)" != "" && "$(IS_DDK)" != ""
LB      =$(DEVROOT)\tools\c816\bin\lib
!else
LB      =lib
!endif
!endif
!ifdef NOLOGO
LB      =$(LB) /nologo
!endif
LBFLAGS =$(LBFLAGS)

########## Definitions for mapsym #################

MAPSYM  =mapsym
!ifdef NOLOGO
MAPSYM  =$(MAPSYM) -nologo
!endif
MFLAGS  =$(MFLAGS) -s


########## Goals ###############################
goal:	$(GOALS)
	@echo ***** Finished making $(DEBUG) $(NAME) *****

########## Suffixes ###############################

{}.exe{$(PBIN)}.exe:
	copy $(@F) $@

{}.vbx{$(PBIN)}.vbx:
	copy $(@F) $@

{}.dll{$(PBIN)}.dll:
	copy $(@F) $@

{}.mmh {$(PBIN)}.mmh:
	copy $(@F) $@

{}.drv{$(PBIN)}.drv:
	copy $(@F) $@

{}.386{$(PBIN)}.386:
	copy $(@F) $@

{}.vxd{$(PBIN)}.vxd:
	copy $(@F) $@

{}.cpl{$(PBIN)}.cpl:
	copy $(@F) $@

{..\..}.reg{$(PBIN)}.reg:
	copy ..\..\$(@F) $@

{..\..}.h{$(PINC)}.h:
	copy ..\..\$(@F) $@

{..\..}.hxx{$(PINC)}.hxx:
	copy ..\..\$(@F) $@

{..\..}.inc{$(PINC)}.inc:
	copy ..\..\$(@F) $@

{}.lib{$(PLIB)}.lib:
	copy $(@F) $@

{}.sym{$(PBIN)}.sym:
	copy $(@F) $@

{}.tsk{$(PBIN)}.tsk:
	copy $(@F) $@

{}.acm{$(PBIN)}.acm:
	copy $(@F) $@

{}.mci{$(PBIN)}.mci:
	copy $(@F) $@

{..\..}.rc{$(PINC)}.rc:
	copy ..\..\$(@F) $@

{}.res{}.rbj:
	@$(CVTRES) $(CRFLAGS) -o $(@B).rbj $(@B).res

{..\..}.idf{$(PBIN)}.idf:
	copy ..\..\$(@F) $@

{}.inx{$(PBIN)}.inf:
	copy $(@B).inx $@

{..\..}.inf{$(PBIN)}.inf:
	copy ..\..\$(@F) $@

{..\..}.pat{$(PBIN)}.pat:
	copy ..\..\$(@F) $@

{}.dlg{$(PINC)}.dlg:
	copy $(@F) $@

{..\..\}.dlg{$(PINC)}.dlg:
	copy ..\..\$(@F) $@

{}.rc{}.rbj:
	$(rc) $(INC) $(rcvars) -fo $(@B).res $(@B).rc
	cvtres $(@B).res -o $@

{..\..}.asm{}.obj:
	$(ASM) $(AFLAGS) ..\..\$(@B),$@;

{..\..}.wav{$(PBIN)}.wav:
	copy ..\..\$(@F) $@

{}.wav{$(PBIN)}.wav:
	copy $(@F) $@

{..\..}.mid{$(PBIN)}.mid:
	copy ..\..\$(@F) $@

{}.mid{$(PBIN)}.mid:
	copy $(@F) $@

{..\..}.avi{$(PBIN)}.avi:
	copy ..\..\$(@F) $@

{}.avi{$(PBIN)}.avi:
	copy $(@F) $@

{..\..}.ini{$(PBIN)}.ini:
	copy ..\..\$(@F) $@

{dos}.drv{$(PBIN)}.drv:
	copy dos\$(@F) $@

{dos}.dll{$(PBIN)}.dll:
	copy dos\$(@F) $@

{i386}.drv{$(PBIN)}.drv:
	copy i386\$(@F) $@

{i386}.dll{$(PBIN)}.dll:
	copy i386\$(@F) $@

!if "$(OS)" == "Windows_NT"

{..\..}.c{}.obj:
	@$(CC) $(CFLAGS) ..\..\$(@B).c

{..\..}.cpp{}.obj:
	@$(CC) $(CFLAGS) ..\..\$(@B).cpp

{..\..}.cxx{}.obj:
	@$(CC) $(CFLAGS) ..\..\$(@B).cxx

!else

{..\..}.c{}.obj:
	@$(CL) @<<
$(CFLAGS)
..\..\$(@B).c
<<

{..\..}.cpp{}.obj:
	@$(CL) @<<
$(CFLAGS)
..\..\$(@B).cpp
<<

{..\..}.cxx{}.obj:
	@$(CL) @<<
$(CFLAGS)
..\..\$(@B).cxx
<<

!endif	# "$(OS)" != "Windows_NT"

.SUFFIXES: .asm .c .cpp .exe .vbx .dll .drv .386 .vxd .cpl .hlp .reg .h .inc .lib .sym .tsk .acm .rc .mci .res .idf .inx .inf .dlg .mmh .pat .wav .mid .avi .ini .cxx .hxx

########## Library goal ###########################

!ifdef IS_16
!if "$(EXT)" == "dll" || "$(EXT)" == "DLL" || "$(EXT)" == "drv" || "$(EXT)" == "DRV"
$(NAME).lib:	$$(@B).$(EXT)
!ifdef DEFFILE
	@mkpublic ..\..\$(DEFFILE).def $(NAME)
!else
	@mkpublic ..\..\$(NAME).def $(NAME)
!endif
	@implib $@ $(NAME)
!else
!if "$(EXT)" == "lib" || "$(EXT)" == "LIB"
$(NAME).lib:	$(OBJS)
	@if exist $@ del $@ >nul
	@$(LB) $(LBFLAGS) @<<
$@
y
$(OBJS);
<<
!endif
!endif
!else   # ifdef IS_16
!if "$(EXT)" == "lib" || "$(EXT)" == "LIB"
$(NAME).lib:	$(OBJS)
	@if exist $@ del $@ >nul
	@$(LB) $(LBFLAGS) @<<
-out:$@
$(OBJS)
<<

!endif
!endif

########## Map goal ###############################

#!if (("$(DEBUG)" == "debug") || ("$(DEBUG)" == "retail")) && exists($(NAME).grp)
#$(NAME).sym:	$$(@B).map debug.sym
#	@qgrep -f ..\..\$(@B).grp $(@B).map >retail.map
#	@$(MAPSYM) $(MFLAGS) -mo $@ retail.map
#
#debug.sym:	$(NAME).map
#!else
$(NAME).sym:	$$(@B).map
#!endif
	@$(MAPSYM) $(MFLAGS) -mo $@ $(NAME).map
