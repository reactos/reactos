<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<if property="ARCH" value="arm">
	<module name="hal" type="kernelmodedll">
		<importlibrary definition="hal_arm.def" />
		<include base="ntoskrnl">include</include>
		<library>ntoskrnl</library>
		<define name="_NTHAL_" />
		<file>hal.c</file>
		<file>hal.rc</file>
		<file>hal.spec</file>
	</module>
	</if>
	<if property="ARCH" value="i386">
	<module name="hal" type="kernelmodedll">
		<importlibrary definition="hal.spec" />
		<include base="ntoskrnl">include</include>
		<library>ntoskrnl</library>
		<define name="_NTHAL_" />
		<file>hal.c</file>
		<file>hal.rc</file>
		<file>hal.spec</file>
	</module>
	</if>
	<if property="ARCH" value="i386">
		<module ifnot="false" name="halupalias" type="alias" installbase="system32" installname="hal.dll" aliasof="halup">
		</module>
		<module if="false" name="halmpalias" type="alias" installbase="system32" installname="hal.dll" aliasof="halmp">
		</module>
	</if>
	<if property="ARCH" value="powerpc">
		<module name="halupalias" type="alias" installbase="system32" installname="hal.dll" aliasof="halppc_up"/>
	</if>
	<if property="ARCH" value="amd64">
	<module name="hal" type="kernelmodedll">
		<importlibrary definition="hal_amd64.def" />
		<include base="ntoskrnl">include</include>
		<library>ntoskrnl</library>
		<define name="_NTHAL_" />
		<file>hal.c</file>
		<file>hal.rc</file>
	</module>
	</if>
</group>
