<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="dciman32" type="win32dll" baseaddress="${BASEADDRESS_DCIMAN32}" installbase="system32" installname="dciman32.dll" allowwarnings="true">
	<importlibrary definition="dciman32.spec" />
	<include base="dciman32">.</include>
	<file>dciman_main.c</file>
	<file>dciman32.spec</file>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>gdi32</library>	
	<library>user32</library>	
</module>
</group>
