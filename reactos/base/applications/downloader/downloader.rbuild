<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<installfile base="system32">downloader.xml</installfile>    
<module name="downloader" type="win32gui" installbase="system32" installname="downloader.exe">
	<include base="downloader">.</include>
	<include base="expat">.</include>

	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<define name="WINVER">0x0501</define>
	<define name="_WIN32_IE>0x0600</define>
	
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<library>msimg32</library>
	<library>shlwapi</library>
	<library>urlmon</library>
	<library>uuid</library>
	<library>expat</library>

	<file>main.c</file>
	<file>xml.c</file>
	<file>download.c</file>
	<file>downloader.rc</file>
</module>
