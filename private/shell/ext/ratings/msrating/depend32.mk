$(OBJDIR)\comobj.obj $(OBJDIR)\comobj.lst: .\comobj.cpp $(COMMON)\h\netlib.h \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h \
	..\..\..\..\dev\ddk\inc\netspi.h ..\..\..\..\dev\inc\commdlg.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\inc\ratings.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\msluglob.h \
	.\msrating.h .\ratguid.h .\rors.h
.PRECIOUS: $(OBJDIR)\comobj.lst

$(OBJDIR)\filedlg.obj $(OBJDIR)\filedlg.lst: .\filedlg.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commctrl.h ..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\dev\inc\imm.h ..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\dev\inc\prsht.h ..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\dev\inc\winbase.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\inc\contxids.h \
	..\..\..\..\inet\ohare\inc\ratings.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\buffer.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\mslubase.h .\msluglob.h .\msrating.h .\ratguid.h .\resource.h
.PRECIOUS: $(OBJDIR)\filedlg.lst

$(OBJDIR)\mslubase.obj $(OBJDIR)\mslubase.lst: .\mslubase.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\buffer.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\mslubase.h .\msluglob.h .\msrating.h .\ratguid.h .\resource.h
.PRECIOUS: $(OBJDIR)\mslubase.lst

$(OBJDIR)\msludlg.obj $(OBJDIR)\msludlg.lst: .\msludlg.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
	..\..\..\..\dev\inc\commctrl.h ..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\dev\inc\imm.h ..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\dev\inc\prsht.h ..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\dev\inc\shlwapi.h ..\..\..\..\dev\inc\winbase.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\inc\contxids.h \
	..\..\..\..\inet\ohare\inc\ratings.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\buffer.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\mslubase.h .\msluglob.h .\msrating.h .\ratguid.h .\resource.h
.PRECIOUS: $(OBJDIR)\msludlg.lst

$(OBJDIR)\msrating.obj $(OBJDIR)\msrating.lst: .\msrating.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\inc\ratings.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\msluglob.h \
	.\msrating.h .\ratguid.h
.PRECIOUS: $(OBJDIR)\msrating.lst

$(OBJDIR)\parselbl.obj $(OBJDIR)\parselbl.lst: .\parselbl.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\convtime.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\msluglob.h .\msrating.h .\parselbl.h .\ratguid.h
.PRECIOUS: $(OBJDIR)\parselbl.lst

$(OBJDIR)\parserat.obj $(OBJDIR)\parserat.lst: .\parserat.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\mslubase.h .\msluglob.h .\msrating.h .\parselbl.h .\ratguid.h \
	.\resource.h
.PRECIOUS: $(OBJDIR)\parserat.lst

$(OBJDIR)\pch.obj $(OBJDIR)\pch.lst: .\pch.cpp $(COMMON)\h\netlib.h \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h \
	..\..\..\..\dev\ddk\inc\netspi.h ..\..\..\..\dev\inc\commdlg.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\msrating.h \
	.\ratguid.h
.PRECIOUS: $(OBJDIR)\pch.lst

$(OBJDIR)\picsuser.obj $(OBJDIR)\picsuser.lst: .\picsuser.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\mslubase.h .\msluglob.h .\msrating.h .\ratguid.h .\resource.h
.PRECIOUS: $(OBJDIR)\picsuser.lst

$(OBJDIR)\ratguid.obj $(OBJDIR)\ratguid.lst: .\ratguid.cpp \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h .\ratguid.h
.PRECIOUS: $(OBJDIR)\ratguid.lst

$(OBJDIR)\ratings.obj $(OBJDIR)\ratings.lst: .\ratings.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\inc\contxids.h \
	..\..\..\..\inet\ohare\inc\ratings.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\convtime.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\mslubase.h .\msluglob.h .\msrating.h .\parselbl.h .\ratguid.h \
	.\resource.h
.PRECIOUS: $(OBJDIR)\ratings.lst

$(OBJDIR)\rocycle.obj $(OBJDIR)\rocycle.lst: .\rocycle.cpp \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\inc\ratings.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\mslubase.h .\msluglob.h .\msrating.h .\ratguid.h .\resource.h \
	.\roll.h .\rors.h
.PRECIOUS: $(OBJDIR)\rocycle.lst

$(OBJDIR)\roll.obj $(OBJDIR)\roll.lst: .\roll.cpp $(COMMON)\h\netlib.h \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h \
	..\..\..\..\dev\ddk\inc\netspi.h ..\..\..\..\dev\inc\commdlg.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\inc\ratings.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\msrating.h \
	.\ratguid.h .\roll.h
.PRECIOUS: $(OBJDIR)\roll.lst

$(OBJDIR)\rors.obj $(OBJDIR)\rors.lst: .\rors.cpp $(COMMON)\h\netlib.h \
	$(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h $(COMMON)\h\pshpack2.h \
	$(COMMON)\h\pshpack4.h $(COMMON)\h\pshpack8.h \
	..\..\..\..\dev\ddk\inc\netspi.h ..\..\..\..\dev\inc\commdlg.h \
	..\..\..\..\dev\inc\imm.h ..\..\..\..\dev\inc\mcx.h \
	..\..\..\..\dev\inc\mmsystem.h ..\..\..\..\dev\inc\netmpr.h \
	..\..\..\..\dev\inc\prsht.h ..\..\..\..\dev\inc\shellapi.h \
	..\..\..\..\dev\inc\winbase.h ..\..\..\..\dev\inc\wincon.h \
	..\..\..\..\dev\inc\windef.h ..\..\..\..\dev\inc\windows.h \
	..\..\..\..\dev\inc\wingdi.h ..\..\..\..\dev\inc\wininet.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\inc\ratings.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\array.h \
	.\mslubase.h .\msluglob.h .\msrating.h .\ratguid.h .\resource.h \
	.\rors.h
.PRECIOUS: $(OBJDIR)\rors.lst

$(OBJDIR)\superpw.obj $(OBJDIR)\superpw.lst: .\superpw.cpp $(COMMON)\h\md5.h \
	$(COMMON)\h\netlib.h $(COMMON)\h\poppack.h $(COMMON)\h\pshpack1.h \
	$(COMMON)\h\pshpack2.h $(COMMON)\h\pshpack4.h \
	$(COMMON)\h\pshpack8.h ..\..\..\..\dev\ddk\inc\netspi.h \
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
	..\..\..\..\dev\tools\c1032\inc\cderr.h \
	..\..\..\..\dev\tools\c1032\inc\ctype.h \
	..\..\..\..\dev\tools\c1032\inc\dde.h \
	..\..\..\..\dev\tools\c1032\inc\ddeml.h \
	..\..\..\..\dev\tools\c1032\inc\lzexpand.h \
	..\..\..\..\dev\tools\c1032\inc\nb30.h \
	..\..\..\..\dev\tools\c1032\inc\rpc.h \
	..\..\..\..\dev\tools\c1032\inc\rpcndr.h \
	..\..\..\..\dev\tools\c1032\inc\rpcnsip.h \
	..\..\..\..\dev\tools\c1032\inc\stdarg.h \
	..\..\..\..\dev\tools\c1032\inc\stdlib.h \
	..\..\..\..\dev\tools\c1032\inc\string.h \
	..\..\..\..\dev\tools\c1032\inc\winperf.h \
	..\..\..\..\dev\tools\c1032\inc\winsock.h \
	..\..\..\..\dev\tools\c1032\inc\winsvc.h \
	..\..\..\..\dev\tools\c1032\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\msluglob.h \
	.\msrating.h .\ratguid.h
.PRECIOUS: $(OBJDIR)\superpw.lst

