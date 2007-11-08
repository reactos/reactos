<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ksuser" type="win32dll" installbase="system32" installname="ksuser.dll" allowwarnings="true">
	<importlibrary definition="ksuser.def" />
	<library>kernel32</library>
	<library>ntdll</library>
	<file>ksuser.c</file>
	<file>ksuser.rc</file>
</module>
