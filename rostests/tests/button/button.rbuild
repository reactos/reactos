<module name="button" type="win32gui" installbase="bin" installname="button.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>buttontst.c</file>
</module>
