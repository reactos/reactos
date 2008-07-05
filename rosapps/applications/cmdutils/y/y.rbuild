<module name="y" type="win32cui" installbase="system32" installname="y.exe">
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<file>y.c</file>
	<file>y.rc</file>
</module>
