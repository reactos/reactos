<module name="nts2w32err" type="win32cui" installbase="bin" installname="nts2w32err.exe" >
	<define name="__USE_W32API" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>nts2w32err.c</file>
</module>