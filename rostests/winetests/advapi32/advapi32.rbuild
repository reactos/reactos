<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="advapi32_winetest" type="win32cui" installbase="bin" installname="advapi32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
    <include base="advapi32_winetest">.</include>
    <define name="__USE_W32API" />
    <define name="__ROS_LONG64__" />
    <library>advapi32</library>
    <library>kernel32</library>
    <library>ntdll</library>
    <library>uuid</library>
    <library>ole32</library>
    <file>cred.c</file>
    <file>crypt.c</file>
    <file>crypt_lmhash.c</file>
    <file>crypt_md4.c</file>
    <file>crypt_md5.c</file>
    <file>crypt_sha.c</file>
    <file>lsa.c</file>
    <file>registry.c</file>
    <file>security.c</file>
    <file>service.c</file>
    <file>testlist.c</file>
</module>
</group>
