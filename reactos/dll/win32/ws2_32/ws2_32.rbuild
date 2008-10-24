<module name="ws2_32" type="win32dll" baseaddress="${BASEADDRESS_WS2_32}" installbase="system32" installname="ws2_32.dll" unicode="yes">
	<importlibrary definition="ws2_32.spec" />
	<include base="ws2_32">include</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="LE" />
	<define name="_WIN32_WINNT">0x0500</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>dnsapi</library>
	<directory name="include">
		<pch>ws2_32.h</pch>
	</directory>
	<directory name="misc">
		<file>bsd.c</file>
		<file>catalog.c</file>
		<file>dllmain.c</file>
		<file>event.c</file>
		<file>handle.c</file>
		<file>ns.c</file>
		<file>sndrcv.c</file>
		<file>stubs.c</file>
		<file>upcall.c</file>
		<file>async.c</file>
	</directory>
	<file>ws2_32.rc</file>
	<file>ws2_32.spec</file>
</module>

