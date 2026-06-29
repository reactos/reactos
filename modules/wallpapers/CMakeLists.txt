
file(GLOB WALLPAPER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.bmp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.txt"
)

list(REMOVE_ITEM WALLPAPER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")

foreach(f ${WALLPAPER_FILES})
    add_cd_file(FILE "${f}" DESTINATION reactos/Web/Wallpaper FOR all)
endforeach()
