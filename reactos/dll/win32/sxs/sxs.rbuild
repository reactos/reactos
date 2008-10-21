<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="sxs" type="win32dll" baseaddress="${BASEADDRESS_SXS}" installbase="system32" installname="sxs.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="sxs.spec" />
        <include base="sxs">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <define name="WINVER">0x600</define>
        <define name="_WIN32_WINNT">0x600</define>
        <file>sxs.c</file>
        <file>sxs.spec</file>
        <library>wine</library>
        <library>kernel32</library>
        <library>ntdll</library>
</module>
</group>