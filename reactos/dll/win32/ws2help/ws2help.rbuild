<module name="ws2help" type="win32dll" baseaddress="${BASEADDRESS_WS2HELP}" installbase="system32" installname="ws2help.dll" unicode="yes">
	<importlibrary definition="ws2help.def" />
	<include base="ws2help">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>ws2help.c</file>
	<file>ws2help.rc</file>
</module>
