<module name="oleacc" type="win32dll" baseaddress="${BASEADDRESS_OLEACC}" installbase="system32" installname="oleacc.dll">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="oleacc.spec.def" />
	<include base="oleacc">.</include>
	<include base="ReactOS">include/wine</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>wine</library>
	<file>main.c</file>
	<file>oleacc.spec</file>
</module>
