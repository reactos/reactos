<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="kdlib" type="staticlibrary">
	<include base="kdlib">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kddll.c</file>
</module>

<module name="kdserial" type="staticlibrary">
	<include base="kdserial">.</include>
	<library>ntoskrnl</library>
	<file>kdserial.c</file>
</module>

<module name="kdcom" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kdcom.dll">
	<importlibrary definition="kddll.spec"></importlibrary>
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="kdcom.dll" />
	<include base="kdcom">.</include>
	<library>kdlib</library>
	<library>kdserial</library>
	<file>kdcom.c</file>
</module>
</group>
