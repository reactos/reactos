.\regbined.obj .\regbined.lst: ..\regbined.c ..\reghelp.h ..\regresid.h

.\regcdhk.obj .\regcdhk.lst: ..\regcdhk.c ..\regcdhk.h ..\regedit.h \
	..\reghelp.h ..\regkey.h ..\regresid.h

.\regdebug.obj .\regdebug.lst: ..\regdebug.c

.\regdrag.obj .\regdrag.lst: ..\regdrag.c ..\regedit.h

.\regdwded.obj .\regdwded.lst: ..\regdwded.c ..\reghelp.h ..\regresid.h

.\regedit.obj .\regedit.lst: ..\regedit.c \
	..\..\..\..\..\dev\sdk\inc\regstr.h ..\regedit.h ..\regfile.h \
	..\regfind.h ..\regkey.h ..\regnet.h ..\regprint.h ..\regresid.h \
	..\regvalue.h

.\regfile.obj .\regfile.lst: ..\regfile.c ..\regcdhk.h ..\regedit.h \
	..\regfile.h ..\regkey.h ..\regresid.h

.\regfind.obj .\regfind.lst: ..\regfind.c ..\regedit.h ..\reghelp.h \
	..\regkey.h ..\regresid.h

.\regkey.obj .\regkey.lst: ..\regkey.c ..\regedit.h ..\regkey.h \
	..\regresid.h ..\regvalue.h

.\regmain.obj .\regmain.lst: ..\regmain.c \
	..\..\..\..\..\dev\sdk\inc\regstr.h ..\regbined.h ..\regedit.h \
	..\regfile.h ..\regresid.h

.\regmisc.obj .\regmisc.lst: ..\regmisc.c

.\regnet.obj .\regnet.lst: ..\regnet.c ..\..\..\..\..\dev\inc16\commctrl.h \
	..\..\..\..\..\dev\inc16\ole2.h ..\..\..\..\..\dev\inc16\prsht.h \
	..\..\..\..\..\dev\inc\shlguid.h ..\..\..\..\..\dev\inc\shlobj.h \
	..\..\..\..\..\dev\sdk\inc16\coguid.h \
	..\..\..\..\..\dev\sdk\inc16\compobj.h \
	..\..\..\..\..\dev\sdk\inc16\dvobj.h \
	..\..\..\..\..\dev\sdk\inc16\initguid.h \
	..\..\..\..\..\dev\sdk\inc16\moniker.h \
	..\..\..\..\..\dev\sdk\inc16\oleguid.h \
	..\..\..\..\..\dev\sdk\inc16\scode.h \
	..\..\..\..\..\dev\sdk\inc16\storage.h \
	..\..\..\..\..\dev\sdk\inc\poppack.h \
	..\..\..\..\..\dev\sdk\inc\pshpack1.h \
	..\regedit.h \
	..\reghelp.h ..\regkey.h ..\regresid.h

.\regporte.obj .\regporte.lst: ..\regporte.c ..\reg1632.h ..\regresid.h

.\regprint.obj .\regprint.lst: ..\regprint.c ..\regcdhk.h ..\regprint.h \
	..\regresid.h

.\regstred.obj .\regstred.lst: ..\regstred.c ..\reghelp.h ..\regresid.h

.\regvalue.obj .\regvalue.lst: ..\regvalue.c ..\regbined.h ..\regdwded.h \
	..\regedit.h ..\regresid.h ..\regstred.h ..\regvalue.h

