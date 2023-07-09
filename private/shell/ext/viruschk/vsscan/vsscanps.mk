
vsscanps.dll: dlldata.obj vsscan_p.obj vsscan_i.obj
	link /dll /out:vsscanps.dll /def:vsscanps.def /entry:DllMain dlldata.obj vsscan_p.obj vsscan_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del vsscanps.dll
	@del vsscanps.lib
	@del vsscanps.exp
	@del dlldata.obj
	@del vsscan_p.obj
	@del vsscan_i.obj
