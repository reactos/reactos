<module name="mstest" type="win32gui" installbase="bin" installname="mstest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>mstest.c</file>
</module>
