<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="bootvid" type="kernelmodedll" entrypoint="DriverEntry@8" installbase="system32/drivers" installname="bootvid.dll">
	<importlibrary definition="bootvid.def"></importlibrary>
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="bootvid.dll" />
	<include base="bootvid">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>bootvid.c</file>
	<file>bootdata.c</file>
	<file>vga.c</file>
	<file>bootvid.rc</file>
	<pch>precomp.h</pch>
</module>