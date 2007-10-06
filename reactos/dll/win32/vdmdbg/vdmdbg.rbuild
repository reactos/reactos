<module name="vdmdbg" type="win32dll" baseaddress="${BASEADDRESS_VDMDBG}" installbase="system32" installname="vdmdbg.dll">
	<importlibrary definition="vdmdbg.def" />
	<include base="vdmdbg">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<file>vdmdbg.c</file>
	<pch>vdmdbg.h</pch>
</module>
