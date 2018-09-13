!if "$(NTMAKEENV)" != ""
!INCLUDE $(NTMAKEENV)\makefile.def
!else


#===========================================================================;
#
#   LANGPLAY -- Movie Player Application that handles multiple langauge tracks
#
#############################################################################
#
#   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
#   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
#   PURPOSE.
#
#   Copyright (c) 1992, 1993  Microsoft Corporation.  All Rights Reserved.
#
#############################################################################
#   to make a NON-DEBUG build, type the following line:
#       nmake DEBUG=NO
#   just typing 'nmake' will build a DEBUG build
#
#   You can also set the environment variable DEBUG to NO
#
#===========================================================================;

TARGETNAME  = langplay
TARGETEXT   = exe
TARGETTYPE  = WINAPP            # WINAPP[ST], WINDLL
TARGETMODEL = MEDIUM            # SMALL, MEDIUM
TARGETAPI   = -DWIN31           # -DWIN30, -DWIN31, -DWIN4

TARGETWARN  = -W3



########## Platform dependence ############

WIN16       = 1
TARGETOPTZ  = -Ox -G2sw $(TARGETWARN)


!if ("$(TARGETMODEL)" != "SMALL") && ("$(TARGETMODEL)" != "MEDIUM")
!error Invalid TARGETMODEL environment variable: specify SMALL or MEDIUM!
!endif


#### Default executable and flag declarations ####

CVARS       = -DWIN16 -DSTRICT $(TARGETAPI)

CC          = cl
CFLAGS      = -c -Zp $(TARGETOPTZ)

RC          = rc
RFLAGS      =

LN          = link
LFLAGS      = /NOPACKC/NOD/NOE/MAP/AL:16/ONERROR:NOEXE

LB          = lib
LBFLAGS     =

CRFLAGS     =


!if "$(DEBUG)" != "NO"
CVARS       = $(CVARS) -DDEBUG
CFLAGS      = $(CFLAGS) -Zid
LFLAGS      = $(LFLAGS) /CO/LI
!endif



############ Standard library stuff ##############
#
#   WINLFLAGS:

WINLIBSBASE = libw commdlg shell mmsystem vfw

!if "$(TARGETMODEL)" == "SMALL"
WINLIBSST   = slibcew.lib $(WINLIBSBASE)
WINLIBSMT   =
WINLIBSDLL  = sdllcew.lib $(WINLIBSBASE)
!endif

!if "$(TARGETMODEL)" == "MEDIUM"
WINLIBSST   = mlibcew.lib $(WINLIBSBASE)
WINLIBSMT   =
WINLIBSDLL  = mdllcew.lib $(WINLIBSBASE)
!endif

#
#
#
#

!if ("$(TARGETTYPE)" == "WINAPPST") || ("$(TARGETTYPE)" == "WINAPP")
!if "$(TARGETMODEL)" == "SMALL"
CFLAGS      = $(CFLAGS) -AS
!endif
!if "$(TARGETMODEL)" == "MEDIUM"
CFLAGS      = $(CFLAGS) -AM
!endif
CVARS       = $(CVARS)
WINLIBS     = $(WINLIBSST)
!endif

!if "$(TARGETTYPE)" == "WINDLL"
!if "$(TARGETMODEL)" == "SMALL"
CFLAGS      = $(CFLAGS) -Asnw
!endif
!if "$(TARGETMODEL)" == "MEDIUM"
CFLAGS      = $(CFLAGS) -Alnw
!endif
CVARS       = $(CVARS) -D_DLL -D_WINDLL
WINLIBS     = $(WINLIBSDLL)
!endif
TARGETLIBS  = $(WINLIBS)
TARGETDEF   = $(TARGETNAME)16




############ Goals and objects ############

GOALS   = $(TARGETNAME).$(TARGETEXT)

OBJ1    = $(TARGETNAME).obj


goal: $(GOALS)
        @echo ***** Finished building $(TARGETNAME).$(TARGETEXT)  *****



################ Build rules ##############

$(TARGETNAME).$(TARGETEXT): $(OBJ1) $(TARGETNAME).res $(TARGETNAME).def
    $(LN) @<<
$(LFLAGS) $(WINLFLAGS)+
$(OBJ1),
$(@R).$(TARGETEXT),
$(@R).map,
$(TARGETLIBS),
$(TARGETNAME).def
<<
        mapsym $(@R).map
        $(RC) -t -31 $(TARGETNAME).res $(@R).$(TARGETEXT)



$(TARGETNAME).res: $(TARGETNAME).rc $(TARGETNAME).ico $(TARGETNAME).h
        $(RC) -r $(RFLAGS) $(CVARS) -Fo $@ $(TARGETNAME).rc


$(TARGETNAME).obj: $$(@B).c $(TARGETNAME).h
        $(CC) $(CFLAGS) $(CVARS) -Fo$@ $(@B).c




############### Clean ###############

clean:
        @echo y >y.zyz
        @del *.exe <y.zyz >nul
        @del *.res <y.zyz >nul
        @del *.map <y.zyz >nul
        @del *.sym <y.zyz >nul
        @del *.obj <y.zyz >nul
        @del *.pdb <y.zyz >nul
        @del y.zyz
        @echo ***** Finished cleaning $(TARGETNAME) *****

copy:
	copy $(TARGETNAME).$(TARGETEXT) ..\..\bin
	copy $(TARGETNAME).sym ..\..\bin

!endif
