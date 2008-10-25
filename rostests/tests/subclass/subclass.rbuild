<module name="subclass" type="win32gui" installbase="bin" installname="subclass.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>subclass.c</file>
</module>
