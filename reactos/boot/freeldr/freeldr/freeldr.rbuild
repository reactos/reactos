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
			<library>rtl_kmode</library>
			<library>libcntpr</library>
			<linkerflag>-static</linkerflag>
			<linkerflag>-lgcc</linkerflag>
		</module>
	</if>
	<if property="ARCH" value="arm">
		<module name="freeldr" type="bootloader" installbase=".." installname="freeldr.sys">
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
			<linkerflag>-lgcc</linkerflag>
			<linkerflag>-static</linkerflag>
			<linkerflag>-Wl,--section-start,pagedata=0x50000</linkerflag>
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
</group>
