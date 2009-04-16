<module name="accelerator" type="win32gui" installbase="bin" installname="accelerator.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>accelerator.c</file>
</module>
