<module name="regtest" type="win32gui" installbase="bin" installname="regtest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>regtest.c</file>
</module>
