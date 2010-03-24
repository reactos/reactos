<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="urlmon" type="win32dll" baseaddress="${BASEADDRESS_URLMON}" installbase="system32" installname="urlmon.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="urlmon.spec" />
	<include base="urlmon">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<file>bindctx.c</file>
	<file>binding.c</file>
	<file>bindprot.c</file>
	<file>download.c</file>
	<file>file.c</file>
	<file>format.c</file>
	<file>ftp.c</file>
	<file>gopher.c</file>
	<file>http.c</file>
	<file>internet.c</file>
	<file>mimefilter.c</file>
	<file>mk.c</file>
	<file>protocol.c</file>
	<file>protproxy.c</file>
	<file>regsvr.c</file>
	<file>sec_mgr.c</file>
	<file>session.c</file>
	<file>umon.c</file>
	<file>umstream.c</file>
	<file>uri.c</file>
	<file>urlmon_main.c</file>
	<file>usrmarshal.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>rpcrt4</library>
	<library>ole32</library>
	<library>shlwapi</library>
	<library>wininet</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>pseh</library>
	<library>urlmon_proxy</library>
	<library>ntdll</library>
</module>
<module name="urlmon_proxy" type="rpcproxy" allowwarnings="true">
	<define name="__WINESRC__" />
	<define name="ENTRY_PREFIX">URLMON_</define>
	<define name="PROXY_DELEGATION"/>
	<define name="REGISTER_PROXY_DLL"/>
	<define name="_URLMON_"/>
	<define name="PROXY_CLSID_IS">"{0x79EAC9F1,0xBAF9,0x11CE,{0x8C,0x82,0x00,0xAA,0x00,0x4B,0xA9,0x0B}}"</define>
	<file>urlmon_urlmon.idl</file>
</module>
</group>
