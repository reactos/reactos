<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="atapi">
		<xi:include href="atapi/atapi.rbuild" />
	</directory>
	<directory name="pciide">
		<xi:include href="pciide/pciide.rbuild" />
	</directory>
	<directory name="pciidex">
		<xi:include href="pciidex/pciidex.rbuild" />
	</directory>
	
	<!-- Currently excluded from build, someone else please check the driver first
	<directory name="uniata">
		<xi:include href="uniata/uniata.rbuild" />
	</directory>
	-->
</group>
