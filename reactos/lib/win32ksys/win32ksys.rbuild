<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="win32ksys" type="staticlibrary">
	<define name="_DISABLE_TIDENTS" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />

	<directory name="." root="intermediate">
		<file>win32k.S</file>
	</directory>
</module>
