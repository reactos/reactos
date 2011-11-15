<module name="ws2help" type="win32dll" baseaddress="${BASEADDRESS_WS2HELP}" installbase="system32" installname="ws2help.dll">
	<importlibrary definition="ws2help.spec" />
	<include base="ws2help">.</include>
	<include base="ReactOS">include/reactos/winsock</include>
	<library>advapi32</library>
	<library>ntdll</library>
	<library>ws2_32</library>
	<file>apc.c</file>
	<file>context.c</file>
	<file>dllmain.c</file>
	<file>handle.c</file>
	<file>notify.c</file>
	<file>ws2help.rc</file>
	<pch>precomp.h</pch>
</module>
