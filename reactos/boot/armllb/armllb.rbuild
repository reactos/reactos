<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
		<module name="armllb" type="bootloader" installbase=".." installname="armllb.bin">
			<bootstrap installbase="loader" />
			<library>libcntpr</library>
			<library>rtl</library>
			<include base="armllb">./inc</include>
			<if property="SARCH" value="omap3">
                <define name="_OMAP3_" />
                <group linkerset="ld">
                    <linkerflag>-Wl,--image-base=0x401FEFF8</linkerflag>
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
            <directory name="hw">
                <file>serial.c</file>
                <file>video.c</file>
                <if property="SARCH" value="omap3">
                    <directory name="omap3">
                        <file>hwdata.c</file>
                        <file>hwdss.c</file>
                        <file>hwuart.c</file>
                        <file>hwinfo.c</file>
                        <file<hwinit.c</file>
                    </directory>
                </if>
                <if property="SARCH" value="versatile">
                    <directory name="versatile">
                        <file>hwclcd.c</file>
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
            <group linkerset="ld">
                <linkerflag>-lgcc</linkerflag>
            </group>
		</module>
</group>
