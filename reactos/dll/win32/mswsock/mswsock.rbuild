<module name="mswsock" type="win32dll" baseaddress="${BASEADDRESS_MSWSOCK}" installbase="system32" installname="mswsock.dll">
	<importlibrary definition="mswsock.def" />
	<define name="UNICODE" />
	<define name="LE" />
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>extensions.c</file>
	<file>stubs.c</file>
	<file>mswsock.rc</file>
</module>
