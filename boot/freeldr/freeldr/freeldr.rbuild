<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">

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

<if property="ARCH" value="arm">
	<module name="freeldr" type="kernel" entrypoint="_start" baseaddress="$(BASEADDRESS_FREELDR)">
		<linkerscript>freeldr_$(ARCH).lnk</linkerscript>
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
	</module>
</if>

<ifnot property="ARCH" value="powerpc">
	<module name="freeldr" type="bootloader" baseaddress="$(BASEADDRESS_FREELDR)">
		<linkerscript>freeldr_$(ARCH).lnk</linkerscript>
		<bootstrap installbase="loader" />
		<library>freeldr_startup</library>
		<library>freeldr_base64k</library>
		<library>freeldr_base</library>
		<if property="ARCH" value="i386">
			<library>mini_hal</library>
		</if>
		<library>freeldr_arch</library>
		<library>freeldr_main</library>
		<library>rossym</library>
		<library>cmlib</library>
		<library>rtl</library>
		<library>libcntpr</library>
		<library>cportlib</library>
		<group linkerset="ld">
			<linkerflag>-static</linkerflag>
			<linkerflag>-lgcc</linkerflag>
		</group>
	</module>
</ifnot>
