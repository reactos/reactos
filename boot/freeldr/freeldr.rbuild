<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">

	<if property="ARCH" value="i386">
		<property name="BASEADDRESS_FREELDR" value="0x8000" />
	</if>
	<if property="ARCH" value="amd64">
		<property name="BASEADDRESS_FREELDR" value="0x8000" />
	</if>
	<if property="ARCH" value="arm">
		<if property="SARCH" value="omap3-beagle">
			<property name="BASEADDRESS_FREELDR" value="0x80FFF000" />
		</if>
		<if property="SARCH" value="omap3-zoom2">
			<define name="_ZOOM2_" />
			<property name="BASEADDRESS_FREELDR" value="0x8106F000" />
		</if>
		<if property="SARCH" value="versatile">
			<property name="BASEADDRESS_FREELDR" value="0x0001F000" />
		</if>
	</if>

	<directory name="bootsect">
		<xi:include href="bootsect/bootsect.rbuild" />
	</directory>
	<directory name="freeldr">
		<xi:include href="freeldr/freeldr_startup.rbuild" />
		<xi:include href="freeldr/freeldr_base64k.rbuild" />
		<xi:include href="freeldr/freeldr_base.rbuild" />
		<xi:include href="freeldr/freeldr_arch.rbuild" />
		<xi:include href="freeldr/freeldr_main.rbuild" />
		<xi:include href="freeldr/freeldr.rbuild" />
		<if property="ARCH" value="i386">
			<xi:include href="freeldr/setupldr_main.rbuild" />
			<xi:include href="freeldr/setupldr.rbuild" />
		</if>
		<if property="ARCH" value="ppc">
			<xi:include href="freeldr/setupldr_main.rbuild" />
			<xi:include href="freeldr/setupldr.rbuild" />
		</if>
		<if property="ARCH" value="amd64">
			<xi:include href="freeldr/setupldr_main.rbuild" />
			<xi:include href="freeldr/setupldr.rbuild" />
		</if>
	</directory>
</group>
