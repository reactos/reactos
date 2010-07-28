<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<if property="USERMODE" value="1">
	<directory name="audio_test">
		<xi:include href="audio_test/audio_test.rbuild" />
	</directory>
	</if>
	<directory name="portcls">
		<xi:include href="portcls/portcls.rbuild" />
	</directory>
</group>
