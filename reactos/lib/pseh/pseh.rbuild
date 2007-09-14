<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="pseh" type="staticlibrary">
	<define name="__USE_W32API" />
	<if property="ARCH" value="i386">
		<directory name="i386">
			<file>framebased.S</file>
		</directory>
	</if>
	<file>framebased.c</file>
</module>
