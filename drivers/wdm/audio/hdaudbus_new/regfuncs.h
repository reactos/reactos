/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/hdaudbus/regfuncs.h
 * PURPOSE:         HDAUDBUS Driver
 * PROGRAMMER:      Coolstar TODO
                    Johannes Anderwald
 */

static inline UINT8 read8(PVOID addr) {
	UINT8 ret = *(UINT8*)addr;
	return ret;
}

static inline void write8(PVOID addr, UINT8 data) {
	*(UINT8*)addr = data;
}

static inline UINT16 read16(PVOID addr) {
	UINT16 ret = *(UINT16*)addr;
	return ret;
}

static inline void write16(PVOID addr, UINT16 data) {
	*(UINT16*)addr = data;
}

static inline UINT32 read32(PVOID addr) {
	UINT32 ret = *(UINT32*)addr;
	return ret;
}

static inline void write32(PVOID addr, UINT32 data) {
	*(UINT32*)addr = data;
}

static inline void pci_read_cfg_byte(PBUS_INTERFACE_STANDARD pciInterface, UINT reg, BYTE* data) {
	if (!data) {
		return;
	}
	pciInterface->GetBusData(pciInterface->Context, PCI_WHICHSPACE_CONFIG, data, reg, sizeof(BYTE));
}

static inline void pci_read_cfg_dword(PBUS_INTERFACE_STANDARD pciInterface, UINT reg, UINT32* data) {
	if (!data) {
		return;
	}
	pciInterface->GetBusData(pciInterface->Context, PCI_WHICHSPACE_CONFIG, data, reg, sizeof(UINT32));
}

static inline void pci_write_cfg_byte(PBUS_INTERFACE_STANDARD pciInterface, UINT reg, BYTE data) {
	pciInterface->SetBusData(pciInterface->Context, PCI_WHICHSPACE_CONFIG, &data, reg, sizeof(BYTE));
}

static inline void pci_write_cfg_dword(PBUS_INTERFACE_STANDARD pciInterface, UINT reg, UINT32 data) {
	pciInterface->GetBusData(pciInterface->Context, PCI_WHICHSPACE_CONFIG, &data, reg, sizeof(UINT32));
}

static inline void update_pci_byte(PBUS_INTERFACE_STANDARD pciInterface, UINT reg, BYTE mask, BYTE val) {
	BYTE data;
	pci_read_cfg_byte(pciInterface, reg, &data);
	data &= ~mask;
	data |= (val & mask);
	pci_write_cfg_byte(pciInterface, reg, data);
}

#define HDA_RATE(base, mult, div) \
	(AC_FMT_BASE_##base##K | (((mult) - 1) << AC_FMT_MULT_SHIFT) | \
	 (((div) - 1) << AC_FMT_DIV_SHIFT))

#define hda_read8(ctx, reg) read8((ctx)->m_BAR0.Base.baseptr + HDA_REG_##reg)
#define hda_write8(ctx, reg, data) write8((ctx)->m_BAR0.Base.baseptr + HDA_REG_##reg, data)
#define hda_update8(ctx, reg, mask, val) hda_write8(ctx, reg, (hda_read8(ctx, reg) & ~(mask)) | (val))
#define hda_read16(ctx, reg) read16((ctx)->m_BAR0.Base.baseptr + HDA_REG_##reg)
#define hda_write16(ctx, reg, data) write16((ctx)->m_BAR0.Base.baseptr + HDA_REG_##reg, data)
#define hda_update16(ctx, reg, mask, val) hda_write16(ctx, reg, (hda_read16(ctx, reg) & ~(mask)) | (val))
#define hda_read32(ctx, reg) read32((ctx)->m_BAR0.Base.baseptr + HDA_REG_##reg)
#define hda_write32(ctx, reg, data) write32((ctx)->m_BAR0.Base.baseptr + HDA_REG_##reg, data)
#define hda_update32(ctx, reg, mask, val) hda_write32(ctx, reg, (hda_read32(ctx, reg) & ~(mask)) | (val))

#define stream_read8(ctx, reg) read8((ctx)->sdAddr + HDA_REG_##reg)
#define stream_write8(ctx, reg, data) write8((ctx)->sdAddr + HDA_REG_##reg, data)
#define stream_update8(ctx, reg, mask, val) stream_write8(ctx, reg, (stream_read8(ctx, reg) & ~(mask)) | (val))
#define stream_read16(ctx, reg) read16((ctx)->sdAddr + HDA_REG_##reg)
#define stream_write16(ctx, reg, data) write16((ctx)->sdAddr + HDA_REG_##reg, data)
#define stream_update16(ctx, reg, mask, val) stream_write16(ctx, reg, (stream_read16(ctx, reg) & ~(mask)) | (val))
#define stream_read32(ctx, reg) read32((ctx)->sdAddr + HDA_REG_##reg)
#define stream_write32(ctx, reg, data) write32((ctx)->sdAddr + HDA_REG_##reg, data)
#define stream_update32(ctx, reg, mask, val) stream_write32(ctx, reg, (stream_read32(ctx, reg) & ~(mask)) | (val))

/* update register macro */
#define hdac_update32(addr, reg, mask, val)		\
	write32(addr + reg, ((read32(addr + reg) & ~(mask)) | (val)))

#define hdac_update16(addr, reg, mask, val)		\
	write16(addr + reg,((read16(addr + reg) & ~(mask)) | (val)))
