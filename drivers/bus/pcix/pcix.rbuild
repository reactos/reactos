<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="pcix" type="kernelmodedriver" installbase="system32/drivers" installname="pcix.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="pcix">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
    <directory name="arb">
        <file>ar_busno.c</file>
        <file>ar_memio.c</file>
        <file>arb_comn.c</file>
        <file>tr_irq.c</file>
    </directory>
    <directory name="intrface">
        <file>agpintrf.c</file>
        <file>busintrf.c</file>
        <file>cardbus.c</file>
        <file>ideintrf.c</file>
        <file>intrface.c</file>
        <file>lddintrf.c</file>
        <file>locintrf.c</file>
        <file>pmeintf.c</file>
        <file>routintf.c</file>
    </directory>
    <directory name="pci">
        <file>busno.c</file>
        <file>config.c</file>
        <file>devhere.c</file>
        <file>id.c</file>
        <file>ppbridge.c</file>
        <file>romimage.c</file>
        <file>state.c</file>
    </directory>
    <file>debug.c</file>
    <file>device.c</file>
    <file>dispatch.c</file>
    <file>enum.c</file>
    <file>fdo.c</file>
    <file>guid.c</file>
    <file>hookhal.c</file>
    <file>init.c</file>
    <file>pcivrify.c</file>
    <file>pdo.c</file>
    <file>power.c</file>
    <file>usage.c</file>
    <file>utils.c</file>
	<file>pci.rc</file>
	<pch>pci.h</pch>
</module>
