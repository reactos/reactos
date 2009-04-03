<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="olecli32" type="win32dll" baseaddress="${BASEADDRESS_OLECLI32}" installbase="system32" installname="olecli32.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="olecli32.spec" />
        <include base="olecli32">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <file>olecli_main.c</file>
        <library>wine</library>
        <library>ole32</library>
        <library>gdi32</library>
        <library>kernel32</library>
        <library>ntdll</library>
</module>
</group>
