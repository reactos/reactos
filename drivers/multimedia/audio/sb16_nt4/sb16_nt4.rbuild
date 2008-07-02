<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sb16_nt4" type="kernelmodedriver" installbase="system32/drivers" installname="sndblst.sys" allowwarnings="true">
    <linkerflag>-lgcc</linkerflag>
	<include base="sb16_nt4">.</include>
	<include base="sb16_nt4">..</include>
    <include base="ReactOS">include/reactos/libs/sound</include>
	<importlibrary definition="sb16_nt4.def" />
    <library>soundblaster</library>
    <library>audio</library>
    <library>audioleg</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>main.c</file>
</module>
