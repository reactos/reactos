<module name="systeminfo" type="win32cui" installbase="system32" installname="systeminfo.exe">
	<library>user32</library>
	<library>ntdll</library>
	<library>advapi32</library>
	<library>netapi32</library>
	<library>shlwapi</library>
	<library>iphlpapi</library>
	<library>ws2_32</library>
	<file>systeminfo.c</file>
	<file>systeminfo.rc</file>
	<file>rsrc.rc</file>
</module>
