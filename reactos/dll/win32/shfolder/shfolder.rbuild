<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="shfolder" type="win32dll" baseaddress="${BASEADDRESS_SHFOLDER}" installbase="system32" installname="shfolder.dll" entrypoint="0">
	<importlibrary definition="shfolder.spec" />
	<include base="shfolder">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>version.rc</file>
	<library>wine</library>
	<library>shell32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
