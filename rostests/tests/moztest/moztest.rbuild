<module name="moztest" type="win32cui" installbase="bin" installname="moztest.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ntdll</library>
	<library>ws2_32</library>
	<file>moztest.c</file>
</module>
