<module name="man" type="win32cui" installbase="system32" installname="man.exe">
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>

	<file>man.c</file>
</module>
