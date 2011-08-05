<module name="wsock32" type="win32dll" baseaddress="${BASEADDRESS_WSOCK32}" installbase="system32" installname="wsock32.dll" unicode="yes">
	<importlibrary definition="wsock32.spec" />
	<include base="wsock32">.</include>
	<library>ntdll</library>
	<library>ws2_32</library>
	<file>stubs.c</file>
	<file>wsock32.rc</file>
</module>
