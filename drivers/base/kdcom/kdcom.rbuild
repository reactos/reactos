<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kdcom" type="kernelmodedll" entrypoint="DriverEntry@8" installbase="system32" installname="kdcom.dll">
	<importlibrary definition="kdcom.spec"></importlibrary>
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="kdcom.dll" />
	<include base="kdcom">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<if property="ARCH" value="i386">
		<directory name="i386">
				<file>kdbg.c</file>
		</directory>
	</if>
	<if property="ARCH" value="arm">
		<directory name="arm">
				<file>kdbg.c</file>
		</directory>
	</if>
	<file>kdcom.spec</file>
</module>
