/*++

Copyright (c) 2002-2005 Alexandr A. Telyatnikov (Alter)

Module Name:
    bm_devs.h

Abstract:
    This file contains list of 'well known' PCI IDE controllers

Author:
    Alexander A. Telyatnikov (Alter)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:

--*/

#define IDE_MAX_CHAN          8
// Thanks to SATA Port Multipliers:
#define IDE_MAX_LUN_PER_CHAN  16
#define IDE_MAX_LUN           (AHCI_MAX_PORT*IDE_MAX_LUN_PER_CHAN)

#define MAX_QUEUE_STAT        8

//
// values for TransferMode
//
#define         ATA_PIO                 0x00
#define 	ATA_PIO_NRDY            0x01

#define         ATA_PIO0                0x08
#define         ATA_PIO1                0x09
#define         ATA_PIO2                0x0a
#define         ATA_PIO3                0x0b
#define         ATA_PIO4                0x0c
#define         ATA_PIO5                0x0d

#define         ATA_DMA                 0x10
#define         ATA_SDMA                0x10
#define         ATA_SDMA0               0x10
#define         ATA_SDMA1               0x11
#define         ATA_SDMA2               0x12

#define         ATA_WDMA                0x20
#define         ATA_WDMA0               0x20
#define         ATA_WDMA1               0x21
#define         ATA_WDMA2               0x22

#define         ATA_UDMA                0x40
#define         ATA_UDMA0               0x40 // ATA-16
#define         ATA_UDMA1               0x41 // ATA-25
#define         ATA_UDMA2               0x42 // ATA-33
#define         ATA_UDMA3               0x43 // ATA-44
#define         ATA_UDMA4               0x44 // ATA-66
#define         ATA_UDMA5               0x45 // ATA-100
#define         ATA_UDMA6               0x46 // ATA-133
//#define         ATA_UDMA7               0x47 // ATA-166

#define         ATA_SA150               0x47 /*0x80*/
#define         ATA_SA300               0x48 /*0x81*/

// define PIO timings in nanoseconds
#define         PIO0_TIMING             600

//#define UniataGetPioTiming(LunExt)      ((LunExt->TransferMode <= ATA_PIO0) ? PIO0_TIMING : 0)
#define UniataGetPioTiming(LunExt)      0 //ktp

#ifndef __IDE_BUSMASTER_DEVICES_H__
#define __IDE_BUSMASTER_DEVICES_H__

typedef struct _BUSMASTER_CONTROLLER_INFORMATION {
    PCHAR   VendorId;
    ULONG   VendorIdLength;
    ULONG   nVendorId;
    PCHAR   DeviceId;
    ULONG   DeviceIdLength;
    ULONG   nDeviceId;
    ULONG   nRevId;
    ULONG   MaxTransferMode;
    PCHAR   FullDevName;
    ULONG   RaidFlags;
    CHAR    VendorIdStr[4];
    CHAR    DeviceIdStr[4];
    ULONG   slotNumber;
    ULONG   busNumber;
    CHAR    channel;
//    CHAR    numOfChannes;
    CHAR    MasterDev;
    BOOLEAN Known;
#ifndef USER_MODE
    CHAR    ChanInitOk;             // 0x01 - primary,  0x02 - secondary
    BOOLEAN Isr2Enable;
    PDEVICE_OBJECT Isr2DevObj;
    KIRQL      Isr2Irql;
    KAFFINITY  Isr2Affinity;
    ULONG      Isr2Vector;
    PKINTERRUPT Isr2InterruptObject;
    CHAR    AltInitMasterDev;       // 0xff - uninitialized,  0x00 - normal,  0x01 - change ISA to PCI
#endif

}BUSMASTER_CONTROLLER_INFORMATION, *PBUSMASTER_CONTROLLER_INFORMATION;

/* defines for known chipset PCI id's */
#define ATA_ACARD_ID		0x1191
#define ATA_ATP850		0x00021191
#define ATA_ATP850A		0x00041191
#define ATA_ATP850R		0x00051191
#define ATA_ATP860A		0x00061191
#define ATA_ATP860R		0x00071191
#define ATA_ATP865A		0x00081191
#define ATA_ATP865R		0x00091191

#define ATA_AMD_ID		0x1022
#define ATA_AMD755		0x74011022
#define ATA_AMD756		0x74091022
#define ATA_AMD766		0x74111022
#define ATA_AMD768		0x74411022
#define ATA_AMD8111		0x74691022
#define ATA_AMD5536             0x209a1022

#define ATA_ACER_LABS_ID	0x10b9
#define ATA_ALI_1533            0x153310b9
#define ATA_ALI_5229            0x522910b9
#define ATA_ALI_5281            0x528110b9
#define ATA_ALI_5287            0x528710b9
#define ATA_ALI_5288            0x528810b9
#define ATA_ALI_5289            0x528910b9

#define ATA_ATI_ID              0x1002
#define ATA_ATI_IXP200          0x43491002
#define ATA_ATI_IXP300          0x43691002
#define ATA_ATI_IXP300_S1       0x436e1002
#define ATA_ATI_IXP400          0x43761002
#define ATA_ATI_IXP400_S1       0x43791002
#define ATA_ATI_IXP400_S2       0x437a1002
#define ATA_ATI_IXP600          0x438c1002
#define ATA_ATI_IXP600_S1       0x43801002
#define ATA_ATI_IXP700          0x439c1002
#define ATA_ATI_IXP700_S1       0x43901002

#define ATA_CENATEK_ID          0x16ca
#define ATA_CENATEK_ROCKET      0x000116ca

#define ATA_CYRIX_ID		0x1078
#define ATA_CYRIX_5530		0x01021078

#define ATA_CYPRESS_ID		0x1080
#define ATA_CYPRESS_82C693	0xc6931080

#define ATA_DEC_21150		0x00221011
#define ATA_DEC_21150_1		0x00231011

#define ATA_HIGHPOINT_ID	0x1103
#define ATA_HPT366		0x00041103
#define ATA_HPT372		0x00051103
#define ATA_HPT302		0x00061103
#define ATA_HPT371		0x00071103
#define ATA_HPT374		0x00081103

#define ATA_INTEL_ID		0x8086
#define ATA_I960RM		0x09628086
#define ATA_I82371FB		0x12308086
#define ATA_I82371SB		0x70108086
#define ATA_I82371AB		0x71118086
#define ATA_I82443MX		0x71998086
#define ATA_I82451NX		0x84ca8086
#define ATA_I82372FB		0x76018086
#define ATA_I82801AB		0x24218086
#define ATA_I82801AA		0x24118086
#define ATA_I82801BA		0x244a8086
#define ATA_I82801BA_1		0x244b8086
#define ATA_I82801CA		0x248a8086
#define ATA_I82801CA_1		0x248b8086
#define ATA_I82801DB		0x24cb8086
#define ATA_I82801DB_1		0x24ca8086
#define ATA_I82801EB		0x24db8086
#define ATA_I82801EB_1		0x24d18086
#define ATA_I82801EB_2		0x24df8086
#define ATA_I6300ESB		0x25a28086
#define ATA_I6300ESB_1		0x25a38086
#define ATA_I6300ESB_2		0x25b08086
#define ATA_I63XXESB2           0x269e8086
#define ATA_I63XXESB2_S1        0x26808086
#define ATA_I63XXESB2_S2        0x26818086
#define ATA_I63XXESB2_R1        0x26828086
#define ATA_I63XXESB2_R2        0x26838086
#define ATA_I82801FB		0x266f8086
#define ATA_I82801FB_S1		0x26518086
#define ATA_I82801FB_R1		0x26528086
#define ATA_I82801FB_M          0x26538086
#define ATA_I82801GB            0x27df8086
#define ATA_I82801GB_S1         0x27c08086
#define ATA_I82801GB_R1         0x27c38086
#define ATA_I82801GB_AH         0x27c18086
#define ATA_I82801GB_M          0x27c58086

#define ATA_MARVELL_ID          0x11ab
#define ATA_M88SX5040           0x504011ab
#define ATA_M88SX5041           0x504111ab
#define ATA_M88SX5080           0x508011ab
#define ATA_M88SX5081           0x508111ab
#define ATA_M88SX6041           0x604111ab
#define ATA_M88SX6081           0x608111ab
#define ATA_M88SX6101           0x610111ab
#define ATA_M88SX6145           0x614511ab

#define ATA_MICRON_ID           0x1042
#define ATA_MICRON_RZ1000       0x10001042
#define ATA_MICRON_RZ1001       0x10011042

#define ATA_NATIONAL_ID		0x100b
#define ATA_SC1100		0x0502100b

#define ATA_NETCELL_ID          0x169c
#define ATA_NETCELL_SR          0x0044169c

#define ATA_NVIDIA_ID		0x10de
#define ATA_NFORCE1		0x01bc10de
#define ATA_NFORCE2		0x006510de
#define ATA_NFORCE2_PRO         0x008510de
#define ATA_NFORCE2_PRO_S1      0x008e10de
#define ATA_NFORCE3		0x00d510de
#define ATA_NFORCE3_PRO		0x00e510de
#define ATA_NFORCE3_PRO_S1	0x00e310de
#define ATA_NFORCE3_PRO_S2	0x00ee10de
#define ATA_NFORCE_MCP04        0x003510de
#define ATA_NFORCE_MCP04_S1     0x003610de
#define ATA_NFORCE_MCP04_S2     0x003e10de
#define ATA_NFORCE_CK804        0x005310de
#define ATA_NFORCE_CK804_S1     0x005410de
#define ATA_NFORCE_CK804_S2     0x005510de
#define ATA_NFORCE_MCP51        0x026510de
#define ATA_NFORCE_MCP51_S1     0x026610de
#define ATA_NFORCE_MCP51_S2     0x026710de
#define ATA_NFORCE_MCP55        0x036e10de
#define ATA_NFORCE_MCP55_S1     0x037e10de
#define ATA_NFORCE_MCP55_S2     0x037f10de
#define ATA_NFORCE_MCP61        0x03ec10de
#define ATA_NFORCE_MCP61_S1     0x03e710de
#define ATA_NFORCE_MCP61_S2     0x03f610de
#define ATA_NFORCE_MCP61_S3     0x03f710de
#define ATA_NFORCE_MCP65        0x044810de
#define ATA_NFORCE_MCP67        0x056010de
#define ATA_NFORCE_MCP73        0x056c10de
#define ATA_NFORCE_MCP77        0x075910de

#define ATA_PROMISE_ID		0x105a
#define ATA_PDC20246		0x4d33105a
#define ATA_PDC20262		0x4d38105a
#define ATA_PDC20263		0x0d38105a
#define ATA_PDC20265		0x0d30105a
#define ATA_PDC20267		0x4d30105a
#define ATA_PDC20268		0x4d68105a
#define ATA_PDC20269		0x4d69105a
#define ATA_PDC20270		0x6268105a
#define ATA_PDC20271		0x6269105a
#define ATA_PDC20275		0x1275105a
#define ATA_PDC20276		0x5275105a
#define ATA_PDC20277		0x7275105a
#define ATA_PDC20318		0x3318105a
#define ATA_PDC20319		0x3319105a
#define ATA_PDC20371		0x3371105a
#define ATA_PDC20375		0x3375105a
#define ATA_PDC20376		0x3376105a
#define ATA_PDC20377		0x3377105a
#define ATA_PDC20378		0x3373105a
#define ATA_PDC20379		0x3372105a
#define ATA_PDC20617		0x6617105a
#define ATA_PDC20618		0x6626105a
#define ATA_PDC20619		0x6629105a
#define ATA_PDC20620		0x6620105a
#define ATA_PDC20621		0x6621105a
#define ATA_PDC20622		0x6622105a

#define ATA_SERVERWORKS_ID	0x1166
#define ATA_ROSB4_ISA		0x02001166
#define ATA_ROSB4		0x02111166
#define ATA_CSB5		0x02121166
#define ATA_CSB6		0x02131166
#define ATA_CSB6_1		0x02171166
#define ATA_HT1000              0x02141166
#define ATA_HT1000_S1           0x024b1166
#define ATA_HT1000_S2           0x024a1166
#define ATA_K2			0x02401166
#define ATA_FRODO4		0x02411166
#define ATA_FRODO8		0x02421166

#define ATA_SILICON_IMAGE_ID	0x1095
#define ATA_SII3114		0x31141095
#define ATA_SII3512		0x35121095
#define ATA_SII3112		0x31121095
#define ATA_SII3112_1		0x02401095
#define ATA_SII0680		0x06801095
#define ATA_CMD646		0x06461095
#define ATA_CMD648		0x06481095
#define ATA_CMD649		0x06491095

#define ATA_SIS_ID		0x1039
#define ATA_SISSOUTH		0x00081039
#define ATA_SIS5511		0x55111039
#define ATA_SIS5513		0x55131039
#define ATA_SIS5517		0x55171039
#define ATA_SIS5518		0x55181039
#define ATA_SIS5571		0x55711039
#define ATA_SIS5591		0x55911039
#define ATA_SIS5596		0x55961039
#define ATA_SIS5597		0x55971039
#define ATA_SIS5598		0x55981039
#define ATA_SIS5600		0x56001039
#define ATA_SIS530		0x05301039
#define ATA_SIS540		0x05401039
#define ATA_SIS550		0x05501039
#define ATA_SIS620		0x06201039
#define ATA_SIS630		0x06301039
#define ATA_SIS635		0x06351039
#define ATA_SIS633		0x06331039
#define ATA_SIS640		0x06401039
#define ATA_SIS645		0x06451039
#define ATA_SIS646		0x06461039
#define ATA_SIS648		0x06481039
#define ATA_SIS650		0x06501039
#define ATA_SIS651		0x06511039
#define ATA_SIS652		0x06521039
#define ATA_SIS655		0x06551039
#define ATA_SIS658		0x06581039
#define ATA_SIS661		0x06611039
#define ATA_SIS730		0x07301039
#define ATA_SIS733		0x07331039
#define ATA_SIS735		0x07351039
#define ATA_SIS740		0x07401039
#define ATA_SIS745		0x07451039
#define ATA_SIS746		0x07461039
#define ATA_SIS748		0x07481039
#define ATA_SIS750		0x07501039
#define ATA_SIS751		0x07511039
#define ATA_SIS752		0x07521039
#define ATA_SIS755		0x07551039
#define ATA_SIS961		0x09611039
#define ATA_SIS962		0x09621039
#define ATA_SIS963		0x09631039
#define ATA_SIS964		0x09641039
#define ATA_SIS964_1		0x01801039

#define ATA_VIA_ID		0x1106
#define ATA_VIA82C571		0x05711106
#define ATA_VIA82C586		0x05861106
#define ATA_VIA82C596		0x05961106
#define ATA_VIA82C686		0x06861106
#define ATA_VIA8231		0x82311106
#define ATA_VIA8233		0x30741106
#define ATA_VIA8233A		0x31471106
#define ATA_VIA8233C		0x31091106
#define ATA_VIA8235		0x31771106
#define ATA_VIA8237		0x32271106
#define ATA_VIA8237A            0x05911106
#define ATA_VIA8237S		0x53371106
#define ATA_VIA8251             0x33491106
#define ATA_VIA8361		0x31121106
#define ATA_VIA8363		0x03051106
#define ATA_VIA8371		0x03911106
#define ATA_VIA8662		0x31021106
#define ATA_VIA6410		0x31641106
#define ATA_VIA6420		0x31491106
#define ATA_VIA6421             0x32491106

#define ATA_ITE_ID		0x1283
#define ATA_IT8172G		0x81721283
#define ATA_IT8212F		0x82121283

#define ATA_OPTI_ID             0x1045
#define ATA_OPTI82C621          0xc6211045
#define ATA_OPTI82C625          0xd5681045

#define ATA_HINT_ID	        0x3388
#define ATA_HINTEIDE_ID	        0x80133388

/* chipset setup related defines          */
/* Used in HW_DEVICE_EXTENSION.InitMethod */

#define CHIPTYPE_MASK   0x000000ff
#define CHIPFLAG_MASK   0xffffff00

#define UNIATA_RAID_CONTROLLER  0x80000000
#define UNIATA_SIMPLEX_ONLY     0x40000000
#define UNIATA_NO_SLAVE         0x20000000
#define UNIATA_SATA             0x10000000
#define UNIATA_NO_DPC           0x08000000
#define UNIATA_NO_DPC_ATAPI     0x04000000
#define UNIATA_AHCI             0x02000000
#define UNIATA_NO80CHK          0x01000000

#define ATPOLD		0x0100

#define ALIOLD		0x0100
#define ALINEW		0x0200

#define HPT366		0
#define HPT370		1
#define HPT372		2
#define HPT374		3
#define HPTOLD		0x0100

#define PROLD		0
#define PRNEW		1
#define PRTX		2
#define PRMIO		3
#define PRIDX		4
#define PRTX4		0x0100
#define PRSX4K		0x0200
#define PRSX6K		0x0400
#define PRSATA		0x0800
#define PRDUAL		0x1000

#define SWKS33		0
#define SWKS66		1
#define SWKS100		2
#define SWKSMIO		3

#define SIIOLD          0
#define SIICMD          1
#define SIIMIO          2

#define SIIINTR	        0x0100
#define SIIENINTR       0x0200
#define SII4CH          0x0400
#define SIISETCLK       0x0800
#define SIIBUG          0x1000
#define SIINOSATAIRQ    0x2000

//#define SIS_SOUTH	1
#define SISSATA		2
#define SIS133NEW	3
#define SIS133OLD	4
#define SIS100NEW	5
#define SIS100OLD	6
#define SIS66		7
#define SIS33		8

#define SIS_BASE        0x0100
#define SIS_SOUTH       0x0200

#define ICH4_FIX        0x0100

#define NV4OFF          0x0100
#define NVQ             0x0200

#define VIA33		0
#define VIA66		1
#define VIA100		2
#define VIA133		3
#define AMDNVIDIA	4
#define AMDCABLE	0x0100
#define AMDBUG		0x0200
#define VIABAR		0x0400
#define VIACLK		0x0800
#define VIABUG		0x1000
#define VIASOUTH	0x2000
#define VIAAST		0x4000
#define VIAPRQ		0x8000

#define ITE_33          0
#define ITE_133         1

#ifdef USER_MODE
  #define PCI_DEV_HW_SPEC_BM(idhi, idlo, rev, mode, name, flags) \
    { #idlo, 4, 0x##idlo, #idhi, 4, 0x##idhi, rev, mode, name, flags}
#else
  #define PCI_DEV_HW_SPEC_BM(idhi, idlo, rev, mode, name, flags) \
    { (PCHAR) #idlo, 4, 0x##idlo, (PCHAR) #idhi, 4, 0x##idhi, rev, mode, NULL, flags}
#endif

#define BMLIST_TERMINATOR   (0xffffffffL)

BUSMASTER_CONTROLLER_INFORMATION const BusMasterAdapters[] = {

    PCI_DEV_HW_SPEC_BM( 0005, 1191, 0x00, ATA_UDMA2, "Acard ATP850"     , ATPOLD | UNIATA_SIMPLEX_ONLY            ),
    PCI_DEV_HW_SPEC_BM( 0006, 1191, 0x00, ATA_UDMA4, "Acard ATP860A"    , UNIATA_NO80CHK                          ),
    PCI_DEV_HW_SPEC_BM( 0007, 1191, 0x00, ATA_UDMA4, "Acard ATP860R"    , UNIATA_NO80CHK                          ),
    PCI_DEV_HW_SPEC_BM( 0008, 1191, 0x00, ATA_UDMA6, "Acard ATP865A"    , UNIATA_NO80CHK                          ),
    PCI_DEV_HW_SPEC_BM( 0009, 1191, 0x00, ATA_UDMA6, "Acard ATP865R"    , UNIATA_NO80CHK                          ),
                                          
    PCI_DEV_HW_SPEC_BM( 5289, 10b9, 0x00, ATA_SA150, "ALI M5289"        , UNIATA_SATA | UNIATA_NO_SLAVE           ),
    PCI_DEV_HW_SPEC_BM( 5288, 10b9, 0x00, ATA_SA300, "ALI M5288"        , UNIATA_SATA | UNIATA_NO_SLAVE           ),
    PCI_DEV_HW_SPEC_BM( 5287, 10b9, 0x00, ATA_SA150, "ALI M5287"        , UNIATA_SATA | UNIATA_NO_SLAVE           ),
    PCI_DEV_HW_SPEC_BM( 5281, 10b9, 0x00, ATA_SA150, "ALI M5281"        , UNIATA_SATA | UNIATA_NO_SLAVE           ),
    PCI_DEV_HW_SPEC_BM( 5229, 10b9, 0xc5, ATA_UDMA6, "ALI M5229 UDMA6"  , ALINEW                                  ),
    PCI_DEV_HW_SPEC_BM( 5229, 10b9, 0xc4, ATA_UDMA5, "ALI M5229 UDMA5"  , ALINEW                                  ),
    PCI_DEV_HW_SPEC_BM( 5229, 10b9, 0xc2, ATA_UDMA4, "ALI M5229 UDMA4"  , ALINEW                                  ),
    PCI_DEV_HW_SPEC_BM( 5229, 10b9, 0x20, ATA_UDMA2, "ALI M5229 UDMA2"  , ALIOLD                                  ),
    PCI_DEV_HW_SPEC_BM( 5229, 10b9, 0x00, ATA_WDMA2, "ALI M5229 WDMA2"  , ALIOLD                                  ),

    PCI_DEV_HW_SPEC_BM( 7401, 1022, 0x00, ATA_UDMA2, "AMD 755"          , AMDNVIDIA | 0x00                        ),
    PCI_DEV_HW_SPEC_BM( 7409, 1022, 0x00, ATA_UDMA4, "AMD 756"          , AMDNVIDIA | UNIATA_NO80CHK              ),
    PCI_DEV_HW_SPEC_BM( 7411, 1022, 0x00, ATA_UDMA5, "AMD 766"          , AMDNVIDIA | AMDBUG                      ),
    PCI_DEV_HW_SPEC_BM( 7441, 1022, 0x00, ATA_UDMA5, "AMD 768"          , AMDNVIDIA                               ),
    PCI_DEV_HW_SPEC_BM( 7469, 1022, 0x00, ATA_UDMA6, "AMD 8111"         , AMDNVIDIA                               ),
    PCI_DEV_HW_SPEC_BM( 209a, 1022, 0x00, ATA_UDMA5, "AMD CS5536"       , AMDNVIDIA                               ),

    PCI_DEV_HW_SPEC_BM( 4349, 1002, 0x00, ATA_UDMA5, "ATI IXP200"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 4369, 1002, 0x00, ATA_UDMA6, "ATI IXP300"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 4376, 1002, 0x00, ATA_UDMA6, "ATI IXP400"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 436e, 1002, 0x00, ATA_SA150, "ATI IXP300"       , SIIMIO |                UNIATA_SATA     ),
    PCI_DEV_HW_SPEC_BM( 4379, 1002, 0x00, ATA_SA150, "ATI IXP400"       , SIIMIO | SIINOSATAIRQ | UNIATA_SATA     ),
    PCI_DEV_HW_SPEC_BM( 437a, 1002, 0x00, ATA_SA300, "ATI IXP400"       , SIIMIO | SIINOSATAIRQ | UNIATA_SATA     ),
    PCI_DEV_HW_SPEC_BM( 438c, 1002, 0x00, ATA_UDMA6, "ATI IXP600"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 4380, 1002, 0x00, ATA_SA150, "ATI IXP600"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 439c, 1002, 0x00, ATA_UDMA6, "ATI IXP700"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 4390, 1002, 0x00, ATA_SA150, "ATI IXP700"       , UNIATA_SATA | UNIATA_AHCI               ),

    PCI_DEV_HW_SPEC_BM( 0004, 1103, 0x05, ATA_UDMA6, "HighPoint HPT372" , HPT372 | 0x00   | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0004, 1103, 0x03, ATA_UDMA5, "HighPoint HPT370" , HPT370 | 0x00   | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0004, 1103, 0x02, ATA_UDMA4, "HighPoint HPT368" , HPT366 | 0x00   | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0004, 1103, 0x00, ATA_UDMA4, "HighPoint HPT366" , HPT366 | HPTOLD | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0005, 1103, 0x01, ATA_UDMA6, "HighPoint HPT372" , HPT372 | 0x00   | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0005, 1103, 0x00, ATA_UDMA4, "HighPoint HPT372" , HPT372 | HPTOLD | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0006, 1103, 0x01, ATA_UDMA6, "HighPoint HPT302" , HPT372 | 0x00   | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0007, 1103, 0x01, ATA_UDMA6, "HighPoint HPT371" , HPT372 | 0x00   | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0008, 1103, 0x07, ATA_UDMA6, "HighPoint HPT374" , HPT374 | 0x00   | UNIATA_RAID_CONTROLLER),

    PCI_DEV_HW_SPEC_BM( 1230, 8086, 0x00, ATA_WDMA2, "Intel PIIX"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 7010, 8086, 0x00, ATA_WDMA2, "Intel PIIX3"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 7111, 8086, 0x00, ATA_UDMA3, "Intel PIIX4"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 7199, 8086, 0x00, ATA_UDMA2, "Intel PIIX4"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 84ca, 8086, 0x00, ATA_UDMA2, "Intel PIIX4"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 7601, 8086, 0x00, ATA_UDMA2, "Intel ICH0"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2421, 8086, 0x00, ATA_UDMA4, "Intel ICH"        , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2411, 8086, 0x00, ATA_UDMA4, "Intel ICH"        , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 244a, 8086, 0x00, ATA_UDMA5, "Intel ICH2"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 244b, 8086, 0x00, ATA_UDMA5, "Intel ICH2"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 248a, 8086, 0x00, ATA_UDMA5, "Intel ICH3"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 248b, 8086, 0x00, ATA_UDMA5, "Intel ICH3"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 24cb, 8086, 0x00, ATA_UDMA5, "Intel ICH4"       , ICH4_FIX | UNIATA_NO_DPC                ),
    PCI_DEV_HW_SPEC_BM( 24ca, 8086, 0x00, ATA_UDMA5, "Intel ICH4"       , ICH4_FIX | UNIATA_NO_DPC                ),
    PCI_DEV_HW_SPEC_BM( 24db, 8086, 0x00, ATA_UDMA5, "Intel ICH5 EB"    , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 24d1, 8086, 0x00, ATA_SA150, "Intel ICH5 EB1"   , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 24df, 8086, 0x00, ATA_SA150, "Intel ICH5 EB2"   , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 25a2, 8086, 0x00, ATA_UDMA5, "Intel 6300ESB"    , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 25a3, 8086, 0x00, ATA_SA150, "Intel 6300ESB1"   , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 25b0, 8086, 0x00, ATA_SA150, "Intel 6300ESB2"   , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 266f, 8086, 0x00, ATA_UDMA5, "Intel ICH6"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2651, 8086, 0x00, ATA_SA150, "Intel ICH6"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2652, 8086, 0x00, ATA_SA150, "Intel ICH6"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2653, 8086, 0x00, ATA_SA150, "Intel ICH6M"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 27c0, 8086, 0x00, ATA_UDMA5, "Intel ICH7"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 27c1, 8086, 0x00, ATA_SA300, "Intel ICH7"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 27c3, 8086, 0x00, ATA_SA300, "Intel ICH7"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 27c4, 8086, 0x00, ATA_UDMA5, "Intel ICH7M"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 27c5, 8086, 0x00, ATA_SA150, "Intel ICH7M"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 27c6, 8086, 0x00, ATA_SA150, "Intel ICH7M"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 27df, 8086, 0x00, ATA_UDMA5, "Intel ICH7"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 269e, 8086, 0x00, ATA_UDMA5, "Intel 63XXESB2"   , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2680, 8086, 0x00, ATA_SA300, "Intel 63XXESB2"   , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2681, 8086, 0x00, ATA_SA300, "Intel 63XXESB2"   , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2682, 8086, 0x00, ATA_SA300, "Intel 63XXESB2"   , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2683, 8086, 0x00, ATA_SA300, "Intel 63XXESB2"   , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2820, 8086, 0x00, ATA_UDMA5, "Intel ICH8"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2821, 8086, 0x00, ATA_SA300, "Intel ICH8"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2822, 8086, 0x00, ATA_SA300, "Intel ICH8"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2824, 8086, 0x00, ATA_SA300, "Intel ICH8"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2825, 8086, 0x00, ATA_UDMA5, "Intel ICH8"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2828, 8086, 0x00, ATA_UDMA5, "Intel ICH8M"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2829, 8086, 0x00, ATA_SA300, "Intel ICH8M"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 282a, 8086, 0x00, ATA_SA300, "Intel ICH8M"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2850, 8086, 0x00, ATA_UDMA5, "Intel ICH8M"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2920, 8086, 0x00, ATA_UDMA5, "Intel ICH9R/DO/DH", 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2921, 8086, 0x00, ATA_UDMA5, "Intel ICH9"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2922, 8086, 0x00, ATA_SA300, "Intel ICH9"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2923, 8086, 0x00, ATA_SA300, "Intel ICH9"       , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 2926, 8086, 0x00, ATA_UDMA5, "Intel ICH9"       , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2928, 8086, 0x00, ATA_UDMA5, "Intel ICH9M/M-E"  , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 2929, 8086, 0x00, ATA_SA300, "Intel ICH9M/M-E"  , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 292d, 8086, 0x00, ATA_UDMA5, "Intel ICH9M/M-E"  , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 3a00, 8086, 0x00, ATA_UDMA5, "Intel ICH10"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 3a02, 8086, 0x00, ATA_SA300, "Intel ICH10"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 3a03, 8086, 0x00, ATA_SA300, "Intel ICH10"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 3a06, 8086, 0x00, ATA_UDMA5, "Intel ICH10"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 3a20, 8086, 0x00, ATA_UDMA5, "Intel ICH10"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 3a22, 8086, 0x00, ATA_SA300, "Intel ICH10"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 3a23, 8086, 0x00, ATA_SA300, "Intel ICH10"      , UNIATA_SATA | UNIATA_AHCI               ),
    PCI_DEV_HW_SPEC_BM( 3a26, 8086, 0x00, ATA_UDMA5, "Intel ICH10"      , 0                                       ),

//    PCI_DEV_HW_SPEC_BM( 3200, 8086, 0x00, ATA_SA150, "Intel 31244"      , UNIATA_SATA                             ),
/*    
    PCI_DEV_HW_SPEC_BM( 5040, 11ab, 0x00, ATA_SA150, "Marvell 88SX5040" , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 5041, 11ab, 0x00, ATA_SA150, "Marvell 88SX5041" , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 5080, 11ab, 0x00, ATA_SA150, "Marvell 88SX5080" , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 5081, 11ab, 0x00, ATA_SA150, "Marvell 88SX5081" , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 6041, 11ab, 0x00, ATA_SA300, "Marvell 88SX6041" , UNIATA_SATA                             ),
    PCI_DEV_HW_SPEC_BM( 6081, 11ab, 0x00, ATA_SA300, "Marvell 88SX6081" , UNIATA_SATA                             ),*/
    PCI_DEV_HW_SPEC_BM( 6101, 11ab, 0x00, ATA_UDMA6, "Marvell 88SX6101" , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 6145, 11ab, 0x00, ATA_UDMA6, "Marvell 88SX6145" , 0                                       ),

    PCI_DEV_HW_SPEC_BM( 01bc, 10de, 0x00, ATA_UDMA5, "nVidia nForce"    , AMDNVIDIA                               ),
    PCI_DEV_HW_SPEC_BM( 0065, 10de, 0x00, ATA_UDMA6, "nVidia nForce2"   , AMDNVIDIA                               ),
    PCI_DEV_HW_SPEC_BM( 0085, 10de, 0x00, ATA_UDMA6, "nVidia nForce2 Pro",AMDNVIDIA                               ),
    PCI_DEV_HW_SPEC_BM( 008e, 10de, 0x00, ATA_SA150, "nVidia nForce2 Pro S1",UNIATA_SATA                          ),
    PCI_DEV_HW_SPEC_BM( 00d5, 10de, 0x00, ATA_UDMA6, "nVidia nForce3"   , AMDNVIDIA                               ),
    PCI_DEV_HW_SPEC_BM( 00e5, 10de, 0x00, ATA_UDMA6, "nVidia nForce3 Pro",AMDNVIDIA                               ),
    PCI_DEV_HW_SPEC_BM( 00e3, 10de, 0x00, ATA_SA150, "nVidia nForce3 Pro S1",UNIATA_SATA                          ),
    PCI_DEV_HW_SPEC_BM( 00ee, 10de, 0x00, ATA_SA150, "nVidia nForce3 Pro S2",UNIATA_SATA                          ),
    PCI_DEV_HW_SPEC_BM( 0035, 10de, 0x00, ATA_UDMA6, "nVidia nForce MCP", AMDNVIDIA                               ),
    PCI_DEV_HW_SPEC_BM( 0036, 10de, 0x00, ATA_SA150, "nVidia nForce MCP S1",NV4OFF | UNIATA_SATA                  ),
    PCI_DEV_HW_SPEC_BM( 003e, 10de, 0x00, ATA_SA150, "nVidia nForce MCP S2",NV4OFF | UNIATA_SATA                  ),
    PCI_DEV_HW_SPEC_BM( 0053, 10de, 0x00, ATA_UDMA6, "nVidia nForce CK804", AMDNVIDIA                             ),
    PCI_DEV_HW_SPEC_BM( 0054, 10de, 0x00, ATA_SA300, "nVidia nForce CK804 S1",NV4OFF | UNIATA_SATA                ),
    PCI_DEV_HW_SPEC_BM( 0055, 10de, 0x00, ATA_SA300, "nVidia nForce CK804 S2",NV4OFF | UNIATA_SATA                ),
    PCI_DEV_HW_SPEC_BM( 0265, 10de, 0x00, ATA_UDMA6, "nVidia nForce MCP51", AMDNVIDIA                             ),
    PCI_DEV_HW_SPEC_BM( 0266, 10de, 0x00, ATA_SA300, "nVidia nForce MCP51 S1",NV4OFF | NVQ | UNIATA_SATA          ),
    PCI_DEV_HW_SPEC_BM( 0267, 10de, 0x00, ATA_SA300, "nVidia nForce MCP51 S2",NV4OFF | NVQ | UNIATA_SATA          ),
    PCI_DEV_HW_SPEC_BM( 036e, 10de, 0x00, ATA_UDMA6, "nVidia nForce MCP55", AMDNVIDIA                             ),
    PCI_DEV_HW_SPEC_BM( 037e, 10de, 0x00, ATA_SA300, "nVidia nForce MCP55 S1",NV4OFF | NVQ | UNIATA_SATA          ),
    PCI_DEV_HW_SPEC_BM( 037f, 10de, 0x00, ATA_SA300, "nVidia nForce MCP55 S2",NV4OFF | NVQ | UNIATA_SATA          ),
    PCI_DEV_HW_SPEC_BM( 03ec, 10de, 0x00, ATA_UDMA6, "nVidia nForce MCP61", AMDNVIDIA                             ),
    PCI_DEV_HW_SPEC_BM( 03e7, 10de, 0x00, ATA_SA300, "nVidia nForce MCP61 S1",NV4OFF | NVQ | UNIATA_SATA          ),
    PCI_DEV_HW_SPEC_BM( 03f6, 10de, 0x00, ATA_SA300, "nVidia nForce MCP61 S2",NV4OFF | NVQ | UNIATA_SATA          ),
    PCI_DEV_HW_SPEC_BM( 03f7, 10de, 0x00, ATA_SA300, "nVidia nForce MCP61 S3",NV4OFF | NVQ | UNIATA_SATA          ),
    PCI_DEV_HW_SPEC_BM( 0448, 10de, 0x00, ATA_UDMA6, "nVidia nForce MCP65", AMDNVIDIA                             ),
    PCI_DEV_HW_SPEC_BM( 0560, 10de, 0x00, ATA_UDMA6, "nVidia nForce MCP67", AMDNVIDIA                             ),
    PCI_DEV_HW_SPEC_BM( 056c, 10de, 0x00, ATA_UDMA6, "nVidia nForce MCP73", AMDNVIDIA                             ),
    PCI_DEV_HW_SPEC_BM( 0759, 10de, 0x00, ATA_UDMA6, "nVidia nForce MCP77", AMDNVIDIA                             ),

    PCI_DEV_HW_SPEC_BM( 0502, 100b, 0x00, ATA_UDMA2, "National Geode SC1100", 0                                   ),

    PCI_DEV_HW_SPEC_BM( 4d33, 105a, 0x00, ATA_UDMA2, "Promise PDC20246" , PROLD | 0x00                            ),
    PCI_DEV_HW_SPEC_BM( 4d38, 105a, 0x00, ATA_UDMA4, "Promise PDC20262" , PRNEW | 0x00                            ),
    PCI_DEV_HW_SPEC_BM( 0d38, 105a, 0x00, ATA_UDMA4, "Promise PDC20263" , PRNEW | 0x00    | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0d30, 105a, 0x00, ATA_UDMA5, "Promise PDC20265" , PRNEW | 0x00    | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 4d30, 105a, 0x00, ATA_UDMA5, "Promise PDC20267" , PRNEW | 0x00    | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 4d68, 105a, 0x00, ATA_UDMA5, "Promise PDC20268" , PRTX  | PRTX4   | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 4d69, 105a, 0x00, ATA_UDMA6, "Promise PDC20269" , PRTX  | 0x00    | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 6268, 105a, 0x00, ATA_UDMA5, "Promise PDC20270" , PRTX  | PRTX4   | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 6269, 105a, 0x00, ATA_UDMA6, "Promise PDC20271" , PRTX  | 0x00    | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 1275, 105a, 0x00, ATA_UDMA6, "Promise PDC20275" , PRTX  | 0x00    | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 5275, 105a, 0x00, ATA_UDMA6, "Promise PDC20276" , PRTX  | PRSX6K  | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 7275, 105a, 0x00, ATA_UDMA6, "Promise PDC20277" , PRTX  | 0x00    | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 3318, 105a, 0x00, ATA_SA150, "Promise PDC20318" , PRMIO | PRSATA  | UNIATA_RAID_CONTROLLER | UNIATA_SATA),
    PCI_DEV_HW_SPEC_BM( 3319, 105a, 0x00, ATA_SA150, "Promise PDC20319" , PRMIO | PRSATA  | UNIATA_RAID_CONTROLLER | UNIATA_SATA),
    PCI_DEV_HW_SPEC_BM( 3371, 105a, 0x00, ATA_SA150, "Promise PDC20371" , PRMIO | PRSATA  | UNIATA_RAID_CONTROLLER | UNIATA_SATA),
    PCI_DEV_HW_SPEC_BM( 3375, 105a, 0x00, ATA_SA150, "Promise PDC20375" , PRMIO | PRSATA  | UNIATA_RAID_CONTROLLER | UNIATA_SATA),
    PCI_DEV_HW_SPEC_BM( 3376, 105a, 0x00, ATA_SA150, "Promise PDC20376" , PRMIO | PRSATA  | UNIATA_RAID_CONTROLLER | UNIATA_SATA),
    PCI_DEV_HW_SPEC_BM( 3377, 105a, 0x00, ATA_SA150, "Promise PDC20377" , PRMIO | PRSATA  | UNIATA_RAID_CONTROLLER | UNIATA_SATA),
    PCI_DEV_HW_SPEC_BM( 3373, 105a, 0x00, ATA_SA150, "Promise PDC20378" , PRMIO | PRSATA  | UNIATA_RAID_CONTROLLER | UNIATA_SATA),
    PCI_DEV_HW_SPEC_BM( 3372, 105a, 0x00, ATA_SA150, "Promise PDC20379" , PRMIO | PRSATA  | UNIATA_RAID_CONTROLLER | UNIATA_SATA),
    PCI_DEV_HW_SPEC_BM( 6617, 105a, 0x00, ATA_UDMA6, "Promise PDC20617" , PRMIO | PRDUAL  | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 6626, 105a, 0x00, ATA_UDMA6, "Promise PDC20618" , PRMIO | PRDUAL  | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 6629, 105a, 0x00, ATA_UDMA6, "Promise PDC20619" , PRMIO | PRDUAL  | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 6620, 105a, 0x00, ATA_UDMA6, "Promise PDC20620" , PRMIO | PRDUAL  | UNIATA_RAID_CONTROLLER),
/*    PCI_DEV_HW_SPEC_BM( 6621, 105a, 0x00, ATA_UDMA6, "Promise PDC20621" , PRMIO | PRSX4X  | UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 6622, 105a, 0x00, ATA_UDMA6, "Promise PDC20622" , PRMIO | PRSX4X  | UNIATA_RAID_CONTROLLER),*/

    PCI_DEV_HW_SPEC_BM( 0211, 1166, 0x00, ATA_UDMA2, "ServerWorks ROSB4", SWKS33  | UNIATA_NO_DPC                 ),
    PCI_DEV_HW_SPEC_BM( 0212, 1166, 0x92, ATA_UDMA5, "ServerWorks CSB5" , SWKS100                                 ),
    PCI_DEV_HW_SPEC_BM( 0212, 1166, 0x00, ATA_UDMA4, "ServerWorks CSB5" , SWKS66                                  ),
    PCI_DEV_HW_SPEC_BM( 0213, 1166, 0x00, ATA_UDMA5, "ServerWorks CSB6" , SWKS100                                 ),
    PCI_DEV_HW_SPEC_BM( 0217, 1166, 0x00, ATA_UDMA4, "ServerWorks CSB6" , SWKS66                                  ),
    PCI_DEV_HW_SPEC_BM( 0214, 1166, 0x00, ATA_UDMA5, "ServerWorks HT1000" , SWKS100                               ),
    PCI_DEV_HW_SPEC_BM( 024b, 1166, 0x00, ATA_SA150, "ServerWorks HT1000" , SWKS100                               ),
    PCI_DEV_HW_SPEC_BM( 024a, 1166, 0x00, ATA_SA150, "ServerWorks HT1000" , SWKSMIO                               ),
    PCI_DEV_HW_SPEC_BM( 0240, 1166, 0x00, ATA_SA150, "ServerWorks K2"     , SWKSMIO                               ),
    PCI_DEV_HW_SPEC_BM( 0241, 1166, 0x00, ATA_SA150, "ServerWorks Frodo4" , SWKSMIO                               ),
    PCI_DEV_HW_SPEC_BM( 0242, 1166, 0x00, ATA_SA150, "ServerWorks Frodo8" , SWKSMIO                               ),

    PCI_DEV_HW_SPEC_BM( 3114, 1095, 0x00, ATA_SA150, "SiI 3114"         , SIIMIO | SII4CH | UNIATA_SATA           ),
    PCI_DEV_HW_SPEC_BM( 3512, 1095, 0x02, ATA_SA150, "SiI 3512"         , SIIMIO |          UNIATA_SATA           ),
    PCI_DEV_HW_SPEC_BM( 3112, 1095, 0x02, ATA_SA150, "SiI 3112"         , SIIMIO |          UNIATA_SATA           ),
    PCI_DEV_HW_SPEC_BM( 0240, 1095, 0x02, ATA_SA150, "SiI 3112"         , SIIMIO |          UNIATA_SATA           ),
    PCI_DEV_HW_SPEC_BM( 3512, 1095, 0x00, ATA_SA150, "SiI 3512"         , SIIMIO | SIIBUG | UNIATA_SATA           ),
    PCI_DEV_HW_SPEC_BM( 3112, 1095, 0x00, ATA_SA150, "SiI 3112"         , SIIMIO | SIIBUG | UNIATA_SATA           ),
    PCI_DEV_HW_SPEC_BM( 0240, 1095, 0x00, ATA_SA150, "SiI 3112"         , SIIMIO | SIIBUG | UNIATA_SATA           ),
    PCI_DEV_HW_SPEC_BM( 0680, 1095, 0x00, ATA_UDMA6, "SiI 0680"         , SIIMIO | SIISETCLK                      ),
    PCI_DEV_HW_SPEC_BM( 0649, 1095, 0x00, ATA_UDMA5, "CMD 649"          , SIICMD | SIIINTR | UNIATA_NO_DPC_ATAPI  ),
    PCI_DEV_HW_SPEC_BM( 0648, 1095, 0x00, ATA_UDMA4, "CMD 648"          , SIICMD | SIIINTR                        ),
    PCI_DEV_HW_SPEC_BM( 0646, 1095, 0x07, ATA_UDMA2, "CMD 646U2"        , SIICMD | 0                              ),
    PCI_DEV_HW_SPEC_BM( 0646, 1095, 0x00, ATA_WDMA2, "CMD 646"          , SIICMD | 0                              ),
    PCI_DEV_HW_SPEC_BM( 0640, 1095, 0x00, ATA_PIO4 , "CMD 640"          , SIIOLD | SIIENINTR | UNIATA_SIMPLEX_ONLY),

/**    PCI_DEV_HW_SPEC_BM( 0963, 1039, 0x00, ATA_UDMA6, "SiS 963"          , SIS133NEW                               ),
    PCI_DEV_HW_SPEC_BM( 0962, 1039, 0x00, ATA_UDMA6, "SiS 962"          , SIS133NEW                               ),

    PCI_DEV_HW_SPEC_BM( 0755, 1039, 0x00, ATA_UDMA6, "SiS 755"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0752, 1039, 0x00, ATA_UDMA6, "SiS 752"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0751, 1039, 0x00, ATA_UDMA6, "SiS 751"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0750, 1039, 0x00, ATA_UDMA6, "SiS 750"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0748, 1039, 0x00, ATA_UDMA6, "SiS 748"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0746, 1039, 0x00, ATA_UDMA6, "SiS 746"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0745, 1039, 0x00, ATA_UDMA5, "SiS 745"          , SIS100NEW                               ),
    PCI_DEV_HW_SPEC_BM( 0740, 1039, 0x00, ATA_UDMA5, "SiS 740"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0735, 1039, 0x00, ATA_UDMA5, "SiS 735"          , SIS100NEW                               ),
    PCI_DEV_HW_SPEC_BM( 0733, 1039, 0x00, ATA_UDMA5, "SiS 733"          , SIS100NEW                               ),
    PCI_DEV_HW_SPEC_BM( 0730, 1039, 0x00, ATA_UDMA5, "SiS 730"          , SIS100OLD                               ),

    PCI_DEV_HW_SPEC_BM( 0658, 1039, 0x00, ATA_UDMA6, "SiS 658"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0655, 1039, 0x00, ATA_UDMA6, "SiS 655"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0652, 1039, 0x00, ATA_UDMA6, "SiS 652"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0651, 1039, 0x00, ATA_UDMA6, "SiS 651"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0650, 1039, 0x00, ATA_UDMA6, "SiS 650"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0648, 1039, 0x00, ATA_UDMA6, "SiS 648"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0646, 1039, 0x00, ATA_UDMA6, "SiS 645DX"        , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0645, 1039, 0x00, ATA_UDMA6, "SiS 645"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0640, 1039, 0x00, ATA_UDMA4, "SiS 640"          , SIS_SOUTH                               ),
    PCI_DEV_HW_SPEC_BM( 0635, 1039, 0x00, ATA_UDMA5, "SiS 635"          , SIS100NEW                               ),
    PCI_DEV_HW_SPEC_BM( 0633, 1039, 0x00, ATA_UDMA5, "SiS 633"          , SIS100NEW                               ),
    PCI_DEV_HW_SPEC_BM( 0630, 1039, 0x00, ATA_UDMA5, "SiS 630S"         , SIS100OLD                               ),
    PCI_DEV_HW_SPEC_BM( 0630, 1039, 0x00, ATA_UDMA4, "SiS 630"          , SIS66,                                  ),
    PCI_DEV_HW_SPEC_BM( 0620, 1039, 0x00, ATA_UDMA4, "SiS 620"          , SIS66,                                  ),

    PCI_DEV_HW_SPEC_BM( 0550, 1039, 0x00, ATA_UDMA5, "SiS 550"          , SIS66,                                  ),
    PCI_DEV_HW_SPEC_BM( 0540, 1039, 0x00, ATA_UDMA4, "SiS 540"          , SIS66,                                  ),
    PCI_DEV_HW_SPEC_BM( 0530, 1039, 0x00, ATA_UDMA4, "SiS 530"          , SIS66,                                  ),
*/
    PCI_DEV_HW_SPEC_BM( 5513, 1039, 0xc2, ATA_UDMA2, "SiS ATA-xxx"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 5513, 1039, 0x00, ATA_WDMA2, "SiS ATA-xxx"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 0601, 1039, 0x00, ATA_WDMA2, "SiS ATA-xxx"      , 0                                       ),

/*    PCI_DEV_HW_SPEC_BM( 0586, 1106, 0x41, ATA_UDMA2, "VIA 82C586B"      , VIA33  | 0x00                           ),
    PCI_DEV_HW_SPEC_BM( 0586, 1106, 0x40, ATA_UDMA2, "VIA 82C586B"      , VIA33  | VIAPRQ                         ),
    PCI_DEV_HW_SPEC_BM( 0586, 1106, 0x02, ATA_UDMA2, "VIA 82C586B"      , VIA33  | 0x00                           ),
    PCI_DEV_HW_SPEC_BM( 0586, 1106, 0x00, ATA_WDMA2, "VIA 82C586"       , VIA33  | 0x00                           ),
    PCI_DEV_HW_SPEC_BM( 0596, 1106, 0x12, ATA_UDMA4, "VIA 82C596B"      , VIA66  | VIACLK                         ),
    PCI_DEV_HW_SPEC_BM( 0596, 1106, 0x00, ATA_UDMA2, "VIA 82C596"       , VIA33  | 0x00                           ),
    PCI_DEV_HW_SPEC_BM( 0686, 1106, 0x40, ATA_UDMA5, "VIA 82C686B"      , VIA100 | VIABUG                         ),
    PCI_DEV_HW_SPEC_BM( 0686, 1106, 0x10, ATA_UDMA4, "VIA 82C686A"      , VIA66  | VIACLK                         ),
    PCI_DEV_HW_SPEC_BM( 0686, 1106, 0x00, ATA_UDMA2, "VIA 82C686"       , VIA33  | 0x00                           ),
    PCI_DEV_HW_SPEC_BM( 8231, 1106, 0x00, ATA_UDMA5, "VIA 8231"         , VIA100 | VIABUG                         ),
    PCI_DEV_HW_SPEC_BM( 3074, 1106, 0x00, ATA_UDMA5, "VIA 8233"         , VIA100 | 0x00                           ),
    PCI_DEV_HW_SPEC_BM( 3109, 1106, 0x00, ATA_UDMA5, "VIA 8233C"        , VIA100 | 0x00                           ),
    PCI_DEV_HW_SPEC_BM( 3147, 1106, 0x00, ATA_UDMA6, "VIA 8233A"        , VIA133 | VIAAST                         ),
    PCI_DEV_HW_SPEC_BM( 3177, 1106, 0x00, ATA_UDMA6, "VIA 8235"         , VIA133 | VIAAST                         ),
*/
    PCI_DEV_HW_SPEC_BM( 0571, 1106, 0x00, ATA_UDMA2, "VIA ATA-xxx"      , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 3164, 1106, 0x00, ATA_UDMA6, "VIA 6410"         , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 3149, 1106, 0x00, ATA_SA150, "VIA 6420"         , 0      | UNIATA_SATA                    ),
    PCI_DEV_HW_SPEC_BM( 3249, 1106, 0x00, ATA_SA150, "VIA 6421"         , VIABAR | UNIATA_SATA                    ),
    PCI_DEV_HW_SPEC_BM( 0591, 1106, 0x00, ATA_SA150, "VIA 8237A"        , 0      | UNIATA_SATA                    ),
    PCI_DEV_HW_SPEC_BM( 5337, 1106, 0x00, ATA_SA150, "VIA 8237S"        , 0      | UNIATA_SATA                    ),
    //PCI_DEV_HW_SPEC_BM( 3349, 1106, 0x00, ATA_SA150, "VIA 8251"         , VIAAHCI| UNIATA_SATA                    ),

    PCI_DEV_HW_SPEC_BM( c693, 1080, 0x00, ATA_WDMA2, "Cypress 82C693"   ,0                                        ),

/*                                                                                     
    PCI_DEV_HW_SPEC_BM( 4d68, 105a, 0, 0, "Promise TX2 ATA-100 controller",          UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 6268, 105a, 0, 0, "Promise TX2 ATA-100 controller",          UNIATA_RAID_CONTROLLER),

    PCI_DEV_HW_SPEC_BM( 4d69, 105a, 0, 0, "Promise TX2 ATA-133 controller",          UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 5275, 105a, 0, 0, "Promise TX2 ATA-133 controller",          UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 6269, 105a, 0, 0, "Promise TX2 ATA-133 controller",          UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 7275, 105a, 0, 0, "Promise TX2 ATA-133 controller",          UNIATA_RAID_CONTROLLER),

    PCI_DEV_HW_SPEC_BM( 4d33, 105a, 0, 0, "Promise Ultra/FastTrak-33 controller",    UNIATA_RAID_CONTROLLER),

    PCI_DEV_HW_SPEC_BM( 0d38, 105a, 0, 0, "Promise FastTrak 66 controller",          0),
    PCI_DEV_HW_SPEC_BM( 4d38, 105a, 0, 0, "Promise Ultra/FastTrak-66 controller",    UNIATA_RAID_CONTROLLER),

    PCI_DEV_HW_SPEC_BM( 4d30, 105a, 0, 0, "Promise Ultra/FastTrak-100 controller",   UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0d30, 105a, 0, 0, "Promise OEM ATA-100 controllers",         UNIATA_RAID_CONTROLLER),

    PCI_DEV_HW_SPEC_BM( 0004, 1103, 0, 0, "HighPoint HPT366/368/370/372 controller", UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0005, 1103, 0, 0, "HighPoint HPT372 controller",             UNIATA_RAID_CONTROLLER),
    PCI_DEV_HW_SPEC_BM( 0008, 1103, 0, 0, "HighPoint HPT374 controller",             UNIATA_RAID_CONTROLLER),
*/
    PCI_DEV_HW_SPEC_BM( 0001, 16ca, 0x00, ATA_WDMA2, "Cenatek Rocket Drive",0                                     ),

    PCI_DEV_HW_SPEC_BM( 0102, 1078, 0x00, ATA_UDMA2, "Cyrix 5530"       , 0                                       ),

    PCI_DEV_HW_SPEC_BM( 0102, 1042, 0x00, ATA_PIO4,  "RZ 100x"          , 0                                       ),

    PCI_DEV_HW_SPEC_BM( 8172, 1283, 0x00, ATA_UDMA2, "IT8172"           , 0                                       ),
    PCI_DEV_HW_SPEC_BM( 8212, 1283, 0x00, ATA_UDMA6, "IT8212F"          , ITE_133                                 ),

    PCI_DEV_HW_SPEC_BM( 0044, 169c, 0x00, ATA_UDMA2, "Netcell SR3000/5000", 0                                     ),

    PCI_DEV_HW_SPEC_BM( 8013, 3388, 0x00, ATA_DMA,   "HiNT VXII EIDE"   , 0                                       ),

    // Terminator
    PCI_DEV_HW_SPEC_BM( ffff, ffff, 0xff, BMLIST_TERMINATOR, NULL       , BMLIST_TERMINATOR )
};

#define NUM_BUSMASTER_ADAPTERS (sizeof(BusMasterAdapters) / sizeof(BUSMASTER_CONTROLLER_INFORMATION))

/*
    Looks for device with specified Device/Vendor and Revision
    in specified device list and returnts its index.
    If no matching record found, -1 is returned
*/
__inline
ULONG
Ata_is_dev_listed(
    PBUSMASTER_CONTROLLER_INFORMATION BusMasterAdapters,
    ULONG VendorId,
    ULONG DeviceId,
    ULONG RevId, // min suitable revision
    ULONG lim
    )
{
    for(ULONG k=0; k<lim; k++) {
        if(BusMasterAdapters[k].nVendorId == 0xffff &&
           BusMasterAdapters[k].nDeviceId == 0xffff &&
           BusMasterAdapters[k].nRevId    == 0xff) {
            if(lim != BMLIST_TERMINATOR)
                continue;
            return BMLIST_TERMINATOR;
        }
        if(BusMasterAdapters[k].nVendorId == VendorId &&
           (BusMasterAdapters[k].nDeviceId == DeviceId || DeviceId == 0xffff) &&
           (!RevId || BusMasterAdapters[k].nRevId <= RevId) )
            return k;
    }
    return BMLIST_TERMINATOR;
}

#define Ata_is_supported_dev(pciData) \
    ((pciData)->BaseClass == PCI_DEV_CLASS_STORAGE && \
     (pciData)->SubClass == PCI_DEV_SUBCLASS_IDE)


#endif //__IDE_BUSMASTER_H__
