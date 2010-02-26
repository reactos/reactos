<module name="psapi" type="win32dll" baseaddress="${BASEADDRESS_PSAPI}" installbase="system32" installname="psapi.dll">
	<importlibrary definition="psapi.spec" />
	<include base="psapi">.</include>
	<include base="psapi">include</include>
	<library>epsapi</library>
	<library>pseh</library>
	<library>ntdll</library>
	<pch>precomp.h</pch>
	<file>malloc.c</file>
	<file>psapi.c</file>
	<file>psapi.rc</file>
</module>
