<module name="samlib" type="win32dll" baseaddress="${BASEADDRESS_SAMLIB}" installbase="system32" installname="samlib.dll">
	<importlibrary definition="samlib.def" />
	<include base="samlib">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>dllmain.c</file>
	<file>samlib.c</file>
	<file>samlib.rc</file>
</module>
