<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<if property="ARCH" value="i386">
		<module name="freeldr" type="bootloader">
			<bootstrap installbase="loader" />
			<library>freeldr_startup</library>
			<library>freeldr_base64k</library>
			<library>freeldr_base</library>
			<library>freeldr_arch</library>
			<library>freeldr_main</library>
			<library>rossym</library>
			<library>cmlib</library>
			<library>rtl</library>
			<library>libcntpr</library>
			<group linkerset="ld">
				<linkerflag>-static</linkerflag>
				<linkerflag>-lgcc</linkerflag>
			</group>
		</module>
	</if>
	<if property="ARCH" value="arm">
		<module name="freeldr" type="bootloader" installbase=".." installname="freeldr.sys">
			<bootstrap installbase="loader" />
			<library>freeldr_arch</library>
			<library>freeldr_startup</library>
			<library>freeldr_base64k</library>
			<library>freeldr_base</library>
			<library>freeldr_main</library>
			<library>rossym</library>
			<library>cmlib</library>
			<library>rtl</library>
			<library>libcntpr</library>
			<group linkerset="ld">
				<linkerflag>-lgcc</linkerflag>
				<linkerflag>-Wl,--image-base=0x80FFF000</linkerflag>
			</group>
		</module>
	</if>
	<if property="ARCH" value="powerpc">
		<module name="ofwldr" type="elfexecutable" buildtype="OFWLDR">
			<library>freeldr_startup</library>
			<library>freeldr_base64k</library>
			<library>freeldr_base</library>
			<library>freeldr_arch</library>
			<library>freeldr_main</library>
			<library>rossym</library>
			<library>cmlib</library>
			<library>rtl</library>
			<library>libcntpr</library>
			<library>ppcmmu</library>
		</module>
	</if>
	<if property="ARCH" value="amd64">
		<module name="freeldr" type="bootloader">
			<bootstrap installbase="loader" />
			<library>freeldr_startup</library>
			<library>freeldr_base64k</library>
			<library>freeldr_base</library>
			<library>freeldr_arch</library>
			<library>freeldr_main</library>
			<library>rossym</library>
			<library>cmlib</library>
			<library>rtl</library>
			<library>libcntpr</library>
			<group linkerset="ld">
				<linkerflag>-static</linkerflag>
				<linkerflag>-lgcc</linkerflag>
			</group>
		</module>
	</if>
</group>
