<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<ifnot property="ARCH" value="amd64">
		<property name="BASEADDRESS_NTOSKRNL" value="0x80800000" />
	</ifnot>
	<if property="ARCH" value="amd64">
		<property name="BASEADDRESS_NTOSKRNL" value="0xfffff80000800000" />
	</if>
	<module name="ntoskrnl" type="kernel" installbase="system32" installname="ntoskrnl.exe" baseaddress="${BASEADDRESS_NTOSKRNL}" entrypoint="KiSystemStartup" allowwarnings="true">
		<xi:include href="ntoskrnl-generic.rbuild" />
	</module>
</group>
