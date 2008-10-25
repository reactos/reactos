<module name="vmtest" type="win32cui" installbase="bin" installname="vmtest.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>vmtest.c</file>
</module>
