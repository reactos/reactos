#
# Build the combined VFW library from the component libraries
#
#

OUTPUT = $(_NTROOT)\public\sdk\lib\$(TARGET_DIRECTORY)

all: $(OUTPUT)\vfw32.lib

$(OUTPUT)\vfw32.lib: ..\lib\$(TARGET_DIRECTORY)\msvfw32.lib \
                     ..\lib\$(TARGET_DIRECTORY)\avifil32.lib \
                     ..\lib\$(TARGET_DIRECTORY)\avicap32.lib
   lib ..\lib\$(TARGET_DIRECTORY)\msvfw32.lib \
       ..\lib\$(TARGET_DIRECTORY)\avifil32.lib \
       ..\lib\$(TARGET_DIRECTORY)\avicap32.lib \
       /OUT:$(OUTPUT)\vfw32.lib
