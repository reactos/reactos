<module name="count" type="win32cui" installbase="bin" installname="count.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>count.c</file>
</module>
