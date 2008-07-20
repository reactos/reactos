<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cdfs" type="kernelmodedriver" installbase="system32/drivers" installname="cdfs.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="cdfs">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
    <file>allocsup.c</file>
    <file>cachesup.c</file>
    <file>cddata.c</file>
    <file>cdinit.c</file>
    <file>cleanup.c</file>
    <file>close.c</file>
    <file>create.c</file>
    <file>devctrl.c</file>
    <file>deviosup.c</file>
    <file>dirctrl.c</file>
    <file>dirsup.c</file>
    <file>fileinfo.c</file>
    <file>filobsup.c</file>
    <file>fsctrl.c</file>
    <file>fspdisp.c</file>
    <file>lockctrl.c</file>
    <file>namesup.c</file>
    <file>pathsup.c</file>
    <file>pnp.c</file>
    <file>prefxsup.c</file>
    <file>read.c</file>
    <file>resrcsup.c</file>
    <file>strucsup.c</file>
    <file>verfysup.c</file>
    <file>volinfo.c</file>
    <file>workque.c</file>
	<file>cdfs.rc</file>
	<pch>cdprocs.h</pch>
</module>
