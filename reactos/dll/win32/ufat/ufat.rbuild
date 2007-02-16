<module name="ufat" type="win32dll" baseaddress="${BASEADDRESS_UFAT}" installbase="system32" installname="ufat.dll">
	<importlibrary definition="ufat.def" />
	<include base="ufat">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0600</define>
	<library>vfatlib</library>
	<library>ntdll</library>
	<file>ufat.c</file>
	<file>ufat.rc</file>
</module>
