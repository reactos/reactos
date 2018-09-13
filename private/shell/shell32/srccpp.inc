!IF 0

Copyright (c) 1989-1996 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Common ccshell makefile and cleanup 15-Apr-96 Scott Hysom (scotth)
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

!include $(CCSHELL_DIR)\shell32\shell32.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\shell32\shell32.inc

NO_BROWSER_FILE = 1

PRECOMPILED_CXX = 1

TARGETNAME      = srccpp
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

SOURCES         = \
                  ..\atlcreat.cpp  \
                  ..\bitbuck1.cpp  \
                  ..\browse.cpp    \
                  ..\colhndlr.cpp  \
                  ..\combview.cpp  \
                  ..\copyfgd.cpp   \
                  ..\cowsite.cpp  \
                  ..\ctrlfldr.cpp  \
                  ..\dataprv.cpp   \
                  ..\datautil.cpp  \
                  ..\debdump.cpp   \
                  ..\defcm.cpp     \
                  ..\defview.cpp   \
                  ..\defvphst.cpp  \
                  ..\deskfldr.cpp  \
                  ..\dfcmd.cpp     \
                  ..\dll.cpp       \
                  ..\dvtasks.cpp   \
                  ..\docfind.cpp   \
                  ..\docfindx.cpp  \
                  ..\dragdrop.cpp  \
                  ..\drivfldr.cpp  \
                  ..\drvx.cpp      \
                  ..\enum.cpp      \
                  ..\execute.cpp   \
                  ..\exedrop.cpp   \
                  ..\favorite.cpp  \
                  ..\filetype.cpp  \
                  ..\fldrscut.cpp  \
                  ..\foldertip.cpp \
                  ..\fsdrptgt.cpp  \
                  ..\fsearch.cpp   \
                  ..\fsrchdlg.cpp  \
                  ..\fsmenu.cpp    \
                  ..\fstree.cpp    \
                  ..\fstorage.cpp  \
                  ..\ftaction.cpp  \
                  ..\ftadv.cpp     \
                  ..\ftascstr.cpp  \
                  ..\ftassoc.cpp   \
                  ..\ftdlg.cpp     \
                  ..\ftedit.cpp    \
                  ..\ftenum.cpp    \
                  ..\ftprop.cpp    \
                  ..\infotip.cpp   \
                  ..\linkwnd.cpp   \
                  ..\mkhelp.cpp    \
                  ..\mountvol.cpp  \
                  ..\mtpt.cpp      \
                  ..\mtptarun.cpp  \
                  ..\mtptis.cpp    \
                  ..\mtptmgmt.cpp  \
                  ..\mtptr.cpp     \
                  ..\mtptl.cpp     \
                  ..\netdsfnd.cpp  \
                  ..\netfldr.cpp   \
                  ..\olestuff.cpp  \
                  ..\openwith.cpp  \
                  ..\ovrlaymn.cpp  \
                  ..\prnfldr.cpp   \
                  ..\prop.cpp	   \
                  ..\prshtcpp.cpp  \
                  ..\regfldr.cpp   \
                  ..\recclean.cpp  \
                  ..\recdocs.cpp   \
                  ..\regsuprt.cpp  \
                  ..\regsprtb.cpp  \
                  ..\rgsprtc.cpp   \
                  ..\rscchvr.cpp   \
                  ..\rssdnobf.cpp  \
                  ..\rssubdat.cpp  \
                  ..\rundlg.cpp    \
                  ..\security.cpp  \
                  ..\sencrypt.cpp  \
                  ..\sfvcmpt.cpp   \
                  ..\sfvext.cpp    \
                  ..\shcombox.cpp  \
                  ..\shelllnk.cpp  \
                  ..\subdata.cpp   \
                  ..\treewalk.cpp  \
                  ..\treewkcb.cpp  \
                  ..\urlexec.cpp   \
                  ..\util.cpp

PRECOMPILED_INCLUDE = ..\shellprv.h
