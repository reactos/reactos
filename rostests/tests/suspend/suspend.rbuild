<module name="suspend" type="win32gui" installbase="bin" installname="suspend.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>suspend.c</file>
</module>
