<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="pstorec_tlb" type="embeddedtypelib">
	<dependency>stdole2</dependency>
	<file>pstorec_tlb.idl</file>
</module>
<module name="pstorec" type="win32dll" baseaddress="${BASEADDRESS_PSTOREC}" installbase="system32" installname="pstorec.dll">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="pstorec.spec" />
	<include base="pstorec" root="intermediate">.</include>
	<dependency>pstorec_tlb</dependency>
	<library>wine</library>
	<library>uuid</library>
	<file>pstorec.c</file>
	<file>rsrc.rc</file>
</module>
</group>
