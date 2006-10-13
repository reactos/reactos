<module name="partinfo" type="win32cui" installbase="bin" installname="partinfo.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>partinfo.c</file>
</module>