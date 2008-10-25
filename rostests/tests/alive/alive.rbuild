<module name="alive" type="win32cui" installbase="bin" installname="alive.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>alive.c</file>
</module>
