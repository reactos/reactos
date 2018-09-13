$(OBJDIR)\mslocusr.obj $(OBJDIR)\mslocusr.lst: .\mslocusr.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\pcache.h $(COMMON)\h\pcerr.h \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h $(COMMON)\h\rc4.h \
	$(COMMON)\h\secdefs.h ..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\dev\ddk\inc\netcons.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commdlg.h ..\..\..\..\dev\inc\imm.h \
	..\..\..\..\dev\inc\mcx.h ..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\dev\inc\netmpr.h ..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\dev\inc\shellapi.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\wincon.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\windows.h ..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\dev\inc\winspool.h ..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\dev\sdk\inc\excpt.h ..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\dev\sdk\inc\objbase.h ..\..\..\..\dev\sdk\inc\objidl.h \
	..\..\..\..\dev\sdk\inc\ole.h ..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\dev\sdk\inc\oleauto.h ..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\dev\sdk\inc\regstr.h ..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\dev\tools\c932\inc\stddef.h \
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\mslocusr.h \
	.\msluapi.h .\msluglob.h .\msluguid.h
.PRECIOUS: $(OBJDIR)\mslocusr.lst

$(OBJDIR)\msludb.obj $(OBJDIR)\msludb.lst: .\msludb.cpp $(COMMON)\h\netlib.h \
	$(COMMON)\h\pcache.h $(COMMON)\h\pcerr.h $(COMMON)\h\poppack.h \
	$(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h $(COMMON)\h\rc4.h \
	$(COMMON)\h\secdefs.h ..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\dev\ddk\inc\netcons.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commdlg.h ..\..\..\..\dev\inc\imm.h \
	..\..\..\..\dev\inc\mcx.h ..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\dev\inc\netmpr.h ..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\dev\inc\shellapi.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\wincon.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\windows.h ..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\dev\inc\winspool.h ..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\dev\sdk\inc\excpt.h ..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\dev\sdk\inc\objbase.h ..\..\..\..\dev\sdk\inc\objidl.h \
	..\..\..\..\dev\sdk\inc\ole.h ..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\dev\sdk\inc\oleauto.h ..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\dev\sdk\inc\regstr.h ..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\dev\tools\c932\inc\stddef.h \
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\mslocusr.h \
	.\msluapi.h .\msluglob.h .\msluguid.h
.PRECIOUS: $(OBJDIR)\msludb.lst

$(OBJDIR)\msluenum.obj $(OBJDIR)\msluenum.lst: .\msluenum.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\pcache.h $(COMMON)\h\pcerr.h \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h $(COMMON)\h\rc4.h \
	$(COMMON)\h\secdefs.h ..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\dev\ddk\inc\netcons.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commdlg.h ..\..\..\..\dev\inc\imm.h \
	..\..\..\..\dev\inc\mcx.h ..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\dev\inc\netmpr.h ..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\dev\inc\shellapi.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\wincon.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\windows.h ..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\dev\inc\winspool.h ..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\dev\sdk\inc\excpt.h ..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\dev\sdk\inc\objbase.h ..\..\..\..\dev\sdk\inc\objidl.h \
	..\..\..\..\dev\sdk\inc\ole.h ..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\dev\sdk\inc\oleauto.h ..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\dev\sdk\inc\regstr.h ..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\dev\tools\c932\inc\stddef.h \
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\mslocusr.h \
	.\msluapi.h .\msluglob.h .\msluguid.h
.PRECIOUS: $(OBJDIR)\msluenum.lst

$(OBJDIR)\msluguid.obj $(OBJDIR)\msluguid.lst: .\msluguid.cpp \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h \
	..\..\..\..\dev\inc\commdlg.h ..\..\..\..\dev\inc\imm.h \
	..\..\..\..\dev\inc\mcx.h ..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\dev\inc\netmpr.h ..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\dev\inc\shellapi.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\wincon.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\windows.h ..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\dev\inc\winspool.h ..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\dev\sdk\inc\coguid.h \
	..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\oaidl.h ..\..\..\..\dev\sdk\inc\objbase.h \
	..\..\..\..\dev\sdk\inc\objidl.h ..\..\..\..\dev\sdk\inc\ole.h \
	..\..\..\..\dev\sdk\inc\ole2.h ..\..\..\..\dev\sdk\inc\oleauto.h \
	..\..\..\..\dev\sdk\inc\oleguid.h ..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\dev\sdk\inc\unknwn.h ..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h .\msluguid.h
.PRECIOUS: $(OBJDIR)\msluguid.lst

$(OBJDIR)\mslunp.obj $(OBJDIR)\mslunp.lst: .\mslunp.cpp $(COMMON)\h\netlib.h \
	$(COMMON)\h\pcache.h $(COMMON)\h\pcerr.h $(COMMON)\h\poppack.h \
	$(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h $(COMMON)\h\rc4.h \
	$(COMMON)\h\secdefs.h ..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\dev\ddk\inc\netcons.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commctrl.h ..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\dev\inc\imm.h ..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\dev\inc\prsht.h ..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\dev\inc\winbase.h ..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\dev\inc\windef.h ..\..\..\..\dev\inc\windows.h \
	..\..\..\..\dev\inc\wingdi.h ..\..\..\..\dev\inc\winnetwk.h \
	..\..\..\..\dev\inc\winnls.h ..\..\..\..\dev\inc\winnt.h \
	..\..\..\..\dev\inc\winreg.h ..\..\..\..\dev\inc\winspool.h \
	..\..\..\..\dev\inc\winuser.h ..\..\..\..\dev\sdk\inc\cguid.h \
	..\..\..\..\dev\sdk\inc\dlgs.h ..\..\..\..\dev\sdk\inc\excpt.h \
	..\..\..\..\dev\sdk\inc\oaidl.h ..\..\..\..\dev\sdk\inc\objbase.h \
	..\..\..\..\dev\sdk\inc\objidl.h ..\..\..\..\dev\sdk\inc\ole.h \
	..\..\..\..\dev\sdk\inc\ole2.h ..\..\..\..\dev\sdk\inc\oleauto.h \
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\regstr.h \
	..\..\..\..\dev\sdk\inc\unknwn.h ..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\dev\tools\c932\inc\stddef.h \
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\mslocusr.h \
	.\msluapi.h .\msluglob.h .\msluguid.h .\resource.h
.PRECIOUS: $(OBJDIR)\mslunp.lst

$(OBJDIR)\msluobj.obj $(OBJDIR)\msluobj.lst: .\msluobj.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\pcache.h $(COMMON)\h\pcerr.h \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h $(COMMON)\h\rc4.h \
	$(COMMON)\h\secdefs.h ..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\dev\ddk\inc\netcons.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commdlg.h ..\..\..\..\dev\inc\imm.h \
	..\..\..\..\dev\inc\mcx.h ..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\dev\inc\netmpr.h ..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\dev\inc\shellapi.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\wincon.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\windows.h ..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\dev\inc\winspool.h ..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\dev\sdk\inc\excpt.h ..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\dev\sdk\inc\objbase.h ..\..\..\..\dev\sdk\inc\objidl.h \
	..\..\..\..\dev\sdk\inc\ole.h ..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\dev\sdk\inc\oleauto.h ..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\dev\sdk\inc\regstr.h ..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\dev\tools\c932\inc\stddef.h \
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\mslocusr.h \
	.\msluapi.h .\msluguid.h
.PRECIOUS: $(OBJDIR)\msluobj.lst

$(OBJDIR)\msluuser.obj $(OBJDIR)\msluuser.lst: .\msluuser.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\pcache.h $(COMMON)\h\pcerr.h \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h $(COMMON)\h\rc4.h \
	$(COMMON)\h\secdefs.h ..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\dev\ddk\inc\netcons.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commdlg.h ..\..\..\..\dev\inc\imm.h \
	..\..\..\..\dev\inc\mcx.h ..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\dev\inc\netmpr.h ..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\dev\inc\shellapi.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\wincon.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\windows.h ..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\dev\inc\winspool.h ..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\dev\sdk\inc\excpt.h ..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\dev\sdk\inc\objbase.h ..\..\..\..\dev\sdk\inc\objidl.h \
	..\..\..\..\dev\sdk\inc\ole.h ..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\dev\sdk\inc\oleauto.h ..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\dev\sdk\inc\regstr.h ..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\dev\tools\c932\inc\stddef.h \
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\mslocusr.h \
	.\msluapi.h .\msluglob.h .\msluguid.h
.PRECIOUS: $(OBJDIR)\msluuser.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: .\pch.cpp $(COMMON)\h\netlib.h \
	$(COMMON)\h\pcache.h $(COMMON)\h\pcerr.h $(COMMON)\h\poppack.h \
	$(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h $(COMMON)\h\rc4.h \
	$(COMMON)\h\secdefs.h ..\..\..\..\dev\ddk\inc\net32def.h \
	..\..\..\..\dev\ddk\inc\netcons.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commdlg.h ..\..\..\..\dev\inc\imm.h \
	..\..\..\..\dev\inc\mcx.h ..\..\..\..\dev\inc\mmsystem.h \
	..\..\..\..\dev\inc\netmpr.h ..\..\..\..\dev\inc\prsht.h \
	..\..\..\..\dev\inc\shellapi.h ..\..\..\..\dev\inc\winbase.h \
	..\..\..\..\dev\inc\wincon.h ..\..\..\..\dev\inc\windef.h \
	..\..\..\..\dev\inc\windows.h ..\..\..\..\dev\inc\wingdi.h \
	..\..\..\..\dev\inc\winnetwk.h ..\..\..\..\dev\inc\winnls.h \
	..\..\..\..\dev\inc\winnt.h ..\..\..\..\dev\inc\winreg.h \
	..\..\..\..\dev\inc\winspool.h ..\..\..\..\dev\inc\winuser.h \
	..\..\..\..\dev\sdk\inc\cguid.h ..\..\..\..\dev\sdk\inc\dlgs.h \
	..\..\..\..\dev\sdk\inc\excpt.h ..\..\..\..\dev\sdk\inc\oaidl.h \
	..\..\..\..\dev\sdk\inc\objbase.h ..\..\..\..\dev\sdk\inc\objidl.h \
	..\..\..\..\dev\sdk\inc\ole.h ..\..\..\..\dev\sdk\inc\ole2.h \
	..\..\..\..\dev\sdk\inc\oleauto.h ..\..\..\..\dev\sdk\inc\oleidl.h \
	..\..\..\..\dev\sdk\inc\regstr.h ..\..\..\..\dev\sdk\inc\unknwn.h \
	..\..\..\..\dev\sdk\inc\winerror.h ..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\nb30.h \
	..\..\..\..\dev\tools\c932\inc\rpc.h \
	..\..\..\..\dev\tools\c932\inc\rpcndr.h \
	..\..\..\..\dev\tools\c932\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c932\inc\stdarg.h \
	..\..\..\..\dev\tools\c932\inc\stddef.h \
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\mslocusr.h \
	.\msluapi.h .\msluguid.h
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\logonui.res $(OBJDIR)\logonui.lst: .\logonui.rc \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	..\..\..\..\dev\inc\commctrl.h ..\..\..\..\dev\inc\prsht.h \
	.\resource.h
.PRECIOUS: $(OBJDIR)\logonui.lst

