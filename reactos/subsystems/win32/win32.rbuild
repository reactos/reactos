<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
<if property="USERMODE" value="1">
	<directory name="csrss">
		<xi:include href="csrss/csrss.rbuild" />
	</directory>
</if>
<directory name="win32k">
	<xi:include href="win32k/win32k.rbuild" />
</directory>
</group>
