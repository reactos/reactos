
PROJECT(SHELL)

list(APPEND SOURCE
    filebrowser.c
    filebrowser.rc)

add_executable(filebrowser ${SOURCE})
set_module_type(filebrowser win32gui UNICODE)
add_importlibs(filebrowser
    shell32
    msvcrt
    kernel32)

add_custom_command(TARGET filebrowser POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:rshell>"
     "$<TARGET_FILE_DIR:filebrowser>/$<TARGET_FILE_NAME:rshell>" 
  COMMENT "Copying $<TARGET_FILE_NAME:rshell> to output")

add_custom_command(TARGET filebrowser POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:browseui>"
     "$<TARGET_FILE_DIR:filebrowser>/$<TARGET_FILE_NAME:browseui>" 
  COMMENT "Copying $<TARGET_FILE_NAME:browseui> to output")
