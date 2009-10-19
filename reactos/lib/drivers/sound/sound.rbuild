<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="legacy">
		<xi:include href="legacy/legacy.rbuild" />
	</directory>
	<directory name="shared">
		<xi:include href="shared/shared.rbuild" />
	</directory>
	<directory name="soundblaster">
		<xi:include href="soundblaster/soundblaster.rbuild" />
	</directory>
	<directory name="uartmidi">
		<xi:include href="uartmidi/uartmidi.rbuild" />
	</directory>
	<ifnot property="ARCH" value="amd64">
		<directory name="mmebuddy">
			<xi:include href="mmebuddy/mmebuddy.rbuild" />
		</directory>
	</ifnot>
	<directory name="mment4">
		<xi:include href="mment4/mment4.rbuild" />
	</directory>
</group>
