<module name="enumws" type="win32gui" installbase="bin" installname="enumws.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>enumws.c</file>
</module>
