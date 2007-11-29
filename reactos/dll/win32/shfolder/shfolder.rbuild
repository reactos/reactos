<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="shfolder" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_SHFOLDER}" installbase="system32" installname="shfolder.dll" allowwarnings="true">
	<importlibrary definition="shfolder.spec.def" />
	<include base="shfolder">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>shell32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>version.rc</file>
	<file>shfolder.spec</file>
</module>
