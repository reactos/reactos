<module name="vbltest" type="win32gui" installbase="bin" installname="vbltest.exe">
	<define name="__USE_W32API" />
	<include base="vbltest">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>vbltest.c</file>
	<file>vbltest.rc</file>
</module>