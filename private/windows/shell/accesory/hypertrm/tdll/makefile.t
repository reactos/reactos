#  File: D:\WACKER\tdll\makefile.t (Created: 26-Nov-1993)
#
#  Copyright 1993, 1998 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 5 $
#  $Date: 7/01/99 3:09p $
#

.PATH.c=.;\wacker\emu;\wacker\xfer;\wacker\cncttapi;\wacker\comstd;\
        \wacker\ext;\wacker\comwsock;

.PATH.cpp=.;\shared\classes

MKMF_SRCS   = \
	  \wacker\tdll\tdll.c           \wacker\tdll\globals.c      \
	  \wacker\tdll\sf.c             \wacker\tdll\sessproc.c     \
	  \wacker\tdll\misc.c           \wacker\tdll\dodialog.c     \
	  \wacker\tdll\assert.c         \wacker\tdll\sidebar.c      \
	  \wacker\tdll\com.c            \wacker\tdll\comdef.c       \
	  \wacker\tdll\comsend.c        \
	  \wacker\tdll\getchar.c        \wacker\tdll\sesshdl.c      \
	  \wacker\tdll\toolbar.c        \wacker\tdll\statusbr.c     \
	  \wacker\tdll\aboutdlg.c       \wacker\tdll\termproc.c     \
	  \wacker\tdll\tchar.c          \wacker\tdll\update.c       \
	  \wacker\tdll\backscrl.c       \wacker\tdll\termhdl.c      \
	  \wacker\tdll\termupd.c        \wacker\tdll\termpnt.c      \
	  \wacker\tdll\genrcdlg.c       \
	  \wacker\tdll\load_res.c       \wacker\tdll\errorbox.c     \
	  \wacker\tdll\send_dlg.c       \wacker\tdll\timers.c       \
	  \wacker\tdll\open_msc.c       \wacker\tdll\termutil.c     \
	  \wacker\tdll\file_msc.c       \wacker\tdll\recv_dlg.c     \
	  \wacker\tdll\sessmenu.c       \wacker\tdll\cloop.c        \
	  \wacker\tdll\cloopctl.c       \wacker\tdll\cloopout.c     \
	  \wacker\tdll\xfer_msc.c       \wacker\tdll\sessutil.c     \
	  \wacker\tdll\vu_meter.c       \wacker\tdll\xfdspdlg.c     \
	  \wacker\tdll\cncthdl.c        \wacker\tdll\sessfile.c     \
	  \wacker\tdll\cpf_dlg.c        \wacker\tdll\capture.c      \
	  \wacker\tdll\fontdlg.c        \wacker\tdll\print.c        \
	  \wacker\tdll\cncthdl.c        \wacker\tdll\cnctstub.c     \
	  \wacker\tdll\property.c       \wacker\tdll\printhdl.c     \
	  \wacker\tdll\print.c          \wacker\tdll\termcpy.c      \
	  \wacker\tdll\clipbrd.c        \wacker\tdll\prnecho.c      \
	  \wacker\tdll\termmos.c        \wacker\tdll\termcur.c      \
	  \wacker\tdll\printdc.c        \wacker\tdll\printset.c     \
	  \wacker\tdll\file_io.c        \wacker\tdll\new_cnct.c     \
	  \wacker\tdll\asciidlg.c       \wacker\tdll\cloopset.c     \
	  \wacker\tdll\propterm.c								    \
      \wacker\tdll\banner.c         \
	  \wacker\tdll\autosave.c       \wacker\tdll\translat.c     \
	  \wacker\tdll\telnetck.c		\wacker\tdll\registry.c		\
	  \wacker\tdll\upgrddlg.c       \wacker\tdll\hlptable.c     \
	  \wacker\tdll\key_sdlg.c       \wacker\tdll\key_dlg.c      \
	  \wacker\tdll\keymacro.cpp     \wacker\tdll\keymlist.cpp   \
      \wacker\tdll\keyextrn.cpp     \wacker\tdll\keyutil.c      \
      \wacker\tdll\keyedit.c        \wacker\tdll\nagdlg.c       \
      \wacker\tdll\serialno.c       \wacker\tdll\register.c     \
	  \
	  \wacker\emu\emudlgs.c         \wacker\emu\colrdlg.c		\
	  \wacker\emu\emu.c             \wacker\emu\emu_std.c		\
      \
	  \wacker\emu\emu_ansi.c        \wacker\emu\emu_scr.c       \
	  \wacker\emu\vt52.c                                        \
	  \wacker\emu\ansi.c            \wacker\emu\ansiinit.c      \
	  \wacker\emu\vt100.c           \wacker\emu\vt_xtra.c       \
	  \wacker\emu\emuhdl.c          \wacker\emu\vt100ini.c      \
	  \wacker\emu\vt_chars.c        \wacker\emu\vt52init.c      \
	  \wacker\emu\viewdini.c        \wacker\emu\viewdata.c      \
	  \wacker\emu\autoinit.c        \wacker\emu\minitel.c       \
	  \wacker\emu\minitelf.c        \
	  \wacker\emu\vt220ini.c		\wacker\emu\vt220.c			\
	  \
	  \wacker\xfer\x_kr_dlg.c       \wacker\xfer\xfr_todo.c     \
	  \wacker\xfer\xfr_srvc.c       \wacker\xfer\xfr_dsp.c      \
	  \wacker\xfer\x_entry.c        \wacker\xfer\x_params.c     \
	  \wacker\xfer\itime.c			\
	  \wacker\xfer\foo.c            \wacker\xfer\zmdm.c         \
	  \wacker\xfer\zmdm_snd.c       \wacker\xfer\zmdm_rcv.c     \
	  \wacker\xfer\mdmx.c			\
	  \wacker\xfer\mdmx_sd.c        \wacker\xfer\mdmx_res.c     \
	  \wacker\xfer\mdmx_crc.c       \wacker\xfer\mdmx_rcv.c     \
	  \wacker\xfer\mdmx_snd.c       \wacker\xfer\krm.c          \
	  \wacker\xfer\krm_res.c        \wacker\xfer\krm_rcv.c      \
	  \wacker\xfer\krm_snd.c        \wacker\xfer\x_xy_dlg.c     \
	  \wacker\xfer\x_zm_dlg.c		\
	  \
	  \wacker\cncttapi\cncttapi.c   \wacker\cncttapi\dialdlg.c  \
	  \wacker\cncttapi\enum.c       \wacker\cncttapi\cnfrmdlg.c \
	  \wacker\cncttapi\phonedlg.c   \wacker\cncttapi\pcmcia.c   \
	  \
	  \wacker\comstd\comstd.c       \
	  \
	  \wacker\comwsock\comwsock.c   \wacker\comwsock\comnvt.c   \
	  \
	  \wacker\ext\pageext.c         \wacker\ext\fspage.c        \
          \wacker\ext\defclsf.c         \
	  \

HDRS		=

EXTHDRS		=

SRCS		=

OBJS		=
#-------------------#

RCSFILES = \wacker\tdll\makefile.t              \wacker\tdll\tdll.def           \
		   \wacker\tdll\sess_ids.h              \wacker\term\term.rc            \
		   \wacker\term\tables.rc               \wacker\term\dialogs.rc         \
		   \wacker\emu\emudlgs.rc               \wacker\term\buttons.bmp        \
		   \wacker\term\test.rc                 \wacker\term\banner.bmp         \
		   \wacker\cncttapi\cncttapi.rc         \
		   \wacker\term\newcon.ico              \wacker\term\delphi.ico         \
		   \wacker\term\att.ico                 \wacker\term\dowjones.ico       \
		   \wacker\term\mci.ico                 \wacker\term\genie.ico          \
		   \wacker\term\compuser.ico            \wacker\term\gen01.ico          \
		   \wacker\term\gen02.ico               \wacker\term\gen03.ico          \
		   \wacker\term\gen04.ico               \wacker\term\gen05.ico          \
		   \wacker\term\gen06.ico               \wacker\term\gen07.ico          \
		   \wacker\term\gen08.ico               \wacker\term\gen09.ico          \
		   \wacker\term\gen10.ico               \wacker\term\s_delphi.ico       \
		   \wacker\term\s_newcon.ico            \wacker\term\s_att.ico          \
		   \wacker\term\s_dowj.ico              \wacker\term\s_mci.ico          \
		   \wacker\term\s_genie.ico             \wacker\term\s_compu.ico        \
		   \wacker\term\s_gen01.ico             \wacker\term\s_gen02.ico        \
		   \wacker\term\s_gen03.ico             \wacker\term\s_gen04.ico        \
		   \wacker\term\s_gen05.ico             \wacker\term\s_gen06.ico        \
		   \wacker\term\s_gen07.ico             \wacker\term\s_gen08.ico        \
		   \wacker\term\s_gen09.ico             \wacker\term\s_gen10.ico        \
		   \wacker\tdll\features.h              \wacker\term\sbuttons.bmp       \
           \wacker\term\htperead.doc            \wacker\nih\htpesess.exe        \
	       \wacker\term\globe.avi               \wacker\term\htpebnr.bmp        \
		   \wacker\term\banner1.bmp		        \wacker\term\htpeupgd.rtf       \
           \wacker\term\htntupgd.rtf	        \wacker\term\orderfrm.doc       \
           \wacker\term\htperead.htm            \wacker\term\image1.gif		    \
           \wacker\term\orderfrm.htm            \
           \
           \wacker\help\hyper_pr.rtf            \wacker\help\hypertrm.rtf       \
           \wacker\help\hypertrm.cnt            \wacker\help\hypertrm.hpj       \
           \wacker\help\cshelp.bmp              \
           \wacker\help\hypertrm.hlp            \wacker\term\htpe3bnr.bmp		\
           \wacker\help\hypertrm.chm            \
           \
           \wacker\setup\sessions\htpesess.zip  \
           \wacker\setup\ARIALALT.TTF           \wacker\setup\ARIALALS.TTF      \
           \wacker\setup\HTPE3.WSE              \wacker\setup\htorder.wse       \
           \wacker\setup\htpestub.wse           \wacker\setup\globe.bmp        \
           \wacker\setup\globtext.bmp           \
		   \wacker\nih\msvcrt.dll  \
           \wacker\nih\msvcirt.dll \
           $(SRCS) $(EXTHDRS)

NOTUSED  =  bv_text.c frameprc.c pre_dlg.c      \wacker\tdll\propgnrl.c         \
	        \wacker\emu\emustate.c              \wacker\emu\emudisp.c           \
	        \wacker\tdll\mc.c                   \wacker\tdll\propcolr.c			\
			\wacker\setup\build.bat												\
            \wacker\setup\setup\setup.rul       \wacker\setup\setup\setup.lst   \
            \wacker\term\htperead.txt           \wacker\term\orderfrm.txt       \
	\wacker\setup\FINISHED.DLG      \
           \wacker\setup\WELCOME.DLG            \wacker\setup\README.DLG        \
           \wacker\setup\CHOOSE.DLG             \wacker\setup\GROUP.DLG         \
           \wacker\setup\READY.DLG              \wacker\setup\RADIO.DLG         \
           \wacker\setup\Compnent.dlg           \
           \wacker\setup\where.dlg              \
           \wacker\emu\vt100.hh                    \wacker\emu\vt100.hh         \
           \wacker\tdll\cscript.h               \
           \wacker\tdll\cscript.cpp             \
           \wacker\nih\shmalloc.h               \wacker\nih\smrtheap.h          \
           \wacker\nih\Shdw32md.lib             \wacker\nih\Sh22w32d.dll        \


#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : \wacker\tdll\ver_dll.i hypertrm.dll hypertrm.exp hypertrm.lib

#-------------------#

CFLAGS += /Fd$(BD)\hypertrm

%if defined(USE_BROWSER) && $(VERSION) == WIN_DEBUG
CFLAGS += /Fr$(BD)/
TARGETS : hypertrm.bsc
%endif

%if defined(MAP_AND_SYMBOLS)
TARGETS : hypertrm.sym
%endif

LFLAGS += /DLL /entry:TDllEntry $(**,M\.res) /PDB:$(BD)\hypertrm \
        /NODEFAULTLIB:libcmt.lib \
	  user32.lib gdi32.lib kernel32.lib msvcrt.lib winspool.lib \
	  tapi32.lib shell32.lib uuid.lib comdlg32.lib advapi32.lib \
	  comctl32.lib wsock32.lib ole32.lib oleaut32.lib \
	  \wacker\nih\htmlhelp.lib

#-------------------#

\wacker\tdll\ver_dll.i : \wacker\term\ver_dll.rc
    @cl /nologo /P /D${VERSION} /Tc\wacker\term\ver_dll.rc

hypertrm.dll + hypertrm.exp + hypertrm.lib .MISER : $(OBJS) tdll.def term.res
    @echo Linking $(@,F) ...
    @link $(LFLAGS) $(OBJS:X) /DEF:tdll.def -out:$(@,M\.dll)
    @(cd $(BD) $; bind hypertrm.dll)

hypertrm.bsc : $(OBJS,.obj=.sbr)
    @echo Building browser file $(@,F) ...
    @bscmake /nologo /o$@ $(OBJS,X,.obj=.sbr)

hypertrm.sym : hypertrm.map
	mapsym -o $@ $**

#-------------------#

%if $(VERSION) == WIN_RELEASE
RC_DEFS = /DNDEBUG 
%endif

term.res .MISER : \
	   \wacker\term\term.rc          \wacker\term\res.h             \
	   \wacker\term\tables.rc        \wacker\term\dialogs.rc        \
	   \wacker\emu\emudlgs.rc        \wacker\term\ver_dll.rc        \
	   \wacker\cncttapi\cncttapi.rc  \wacker\term\test.rc           \
	   \wacker\term\buttons.bmp      \wacker\term\banner.bmp        \
	   \wacker\term\sbuttons.bmp     \wacker\term\globe.avi         \
       \wacker\term\htntupgd.rtf     \wacker\term\htpeupgd.rtf
    @echo compiling resources
    # Changed to term dir to build rc files.  This accommadates changes
    # made to the rc files by Microsoft. - mrw:10/20/95
    @(cd \wacker\term $; rc $(RC_DEFS) $(EXTRA_DEFS) /D$(BLD_VER) /DWIN32 \
    /D$(LANG) -i\wacker -fo$@ term.rc)

#-------------------#
### OPUS MKMF:  Do not remove this line!  Generated dependencies follow.

