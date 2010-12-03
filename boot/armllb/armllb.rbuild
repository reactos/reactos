<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group>
<module name="armllb" type="kernel" entrypoint="_start" installbase=".." installname="armllb.bin">
	<bootstrap installbase="loader" />
	<library>libcntpr</library>
	<library>rtl</library>
	<include base="armllb">./inc</include>
	<if property="SARCH" value="omap3-beagle">
		<define name="_OMAP3_" />
		<define name="_BEAGLE_" />
		<group linkerset="ld">
			<linkerflag>-Wl,--image-base=0x401FEFF8</linkerflag>
		</group>
	</if>
	<if property="SARCH" value="omap3-zoom2">
		<define name="_OMAP3_" />
		<define name="_ZOOM2_" />
		<group linkerset="ld">
			<linkerflag>--image-base=0x80FFF000</linkerflag>
		</group>
	</if>
	<if property="SARCH" value="versatile">
		<define name="_VERSATILE_" />
		<group linkerset="ld">
			<linkerflag>-Wl,--image-base=0xF000</linkerflag>
		</group>
	</if>
	<file first="true">boot.s</file>
	<file>main.c</file>
	<file>crtsupp.c</file>
	<file>envir.c</file>
	<file>fw.c</file>
	<directory name="hw">
		<file>keyboard.c</file>
		<file>matrix.c</file>
		<file>serial.c</file>
		<file>time.c</file>
		<file>video.c</file>
		<if property="SARCH" value="omap3-zoom2">
			<directory name="omap3-zoom2">
				<file>hwinfo.c</file>
				<file>hwinit.c</file>
				<file>hwlcd.c</file>
				<file>hwsynkp.c</file>
				<file>hwtwl40x.c</file>
				<file>hwuart.c</file>
			</directory>
		</if>
		<if property="SARCH" value="omap3-beagle">
			<directory name="omap3-beagle">
				<file>hwuart.c</file>
				<file>hwinfo.c</file>
				<file>hwinit.c</file>
			</directory>
		</if>
		<if property="SARCH" value="versatile">
			<directory name="versatile">
				<file>hwclcd.c</file>
				<file>hwkmi.c</file>
				<file>hwuart.c</file>
				<file>hwinfo.c</file>
				<file>hwinit.c</file>
			</directory>
		</if>
	</directory>
	<directory name="os">
		<file>loader.c</file>
	</directory>
	<group compilerset="gcc">
		<compilerflag>-fms-extensions</compilerflag>
		<compilerflag>-ffreestanding</compilerflag>
		<compilerflag>-fno-builtin</compilerflag>
		<compilerflag>-fno-inline</compilerflag>
		<compilerflag>-fno-zero-initialized-in-bss</compilerflag>
		<compilerflag>-Os</compilerflag>
		
	</group>
</module>
</group>
