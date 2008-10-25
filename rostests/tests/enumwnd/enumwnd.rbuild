<module name="enumwnd" type="win32gui" installbase="bin" installname="enumwnd.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>enumwnd.c</file>
</module>
