<module name="more" type="win32cui" installbase="system32" installname="more.exe">
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>more.c</file>
	<file>more.rc</file>
</module>
