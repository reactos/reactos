<module name="mswsock" type="win32dll" baseaddress="${BASEADDRESS_MSWSOCK}" installbase="system32" installname="mswsock.dll" unicode="yes">
	<importlibrary definition="mswsock.spec" />
	<define name="LE" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>extensions.c</file>
	<file>stubs.c</file>
	<file>mswsock.rc</file>
</module>
