<module name="sectest" type="win32gui" installbase="bin" installname="sectest.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>sectest.c</file>
</module>
