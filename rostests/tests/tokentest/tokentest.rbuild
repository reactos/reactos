<module name="tokentest" type="win32gui" installbase="bin" installname="tokentest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>tokentest.c</file>
</module>
