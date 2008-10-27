<module name="linetest" type="win32gui" installbase="bin" installname="linetest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>linetest.c</file>
</module>
