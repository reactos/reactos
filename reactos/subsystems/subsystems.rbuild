<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<if property="USERMODE" value="1">
		<if property="ARCH" value="i386">
			<directory name="ntvdm">
			<xi:include href="ntvdm/ntvdm.rbuild" />
			</directory>
		</if>
	</if>

<directory name="win32">
	<xi:include href="win32/win32.rbuild" />
</directory>
</group>
