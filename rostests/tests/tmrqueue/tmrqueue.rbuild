<module name="tmrqueue" type="win32gui" installbase="bin" installname="tmrqueue.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>tmrqueue.c</file>
</module>
