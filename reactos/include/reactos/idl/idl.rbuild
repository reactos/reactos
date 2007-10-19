<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="pnp_server" type="rpcserver">
	<include base="ReactOS">.</include>
	<include base="ReactOS">w32api/include</include>
	<file>pnp.idl</file>
</module>
<module name="pnp_client" type="rpcclient">
	<include base="ReactOS">.</include>
	<include base="ReactOS">w32api/include</include>
	<file>pnp.idl</file>
</module>
<module name="scm_server" type="rpcserver">
	<include base="ReactOS">.</include>
	<include base="ReactOS">w32api/include</include>
	<file switches="--oldnames">svcctl.idl</file>
</module>
<module name="scm_client" type="rpcclient">
	<include base="ReactOS">.</include>
	<include base="ReactOS">w32api/include</include>
	<file switches="--oldnames">svcctl.idl</file>
</module>
<module name="eventlog_server" type="rpcserver">
	<include base="ReactOS">.</include>
	<include base="ReactOS">w32api/include</include>
	<file switches="--oldnames">eventlogrpc.idl</file>
</module>
<module name="eventlog_client" type="rpcclient" >
	<include base="ReactOS">.</include>
	<include base="ReactOS">w32api/include</include>
	<file switches="--oldnames">eventlogrpc.idl</file>
</module>
<module name="lsa_server" type="rpcserver">
	<include base="ReactOS">.</include>
	<include base="ReactOS">w32api/include</include>
	<file switches="--oldnames">lsa.idl</file>
</module>
<module name="lsa_client" type="rpcclient">
	<include base="ReactOS">.</include>
	<include base="ReactOS">w32api/include</include>
	<file switches="--oldnames">lsa.idl</file>
</module>
</group>
