PROJ = np
PROJFILE = np.mak
DEBUG = 1

PWBRMAKE  = pwbrmake
NMAKEBSC1  = set
NMAKEBSC2  = nmake
CC  = cl
CFLAGS_G  = /AM /W3 /Gw /DWIN16 /Zp /BATCH -P
CFLAGS_D  = /Zi /Gi$(PROJ).mdt /Od /Gs /FPa
CFLAGS_R  = /Oe /Og /Os /Gs /FPa
MAPFILE_D  = NUL
MAPFILE_R  = NUL
LFLAGS_G  = /BATCH
LFLAGS_D  = /CO /NOF
LFLAGS_R  = /NOF
LLIBS_G  = LIBW.LIB
LINKER	= link
ILINK  = ilink
LRF  = echo > NUL
RC  = rc

DEF_FILE  = PBRUSH.DEF
OBJS  = ABORTDLG.obj abortprt.obj ABOUTDLG.obj AIRBRUDP.obj ALLOCIMG.obj\
	BRUSHDLG.obj BRUSHDP.obj CALCVIEW.obj CALCWNDS.obj CLEARDLG.obj\
	COLERADP.obj COLORDLG.obj filedlg.obj FIXEDPT.obj FLIPPOLY.obj\
	fontmenu.obj INFODLG.obj initglob.obj LCUNDODP.obj LINEDP.obj\
	newpick.obj OFFSPOLY.obj OVALDP.obj packbuff.obj POLYTO.obj\
	printdlg.obj printdp.obj printimg.obj scrolimg.obj scrolmag.obj\
	setcurs.obj SETTITLE.obj unpkbuff.obj updatimg.obj validhdr.obj\
	VTOOLS.obj xorcsr.obj ZOOMINDP.obj ZOOMINWP.obj zoomotwp.obj
RESS  = PBRUSH.res
SBRS  = ABORTDLG.sbr abortprt.sbr ABOUTDLG.sbr AIRBRUDP.sbr ALLOCIMG.sbr\
	BRUSHDLG.sbr BRUSHDP.sbr CALCVIEW.sbr CALCWNDS.sbr CLEARDLG.sbr\
	COLERADP.sbr COLORDLG.sbr filedlg.sbr FIXEDPT.sbr FLIPPOLY.sbr\
	fontmenu.sbr INFODLG.sbr initglob.sbr LCUNDODP.sbr LINEDP.sbr\
	newpick.sbr OFFSPOLY.sbr OVALDP.sbr packbuff.sbr POLYTO.sbr\
	printdlg.sbr printdp.sbr printimg.sbr scrolimg.sbr scrolmag.sbr\
	setcurs.sbr SETTITLE.sbr unpkbuff.sbr updatimg.sbr validhdr.sbr\
	VTOOLS.sbr xorcsr.sbr ZOOMINDP.sbr ZOOMINWP.sbr zoomotwp.sbr

all: $(PROJ).exe

.SUFFIXES:
.SUFFIXES: .sbr .obj .res .c .rc

ABORTDLG.obj : ABORTDLG.C onlypbr.h d:\port\port1632.h pbrush.h printimg.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

ABORTDLG.sbr : ABORTDLG.C onlypbr.h d:\port\port1632.h pbrush.h printimg.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

abortprt.obj : abortprt.c onlypbr.h d:\port\port1632.h pbrush.h printimg.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

abortprt.sbr : abortprt.c onlypbr.h d:\port\port1632.h pbrush.h printimg.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

ABOUTDLG.obj : ABOUTDLG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

ABOUTDLG.sbr : ABOUTDLG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

AIRBRUDP.obj : AIRBRUDP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	airbrudp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

AIRBRUDP.sbr : AIRBRUDP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	airbrudp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

ALLOCIMG.obj : ALLOCIMG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

ALLOCIMG.sbr : ALLOCIMG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

BRUSHDLG.obj : BRUSHDLG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

BRUSHDLG.sbr : BRUSHDLG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

BRUSHDP.obj : BRUSHDP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	airbrudp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

BRUSHDP.sbr : BRUSHDP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	airbrudp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

CALCVIEW.obj : CALCVIEW.C onlypbr.h d:\port\port1632.h pbrush.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

CALCVIEW.sbr : CALCVIEW.C onlypbr.h d:\port\port1632.h pbrush.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

CALCWNDS.obj : CALCWNDS.C onlypbr.h d:\port\port1632.h pbrush.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

CALCWNDS.sbr : CALCWNDS.C onlypbr.h d:\port\port1632.h pbrush.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

CLEARDLG.obj : CLEARDLG.C d:\port\port1632.h pbrush.h fixedpt.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

CLEARDLG.sbr : CLEARDLG.C d:\port\port1632.h pbrush.h fixedpt.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

COLERADP.obj : COLERADP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	airbrudp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

COLERADP.sbr : COLERADP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	airbrudp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

COLORDLG.obj : COLORDLG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

COLORDLG.sbr : COLORDLG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

filedlg.obj : filedlg.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

filedlg.sbr : filedlg.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

FIXEDPT.obj : FIXEDPT.C d:\port\port1632.h pbrush.h fixedpt.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

FIXEDPT.sbr : FIXEDPT.C d:\port\port1632.h pbrush.h fixedpt.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

FLIPPOLY.obj : FLIPPOLY.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

FLIPPOLY.sbr : FLIPPOLY.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

fontmenu.obj : fontmenu.c onlypbr.h d:\port\port1632.h pbrush.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

fontmenu.sbr : fontmenu.c onlypbr.h d:\port\port1632.h pbrush.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

INFODLG.obj : INFODLG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

INFODLG.sbr : INFODLG.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

initglob.obj : initglob.c onlypbr.h d:\port\port1632.h pbrush.h menucmd.h\
	pbserver.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

initglob.sbr : initglob.c onlypbr.h d:\port\port1632.h pbrush.h menucmd.h\
	pbserver.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

LCUNDODP.obj : LCUNDODP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	airbrudp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

LCUNDODP.sbr : LCUNDODP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	airbrudp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

LINEDP.obj : LINEDP.C onlypbr.h d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

LINEDP.sbr : LINEDP.C onlypbr.h d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

newpick.obj : newpick.c d:\port\port1632.h pbrush.h airbrudp.h newpick.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

newpick.sbr : newpick.c d:\port\port1632.h pbrush.h airbrudp.h newpick.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

OFFSPOLY.obj : OFFSPOLY.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

OFFSPOLY.sbr : OFFSPOLY.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

OVALDP.obj : OVALDP.C onlypbr.h d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

OVALDP.sbr : OVALDP.C onlypbr.h d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

packbuff.obj : packbuff.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

packbuff.sbr : packbuff.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

POLYTO.obj : POLYTO.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

POLYTO.sbr : POLYTO.C d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

printdlg.obj : printdlg.c onlypbr.h d:\port\port1632.h pbrush.h fixedpt.h\
	printimg.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

printdlg.sbr : printdlg.c onlypbr.h d:\port\port1632.h pbrush.h fixedpt.h\
	printimg.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

printdp.obj : printdp.c onlypbr.h d:\port\port1632.h pbrush.h fixedpt.h\
	printdp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

printdp.sbr : printdp.c onlypbr.h d:\port\port1632.h pbrush.h fixedpt.h\
	printdp.h d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

printimg.obj : printimg.c d:\port\port1632.h pbrush.h printimg.h fixedpt.h\
	printdlg.h vtools.h d:\port\ptypes16.h d:\port\pwin16.h\
	d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h\
	d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

printimg.sbr : printimg.c d:\port\port1632.h pbrush.h printimg.h fixedpt.h\
	printdlg.h vtools.h d:\port\ptypes16.h d:\port\pwin16.h\
	d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h\
	d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

scrolimg.obj : scrolimg.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

scrolimg.sbr : scrolimg.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

scrolmag.obj : scrolmag.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

scrolmag.sbr : scrolmag.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

setcurs.obj : setcurs.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

setcurs.sbr : setcurs.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

SETTITLE.obj : SETTITLE.C onlypbr.h d:\port\port1632.h pbrush.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

SETTITLE.sbr : SETTITLE.C onlypbr.h d:\port\port1632.h pbrush.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

unpkbuff.obj : unpkbuff.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

unpkbuff.sbr : unpkbuff.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

updatimg.obj : updatimg.c onlypbr.h d:\port\port1632.h pbrush.h pbserver.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

updatimg.sbr : updatimg.c onlypbr.h d:\port\port1632.h pbrush.h pbserver.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

validhdr.obj : validhdr.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

validhdr.sbr : validhdr.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

VTOOLS.obj : VTOOLS.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

VTOOLS.sbr : VTOOLS.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

xorcsr.obj : xorcsr.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

xorcsr.sbr : xorcsr.c d:\port\port1632.h pbrush.h d:\port\ptypes16.h\
	d:\port\pwin16.h d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h\
	d:\port\pwin32.h d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

ZOOMINDP.obj : ZOOMINDP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

ZOOMINDP.sbr : ZOOMINDP.C onlypbr.h d:\port\port1632.h pbrush.h vtools.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

ZOOMINWP.obj : ZOOMINWP.C d:\port\port1632.h pbrush.h vtools.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

ZOOMINWP.sbr : ZOOMINWP.C d:\port\port1632.h pbrush.h vtools.h\
	d:\port\ptypes16.h d:\port\pwin16.h d:\port\plan16.h\
	d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h d:\port\plan32.h\
	pbdialog.h pbdecl.h vbm.h

zoomotwp.obj : zoomotwp.c onlypbr.h d:\port\port1632.h pbrush.h fixedpt.h\
	vtools.h printdp.h printimg.h d:\port\ptypes16.h d:\port\pwin16.h\
	d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h\
	d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

zoomotwp.sbr : zoomotwp.c onlypbr.h d:\port\port1632.h pbrush.h fixedpt.h\
	vtools.h printdp.h printimg.h d:\port\ptypes16.h d:\port\pwin16.h\
	d:\port\plan16.h d:\port\ptypes32.h d:\port\pcrt32.h d:\port\pwin32.h\
	d:\port\plan32.h pbdialog.h pbdecl.h vbm.h

PBRUSH.res : PBRUSH.RC pbrush.h pbrush.dlg newdlg.dlg pbdialog.h pbdecl.h\
	vbm.h


$(PROJ).bsc : $(SBRS)
	$(PWBRMAKE) @<<
$(BRFLAGS) $(SBRS)
<<

$(PROJ).exe : $(DEF_FILE) $(OBJS) $(RESS)
!IF $(DEBUG)
	$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_D)
$(LLIBS_G: = +^
) +
$(LLIBS_D: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_D);
<<
!ELSE
	$(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_R)
$(LLIBS_G: = +^
) +
$(LLIBS_R: = +^
) +
$(LIBS: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_R);
<<
!ENDIF
	$(LINKER) @$(PROJ).lrf
	$(RC) $(RESS) $@


.c.sbr :
!IF $(DEBUG)
	$(CC) /Zs $(CFLAGS_G) $(CFLAGS_D) /FR$@ $<
!ELSE
	$(CC) /Zs $(CFLAGS_G) $(CFLAGS_R) /FR$@ $<
!ENDIF

.c.obj :
!IF $(DEBUG)
	$(CC) /c $(CFLAGS_G) $(CFLAGS_D) /Fo$@ $<
!ELSE
	$(CC) /c $(CFLAGS_G) $(CFLAGS_R) /Fo$@ $<
!ENDIF

.rc.res :
	$(RC) /r $<


run: $(PROJ).exe
	WIN $(PROJ).exe $(RUNFLAGS)

debug: $(PROJ).exe
	WIN CVW $(CVFLAGS) $(PROJ).exe $(RUNFLAGS)
