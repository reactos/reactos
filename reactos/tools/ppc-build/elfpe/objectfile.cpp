#include <libelf/libelf.h>
#include <fcntl.h>
#include "objectfile.h"

ElfObjectFile::ElfObjectFile(const std::string &filename) : fd(-1)
{
    Elf_Scn *s = 0;
    Elf32_Ehdr *ehdr;
    fd = open(filename.c_str(), O_RDWR, 0);
    if(fd >= 0) {
	if(elf_version(EV_CURRENT) == EV_NONE) {
	    // Old version
	    return;
	}
	elfHeader = elf_begin(fd, ELF_C_RDWR, (Elf*)0);
	if(elf_kind(elfHeader) != ELF_K_ELF) {
	    // Didn't get an elf object file
	    return;
	}
	ehdr = elf32_getehdr(elfHeader);
	shnum = ehdr->e_shnum;
	phnum = ehdr->e_phnum;
	shstrndx = ehdr->e_shstrndx;
	for(size_t i = 0; i < shnum; i++)
	{
	    s = elf_nextscn(elfHeader, s);
	    if(!s) break;
	    sections.push_back(new Section(*this, s));
	    fprintf(stderr, "Got section %04d %s\n", i, sections[i]->getName().c_str());
	}
    }
}

ElfObjectFile::~ElfObjectFile()
{
    if(elfHeader) elf_end(elfHeader);
    if(fd >= 0) close(fd);
}

uint32_t ElfObjectFile::getEntryPoint() const
{
    Elf32_Ehdr *elf32ehdr = elf32_getehdr(elfHeader);
    return elf32ehdr->e_entry;
}

void ElfObjectFile::addSection(const std::string &name, const secdata_t &data, int type)
{
    Elf_Scn *newsect = elf_newscn(elfHeader), 
	*strsect = elf_getscn(elfHeader, shstrndx);
    Elf32_Shdr *shdr = elf32_getshdr(newsect);
    /* Create data for the new section */
    Elf_Data *edata = elf_newdata(newsect), *strdata = elf_getdata(strsect, 0),
	*newstrdata = elf_newdata(strsect);
    edata->d_align = 0x1000;
    edata->d_size = data.size();
    edata->d_off = 0;
    edata->d_type = ELF_T_BYTE;
    edata->d_version = EV_CURRENT;
    edata->d_buf = malloc(edata->d_size);
    memcpy(edata->d_buf, &data[0], edata->d_size);
    /* Add the name of the new section to the string table */
    newstrdata->d_off = strdata->d_off + strdata->d_size;
    newstrdata->d_size = name.size() + 1;
    newstrdata->d_align = 1;
    newstrdata->d_buf = (void *)name.c_str();
    /* Finish the section */
    shdr->sh_name = newstrdata->d_off;
    shdr->sh_type = type;
    shdr->sh_flags = 0;
    shdr->sh_addr = 0;
    shdr->sh_link = 0;
    shdr->sh_info = 0;

    elf_update(elfHeader, ELF_C_WRITE);
}

const ElfObjectFile::Section *ElfObjectFile::getNamedSection(const std::string &name) const
{
    for(size_t i = 0; i < sections.size(); i++) {
	if(sections[i]->getName() == name) return sections[i];
    }
    return NULL;
}
