<module name="apc" type="win32cui" installbase="bin" installname="apc.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ntdll</library>
	<file>apc.c</file>
</module>
