<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="cmd">
		<xi:include href="cmd/cmd.rbuild" />
	</directory>
	<ifnot property="ARCH" value="amd64">
		<directory name="explorer">
			<xi:include href="explorer/explorer.rbuild" />
		</directory>
	</ifnot>
	<directory name="explorer-new">
		<xi:include href="explorer-new/explorer.rbuild" />
	</directory>
</group>
