<module name="syscalldump" type="win32cui" installname="syscalldump.exe">
	<include base="syscalldump">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>dbghelp</library>
	<file>syscalldump.c</file>
</module>
