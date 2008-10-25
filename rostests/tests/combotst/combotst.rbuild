<module name="combotst" type="win32gui" installbase="bin" installname="combotst.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>combotst.c</file>
	<file>utils.c</file>
</module>
