$(OBJDIR)\alloc.obj $(OBJDIR)\alloc.lst: .\alloc.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npalloc.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\alloc.lst

$(OBJDIR)\bufbase.obj $(OBJDIR)\bufbase.lst: .\bufbase.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\buffer.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\bufbase.lst

$(OBJDIR)\buffer.obj $(OBJDIR)\buffer.lst: .\buffer.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\buffer.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\buffer.lst

$(OBJDIR)\bufglob.obj $(OBJDIR)\bufglob.lst: .\bufglob.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\buffer.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\bufglob.lst

$(OBJDIR)\bufloc.obj $(OBJDIR)\bufloc.lst: .\bufloc.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\buffer.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\bufloc.lst

$(OBJDIR)\chr.obj $(OBJDIR)\chr.lst: .\chr.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\chr.lst

$(OBJDIR)\cmp.obj $(OBJDIR)\cmp.lst: .\cmp.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\cmp.lst

$(OBJDIR)\convtime.obj $(OBJDIR)\convtime.lst: .\convtime.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\convtime.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\convtime.lst

$(OBJDIR)\cpycat.obj $(OBJDIR)\cpycat.lst: .\cpycat.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\cpycat.lst

$(OBJDIR)\dostime.obj $(OBJDIR)\dostime.lst: .\dostime.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\convtime.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\dostime.lst

$(OBJDIR)\iconlbox.obj $(OBJDIR)\iconlbox.lst: .\iconlbox.cpp \
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
	..\..\..\..\dev\sdk\inc\unknwn.h ..\..\..\..\dev\sdk\inc\winerror.h \
	..\..\..\..\dev\sdk\inc\wtypes.h \
	..\..\..\..\dev\tools\c932\inc\cderr.h \
	..\..\..\..\dev\tools\c932\inc\ctype.h \
	..\..\..\..\dev\tools\c932\inc\dde.h \
	..\..\..\..\dev\tools\c932\inc\ddeml.h \
	..\..\..\..\dev\tools\c932\inc\lzexpand.h \
	..\..\..\..\dev\tools\c932\inc\memory.h \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\iconlbox.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\iconlbox.lst

$(OBJDIR)\istr.obj $(OBJDIR)\istr.lst: .\istr.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\istr.lst

$(OBJDIR)\istraux.obj $(OBJDIR)\istraux.lst: .\istraux.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\istraux.lst

$(OBJDIR)\npassert.obj $(OBJDIR)\npassert.lst: .\npassert.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\npassert.lst

$(OBJDIR)\npcrit.obj $(OBJDIR)\npcrit.lst: .\npcrit.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npcrit.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\npcrit.lst

$(OBJDIR)\npgenerr.obj $(OBJDIR)\npgenerr.lst: .\npgenerr.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npmsg.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\npgenerr.lst

$(OBJDIR)\npmsg.obj $(OBJDIR)\npmsg.lst: .\npmsg.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npmsg.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\npmsg.lst

$(OBJDIR)\purecall.obj $(OBJDIR)\purecall.lst: .\purecall.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\purecall.lst

$(OBJDIR)\regentry.obj $(OBJDIR)\regentry.lst: .\regentry.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h \
	..\..\..\..\inet\ohare\ratings\inc\regentry.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\regentry.lst

$(OBJDIR)\sched.obj $(OBJDIR)\sched.lst: .\sched.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h \
	..\..\..\..\inet\ohare\ratings\inc\sched.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\sched.lst

$(OBJDIR)\spn.obj $(OBJDIR)\spn.lst: .\spn.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\spn.lst

$(OBJDIR)\str.obj $(OBJDIR)\str.lst: .\str.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\str.lst

$(OBJDIR)\strassgn.obj $(OBJDIR)\strassgn.lst: .\strassgn.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strassgn.lst

$(OBJDIR)\stratoi.obj $(OBJDIR)\stratoi.lst: .\stratoi.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\stratoi.lst

$(OBJDIR)\stratol.obj $(OBJDIR)\stratol.lst: .\stratol.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\stratol.lst

$(OBJDIR)\strcat.obj $(OBJDIR)\strcat.lst: .\strcat.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strcat.lst

$(OBJDIR)\strchr.obj $(OBJDIR)\strchr.lst: .\strchr.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strchr.lst

$(OBJDIR)\strcmp.obj $(OBJDIR)\strcmp.lst: .\strcmp.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strcmp.lst

$(OBJDIR)\strcspn.obj $(OBJDIR)\strcspn.lst: .\strcspn.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strcspn.lst

$(OBJDIR)\strdss.obj $(OBJDIR)\strdss.lst: .\strdss.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strdss.lst

$(OBJDIR)\stricmp.obj $(OBJDIR)\stricmp.lst: .\stricmp.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\stricmp.lst

$(OBJDIR)\string.obj $(OBJDIR)\string.lst: .\string.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\string.lst

$(OBJDIR)\strinsrt.obj $(OBJDIR)\strinsrt.lst: .\strinsrt.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strinsrt.lst

$(OBJDIR)\stris.obj $(OBJDIR)\stris.lst: .\stris.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\stris.lst

$(OBJDIR)\stristr.obj $(OBJDIR)\stristr.lst: .\stristr.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\stristr.lst

$(OBJDIR)\strload.obj $(OBJDIR)\strload.lst: .\strload.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strload.lst

$(OBJDIR)\strmisc.obj $(OBJDIR)\strmisc.lst: .\strmisc.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strmisc.lst

$(OBJDIR)\strnchar.obj $(OBJDIR)\strnchar.lst: .\strnchar.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strnchar.lst

$(OBJDIR)\strncmp.obj $(OBJDIR)\strncmp.lst: .\strncmp.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strncmp.lst

$(OBJDIR)\strncpy.obj $(OBJDIR)\strncpy.lst: .\strncpy.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strncpy.lst

$(OBJDIR)\strnicmp.obj $(OBJDIR)\strnicmp.lst: .\strnicmp.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strnicmp.lst

$(OBJDIR)\strparty.obj $(OBJDIR)\strparty.lst: .\strparty.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strparty.lst

$(OBJDIR)\strprof.obj $(OBJDIR)\strprof.lst: .\strprof.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strprof.lst

$(OBJDIR)\strqss.obj $(OBJDIR)\strqss.lst: .\strqss.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strqss.lst

$(OBJDIR)\strrchr.obj $(OBJDIR)\strrchr.lst: .\strrchr.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strrchr.lst

$(OBJDIR)\strrss.obj $(OBJDIR)\strrss.lst: .\strrss.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strrss.lst

$(OBJDIR)\strspn.obj $(OBJDIR)\strspn.lst: .\strspn.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strspn.lst

$(OBJDIR)\strstr.obj $(OBJDIR)\strstr.lst: .\strstr.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strstr.lst

$(OBJDIR)\strtok.obj $(OBJDIR)\strtok.lst: .\strtok.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strtok.lst

$(OBJDIR)\strupr.obj $(OBJDIR)\strupr.lst: .\strupr.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npassert.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\strupr.lst

$(OBJDIR)\timedata.obj $(OBJDIR)\timedata.lst: .\timedata.cpp \
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
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\convtime.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\timedata.lst

$(OBJDIR)\upr.obj $(OBJDIR)\upr.lst: .\upr.cpp $(COMMON)\h\netlib.h \
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
	..\..\..\..\dev\sdk\inc\oleidl.h ..\..\..\..\dev\sdk\inc\unknwn.h \
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
	..\..\..\..\dev\tools\c932\inc\stdlib.h \
	..\..\..\..\dev\tools\c932\inc\string.h \
	..\..\..\..\dev\tools\c932\inc\winperf.h \
	..\..\..\..\dev\tools\c932\inc\winsock.h \
	..\..\..\..\dev\tools\c932\inc\winsvc.h \
	..\..\..\..\dev\tools\c932\inc\winver.h \
	..\..\..\..\inet\ohare\ratings\inc\base.h \
	..\..\..\..\inet\ohare\ratings\inc\npdefs.h \
	..\..\..\..\inet\ohare\ratings\inc\npstring.h .\npcommon.h
.PRECIOUS: $(OBJDIR)\upr.lst

