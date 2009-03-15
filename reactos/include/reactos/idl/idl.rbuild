<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="pnp_server" type="rpcserver">
	<define name="_WIN32_WINNT">0x600</define>
	<file>pnp.idl</file>
</module>
<module name="pnp_client" type="rpcclient">
	<define name="_WIN32_WINNT">0x600</define>
	<file>pnp.idl</file>
</module>
<module name="scm_server" type="rpcserver">
	<file>svcctl.idl</file>
</module>
<module name="scm_client" type="rpcclient">
	<file>svcctl.idl</file>
</module>
<module name="eventlog_server" type="rpcserver">
	<file>eventlogrpc.idl</file>
</module>
<module name="eventlog_client" type="rpcclient" >
	<file>eventlogrpc.idl</file>
</module>
<module name="lsa_server" type="rpcserver">
	<file>lsa.idl</file>
</module>
<module name="lsa_client" type="rpcclient">
	<file>lsa.idl</file>
</module>
<module name="wlansvc_server" type="rpcserver">
	<file>wlansvc.idl</file>
</module>
<module name="wlansvc_client" type="rpcclient">
	<file>wlansvc.idl</file>
</module>
</group>
