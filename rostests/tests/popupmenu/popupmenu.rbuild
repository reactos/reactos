<module name="popupmenu" type="win32gui" installbase="bin" installname="popupmenu.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<include base="popupmenu">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>popupmenu.c</file>
	<file>popupmenu.rc</file>
</module>
