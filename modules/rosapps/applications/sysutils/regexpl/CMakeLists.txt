
list(APPEND SOURCE
    ArgumentParser.cpp
    Console.cpp
    RegistryExplorer.cpp
    RegistryKey.cpp
    RegistryTree.cpp
    SecurityDescriptor.cpp
    ShellCommand.cpp
    ShellCommandChangeKey.cpp
    ShellCommandConnect.cpp
    ShellCommandDACL.cpp
    ShellCommandDeleteKey.cpp
    ShellCommandDeleteValue.cpp
    ShellCommandDir.cpp
    ShellCommandExit.cpp
    ShellCommandHelp.cpp
    ShellCommandNewKey.cpp
    ShellCommandOwner.cpp
    ShellCommandSACL.cpp
    ShellCommandSetValue.cpp
    ShellCommandValue.cpp
    ShellCommandVersion.cpp
    ShellCommandsLinkedList.cpp
    TextHistory.cpp
    Completion.cpp
    Pattern.cpp
    Settings.cpp
    Prompt.cpp)

add_executable(regexpl ${SOURCE} regexpl.rc)
target_link_libraries(regexpl cppstl)
set_target_cpp_properties(regexpl WITH_EXCEPTIONS)
set_module_type(regexpl win32cui)
add_importlibs(regexpl user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET regexpl DESTINATION reactos/system32 FOR all)
