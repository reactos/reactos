<module name="tcat" type="win32cui" installbase="system32" installname="tcat.exe">
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>

	<file>cat.c</file>
</module>
