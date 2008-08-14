<module name="symdump" type="win32cui" installbase="system32" installname="symdump.exe">
	<include base="gdihv">.</include>
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>dbghelp</library>
	<library>shlwapi</library>
	<library>kernel32</library>
	<file>symdump.c</file>
</module>
