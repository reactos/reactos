<module name="touch" type="win32cui" installbase="system32" installname="touch.exe">
	<include base="touch">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>err.c</file>
	<file>touch.c</file>
	<file>touch.rc</file>
</module>