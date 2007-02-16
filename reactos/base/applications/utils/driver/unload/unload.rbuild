<module name="unload" type="win32cui" installbase="bin" installname="unload.exe" unicode="true">
	<define name="__USE_W32API" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>user32</library>
	<file>unload.c</file>
</module>