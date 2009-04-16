<module name="terminate" type="win32cui" installbase="bin" installname="terminate.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>terminate.c</file>
</module>
