<module name="wmilib" type="exportdriver" installbase="system32/drivers" installname="wmilib.sys">
	<importlibrary definition="wmilib.def" />
	<include base="wmilib">.</include>
	<library>ntoskrnl</library>
	<file>wmilib.c</file>
	<file>wmilib.rc</file>
</module>
