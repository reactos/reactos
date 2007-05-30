<group>
	<directory name="os2">
		<xi:include href="os2/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="posix">
		<xi:include href="posix/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="rosapps">
		<xi:include href="rosapps/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="rosky">
		<xi:include href="rosky/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="rostests">
		<xi:include href="rostests/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="vms">
		<xi:include href="vms/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
	<directory name="windows">
		<xi:include href="windows/directory.rbuild">
			<xi:fallback>
				<xi:include href="empty.rbuild" />
			</xi:fallback>
		</xi:include>
	</directory>
</group>
