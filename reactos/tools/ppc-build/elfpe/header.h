#ifndef COMPDVR_ELFHEADER_H
#define COMPDVR_ELFHEADER_H

#include <vector>
#include <utility>
#include "pedef.h"
#include "util.h"
#include "objectfile.h"

typedef struct section_mapping_t {
    const ElfObjectFile::Section *section;
    uint32_t rva;
    int index;
    
    section_mapping_t
    (const ElfObjectFile::Section *sect, uint32_t rva, int index) :
	section(sect), rva(rva), index(index) { }
    section_mapping_t(const section_mapping_t &other) :
	section(other.section), rva(other.rva), index(other.index) { }
} section_mapping_t;

class ElfPeHeader {
public:
    ElfPeHeader
	(uint32_t imagebase, 
	 uint32_t imagealign,
	 uint32_t filealign,
	 const ElfObjectFile::Symbol *entry,
	 uint32_t stackreserve,
	 uint32_t stackcommit,
	 uint32_t heapreserve,
	 uint32_t heapcommit,
	 int      subsysid,
	 bool     dll,
	 ElfObjectFile *eof);
    const ElfObjectFile::secdata_t &getData() const;

private:
    void createHeaderSection();
    uint32_t getSectionRvas(std::vector<section_mapping_t> &rvas) const;
    uint32_t getEntryPoint(const std::vector<section_mapping_t> &rvas, const ElfObjectFile::Symbol *entry) const;
    int computeSize() const;
    int getExeFlags() const { return 0; }
    int getDllFlags() const { return dll ? IMAGE_FILE_DLL : 0; }
    u32pair_t getExportInfo(const std::vector<section_mapping_t> &rvas) const;
    u32pair_t getImportInfo(const std::vector<section_mapping_t> &rvas) const;
    u32pair_t getResourceInfo(const std::vector<section_mapping_t> &rvas) const;
    u32pair_t getExceptionInfo() const;
    u32pair_t getSecurityInfo() const;
    u32pair_t getRelocInfo() const;
    u32pair_t getDebugInfo() const;
    u32pair_t getDescrInfo() const;
    u32pair_t getMachInfo() const;
    u32pair_t getTlsInfo() const;
    uint16_t getPeArch() const;
    uint32_t saToRva(int section, uint32_t offset) const;
    uint32_t vaToRva(uint32_t source_addr) const;

    uint32_t 
	imagebase, filealign, sectionalign,
	stackreserve, stackcommit, 
	heapreserve, heapcommit;
    bool dll;
    int subsysid;
    const ElfObjectFile::Symbol *entry;
    ElfObjectFile *eof;
    std::vector<uint8_t> data;
    static const char *mzHeader;
};

#endif//COMPDVR_ELFHEADER_H
