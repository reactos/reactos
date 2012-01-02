<module name="snmpapi" type="win32dll" baseaddress="${BASEADDRESS_SNMPAPI}" allowwarnings="true" installbase="system32" installname="snmpapi.dll" unicode="yes">
	<importlibrary definition="snmpapi.spec" />
	<include base="snmpapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<library>ntdll</library>
	<library>wine</library>
	<file>main.c</file>
	<file>snmpapi.rc</file>
</module>
