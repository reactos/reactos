<module name="lineclip" type="win32gui" installbase="bin" installname="lineclip.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>lineclip.c</file>
</module>
