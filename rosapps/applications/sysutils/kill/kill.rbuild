<module name="kill" type="win32cui" installbase="system32" installname="kill.exe">
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>

	<file>kill.c</file>
	<file>kill.rc</file>
</module>
