<module name="touch" type="win32cui" installbase="system32" installname="touch.exe">
	<include base="touch">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>err.c</file>
	<file>touch.c</file>
	<file>touch.rc</file>
</module>