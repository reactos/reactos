#include "elf.h"

#ifndef ELF_CHECK_ARCH
#error ELF_CHECK_ARCH not defined
#endif

#define ELF_NOTES 1
#define ELF_DEBUG 0

struct elf_state
{
	union {
		Elf32_Ehdr elf32;
		Elf64_Ehdr elf64;
	} e;
	union {
		Elf32_Phdr phdr32[1];
		Elf64_Phdr phdr64[1];
		unsigned char dummy[1024];
	} p;
	unsigned long curaddr;
	int segment;		/* current segment number, -1 for none */
	uint64_t loc;		/* start offset of current block */
	uint64_t skip;		/* padding to be skipped to current segment */
	unsigned long toread;	/* remaining data to be read in the segment */
#if ELF_NOTES
	int check_ip_checksum;
	uint16_t ip_checksum;
	unsigned long ip_checksum_offset;
#endif
};

static struct elf_state estate;

static void elf_boot(unsigned long machine, unsigned long entry)
{
	int result;
	struct Elf_Bhdr *hdr;
	multiboot_boot(entry);
	/* We cleanup unconditionally, and then reawaken the network
	 * adapter after the longjmp.
	 */
	hdr = prepare_boot_params(&estate.e);
	result = elf_start(machine, entry, virt_to_phys(hdr));
	if (result == 0) {
		result = -1;
	}
	printf("Secondary program returned %d\n", result);
	longjmp(restart_etherboot, result);
}

#if ELF_NOTES
static int elf_prep_segment(
	unsigned long start __unused, unsigned long mid __unused, unsigned long end __unused,
	unsigned long istart, unsigned long iend)

{
	if (estate.check_ip_checksum) {
		if ((istart <= estate.ip_checksum_offset) && 
			(iend > estate.ip_checksum_offset)) {
			/* The checksum note is also loaded in a
			 * PT_LOAD segment, so the computed checksum
			 * should be 0.
			 */
			estate.ip_checksum = 0;
		}
	}
	return 1;
}
#else
#define elf_prep_segment(start, mid, end, istart, iend) (1)
#endif


#if ELF_NOTES
static void process_elf_notes(unsigned char *header,
	unsigned long offset, unsigned long length)
{
	unsigned char *note, *end;
	char *program, *version;

	estate.check_ip_checksum = 0;
	note = header + offset;
	end = note + length;
	program = version = 0;
	while(note < end) {
		Elf_Nhdr *hdr;
		unsigned char *n_name, *n_desc, *next;
		hdr = (Elf_Nhdr *)note;
		n_name = note + sizeof(*hdr);
		n_desc = n_name + ((hdr->n_namesz + 3) & ~3);
		next = n_desc + ((hdr->n_descsz + 3) & ~3);
		if (next > end) {
			break;
		}
		if ((hdr->n_namesz == sizeof(ELF_NOTE_BOOT)) && 
			(memcmp(n_name, ELF_NOTE_BOOT, sizeof(ELF_NOTE_BOOT)) == 0)) {
			switch(hdr->n_type) {
			case EIN_PROGRAM_NAME:
				if (n_desc[hdr->n_descsz -1] == 0) {
					program = n_desc;
				}
				break;
			case EIN_PROGRAM_VERSION:
				if (n_desc[hdr->n_descsz -1] == 0) {
					version = n_desc;
				}
				break;
			case EIN_PROGRAM_CHECKSUM:
				estate.check_ip_checksum = 1;
				estate.ip_checksum = *((uint16_t *)n_desc);
				/* Remember where the segment is so
				 * I can detect segment overlaps.
				 */
				estate.ip_checksum_offset = n_desc - header;
#if ELF_DEBUG
				printf("Checksum: %hx\n", estate.ip_checksum);
#endif

				break;
			}
		}
#if ELF_DEBUG
		printf("n_type: %x n_name(%d): %s n_desc(%d): %s\n", 
			hdr->n_type,
			hdr->n_namesz, n_name,
			hdr->n_descsz, n_desc);
#endif
		note = next;
	}
	if (program && version) {
		printf("\nLoading %s version: %s\n", program, version);
	}
}
#endif

#ifdef	ELF_IMAGE
static sector_t elf32_download(unsigned char *data, unsigned int len, int eof);
static inline os_download_t elf32_probe(unsigned char *data, unsigned int len)
{
	unsigned long phdr_size;
	if (len < sizeof(estate.e.elf32)) {
		return 0;
	}
	memcpy(&estate.e.elf32, data, sizeof(estate.e.elf32));
	if ((estate.e.elf32.e_ident[EI_MAG0] != ELFMAG0) ||
		(estate.e.elf32.e_ident[EI_MAG1] != ELFMAG1) ||
		(estate.e.elf32.e_ident[EI_MAG2] != ELFMAG2) ||
		(estate.e.elf32.e_ident[EI_MAG3] != ELFMAG3) ||
		(estate.e.elf32.e_ident[EI_CLASS] != ELFCLASS32) ||
		(estate.e.elf32.e_ident[EI_DATA] != ELFDATA_CURRENT) ||
		(estate.e.elf32.e_ident[EI_VERSION] != EV_CURRENT) ||
		(estate.e.elf32.e_type != ET_EXEC) ||
		(estate.e.elf32.e_version != EV_CURRENT) ||
		(estate.e.elf32.e_ehsize != sizeof(Elf32_Ehdr)) ||
		(estate.e.elf32.e_phentsize != sizeof(Elf32_Phdr)) ||
		!ELF_CHECK_ARCH(estate.e.elf32)) {
		return 0;
	}
	printf("(ELF");
	elf_freebsd_probe();
	multiboot_probe(data, len);
	printf(")... ");
	phdr_size = estate.e.elf32.e_phnum * estate.e.elf32.e_phentsize;
	if (estate.e.elf32.e_phoff + phdr_size > len) {
		printf("ELF header outside first block\n");
		return 0;
	}
	if (phdr_size > sizeof(estate.p.dummy)) {
		printf("Program header to big\n");
		return 0;
	}
	memcpy(&estate.p.phdr32, data + estate.e.elf32.e_phoff, phdr_size);
#if ELF_NOTES
	/* Load ELF notes from the image */
	for(estate.segment = 0; estate.segment < estate.e.elf32.e_phnum; estate.segment++) {
		if (estate.p.phdr32[estate.segment].p_type != PT_NOTE)
			continue;
		if (estate.p.phdr32[estate.segment].p_offset + estate.p.phdr32[estate.segment].p_filesz > len) {
			/* Ignore ELF notes outside of the first block */
			continue;
		}
		process_elf_notes(data, 
			estate.p.phdr32[estate.segment].p_offset, estate.p.phdr32[estate.segment].p_filesz);
	}
#endif
	/* Check for Etherboot related limitations.  Memory
	 * between _text and _end is not allowed.
	 * Reasons: the Etherboot code/data area.
	 */
	for (estate.segment = 0; estate.segment < estate.e.elf32.e_phnum; estate.segment++) {
		unsigned long start, mid, end, istart, iend;
		if (estate.p.phdr32[estate.segment].p_type != PT_LOAD)
			continue;

		elf_freebsd_fixup_segment();

		start = estate.p.phdr32[estate.segment].p_paddr;
		mid = start + estate.p.phdr32[estate.segment].p_filesz;
		end = start + estate.p.phdr32[estate.segment].p_memsz;
		istart = estate.p.phdr32[estate.segment].p_offset;
		iend = istart + estate.p.phdr32[estate.segment].p_filesz;
		if (!prep_segment(start, mid, end, istart, iend)) {
			return 0;
		}
		if (!elf_prep_segment(start, mid, end, istart, iend)) {
			return 0;
		}
	}
	estate.segment = -1;
	estate.loc = 0;
	estate.skip = 0;
	estate.toread = 0;
	return elf32_download;
}

static sector_t elf32_download(unsigned char *data, unsigned int len, int eof)
{
	unsigned long skip_sectors = 0;
	unsigned int offset;	/* working offset in the current data block */
	int i;

	offset = 0;
	do {
		if (estate.segment != -1) {
			if (estate.skip) {
				if (estate.skip >= len - offset) {
					estate.skip -= len - offset;
					break;
				}
				offset += estate.skip;
				estate.skip = 0;
			}
			
			if (estate.toread) {
				unsigned int cplen;
				cplen = len - offset;
				if (cplen >= estate.toread) {
					cplen = estate.toread;
				}
				memcpy(phys_to_virt(estate.curaddr), data+offset, cplen);
				estate.curaddr += cplen;
				estate.toread -= cplen;
				offset += cplen;
				if (estate.toread)
					break;
				elf_freebsd_find_segment_end();
			}
		}
		
		/* Data left, but current segment finished - look for the next
		 * segment (in file offset order) that needs to be loaded. 
		 * We can only seek forward, so select the program headers,
		 * in the correct order.
		 */
		estate.segment = -1;
		for (i = 0; i < estate.e.elf32.e_phnum; i++) {
			if (estate.p.phdr32[i].p_type != PT_LOAD)
				continue;
			if (estate.p.phdr32[i].p_filesz == 0)
				continue;
			if (estate.p.phdr32[i].p_offset < estate.loc + offset)
				continue;	/* can't go backwards */
			if ((estate.segment != -1) &&
				(estate.p.phdr32[i].p_offset >= estate.p.phdr32[estate.segment].p_offset))
				continue;	/* search minimum file offset */
			estate.segment = i;
		}
		if (estate.segment == -1) {
			if (elf_freebsd_debug_loader(offset)) {
				estate.segment = 0; /* -1 makes it not read anymore */
				continue;
			}
			/* No more segments to be loaded, so just start the
			 * kernel.  This saves a lot of network bandwidth if
			 * debug info is in the kernel but not loaded.  */
			goto elf_startkernel;
			break;
		}
		estate.curaddr = estate.p.phdr32[estate.segment].p_paddr;
		estate.skip    = estate.p.phdr32[estate.segment].p_offset - (estate.loc + offset);
		estate.toread  = estate.p.phdr32[estate.segment].p_filesz;
#if ELF_DEBUG
		printf("PHDR %d, size %#lX, curaddr %#lX\n",
			estate.segment, estate.toread, estate.curaddr);
#endif
	} while (offset < len);

	estate.loc += len + (estate.skip & ~0x1ff);
	skip_sectors = estate.skip >> 9;
	estate.skip &= 0x1ff;
	
	if (eof) {
		unsigned long entry;
		unsigned long machine;
elf_startkernel:
		entry = estate.e.elf32.e_entry;
		machine = estate.e.elf32.e_machine;

#if ELF_NOTES
		if (estate.check_ip_checksum) {
			unsigned long bytes = 0;
			uint16_t sum, new_sum;

			sum = ipchksum(&estate.e.elf32, sizeof(estate.e.elf32));
			bytes = sizeof(estate.e.elf32);
#if ELF_DEBUG
			printf("Ehdr: %hx %hx sz: %lx bytes: %lx\n",
				sum, sum, bytes, bytes);
#endif

			new_sum = ipchksum(estate.p.phdr32, sizeof(estate.p.phdr32[0]) * estate.e.elf32.e_phnum);
			sum = add_ipchksums(bytes, sum, new_sum);
			bytes += sizeof(estate.p.phdr32[0]) * estate.e.elf32.e_phnum;
#if ELF_DEBUG
			printf("Phdr: %hx %hx sz: %lx bytes: %lx\n",
				new_sum, sum,
				sizeof(estate.p.phdr32[0]) * estate.e.elf32.e_phnum, bytes);
#endif

			for(i = 0; i < estate.e.elf32.e_phnum; i++) {
				if (estate.p.phdr32[i].p_type != PT_LOAD)
					continue;
				new_sum = ipchksum(phys_to_virt(estate.p.phdr32[i].p_paddr),
						estate.p.phdr32[i].p_memsz);
				sum = add_ipchksums(bytes, sum, new_sum);
				bytes += estate.p.phdr32[i].p_memsz;
#if ELF_DEBUG
			printf("seg%d: %hx %hx sz: %x bytes: %lx\n",
				i, new_sum, sum,
				estate.p.phdr32[i].p_memsz, bytes);
#endif

			}
			if (estate.ip_checksum != sum) {
				printf("\nImage checksum: %hx != computed checksum: %hx\n",
					estate.ip_checksum, sum);
				longjmp(restart_etherboot, -2);
			}
		}
#endif
		done();
		/* Fixup the offset to the program header so you can find the program headers from
		 * the ELF header mknbi needs this.
		 */
		estate.e.elf32.e_phoff = (char *)&estate.p - (char *)&estate.e;
		elf_freebsd_boot(entry);
		elf_boot(machine,entry);
	}
	return skip_sectors;
}
#endif /* ELF_IMAGE */

#ifdef  ELF64_IMAGE
static sector_t elf64_download(unsigned char *data, unsigned int len, int eof);
static inline os_download_t elf64_probe(unsigned char *data, unsigned int len)
{
	unsigned long phdr_size;
	if (len < sizeof(estate.e.elf64)) {
		return 0;
	}
	memcpy(&estate.e.elf64, data, sizeof(estate.e.elf64));
	if ((estate.e.elf64.e_ident[EI_MAG0] != ELFMAG0) ||
		(estate.e.elf64.e_ident[EI_MAG1] != ELFMAG1) ||
		(estate.e.elf64.e_ident[EI_MAG2] != ELFMAG2) ||
		(estate.e.elf64.e_ident[EI_MAG3] != ELFMAG3) ||
		(estate.e.elf64.e_ident[EI_CLASS] != ELFCLASS64) ||
		(estate.e.elf64.e_ident[EI_DATA] != ELFDATA_CURRENT) ||
		(estate.e.elf64.e_ident[EI_VERSION] != EV_CURRENT) ||
		(estate.e.elf64.e_type != ET_EXEC) ||
		(estate.e.elf64.e_version != EV_CURRENT) ||
		(estate.e.elf64.e_ehsize != sizeof(Elf64_Ehdr)) ||
		(estate.e.elf64.e_phentsize != sizeof(Elf64_Phdr)) ||
		!ELF_CHECK_ARCH(estate.e.elf64)) {
		return 0;
	}
	printf("(ELF64)... ");
	phdr_size = estate.e.elf64.e_phnum * estate.e.elf64.e_phentsize;
	if (estate.e.elf64.e_phoff + phdr_size > len) {
		printf("ELF header outside first block\n");
		return 0;
	}
	if (phdr_size > sizeof(estate.p.dummy)) {
		printf("Program header to big\n");
		return 0;
	}
	if (estate.e.elf64.e_entry > ULONG_MAX) {
		printf("ELF entry point exceeds address space\n");
		return 0;
	}
	memcpy(&estate.p.phdr64, data + estate.e.elf64.e_phoff, phdr_size);
#if ELF_NOTES
	/* Load ELF notes from the image */
	for(estate.segment = 0; estate.segment < estate.e.elf64.e_phnum; estate.segment++) {
		if (estate.p.phdr64[estate.segment].p_type != PT_NOTE)
			continue;
		if (estate.p.phdr64[estate.segment].p_offset + estate.p.phdr64[estate.segment].p_filesz > len) {
			/* Ignore ELF notes outside of the first block */
			continue;
		}
		process_elf_notes(data, 
			estate.p.phdr64[estate.segment].p_offset, estate.p.phdr64[estate.segment].p_filesz);
	}
#endif
	/* Check for Etherboot related limitations.  Memory
	 * between _text and _end is not allowed.  
	 * Reasons: the Etherboot code/data area.
	 */
	for (estate.segment = 0; estate.segment < estate.e.elf64.e_phnum; estate.segment++) {
		unsigned long start, mid, end, istart, iend;
		if (estate.p.phdr64[estate.segment].p_type != PT_LOAD) 
			continue;
		if ((estate.p.phdr64[estate.segment].p_paddr > ULONG_MAX) ||
			((estate.p.phdr64[estate.segment].p_paddr + estate.p.phdr64[estate.segment].p_filesz) > ULONG_MAX) ||
			((estate.p.phdr64[estate.segment].p_paddr + estate.p.phdr64[estate.segment].p_memsz) > ULONG_MAX)) {
			printf("ELF segment exceeds address space\n");
			return 0;
		}
		start = estate.p.phdr64[estate.segment].p_paddr;
		mid = start + estate.p.phdr64[estate.segment].p_filesz;
		end = start + estate.p.phdr64[estate.segment].p_memsz;
		istart = iend = ULONG_MAX;
		if ((estate.p.phdr64[estate.segment].p_offset < ULONG_MAX) &&
			((estate.p.phdr64[estate.segment].p_offset + estate.p.phdr64[estate.segment].p_filesz) < ULONG_MAX))
		{
			istart = estate.p.phdr64[estate.segment].p_offset;
			iend   = istart + estate.p.phdr64[estate.segment].p_filesz;
		} 
		if (!prep_segment(start, mid, end, istart, iend)) {
			return 0;
		}
		if (!elf_prep_segment(start, mid, end, istart, iend)) {
			return 0;
		}
	}
	estate.segment = -1;
	estate.loc = 0;
	estate.skip = 0;
	estate.toread = 0;
	return elf64_download;
}

static sector_t elf64_download(unsigned char *data, unsigned int len, int eof)
{
	unsigned long skip_sectors = 0;
	unsigned int offset;	/* working offset in the current data block */
	int i;

	offset = 0;
	do {
		if (estate.segment != -1) {
			if (estate.skip) {
				if (estate.skip >= len - offset) {
					estate.skip -= len - offset;
					break;
				}
				offset += estate.skip;
				estate.skip = 0;
			}
			
			if (estate.toread) {
				unsigned int cplen;
				cplen = len - offset;
				if (cplen >= estate.toread) {
					cplen = estate.toread;
				}
				memcpy(phys_to_virt(estate.curaddr), data+offset, cplen);
				estate.curaddr += cplen;
				estate.toread -= cplen;
				offset += cplen;
				if (estate.toread)
					break;
			}
		}
		
		/* Data left, but current segment finished - look for the next
		 * segment (in file offset order) that needs to be loaded. 
		 * We can only seek forward, so select the program headers,
		 * in the correct order.
		 */
		estate.segment = -1;
		for (i = 0; i < estate.e.elf64.e_phnum; i++) {
			if (estate.p.phdr64[i].p_type != PT_LOAD)
				continue;
			if (estate.p.phdr64[i].p_filesz == 0)
				continue;
			if (estate.p.phdr64[i].p_offset < estate.loc + offset)
				continue;	/* can't go backwards */
			if ((estate.segment != -1) &&
				(estate.p.phdr64[i].p_offset >= estate.p.phdr64[estate.segment].p_offset))
				continue;	/* search minimum file offset */
			estate.segment = i;
		}
		if (estate.segment == -1) {
			/* No more segments to be loaded, so just start the
			 * kernel.  This saves a lot of network bandwidth if
			 * debug info is in the kernel but not loaded.  */
			goto elf_startkernel;
			break;
		}
		estate.curaddr = estate.p.phdr64[estate.segment].p_paddr;
		estate.skip    = estate.p.phdr64[estate.segment].p_offset - (estate.loc + offset);
		estate.toread  = estate.p.phdr64[estate.segment].p_filesz;
#if ELF_DEBUG
		printf("PHDR %d, size %#lX, curaddr %#lX\n",
			estate.segment, estate.toread, estate.curaddr);
#endif
	} while (offset < len);
	
	estate.loc += len + (estate.skip & ~0x1ff);
	skip_sectors = estate.skip >> 9;
	estate.skip &= 0x1ff;
	
	if (eof) {
		unsigned long entry;
		unsigned long machine;
elf_startkernel:
		entry = estate.e.elf64.e_entry;
		machine = estate.e.elf64.e_machine;
#if ELF_NOTES
		if (estate.check_ip_checksum) {
			unsigned long bytes = 0;
			uint16_t sum, new_sum;

			sum = ipchksum(&estate.e.elf64, sizeof(estate.e.elf64));
			bytes = sizeof(estate.e.elf64);
#if ELF_DEBUG
			printf("Ehdr: %hx %hx sz: %lx bytes: %lx\n",
				sum, sum, bytes, bytes);
#endif

			new_sum = ipchksum(estate.p.phdr64, sizeof(estate.p.phdr64[0]) * estate.e.elf64.e_phnum);
			sum = add_ipchksums(bytes, sum, new_sum);
			bytes += sizeof(estate.p.phdr64[0]) * estate.e.elf64.e_phnum;
#if ELF_DEBUG
			printf("Phdr: %hx %hx sz: %lx bytes: %lx\n",
				new_sum, sum,
				sizeof(estate.p.phdr64[0]) * estate.e.elf64.e_phnum, bytes);
#endif

			for(i = 0; i < estate.e.elf64.e_phnum; i++) {
				if (estate.p.phdr64[i].p_type != PT_LOAD)
					continue;
				new_sum = ipchksum(phys_to_virt(estate.p.phdr64[i].p_paddr),
						estate.p.phdr64[i].p_memsz);
				sum = add_ipchksums(bytes, sum, new_sum);
				bytes += estate.p.phdr64[i].p_memsz;
#if ELF_DEBUG
			printf("seg%d: %hx %hx sz: %x bytes: %lx\n",
				i, new_sum, sum,
				estate.p.phdr64[i].p_memsz, bytes);
#endif

			}
			if (estate.ip_checksum != sum) {
				printf("\nImage checksum: %hx != computed checksum: %hx\n",
					estate.ip_checksum, sum);
				longjmp(restart_etherboot, -2);
			}
		}
#endif
		done();
		/* Fixup the offset to the program header so you can find the program headers from
		 * the ELF header mknbi needs this.
		 */
		estate.e.elf64.e_phoff = (char *)&estate.p - (char *)&estate.e;
		elf_boot(machine,entry);
	}
	return skip_sectors;
}

#endif /* ELF64_IMAGE */
