<module name="dxhaltest" type="win32cui" installbase="bin" installname="dxhaltest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>main.c</file>
</module>
