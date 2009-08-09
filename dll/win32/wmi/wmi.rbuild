<group>
<module name="wmi" type="win32dll" baseaddress="${BASEADDRESS_WMI}" installbase="system32" installname="wmi.dll" entrypoint="0">
	<importlibrary definition="wmi.spec" />
	<include base="wmi">.</include>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>wmi.rc</file>
</module>
</group>
