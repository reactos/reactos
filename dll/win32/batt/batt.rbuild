<module name="batt" type="win32dll" baseaddress="${BASEADDRESS_BATT}" installbase="system32" installname="batt.dll" unicode="yes">
	<importlibrary definition="batt.spec" />
	<include base="batt">.</include>
	<library>setupapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>batt.c</file>
	<file>batt.rc</file>
</module>
