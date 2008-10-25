<module name="regdump" type="win32cui" installbase="bin" installname="regdump.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<file>main.c</file>
	<file>regdump.c</file>
	<file>regcmds.c</file>
	<file>regproc.c</file>
</module>
