<module name="syscalldump" type="win32cui" installname="syscalldump.exe">
	<include base="syscalldump">.</include>
	<library>kernel32</library>
	<library>dbghelp</library>
	<file>syscalldump.c</file>
</module>
