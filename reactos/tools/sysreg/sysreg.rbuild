<?xml version="1.0"?>

<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="sysreg" type="buildtool">
	<if property="HOST" value="mingw32-linux">
		<define name="__LINUX__" />
	</if>
	<!--ifnot property="HOST" value="mingw32-linux">
		<include base="ReactOS">include</include>
		<include base="ReactOS">include/reactos</include>
		<include base="ReactOS">include/psdk</include>
		<include base="ReactOS">include/crt</include>
		<include base="ReactOS">include/reactos/libs</include>
		<include base="ReactOS" root="intermediate">include/psdk</include>
	</ifnot-->
	<file>conf_parser.cpp</file>
	<file>env_var.cpp</file>
	<file>rosboot_test.cpp</file>
	<file>namedpipe_reader.cpp</file>
	<file>pipe_reader.cpp</file>
	<file>sysreg.cpp</file>
	<file>file_reader.cpp</file>
	<file>os_support.cpp</file>
	<file>timer_command_callback.cpp</file>
</module>
