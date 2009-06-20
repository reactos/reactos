<module name="psapi" type="win32dll" baseaddress="${BASEADDRESS_PSAPI}" installbase="system32" installname="psapi.dll">
	<importlibrary definition="psapi.spec" />
	<include base="psapi">.</include>
	<include base="psapi">include</include>
	<library>epsapi</library>
	<library>pseh</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38269
	<pch>precomp.h</pch>
	-->
	<file>malloc.c</file>
	<file>psapi.c</file>
	<file>psapi.rc</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>
