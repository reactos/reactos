<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="roshttpd" type="win32cui" installbase="system32" installname="roshttpd.exe" stdlib="host">
	<include base="roshttpd">include</include>
	<define name="__USE_W32_SOCKETS" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>user32</library>
	<compilerflag compiler="cxx" compilerset="gcc">-Wno-non-virtual-dtor</compilerflag>
	<file>config.cpp</file>
	<file>error.cpp</file>
	<file>http.cpp</file>
	<file>httpd.cpp</file>
	<file>roshttpd.cpp</file>
	<directory name="common" >
		<file>list.cpp</file>
		<file>roshttpd.rc</file>
		<file>socket.cpp</file>
		<file>thread.cpp</file>
	</directory>
</module>
