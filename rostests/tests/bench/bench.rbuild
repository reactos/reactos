<module name="bench-thread" type="win32cui" installbase="bin" installname="bench-thread.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>bench-thread.c</file>
</module>
