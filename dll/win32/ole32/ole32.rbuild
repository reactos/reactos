<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="ole32" type="win32dll" baseaddress="${BASEADDRESS_OLE32}" installbase="system32" installname="ole32.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="ole32.spec" />
	<include base="ole32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<define name="_OLE32_" />
	<define name="COM_NO_WINDOWS_H" />
	<library>wine</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ole32_irot_client</library>
	<library>ole32_proxy</library>
	<library>rpcrt4</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>uuid</library>
	<library>pseh</library>
	<file>antimoniker.c</file>
	<file>bindctx.c</file>
	<file>classmoniker.c</file>
	<file>clipboard.c</file>
	<file>compobj.c</file>
	<file>compositemoniker.c</file>
	<file>datacache.c</file>
	<file>defaulthandler.c</file>
	<file>dictionary.c</file>
	<file>enumx.c</file>
	<file>errorinfo.c</file>
	<file>filemoniker.c</file>
	<file>ftmarshal.c</file>
	<file>git.c</file>
	<file>hglobalstream.c</file>
	<file>ifs.c</file>
	<file>itemmoniker.c</file>
	<file>marshal.c</file>
	<file>memlockbytes.c</file>
	<file>moniker.c</file>
	<file>ole2.c</file>
	<file>ole2stubs.c</file>
	<file>ole2impl.c</file>
	<file>ole32_main.c</file>
	<file>oleobj.c</file>
	<file>oleproxy.c</file>
	<file>pointermoniker.c</file>
	<file>regsvr.c</file>
	<file>rpc.c</file>
	<file>stg_bigblockfile.c</file>
	<file>stg_prop.c</file>
	<file>stg_stream.c</file>
	<file>storage32.c</file>
	<file>stubmanager.c</file>
	<file>usrmarshal.c</file>
	<file>ole32res.rc</file>
	<file>dcom.idl</file>
	<file>irot.idl</file>
	<include base="ole32" root="intermediate">.</include>
	<file>ole32.spec</file>
</module>
<module name="ole32_irot_server" type="rpcserver">
	<file>irot.idl</file>
</module>
<module name="ole32_irot_client" type="rpcclient">
	<file>irot.idl</file>
</module>
<module name="ole32_proxy" type="rpcproxy" allowwarnings="true">
	<define name="_OLE32_" />
	<define name="COM_NO_WINDOWS_H" />
	<define name="__WINESRC__" />
	<file>dcom.idl</file>
	<file>ole32_unknwn.idl</file>
	<file>ole32_objidl.idl</file>
	<file>ole32_oleidl.idl</file>
</module>
</group>
