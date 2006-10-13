<module name="cat" type="win32cui" installbase="bin" installname="cat.exe" >
	<define name="__USE_W32API" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>cat.c</file>
</module>