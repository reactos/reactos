<module name="multiwin" type="win32gui" installbase="bin" installname="multiwin.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>multiwin.c</file>
</module>
