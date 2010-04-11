<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="notifyhook" type="win32dll" baseaddress="${BASEADDRESS_NOTIFYHOOK}" installbase="system32" installname="notifyhook.dll">
	<importlibrary definition="notifyhook.def" />
	<include base="notifyhook">.</include>
	<define name="_NOTIFYHOOK_IMPL" />
	<library>user32</library>
	<if property="ARCH" value="amd64">
		<!-- Gross hack to work around broken autoexport -->
		<define name="dllexport">aligned(1)</define>
	</if>	
	<file>notifyhook.c</file>
	<file>notifyhook.rc</file>
</module>
