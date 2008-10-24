<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="shfolder" type="win32dll" baseaddress="${BASEADDRESS_SHFOLDER}" installbase="system32" installname="shfolder.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="shfolder.spec" />
	<include base="shfolder">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>version.rc</file>
	<file>shfolder.spec</file>
	<library>wine</library>
	<library>shell32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
