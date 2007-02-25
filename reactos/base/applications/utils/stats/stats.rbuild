<module name="stats" type="win32cui" installbase="bin" installname="stats.exe" unicode="true" >
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>stats.c</file>
</module>