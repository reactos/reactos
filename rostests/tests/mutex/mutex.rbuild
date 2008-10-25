<module name="mutex" type="win32gui" installbase="bin" installname="mutex.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>mutex.c</file>
</module>
