<module name="mode" type="win32cui" installbase="system32" installname="mode.exe">
	<include base="mode">.</include>
	<library>kernel32</library>
	<library>shell32</library>
	<library>user32</library>
	<file>mode.c</file>
	<file>mode.rc</file>
</module>