<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<module name="hal" type="kernelmodedll">
		<importlibrary basename="hal" definition="hal.def" />
		<include base="ntoskrnl">include</include>
		<library>ntoskrnl</library>
		<define name="_NTHAL_" />
		<define name="__USE_W32API" />
		<linkerflag>-enable-stdcall-fixup</linkerflag>
		<file>hal.c</file>
		<file>hal.rc</file>
	</module>
	<if property="ARCH" value="i386">
		<module ifnot="${MP}" name="halupalias" type="alias" installbase="system32" installname="hal.dll" aliasof="halup">
		</module>
		<module if="${MP}" name="halmpalias" type="alias" installbase="system32" installname="hal.dll" aliasof="halmp">
		</module>
	</if>
	<if property="ARCH" value="powerpc">
		<module name="halupalias" type="alias" installbase="system32" installname="hal.dll" aliasof="halppc_up"/>
	</if>
</group>
