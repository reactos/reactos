<module name="mktime" type="win32gui" installbase="bin" installname="mktime.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>mktime.c</file>
</module>
