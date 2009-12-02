<module name="symdump" type="win32cui" installbase="system32" installname="symdump.exe">
	<include base="gdihv">.</include>
	<library>dbghelp</library>
	<library>shlwapi</library>
	<file>symdump.c</file>
</module>
