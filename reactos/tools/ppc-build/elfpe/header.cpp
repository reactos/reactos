#include <time.h>
#include "util.h"
#include "header.h"

ElfPeHeader::ElfPeHeader
	(uint32_t imagebase, 
	 uint32_t filealign,
	 uint32_t sectionalign,
	 uint32_t stackreserve,
	 uint32_t stackcommit,
	 uint32_t heapreserve,
	 uint32_t heapcommit,
	 int      subsysid,
	 bool     dll,
	 ElfObjectFile *eof) :
	imagebase(imagebase),
	sectionalign(sectionalign),
	stackreserve(stackreserve),
	stackcommit(stackcommit),
	heapreserve(heapreserve),
	heapcommit(heapcommit),
	subsysid(subsysid),
	eof(eof)
{
    data.resize(computeSize());
    createHeaderSection();
}

int ElfPeHeader::computeSize() const
{
    return sectionalign; /* We'll compute it for real later */
}

void ElfPeHeader::createHeaderSection() 
{
    data[0] = 'M'; data[1] = 'Z';
    uint8_t *dataptr = &data[0x3c];
    uint32_t coffHeaderSize, optHeaderSizeMember;
    le32write_postinc(dataptr, 0x80);
    dataptr = &data[0x80];
    le32write_postinc(dataptr, 0x4550);
    le16write_postinc(dataptr, getPeArch());
    le16write_postinc(dataptr, getNumSections());
    le32write_postinc(dataptr, time(NULL));
    le32write_postinc(dataptr, 0);
    le32write_postinc(dataptr, 0);
    optHeaderSizeMember = dataptr - &data[0];
    le16write_postinc(dataptr, 0); // Will fixup sizeof opt header
    le16write_postinc(dataptr, getExeFlags());
    coffHeaderSize = dataptr - &data[0];
    le16write_postinc(dataptr, 0);
    le16write_postinc(dataptr, 0x100);
    le32write_postinc(dataptr, 0);
    le32write_postinc(dataptr, 0);
    le32write_postinc(dataptr, 0);
    le32write_postinc(dataptr, getEntryPoint());
    le32write_postinc(dataptr, 0);
    le32write_postinc(dataptr, 0);
    le32write_postinc(dataptr, imagebase);
    le32write_postinc(dataptr, sectionalign);
    le32write_postinc(dataptr, filealign);
    le16write_postinc(dataptr, 4);
    le16write_postinc(dataptr, 0);
    le16write_postinc(dataptr, 1);
    le16write_postinc(dataptr, 0);
    le16write_postinc(dataptr, 4);
    le16write_postinc(dataptr, 0);
    le32write_postinc(dataptr, 0);
    le32write_postinc(dataptr, getImageSize());
    le32write_postinc(dataptr, computeSize());
    le32write_postinc(dataptr, 0); // No checksum yet
    le16write_postinc(dataptr, subsysid);
    le16write_postinc(dataptr, getDllFlags());
    le32write_postinc(dataptr, stackreserve);
    le32write_postinc(dataptr, stackcommit);
    le32write_postinc(dataptr, heapreserve);
    le32write_postinc(dataptr, heapcommit);
    le32write_postinc(dataptr, 0);
    le32write_postinc(dataptr, 10); // # Directories
    // "Directories"
    le32pwrite_postinc(dataptr, getExportInfo());
    le32pwrite_postinc(dataptr, getImportInfo());
    le32pwrite_postinc(dataptr, getResourceInfo());
    le32pwrite_postinc(dataptr, getExceptionInfo());
    le32pwrite_postinc(dataptr, getSecurityInfo());
    le32pwrite_postinc(dataptr, getRelocInfo());
    le32pwrite_postinc(dataptr, getDebugInfo());
    le32pwrite_postinc(dataptr, getDescrInfo());
    le32pwrite_postinc(dataptr, getMachInfo());
    le32pwrite_postinc(dataptr, getTlsInfo());
    // Fixup size of optional header
    le16write
	(&data[0] + optHeaderSizeMember, 
	 (dataptr - &data[0]) - coffHeaderSize);
}

const std::vector<uint8_t> &ElfPeHeader::getData() const { return data; }

uint32_t ElfPeHeader::getImageSize() const
{
    uint32_t start = 0;
    uint32_t limit = 0;
    for(int i = 0; i < eof->getNumSections(); i++) {
	{ 
	    const ElfObjectFile::Section &sect = eof->getSection(i);
	    limit = roundup(start + sect.logicalSize(), sectionalign);
	}
	start = limit;
    }

    return limit;
}

uint16_t ElfPeHeader::getPeArch() const
{
    return IMAGE_FILE_MACHINE_POWERPCBE; /* for now */
}

u32pair_t getNamedSectionInfo(ElfObjectFile *eof, const std::string &name) 
{
    const ElfObjectFile::Section *sect = eof->getNamedSection(name);
    if(sect)
	return std::make_pair(sect->getStartRva(), sect->logicalSize());
    else
	return std::make_pair(0,0);
}

u32pair_t ElfPeHeader::getExportInfo() const
{
    return getNamedSectionInfo(eof, ".edata");
}

u32pair_t ElfPeHeader::getImportInfo() const
{
    return getNamedSectionInfo(eof, ".idata");
}

u32pair_t ElfPeHeader::getResourceInfo() const
{
    return getNamedSectionInfo(eof, ".rsrc");
}

u32pair_t ElfPeHeader::getExceptionInfo() const 
{
    return std::make_pair(0,0);
}

u32pair_t ElfPeHeader::getSecurityInfo() const
{
    return std::make_pair(0,0);
}

u32pair_t ElfPeHeader::getRelocInfo() const
{
    return std::make_pair(0,0);
}

u32pair_t ElfPeHeader::getDebugInfo() const
{
    return std::make_pair(0,0);
}

u32pair_t ElfPeHeader::getDescrInfo() const
{
    return std::make_pair(0,0);
}

u32pair_t ElfPeHeader::getTlsInfo() const
{
    return std::make_pair(0,0);
}

u32pair_t ElfPeHeader::getMachInfo() const
{
    return std::make_pair(0,0);
}
