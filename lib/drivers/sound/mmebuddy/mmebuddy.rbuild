<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="mmebuddy" type="staticlibrary" allowwarnings="false" unicode="yes">
    <include base="ReactOS">include/reactos/libs/sound</include>
    <file>kernel.c</file>
    <file>nt4.c</file>
    <file>devices.c</file>
    <file>instances.c</file>
    <file>capabilities.c</file>
    <file>thread.c</file>
    <file>utility.c</file>
    <directory name="mme">
        <file>DriverProc.c</file>
        <file>callback.c</file>
    </directory>
    <directory name="wave">
        <file>wodMessage.c</file>
        <file>widMessage.c</file>
        <file>format.c</file>
        <file>streaming.c</file>
        <file>streamcontrol.c</file>
    </directory>
    <directory name="midi">
        <file>modMessage.c</file>
        <file>midMessage.c</file>
    </directory>
    <directory name="mixer">
        <file>mxdMessage.c</file>
    </directory>
    <directory name="auxiliary">
        <file>auxMessage.c</file>
    </directory>
</module>
