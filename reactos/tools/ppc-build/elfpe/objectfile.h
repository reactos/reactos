#ifndef COMPDVR_ELFOBJECT_H
#define COMPDVR_ELFOBJECT_H

#include <string>
#include <vector>
#include <map>
#include <libelf/libelf.h>

class ElfObjectFile {
public:
    typedef std::vector<uint8_t> secdata_t;

    class Symbol {
    public:
	Symbol(const std::string &name, uint32_t offset, 
	       int section, int flags) : 
	    name(name), offset(offset), section(section), flags(flags) { }
	std::string name;
	uint32_t offset;
	int section;
	int flags;
    };

    class Section {
    public:
	Section(const Section &other) : 
	    obj(other.obj), section(other.section), have_data(false), 
	    number(other.number) { 
	    e32shdr = elf32_getshdr(section);
	}
	Section(ElfObjectFile &obj, int number, Elf_Scn *sechdr) : 
	    obj(&obj), section(sechdr), have_data(false), number(number) {
	    e32shdr = elf32_getshdr(section);
	}
	Section &operator = (const Section &other) {
	    obj = other.obj;
	    have_data = false;
	    section = other.section;
	    e32shdr = other.e32shdr;
	    number = other.number;
	}
	operator bool () { return !!section; }
	std::string getName() const { 
	    return obj->getString(e32shdr->sh_name);
	}

	int getType() const {
	    return e32shdr->sh_type;
	}

	int getNumber() const {
	    return number;
	}

	int getLink() const {
	    return e32shdr->sh_link;
	}

	int getInfo() const {
	    return e32shdr->sh_info;
	}

	int getFlags() const {
	    return e32shdr->sh_flags;
	}

	int logicalSize() const {
	    return e32shdr->sh_size;
	}

	uint32_t getStartRva() const {
	    return e32shdr->sh_addr;
	}

	uint32_t getFileOffset() const {
	    return e32shdr->sh_offset;
	}

	uint8_t *getSectionData() const {
	    if(!have_data) {
		data = *elf_getdata(section, NULL);
		have_data = true;
	    }
	    return (uint8_t *)data.d_buf;
	}

    private:
	const ElfObjectFile *obj;
	int number;
	Elf_Scn *section;
	Elf32_Shdr *e32shdr;
	mutable bool have_data;
	mutable Elf_Data data;
    };

    ElfObjectFile(const std::string &filename);
    ~ElfObjectFile();

    Elf *operator -> () { return fd >= 0 ? elfHeader : 0; }
    bool operator ! () const { return fd == -1 ? true : false; }
    int getNumSections() const { return sections.size(); }
    uint32_t getEntryPoint() const;
    std::string getString(int offset) const { 
	return elf_strptr(elfHeader, shstrndx, offset);
    }
    void addSection
	(const std::string &name, const secdata_t &data, int type = SHT_PROGBITS);
    const Section &getSection(int sect) const { return *sections[sect]; }
    const Section *getNamedSection(const std::string &name) const;
    const Symbol &getSymbol(int n) const { return *symbols[n]; }
    const Symbol *getNamedSymbol(const std::string &symname) const;

private:
    int fd;
    int shnum, phnum;
    int shstrndx;
    Elf *elfHeader;
    std::vector<Section*> sections;
    std::map<std::string, const Section *> sections_by_name;
    std::vector<Symbol*> symbols;
    std::map<std::string, const Symbol *> symbols_by_name;

    void populateSymbolTable();
};

#endif//COMPDVR_ELFOBJECT_H
