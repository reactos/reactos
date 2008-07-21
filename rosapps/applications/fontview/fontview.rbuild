<module name="fontview" type="win32gui" installbase="system32" installname="fontview.exe">
	<include base="fontview">.</include>
	<library>gdi32</library>
	<library>user32</library>
	<library>shell32</library>
	<library>kernel32</library>
	<file>fontview.c</file>
	<file>display.c</file>
	<file>fontview.rc</file>
</module>
