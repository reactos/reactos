<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="freeldr">
		<xi:include href="freeldr/freeldr.rbuild" />
	</directory>
    <if property="ARCH" value="arm">
    	<directory name="armllb">
	    	<xi:include href="armllb/armllb.rbuild" />
    	</directory>
    </if>
	<directory name="bootdata">
		<xi:include href="bootdata/bootdata.rbuild" />
	</directory>
</group>
