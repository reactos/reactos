<module name="nptest" type="win32gui" installbase="bin" installname="nptest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>nptest.c</file>
</module>
