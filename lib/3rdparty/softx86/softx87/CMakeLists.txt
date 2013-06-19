
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/softx86)

list(APPEND SOURCE
    optable.c
    softx87.c
### Uncomment the following files for dll compilation:
#   windll.c
#   softx87_dll.def
#   softx87_dll.rc
)

add_library(softx87 ${SOURCE})
