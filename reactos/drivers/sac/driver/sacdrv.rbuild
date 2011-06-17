<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sacdrv" type="kernelmodedriver" installbase="system32/drivers" installname="sacdrv.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<include base="sacdrv">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<dependency>sacmsg</dependency>
	<if property="ARCH" value="i386">
		<group compilerset="gcc">
			<compilerflag>-mrtd</compilerflag>
			<compilerflag>-fno-builtin</compilerflag>
		</group>
	</if>
	<pch>sacdrv.h</pch>
	<file>chanmgr.c</file>
	<file>channel.c</file>
	<file>cmdchan.c</file>
	<file>concmd.c</file>
	<file>conmgr.c</file>
	<file>data.c</file>
	<file>dispatch.c</file>
	<file>init.c</file>
	<file>memory.c</file>
	<file>rawchan.c</file>
	<file>util.c</file>
	<file>vtutf8chan.c</file>
	<file>sacdrv.rc</file>
</module>
