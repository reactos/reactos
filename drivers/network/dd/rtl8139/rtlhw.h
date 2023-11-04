/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS RTL8139 Driver
 * FILE:        rtlhw.h
 * PURPOSE:     8139 NIC definitions
 */

#pragma once

#define MAXIMUM_MULTICAST_ADDRESSES 8
#define DEFAULT_INTERRUPT_MASK (R_I_RXOK | R_I_RXERR | R_I_TXOK |          \
                                R_I_TXERR | R_I_RXOVRFLW | R_I_RXUNDRUN |  \
                                R_I_FIFOOVR | R_I_PCSTMOUT | R_I_PCIERR)
#define TX_DESC_COUNT 4

//Register addresses
#define R_MAC           0x00    //MAC address uses bytes 0-5, 6 and 7 are reserved
#define R_MCAST0        0x08    //Multicast registers
#define R_MCAST1        0x09    //Multicast registers
#define R_MCAST2        0x0A
#define R_MCAST3        0x0B
#define R_MCAST4        0x0C
#define R_MCAST5        0x0D
#define R_MCAST6        0x0E
#define R_MCAST7        0x0F
#define R_TXSTS0        0x10    //TX status, 0x10-0x13, 4 bytes
#define R_TXSTS1        0x14
#define R_TXSTS2        0x18
#define R_TXSTS3        0x1C
#define R_TXSAD0        0x20    //TX start address of descriptor 0
#define R_TXSAD1        0x24
#define R_TXSAD2        0x28
#define R_TXSAD3        0x2C
#define R_RXSA          0x30    //RX buffer start address
#define R_ERXBC         0x34    //Early RX byte count register
#define R_ERXSTS        0x36    //Early RX status register

#define R_TXS_HOSTOWNS  0x00002000 //Driver still owns the buffer
#define R_TXS_UNDERRUN  0x00004000 //TX underrun
#define R_TXS_STATOK    0x00008000 //Successful TX
#define R_TXS_OOW       0x20000000 //Out of window
#define R_TXS_ABORTED   0x40000000 //TX aborted
#define R_TXS_CARLOST   0x80000000 //Carrier lost

#define R_CMD           0x37    //Command register
#define R_CMD_RXEMPTY   0x01    //Receive buffer empty
#define B_CMD_TXE       0x04    //Enable TX
#define B_CMD_RXE       0x08    //Enable RX
#define B_CMD_RST       0x10    //Reset bit

#define R_CAPR          0x38    //Current address of packet read
#define R_CBA           0x3A    //Current buffer address
#define R_IM            0x3C    //Interrupt mask register
#define R_IS            0x3E    //Interrupt status register
#define R_TC            0x40    //Transmit configuration register

#define R_I_RXOK        0x0001  //Receive OK
#define R_I_RXERR       0x0002  //Receive error
#define R_I_TXOK        0x0004  //Transmit OK
#define R_I_TXERR       0x0008  //Trasmit error
#define R_I_RXOVRFLW    0x0010  //Receive overflow
#define R_I_RXUNDRUN    0x0020  //Receive underrun
#define R_I_FIFOOVR     0x0040  //FIFO overflow
#define R_I_PCSTMOUT    0x4000  //PCS timeout
#define R_I_PCIERR      0x8000  //PCI error

#define R_RC            0x44    //Receive configuration register
#define B_RC_AAP        0x01    //Accept all packets
#define B_RC_APM        0x02    //Accept packets sent to device MAC
#define B_RC_AM         0x04    //Accept multicast packets
#define B_RC_AB         0x08    //Accept broadcast packets
#define B_RC_AR         0x10    //Accept runt (smaller than 64bytes) packets

#define R_TCTR          0x48    //Timer counter register
#define R_MPC           0x4C    //Missed packet counter
#define R_9346CR        0x50    //93C46 command register
#define R_CFG0          0x51    //Configuration register 0
#define R_CFG1          0x52
#define R_TINTR         0x54    //Timer interrupt register
#define R_MS            0x58    //Media status register

#define R_MS_LINKDWN    0x04    //Link is down
#define R_MS_SPEED_10   0x08    //Media is at 10mbps

#define R_CFG3          0x59    //Configuration register 3
#define R_CFG4          0x5A    //Configuration register 4
#define R_MINTS         0x5C    //Multiple interrupt select
#define R_PCIID         0x5E    //PCI Revision ID = 0x10
#define R_DTSTS         0x60    //TX status of all descriptors
#define R_BMC           0x62    //Basic mode control register
#define R_BMSTS         0x64    //Basic mode status register
#define R_ANA           0x66    //Auto-negotiation advertisement
#define R_ANLP          0x68    //Auto-negotiation link partner
#define R_ANEX          0x6A    //Auto-negotiation expansion
#define R_DCTR          0x6C    //Disconnect counter
#define R_FCSCTR        0x6E    //False carrier sense counter
#define R_NWT           0x70    //N-way test register
#define R_RXERRCTR      0x72    //RX error counter
#define R_CSCFG         0x74    //CS configuration register

#define R_CSCR_LINKOK     0x00400 //Link up
#define R_CSCR_LINKCHNG   0x00800 //Link changed

#define R_PHYP1         0x78    //PHY parameter 1
#define R_TWP           0x7C    //Twister parameter
#define R_PHYP2         0x80    //PHY parameter 2
#define R_PCRC0         0x84    //Power management CRC for wakeup frame 0
#define R_PCRC1         0x85
#define R_PCRC2         0x86
#define R_PCRC3         0x87
#define R_PCRC4         0x88
#define R_PCRC5         0x89
#define R_PCRC6         0x8A
#define R_PCRC7         0x8B
#define R_WAKE0         0x8C    //Power management wakeup frame 0
#define R_WAKE1         0x94
#define R_WAKE2         0x9C
#define R_WAKE3         0xA4
#define R_WAKE4         0xAC
#define R_WAKE5         0xB4
#define R_WAKE6         0xBC
#define R_WAKE7         0xC4
#define R_LSBCRC0       0xCC    //LSB of the mask byte of wakeup frame 0 within offset 12 to 75
#define R_LSBCRC1       0xCD
#define R_LSBCRC2       0xCE
#define R_LSBCRC3       0xCF
#define R_LSBCRC4       0xD0
#define R_LSBCRC5       0xD1
#define R_LSBCRC6       0xD2
#define R_LSBCRC7       0xD3
#define R_CFG5          0xD8    //Configuration register 5

//EEPROM Control Bytes
#define EE_DATA_READ    0x01    //Chip data out
#define EE_DATA_WRITE   0x02    //Chip data in
#define EE_SHIFT_CLK    0x04    //Chip shift clock
#define EE_CS           0x08    //Chip select
#define EE_ENB          0x88    //Chip enable


//EEPROM Commands
#define EE_READ_CMD     0x06

#define RSR_MAR   0x8000  //Multicast receive
#define RSR_PAM   0x4000  //Physical address match (directed packet)
#define RSR_BAR   0x2000  //Broadcast receive
#define RSR_ISE   0x0020  //Invalid symbol
#define RSR_RUNT  0x0010  //Runt packet
#define RSR_LONG  0x0008  //Long packet
#define RSR_CRC   0x0004  //CRC error
#define RSR_FAE   0x0002  //Frame alignment error
#define RSR_ROK   0x0001  //Receive OK

/* NIC prepended structure to a received packet */
typedef struct _PACKET_HEADER {
    USHORT Status;           /* See RSR_* constants */
    USHORT PacketLength;    /* Length of packet NOT including this header */
} PACKET_HEADER, *PPACKET_HEADER;

#define IEEE_802_ADDR_LENGTH 6

/* Ethernet frame header */
typedef struct _ETH_HEADER {
    UCHAR Destination[IEEE_802_ADDR_LENGTH];
    UCHAR Source[IEEE_802_ADDR_LENGTH];
    USHORT PayloadType;
} ETH_HEADER, *PETH_HEADER;

/* EOF */
