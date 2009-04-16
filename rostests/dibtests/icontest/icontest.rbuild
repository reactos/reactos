<module name="icontest" type="win32gui" installbase="bin" installname="icontest.exe">
	<include base="icontest">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ntdll</library>
	<file>icontest.c</file>
	<file>icontest.rc</file>
</module>
