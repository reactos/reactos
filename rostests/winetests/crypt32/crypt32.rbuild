<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="crypt32_winetest" type="win32cui" installbase="bin" installname="crypt32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="crypt32_winetest">.</include>
	<file>base64.c</file>
	<file>cert.c</file>
	<file>chain.c</file>
	<file>crl.c</file>
	<file>ctl.c</file>
	<file>encode.c</file>
	<file>main.c</file>
	<file>message.c</file>
	<file>msg.c</file>
	<file>object.c</file>
	<file>oid.c</file>
	<file>protectdata.c</file>
	<file>sip.c</file>
	<file>store.c</file>
	<file>str.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>crypt32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
