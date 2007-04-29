#ifndef COMPDVR_ELFOBJECT_H
#define COMPDVR_ELFOBJECT_H

#include <string>
#include <vector>
#include <libelf/libelf.h>

class ElfObjectFile {
public:
    typedef std::vector<uint8_t> secdata_t;

    class Section {
    public:
	Section(const Section &other) : 
	    obj(other.obj), section(other.section) { }
	Section(ElfObjectFile &obj, Elf_Scn *sechdr) : 
	    obj(&obj), section(sechdr) { }
	Section &operator = (const Section &other) {
	    obj = other.obj;
	    section = other.section;
	}
	operator bool () { return !!section; }
	std::string getName() const { 
	    Elf32_Shdr *e32shdr = elf32_getshdr(section);
	    return obj->getString(e32shdr->sh_name);
	}

	int logicalSize() const {
	    Elf32_Shdr *e32shdr = elf32_getshdr(section);
	    return e32shdr->sh_size;
	}

	uint32_t getStartRva() const {
	    Elf32_Shdr *e32shdr = elf32_getshdr(section);
	    return e32shdr->sh_addr;
	}

    private:
	const ElfObjectFile *obj;
	Elf_Scn *section;
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

private:
    int fd;
    int shnum, phnum;
    int shstrndx;
    Elf *elfHeader;
    std::vector<Section*> sections;
};

#endif//COMPDVR_ELFOBJECT_H
