<module name="wintrust" type="win32dll" baseaddress="${BASEADDRESS_WINTRUST}" installbase="system32" installname="wintrust.dll">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="wintrust.spec.def" />
	<include base="wintrust">.</include>
	<include base="ReactOS">include/wine</include>
	<define name="__USE_W32API" />
	<library>wine</library>
	<library>ntdll</library>
	<file>wintrust_main.c</file>
	<file>version.rc</file>
	<file>wintrust.spec</file>
</module>
