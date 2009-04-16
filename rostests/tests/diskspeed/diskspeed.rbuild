<module name="diskspeed" type="win32cui" installbase="bin" installname="diskspeed.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>diskspeed.c</file>
</module>
