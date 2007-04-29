<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<group>
<directory name="cmd">
	<xi:include href="cmd/cmd_test.rbuild" />
</directory>
<directory name="gdi32">
	<xi:include href="gdi32/gdi32_test.rbuild" />
</directory>
<directory name="kernel32">
	<xi:include href="kernel32/directory.rbuild" />
</directory>
<directory name="kmtloader">
	<xi:include href="kmtloader/kmtloader.rbuild" />
</directory>
<directory name="smss">
	<xi:include href="smss/smss.rbuild" />
</directory>
<directory name="user32">
	<xi:include href="user32/user32.rbuild" />
</directory>
<directory name="win32k">
	<xi:include href="win32k/win32k.rbuild" />
</directory>
</group>
