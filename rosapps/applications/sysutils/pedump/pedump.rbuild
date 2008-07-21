<module name="pedump" type="win32cui" installbase="system32" installname="pedump.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>pedump.c</file>
	<file>pedump.rc</file>
</module>
