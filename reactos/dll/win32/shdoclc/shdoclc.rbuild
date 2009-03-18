<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="shdoclc" type="win32dll" installbase="system32" installname="shdoclc.dll" entrypoint="0" allowwarnings="true">
	<importlibrary definition="shdoclc.spec" />
	<include base="shdocvw">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>kernel32</library>
	<file>rsrc.rc</file>
</module>
