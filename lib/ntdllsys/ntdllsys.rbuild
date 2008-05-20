<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="ntdllsys" type="staticlibrary">
	<define name="_DISABLE_TIDENTS" />
	<directory name="." root="intermediate">
		<file>ntdll.S</file>
	</directory>
</module>
