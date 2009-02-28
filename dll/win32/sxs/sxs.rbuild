<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="sxs" type="win32dll" baseaddress="${BASEADDRESS_SXS}" installbase="system32" installname="sxs.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="sxs.spec" />
        <include base="sxs">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <file>sxs.c</file>
        <library>wine</library>
        <library>kernel32</library>
        <library>ntdll</library>
</module>
</group>