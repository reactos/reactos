<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="example">
		<xi:include href="example/example_drv.rbuild" />
	</directory>
	<directory name="ntos_io">
		<xi:include href="ntos_io/iodriverobject_drv.rbuild" />
		<xi:include href="ntos_io/iohelper_drv.rbuild" />
	</directory>
	<xi:include href="kmtest.rbuild" />
	<xi:include href="kmtest_drv.rbuild" />
</group>
