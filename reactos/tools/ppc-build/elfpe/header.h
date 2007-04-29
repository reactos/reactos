#ifndef COMPDVR_ELFHEADER_H
#define COMPDVR_ELFHEADER_H

#include <vector>
#include <utility>
#include "pedef.h"
#include "util.h"
#include "objectfile.h"

class ElfPeHeader {
public:
    ElfPeHeader
	(uint32_t imagebase, 
	 uint32_t imagealign,
	 uint32_t filealign,
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
    int getNumSections() const { return eof->getNumSections(); }
    int computeSize() const;
    int getExeFlags() const { return 0; }
    int getDllFlags() const { return dll ? IMAGE_FILE_DLL : 0; }
    u32pair_t getExportInfo() const;
    u32pair_t getImportInfo() const;
    u32pair_t getResourceInfo() const;
    u32pair_t getExceptionInfo() const;
    u32pair_t getSecurityInfo() const;
    u32pair_t getRelocInfo() const;
    u32pair_t getDebugInfo() const;
    u32pair_t getDescrInfo() const;
    u32pair_t getMachInfo() const;
    u32pair_t getTlsInfo() const;
    uint32_t getEntryPoint() const { return eof->getEntryPoint(); }
    uint32_t getImageSize() const;
    uint16_t getPeArch() const;
    uint32_t saToRva(int section, uint32_t offset) const;
    uint32_t vaToRva(uint32_t source_addr) const;

    uint32_t 
	imagebase, filealign, sectionalign,
	stackreserve, stackcommit, 
	heapreserve, heapcommit;
    bool dll;
    int subsysid;
    ElfObjectFile *eof;
    std::vector<uint8_t> data;
    static const char *mzHeader;
};

#endif//COMPDVR_ELFHEADER_H
