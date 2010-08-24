<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="apitest" type="staticlibrary">
		<include base="apitest">.</include>
		<file>apitest.c</file>
	</module>

	<directory name="dciman32api">
		<xi:include href="dciman32api/dciman32api.rbuild" />
	</directory>

	<directory name="gdi32">
		<xi:include href="gdi32/gdi32_apitest.rbuild" />
	</directory>

	<directory name="user32api">
		<xi:include href="user32api/user32api.rbuild" />
	</directory>

	<if property="ARCH" value="i386">
		<directory name="w32kdll">
			<xi:include href="w32kdll/directory.rbuild" />
		</directory>

		<directory name="w32knapi">
			<xi:include href="w32knapi/w32knapi.rbuild" />
		</directory>
	</if>

	<directory name="ws2_32">
		<xi:include href="ws2_32/ws2_32.rbuild" />
	</directory>
</group>
