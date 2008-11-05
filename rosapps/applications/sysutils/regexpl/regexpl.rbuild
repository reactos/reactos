<module name="regexpl" type="win32cui" installbase="system32" installname="regexpl.exe" stdlib="host">
	<include base="regexpl">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>

	<file>ArgumentParser.cpp</file>
	<file>Console.cpp</file>
	<file>RegistryExplorer.cpp</file>
	<file>RegistryKey.cpp</file>
	<file>RegistryTree.cpp</file>
	<file>SecurityDescriptor.cpp</file>
	<file>ShellCommand.cpp</file>
	<file>ShellCommandChangeKey.cpp</file>
	<file>ShellCommandConnect.cpp</file>
	<file>ShellCommandDACL.cpp</file>
	<file>ShellCommandDeleteKey.cpp</file>
	<file>ShellCommandDeleteValue.cpp</file>
	<file>ShellCommandDir.cpp</file>
	<file>ShellCommandExit.cpp</file>
	<file>ShellCommandHelp.cpp</file>
	<file>ShellCommandNewKey.cpp</file>
	<file>ShellCommandOwner.cpp</file>
	<file>ShellCommandSACL.cpp</file>
	<file>ShellCommandSetValue.cpp</file>
	<file>ShellCommandValue.cpp</file>
	<file>ShellCommandVersion.cpp</file>
	<file>ShellCommandsLinkedList.cpp</file>
	<file>TextHistory.cpp</file>
	<file>Completion.cpp</file>
	<file>Pattern.cpp</file>
	<file>Settings.cpp</file>
	<file>Prompt.cpp</file>
</module>