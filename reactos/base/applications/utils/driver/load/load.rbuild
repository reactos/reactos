<module name="load" type="win32cui" installbase="bin" installname="load.exe" unicode="true" >
	<define name="__USE_W32API" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>user32</library>
	<file>load.c</file>
</module>