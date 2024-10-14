add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.txt DESTINATION reactos/Web/Wallpaper FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt DESTINATION reactos/Web/Wallpaper FOR all)

add_subdirectory(Angelus)
add_subdirectory(Mizu)
