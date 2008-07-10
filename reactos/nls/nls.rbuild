<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="3rdparty"><xi:include href="3rdparty/icu4ros.rbuild" /></directory>

	<directory name="lib">
		<directory name="idna"><xi:include href="lib/idna/idna.rbuild" /></directory>
		<directory name="normalize"><xi:include href="lib/normalize/normalize.rbuild" /></directory>
		<directory name="scripts"><xi:include href="lib/scripts/scripts.rbuild" /></directory>
	</directory>

	<directory name="dll">
		<directory name="idndl"><xi:include href="dll/idndl/idndl.rbuild" /></directory>
		<directory name="idndl_redist"><xi:include href="dll/idndl_redist/idndl_redist.rbuild" /></directory>
		<directory name="normaliz"><xi:include href="dll/normaliz/normaliz.rbuild" /></directory>
		<directory name="normaliz_redist"><xi:include href="dll/normaliz_redist/normaliz_redist.rbuild" /></directory>
	</directory>

	<directory name="tests">
		<directory name="normalization"><xi:include href="tests/normalization/normalizationTest.rbuild" /></directory>
	</directory>
</group>
