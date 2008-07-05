<module name="mode" type="win32cui" installbase="system32" installname="mode.exe">
	<include base="mode">.</include>
	<include base="mode">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>shell32</library>
	<library>user32</library>
	<file>mode.c</file>
	<file>mode.rc</file>
</module>