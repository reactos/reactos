<module name="psapi" type="win32dll" baseaddress="${BASEADDRESS_PSAPI}" installbase="system32" installname="psapi.dll">
	<importlibrary definition="psapi.def" />
	<include base="psapi">.</include>
	<include base="psapi">include</include>
	<define name="_DISABLE_TIDENTS" />
	<library>epsapi</library>
	<library>pseh</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<pch>precomp.h</pch>
	<file>malloc.c</file>
	<file>psapi.c</file>
	<file>psapi.rc</file>
</module>
