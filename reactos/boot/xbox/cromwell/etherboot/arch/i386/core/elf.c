#include "etherboot.h"
#include "elf.h"


#define NAME "Etherboot"

#define LINUXBIOS

#if defined(PCBIOS)
#define FIRMWARE "PCBIOS"
#endif
#if defined(LINUXBIOS)
#define FIRMWARE "LinuxBIOS"
#endif
#if !defined(FIRMWARE)
#error "No BIOS selected"
#endif

#define SZ(X) ((sizeof(X)+3) & ~3)
#define CP(D,S) (memcpy(&(D), &(S), sizeof(S)))

struct elf_notes {
	/* The note header */
	struct Elf_Bhdr hdr;

	/* First the Fixed sized entries that must be well aligned */

	/* Pointer to bootp data */
	Elf_Nhdr nf1;
	char     nf1_name[SZ(EB_PARAM_NOTE)];
	uint32_t nf1_bootp_data;

	/* Pointer to ELF header */
	Elf_Nhdr nf2;
	char     nf2_name[SZ(EB_PARAM_NOTE)];
	uint32_t nf2_header;

	/* A copy of the i386 memory map */
	Elf_Nhdr nf3;
	char     nf3_name[SZ(EB_PARAM_NOTE)];
	struct meminfo nf3_meminfo;

	/* Then the variable sized data string data where alignment does not matter */

	/* The bootloader name */
	Elf_Nhdr nv1;
	char     nv1_desc[SZ(NAME)];
	/* The bootloader version */
	Elf_Nhdr nv2;
	char     nv2_desc[SZ(VERSION)];
	/* The firmware type */
	Elf_Nhdr nv3;
	char     nv3_desc[SZ(FIRMWARE)];
	/* Name of the loaded image */
	Elf_Nhdr nv4;
	char     nv4_loaded_image[128];
	/* An empty command line */
	Elf_Nhdr nv5;
	char     nv5_cmdline[SZ("")];
};

#define ELF_NOTE_COUNT  (3 + 5)

static struct elf_notes notes;
struct Elf_Bhdr *prepare_boot_params(void *header)
{
	memset(&notes, 0, sizeof(notes));
	notes.hdr.b_signature = ELF_BHDR_MAGIC;
	notes.hdr.b_size      = sizeof(notes);
	notes.hdr.b_checksum  = 0;
	notes.hdr.b_records   = ELF_NOTE_COUNT;

	/* Initialize the fixed length entries. */
	notes.nf1.n_namesz = sizeof(EB_PARAM_NOTE);
	notes.nf1.n_descsz = sizeof(notes.nf1_bootp_data);
	notes.nf1.n_type   = EB_BOOTP_DATA;
	CP(notes.nf1_name,   EB_PARAM_NOTE);
	notes.nf1_bootp_data = virt_to_phys(BOOTP_DATA_ADDR);

	notes.nf2.n_namesz = sizeof(EB_PARAM_NOTE);
	notes.nf2.n_descsz = sizeof(notes.nf2_header);
	notes.nf2.n_type   = EB_HEADER;
	CP(notes.nf2_name,   EB_PARAM_NOTE);
	notes.nf2_header   = virt_to_phys(header);

	notes.nf3.n_namesz = sizeof(EB_PARAM_NOTE);
	notes.nf3.n_descsz = sizeof(notes.nf3_meminfo);
	notes.nf3.n_type   = EB_I386_MEMMAP;
	CP(notes.nf3_name,   EB_PARAM_NOTE);
	memcpy(&notes.nf3_meminfo, &meminfo, sizeof(meminfo));

	/* Initialize the variable length entries */
	notes.nv1.n_namesz = 0;
	notes.nv1.n_descsz = sizeof(NAME);
	notes.nv1.n_type   = EBN_BOOTLOADER_NAME;
	CP(notes.nv1_desc,   NAME);

	notes.nv2.n_namesz = 0;
	notes.nv2.n_descsz = sizeof(VERSION);
	notes.nv2.n_type   = EBN_BOOTLOADER_VERSION;
	CP(notes.nv2_desc,   VERSION);

	notes.nv3.n_namesz = 0;
	notes.nv3.n_descsz = sizeof(FIRMWARE);
	notes.nv3.n_type   = EBN_FIRMWARE_TYPE;
	CP(notes.nv3_desc,   FIRMWARE);

	/* Attempt to pass the name of the loaded image */
	notes.nv4.n_namesz = 0;
	notes.nv4.n_descsz = sizeof(notes.nv4_loaded_image);
	notes.nv4.n_type   = EBN_LOADED_IMAGE;
	memcpy(&notes.nv4_loaded_image, KERNEL_BUF, sizeof(notes.nv4_loaded_image));

	/* Pass an empty command line for now */
	notes.nv5.n_namesz = 0;
	notes.nv5.n_descsz = sizeof("");
	notes.nv5.n_type   = EBN_COMMAND_LINE;
	CP(notes.nv5_cmdline,   "");


	notes.hdr.b_checksum = ipchksum(&notes, sizeof(notes));
	/* Like UDP invert a 0 checksum to show that a checksum is present */
	if (notes.hdr.b_checksum == 0) {
		notes.hdr.b_checksum = 0xffff;
	}
	return &notes.hdr;
}

int elf_start(unsigned long machine __unused_i386, unsigned long entry, unsigned long params)
{
#if defined(CONFIG_X86_64)
	if (machine == EM_X86_64) {
		return xstart_lm(entry, params);
	}
#endif
	return xstart32(entry, params);
}
