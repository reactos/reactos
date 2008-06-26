<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="3rdparty"><xi:include href="3rdparty/icu4ros.rbuild" /></directory>

	<directory name="lib">
		<directory name="normalize"><xi:include href="lib/normalize/normalize.rbuild" /></directory>
	</directory>

	<directory name="dll">
		<directory name="normaliz_redist"><xi:include href="dll/normaliz_redist/normaliz_redist.rbuild" /></directory>
	</directory>
</group>
