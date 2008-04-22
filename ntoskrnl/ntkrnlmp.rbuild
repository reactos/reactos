<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../tools/rbuild/project.dtd">
<module name="ntkrnlmp" type="kernel" installbase="system32" installname="ntkrnlmp.exe">
	<define name="CONFIG_SMP" />

	<xi:include href="ntoskrnl-generic.rbuild" />
</module>
