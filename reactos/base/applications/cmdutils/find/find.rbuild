<module name="find" type="win32cui" installbase="system32" installname="find.exe">
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<file>find.c</file>
	<file>find.rc</file>
	<file>rsrc.rc</file>
</module>
