<?xml version="1.0"?>
<!DOCTYPE directory SYSTEM "../../../tools/rbuild/project.dtd">
<directory name="arch">
	<directory name="i386">
		<if property="ARCH" value="i386">
			<module name="freeldr_arch" type="objectlibrary">
				<include base="freeldr_base">include</include>
				<include base="freeldr_base">cache</include>
				<include base="ntoskrnl">include</include>
				<define name="__USE_W32API" />
				<define name="_NTHAL_" />
				<compilerflag>-ffreestanding</compilerflag>
				<compilerflag>-fno-builtin</compilerflag>
				<compilerflag>-fno-inline</compilerflag>
				<compilerflag>-fno-zero-initialized-in-bss</compilerflag>
				<compilerflag>-Os</compilerflag>
				<file>_alloca.S</file>
				<file>archmach.c</file>
				<file>hardware.c</file>
				<file>hwacpi.c</file>
				<file>hwapm.c</file>
				<file>hwpci.c</file>
				<file>i386disk.c</file>
				<file>i386rtl.c</file>
				<file>i386vid.c</file>
				<file>loader.c</file>
				<file>machpc.c</file>
				<file>pccons.c</file>
				<file>pcdisk.c</file>
				<file>pcmem.c</file>
				<file>pcrtc.c</file>
				<file>pcvideo.c</file>
				<file>portio.c</file>
				<file>machxbox.c</file>
				<file>xboxcons.c</file>
				<file>xboxdisk.c</file>
				<file>xboxfont.c</file>
				<file>xboxhw.c</file>
				<file>xboxi2c.c</file>
				<file>xboxmem.c</file>
				<file>xboxrtc.c</file>
				<file>xboxvideo.c</file>
			</module>
		</if>
	</directory>
	<directory name="powerpc">
		<if property="ARCH" value="powerpc">
			<module name="freeldr_arch" type="objectlibrary">
				<include base="freeldr_base">include</include>
				<include base="freeldr_base">cache</include>
				<include base="ntoskrnl">include</include>
				<include base="ReactOS">include/reactos/libs</include>
				<include base="ReactOS">include/reactos/elf</include>
				<define name="__USE_W32API" />
				<define name="_NTHAL_" />
				<compilerflag>-ffreestanding</compilerflag>
				<compilerflag>-fno-builtin</compilerflag>
				<compilerflag>-fno-inline</compilerflag>
				<compilerflag>-fno-zero-initialized-in-bss</compilerflag>
				<compilerflag>-Os</compilerflag>
				<file>boot.s</file>
				<file>loader.c</file>
				<file>mach.c</file>
				<file>mboot.c</file>
				<file>ofw.c</file>
				<file>ofw_util.s</file>
				<file>ofw_calls.s</file>
				<file>prep.c</file>
				<file>prep_ide.c</file>
				<file>prep_pci.c</file>
				<file>prep_vga.c</file>
			</module>
		</if>
	</directory>
	<directory name="mips">
		<if property="ARCH" value="mips">
			<module name="freeldr_arch" type="objectlibrary">
				<include base="freeldr_base">include</include>
				<include base="freeldr_base">cache</include>
				<include base="ntoskrnl">include</include>
				<define name="__USE_W32API" />
				<define name="DEBUG" />
				<define name="_NTHAL_" />
				<file>boot.s</file>
				<file>console.c</file>
				<file>disk.c</file>
				<file>hardware.c</file>
				<file>loader.c</file>
				<file>mach.c</file>
				<file>video.c</file>
			</module>
		</if>
	</directory>
</directory>
