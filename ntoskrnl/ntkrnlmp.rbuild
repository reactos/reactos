<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="ntkrnlmp" type="kernel" installbase="system32" installname="ntkrnlmp.exe">
		<define name="CONFIG_SMP" />

		<xi:include href="ntoskrnl-generic.rbuild" />
	</module>
</group>