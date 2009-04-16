<module name="dibtest" type="win32gui" installbase="bin" installname="dibtest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>dibtest.c</file>
</module>
