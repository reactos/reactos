<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../tools/rbuild/project.dtd">
<group>
<module name="wing32" type="win32dll" entrypoint="0"  installbase="system32" installname="wing32.dll" unicode="yes">
	<importlibrary definition="wing32.spec" />
	<library>user32</library>
	<library>gdi32</library>
	<file>wing32.c</file>
	<linkerflag>--add-stdcall-alias</linkerflag>
</module>
</group>
