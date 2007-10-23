<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="win32ksys" type="staticlibrary">
	<define name="_DISABLE_TIDENTS" />
	<directory name="." root="intermediate">
		<file>win32k.S</file>
	</directory>
</module>
