# DEBUG some flag used by developers, not needed for retail.
# PENWIN, Used by Pen Folks only.(Pbrush is pen aware without this.)

!ifdef PENWIN                       # only the pen folks need this
DEF = -DPENWIN -DWIN16
!else
DEF = -DWIN16
!endif

!ifdef CVW                          # For debug version 
CVC =/Zi                            # set CVW flag
OPT = /Od                           # turn off optimization
LINK = link /NOE/CO/LI/MAP/NOD /A:16
!else                               # RETAIL version 
CVC =                               # no CVW flags
OPT = -Oas                          # optimize
LINK = link /NOD/NOE/LI/MAP /A:16
!endif

!ifdef PENWIN                       # pick up penwin.lib, only pen folks need this
LIBS = libw mlibcaw mnocrtw olesvr commdlg pwin16 shell penwin
!else
LIBS = libw mlibcaw mnocrtw olesvr commdlg pwin16 shell
!endif

CC   = cl -c -W3 -AM -G2sw -Zp $(CVC) $(OPT) $(DEF)
MASM  = masm $(CVC)
RC   = rc $(DEF)

NAME = PBRUSH

OBJ1  = abortdlg.obj abortprt.obj airbrudp.obj allocimg.obj 
OBJ2  = brushdlg.obj brushdp.obj calcview.obj calcwnds.obj cleardlg.obj 
OBJ3  = coleradp.obj colordlg.obj colorwp.obj
OBJ4  = dotparal.obj dotpoly.obj dotrect.obj eraserdp.obj filedlg.obj  
OBJ5  = fixedpt.obj flippoly.obj freeimg.obj  
OBJ6  = freepick.obj fullwp.obj getaspct.obj getinfo.obj getprtdc.obj 
OBJ7  = gettanpt.obj hidecsr.obj infodlg.obj initglob.obj lcundodp.obj 
OBJ8  = linedp.obj loadbit.obj loadcolr.obj loadimg.obj  
OBJ9  = menucmd.obj message.obj metafile.obj mousedlg.obj newpick.obj 
OBJ10 = offspoly.obj ovaldp.obj packbuff.obj paintwp.obj parentwp.obj 
OBJ11 = polyrect.obj polyto.obj printdlg.obj printdp.obj  
OBJ12 = printimg.obj ptools.obj qutil.obj rectdp.obj rndrctdp.obj rollerdp.obj 
OBJ13 = savebit.obj saveclip.obj savecolr.obj saveimg.obj scrolimg.obj
OBJ14 = scrolmag.obj setcurs.obj settitle.obj shapelib.obj shrgrodp.obj 
OBJ15 = sizewp.obj textdp.obj tiltblt.obj tiltdp.obj toolwp.obj             
OBJ16 = trcktool.obj unpkbuff.obj updatimg.obj 
OBJ17 = validhdr.obj vtools.obj windata.obj pbrush.obj wndinit.obj
OBJ18 = xorcsr.obj zoomindp.obj zoominwp.obj zoomotwp.obj 
OBJ19 = srvrinit.obj pbserver.obj newdlg.obj
OBJS  = curvedp.obj polydp.obj

OBJ = $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) $(OBJ7) $(OBJ8) \
      $(OBJ9) $(OBJ10) $(OBJ11) $(OBJ12) $(OBJ13) $(OBJ14) $(OBJ15)   \
      $(OBJ16) $(OBJ17) $(OBJ18) $(OBJS) $(OBJ19)

C   = $(CC) -NT _$(SEG) $*.c

ASM = $(MASM) $*.asm ;

.c.obj:
        $(CC) -NT _TEXT $*.c

.asm.obj:
	$(ASM) $*;

goal: $(NAME).exe

$(NAME).exe $(NAME).map: $(OBJ) $(NAME).res $(NAME).def
	$(LINK) @<<
abortdlg abortprt airbrudp +
allocimg brushdlg brushdp calcview calcwnds cleardlg +
coleradp colordlg colorwp dotparal dotpoly dotrect +
eraserdp filedlg fixedpt flippoly freeimg +
freepick fullwp getaspct getinfo getprtdc gettanpt +
hidecsr infodlg initglob lcundodp linedp loadbit +
loadcolr loadimg menucmd message metafile mousedlg +
newpick offspoly ovaldp packbuff paintwp parentwp +
polyrect polyto printdlg printdp printimg ptools +
qutil rectdp rndrctdp rollerdp savebit saveclip +
savecolr saveimg scrolimg scrolmag setcurs settitle +
shapelib shrgrodp sizewp textdp tiltblt tiltdp toolwp +
trcktool unpkbuff updatimg validhdr vtools windata +
pbrush wndinit xorcsr zoomindp zoominwp zoomotwp +
srvrinit pbserver newdlg $(OBJS),
$(NAME),
pbrushx,
$(LIBS),
$(NAME).def
<<

!ifdef DEBUG        
    	  cvpack -p $(NAME).exe
!endif
        $(RC) $(NAME).res
        mapsym pbrushx.map
        copy pbrushx.* pbrush.*
        del pbrushx.*

$(NAME).res: $(NAME).rc \
             pbrush.h pbrush.ico flood.cur crossh.cur dummy.cur xdummy.cur \
             side2.cur pick.cur ibeam.cur ctools.bmp arrow.bmp pbrush.dlg \
	     newdlg.dlg
        $(RC) -r $(NAME).rc

ws.obj		: ; $(C)


###################### _RES #######################
SEG = RES

windata.obj    : ; $(C)
pbrush.obj    : ; $(C)
message.obj    : ; $(C)
parentwp.obj   : ; $(C)
paintwp.obj    : ; $(C)
calcwnds.obj   : ; $(C)
xorcsr.obj     : ; $(C)
scrolimg.obj   : ; $(C)

###################### _RES2 #######################
SEG = RES2

updatimg.obj   : ; $(C)
settitle.obj   : ; $(C)
hidecsr.obj    : ; $(C)
calcview.obj   : ; $(C)
polyto.obj     : ; $(C)
getaspct.obj   : ; $(C)
setcurs.obj    : ; $(C)
fixedpt.obj    : ; $(C)
shapelib.obj   : ; $(C)
qutil.obj      : ; $(ASM)

###################### _RECT #######################
SEG = RECT

truerect.obj   : ; $(C)
rectdp.obj     : ; $(C)
rndrctdp.obj   : ; $(C)

###################### _INI #######################
SEG = INI

wndinit.obj    : ; $(C)
initglob.obj   : ; $(C)

###################### _CLEAR #######################
SEG = CLEAR

cleardlg.obj   : ; $(C)

###################### _IO #######################
SEG = IO

filedlg.obj    : ; $(C)
infodlg.obj    : ; $(C)
allocimg.obj   : ; $(C)
freeimg.obj    : ; $(C)
getinfo.obj    : ; $(C)

newdlg.obj     : ; $(C)

###################### _IO #######################
SEG = IO2

packbuff.obj   : ; $(C)
unpkbuff.obj   : ; $(C)
loadimg.obj    : ; $(C)
saveimg.obj    : ; $(C)
validhdr.obj   : ; $(C)

###################### _IO3 #######################
SEG = IO3

abortprt.obj   : ; $(C)
savebit.obj    : ; $(C)
getprtdc.obj   : ; $(C)
loadbit.obj    : ; $(C)
loadcolr.obj   : ; $(C)
savecolr.obj   : ; $(C)
printdp.obj    : ; $(C)

###################### _TOOL #######################
SEG = TOOL

toolwp.obj     : ; $(C)
trcktool.obj   : ; $(C)
vtools.obj     : ; $(C)
ptools.obj     : ; $(C)

###################### _SIZE #######################
SEG = SIZE

sizewp.obj     : ; $(C)

###################### _PAL #######################
SEG = PAL

colorwp.obj    : ; $(C)

###################### _MENU #######################
SEG = MENU

menucmd.obj    : ; $(C)

###################### _BRUSH #######################
SEG = BRUSH

brushdp.obj    : ; $(C)
gettanpt.obj   : ; $(C)

###################### _LINE #######################
SEG = LINE

linedp.obj     : ; $(C)

###################### _OVAL #######################
SEG = OVAL

ovaldp.obj     : ; $(C)

###################### _POLY #######################
SEG = POLY

polydp.obj     : ; $(C)
polydp2.obj    : ; $(C)

###################### _ERASE #######################
SEG = ERASE

eraserdp.obj   : ; $(C)
coleradp.obj   : ; $(C)
lcundodp.obj   : ; $(C)

###################### _FLOOD #######################
SEG = FLOOD

rollerdp.obj   : ; $(C)

###################### _AIR #######################
SEG = AIR

airbrudp.obj   : ; $(C)

###################### _TEXTDP #######################
SEG = TEXTDP

textdp.obj     : ; $(C)

###################### _ZOOMIN #######################
SEG = ZOOMIN

zoomindp.obj   : ; $(C)
scrolmag.obj   : ; $(C)
zoominwp.obj   : ; $(C)

###################### _PICK #######################
SEG = PICK

dotrect.obj    : ; $(C)
polyrect.obj   : ; $(C)
dotpoly.obj    : ; $(C)
offspoly.obj   : ; $(C)
flippoly.obj   : ; $(C)
shrgrodp.obj   : ; $(C)
newpick.obj    : ; $(C)
freepick.obj   : ; $(C)
tiltdp.obj     : ; $(C)
dotparal.obj   : ; $(C)
tiltblt.obj    : ; $(C)
formrgn.obj    : ; $(C)

###################### _PRINT #######################
SEG = PRINT

printimg.obj   : ; $(C)
abortdlg.obj   : ; $(C)
printdlg.obj   : ; $(C)

###################### _CURVE #######################
SEG = CURVE

curvedp.obj    : ; $(C)
curvedp2.obj   : ; $(C)

###################### _MSD #######################
SEG = MSD

mousedlg.obj   : ; $(C)

###################### _BRUSHDLG #######################
SEG = BRUSHDLG

brushdlg.obj   : ; $(C)

###################### _COLORDLG #######################
SEG = COLORDLG

colordlg.obj   : ; $(C)

###################### _FULL #######################
SEG = FULL

fullwp.obj     : ; $(C)

###################### _CLIP #######################
SEG = CLIP

saveclip.obj   : ; $(C)

###################### _ZOOMOT #######################
SEG = ZOOMOT

zoomotwp.obj   : ; $(C)

###################### _OLE ########################
SEG = OLEINIT

srvrinit.obj    : ; $(C)

###################### _OLE ########################
SEG = OLE

pbserver.obj    : ; $(C)



clean:   cleancmd

cleancmd:
	del $(NAME).exe
	del *.res
	del *.obj
	del *.map
	del *.sym

depend:
	mv makefile makefile.old
	sed "/^# START Dependencies/,/^# END Dependencies/D" makefile.old > makefile
	del makefile.old
	echo # START Dependencies >> makefile
	includes -l *.c *.asm >> makefile
	echo # END Dependencies >> makefile

# START Dependencies  
# END Dependencies  
