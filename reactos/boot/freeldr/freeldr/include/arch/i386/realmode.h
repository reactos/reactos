

/* These addresses specify the realmode "BSS section" */
#define BSS_START HEX(7000)
#define BSS_CallbackAddress BSS_START + 0
#define BSS_CallbackReturn BSS_START + 8
#define BSS_BootDrive BSS_START + 16
#define BSS_BootPartition BSS_START + 20

#define PE_LOAD_BASE HEX(9000)
#define IMAGE_DOS_HEADER_e_lfanew 36
#define IMAGE_FILE_HEADER_SIZE 20
#define IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint 16
