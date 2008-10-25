<module name="sertest" type="win32gui" installbase="bin" installname="sertest.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>sertest.c</file>
</module>
