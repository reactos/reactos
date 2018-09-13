
ShellExtensionsps.dll: dlldata.obj ShellExtensions_p.obj ShellExtensions_i.obj
	link /dll /out:ShellExtensionsps.dll /def:ShellExtensionsps.def /entry:DllMain dlldata.obj ShellExtensions_p.obj ShellExtensions_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del ShellExtensionsps.dll
	@del ShellExtensionsps.lib
	@del ShellExtensionsps.exp
	@del dlldata.obj
	@del ShellExtensions_p.obj
	@del ShellExtensions_i.obj
