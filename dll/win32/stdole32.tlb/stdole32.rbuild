<group>
<module name="std_ole_v1" type="embeddedtypelib">
	<include base="std_ole_v1">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>std_ole_v1.idl</file>
</module>
<module name="stdole32.tlb" type="win32dll" extension=".tlb" installbase="system32" installname="stdole32.tlb" entrypoint="0">
	<importlibrary definition="stdole32.tlb.spec" />
	<include base="stdole32.tlb">.</include>
	<include base="stdole32.tlb" root="intermediate">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<dependency>std_ole_v1</dependency>
	<define name="__WINESRC__" />
	<library>kernel32</library>
	<file>rsrc.rc</file>
</module>
</group>