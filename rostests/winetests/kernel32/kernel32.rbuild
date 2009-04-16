<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="kernel32_winetest" type="win32cui" installbase="bin" installname="kernel32_winetest.exe" allowwarnings="true">
    <compilerflag compiler="cc">-Wno-format</compilerflag>
    <include base="kernel32_winetest">.</include>
    <define name="__ROS_LONG64__" />
    <library>ntdll</library>
    <library>kernel32</library>
    <library>advapi32</library>
    <file>actctx.c</file>
    <file>alloc.c</file>
    <file>atom.c</file>
    <file>change.c</file>
    <file>codepage.c</file>
    <file>comm.c</file>
    <file>console.c</file>
    <file>debugger.c</file>
    <file>directory.c</file>
    <file>drive.c</file>
    <file>environ.c</file>
    <file>file.c</file>
    <file>format_msg.c</file>
    <!-- <file>generated.c</file> -->
    <file>heap.c</file>
    <file>interlck.c</file>
    <file>loader.c</file>
    <file>locale.c</file>
    <file>mailslot.c</file>
    <file>module.c</file>
    <file>path.c</file>
    <file>pipe.c</file>
    <file>process.c</file>
    <file>profile.c</file>
    <file>resource.c</file>
    <file>sync.c</file>
    <file>thread.c</file>
    <file>time.c</file>
    <file>timer.c</file>
    <file>toolhelp.c</file>
    <file>version.c</file>
    <file>virtual.c</file>
    <file>volume.c</file>
    <file>testlist.c</file>
</module>
</group>
