<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="bootvid" type="kernelmodedll" entrypoint="0" installbase="system32/drivers" installname="bootvid.dll">
	<importlibrary definition="bootvid.spec"></importlibrary>
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="bootvid.dll" />
	<include base="bootvid">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<if property="ARCH" value="i386">
		<directory name="i386">
			<file>bootvid.c</file>
			<file>bootdata.c</file>
			<file>vga.c</file>
		</directory>
	</if>
	<if property="ARCH" value="arm">
		<directory name="arm">
			<file>bootvid.c</file>
			<file>bootdata.c</file>
		</directory>
	</if>
	<file>bootvid.rc</file>
	<pch>precomp.h</pch>
</module>
