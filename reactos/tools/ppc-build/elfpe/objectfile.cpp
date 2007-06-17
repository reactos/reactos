#include <libelf/libelf.h>
#include <fcntl.h>
#include "util.h"
#include "objectfile.h"

ElfObjectFile::ElfObjectFile(const std::string &filename) : fd(-1)
{
    Elf_Scn *s = 0;
    Elf32_Ehdr *ehdr;
    Section *sect;
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
	/* Populate section table */
	for(size_t i = 0; i < shnum; i++)
	{
	    s = elf_nextscn(elfHeader, s);
	    if(!s) break;
	    sect = new Section(*this, i, s);
	    sections.push_back(sect);
	    sections_by_name.insert(std::make_pair(sect->getName(), sect));
	}

	populateSymbolTable();
    }
}

ElfObjectFile::~ElfObjectFile()
{
    if(elfHeader) elf_end(elfHeader);
    if(fd >= 0) close(fd);
}

void ElfObjectFile::populateSymbolTable()
{
    int i = 0, j;
    int type, link, flags, section;
    uint32_t offset;
    std::string name;
    uint8_t *data, *symptr;
    Elf32_Sym *sym;
    Symbol *ourSym;

    for( i = 0; i < getNumSections(); i++ ) {
	type = getSection(i).getType();
	link = getSection(i).getLink();
	if( (type == SHT_SYMTAB) || (type == SHT_DYNSYM) ) {
	    /* Read a symbol */
	    sym = (Elf32_Sym*)getSection(i).getSectionData();
	    for (j = 0; j < getSection(i).logicalSize() / sizeof(Elf32_Sym); j++) {
		name = elf_strptr(elfHeader, link, sym[j].st_name);
		ourSym = new Symbol(name, sym[j].st_value, sym[j].st_shndx, sym[j].st_info);
		symbols.push_back(ourSym);
		symbols_by_name.insert(std::make_pair(name, ourSym));
	    }
	}
    }
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
    shdr->sh_flags = SHF_ALLOC;
    shdr->sh_addr = 0;
    shdr->sh_link = 0;
    shdr->sh_info = 0;

    elf_update(elfHeader, ELF_C_WRITE);
}

const ElfObjectFile::Section *ElfObjectFile::getNamedSection(const std::string &name) const
{
    std::map<std::string, const ElfObjectFile::Section *>::const_iterator i =
	sections_by_name.find(name);
    if(i != sections_by_name.end())
	return i->second;
    else return NULL;
}

const ElfObjectFile::Symbol *ElfObjectFile::getNamedSymbol(const std::string &name) const
{
    std::map<std::string, const ElfObjectFile::Symbol *>::const_iterator i =
	symbols_by_name.find(name);
    if(i != symbols_by_name.end())
	return i->second;
    else return NULL;
}
