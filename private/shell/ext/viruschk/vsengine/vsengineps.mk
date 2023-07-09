
vsengineps.dll: dlldata.obj vsengine_p.obj vsengine_i.obj
	link /dll /out:vsengineps.dll /def:vsengineps.def /entry:DllMain dlldata.obj vsengine_p.obj vsengine_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del vsengineps.dll
	@del vsengineps.lib
	@del vsengineps.exp
	@del dlldata.obj
	@del vsengine_p.obj
	@del vsengine_i.obj
