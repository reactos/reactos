<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kdcom" type="kernelmodedll" entrypoint="DriverEntry@8" installbase="system32/drivers" installname="kdcom.dll">
	<importlibrary definition="kdcom.def"></importlibrary>
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="kdcom.dll" />
	<include base="kdcom">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kdbg.c</file>
</module>
