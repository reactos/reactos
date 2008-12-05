/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "videoprt.h"

#define DDC_EEPROM_ADDRESS  0xA0

/* PRIVATE FUNCTIONS **********************************************************/

#define LOW               0
#define HIGH              1
#define WRITE             0
#define READ              1
#define READ_SDA()        (i2c->ReadDataLine(HwDeviceExtension))
#define READ_SCL()        (i2c->ReadClockLine(HwDeviceExtension))
#define WRITE_SDA(state)  (i2c->WriteDataLine(HwDeviceExtension, state))
#define WRITE_SCL(state)  (i2c->WriteClockLine(HwDeviceExtension, state))

STATIC LARGE_INTEGER HalfPeriodDelay = {{0, 70}};
#define DELAY_HALF()      KeDelayExecutionThread(KernelMode, FALSE, &HalfPeriodDelay)


STATIC BOOL
I2CWrite(PVOID HwDeviceExtension, PI2C_CALLBACKS i2c, UCHAR Data)
{
   UCHAR Bit;
   BOOL Ack;

   /* transmit data */
   for (Bit = (1 << 7); Bit != 0; Bit >>= 1)
     {
        WRITE_SCL(LOW);
        WRITE_SDA((Data & Bit) ? HIGH : LOW);
        DELAY_HALF();
        WRITE_SCL(HIGH);
        DELAY_HALF();
     }

   /* get ack */
   WRITE_SCL(LOW);
   WRITE_SDA(HIGH);
   DELAY_HALF();
   WRITE_SCL(HIGH);
   do
     {
        DELAY_HALF();
     }
   while (READ_SCL() != HIGH);
   Ack = (READ_SDA() == LOW);
   DELAY_HALF();

   INFO_(VIDEOPRT, "I2CWrite: %s\n", Ack ? "Ack" : "Nak");
   return Ack;
}


STATIC UCHAR
I2CRead(PVOID HwDeviceExtension, PI2C_CALLBACKS i2c, BOOL Ack)
{
   INT Bit = 0x80;
   UCHAR Data = 0;

   /* pull down SCL and release SDA */
   WRITE_SCL(LOW);
   WRITE_SDA(HIGH);

   /* read byte */
   for (Bit = (1 << 7); Bit != 0; Bit >>= 1)
     {
        WRITE_SCL(LOW);
        DELAY_HALF();
        WRITE_SCL(HIGH);
        DELAY_HALF();
        if (READ_SDA() == HIGH)
          Data |= Bit;
     }

   /* send ack/nak */
   WRITE_SCL(LOW);
   WRITE_SDA(Ack ? LOW : HIGH);
   DELAY_HALF();
   WRITE_SCL(HIGH);
   do
     {
        DELAY_HALF();
     }
   while (READ_SCL() != HIGH);

   return Data;
}


STATIC VOID
I2CStop(PVOID HwDeviceExtension, PI2C_CALLBACKS i2c)
{
   WRITE_SCL(LOW);
   WRITE_SDA(LOW);
   DELAY_HALF();
   WRITE_SCL(HIGH);
   DELAY_HALF();
   WRITE_SDA(HIGH);
}


STATIC BOOL
I2CStart(PVOID HwDeviceExtension, PI2C_CALLBACKS i2c, UCHAR Address)
{
   /* make sure the bus is free */
   if (READ_SDA() == LOW || READ_SCL() == LOW)
     {
        WARN_(VIDEOPRT, "I2CStart: Bus is not free!\n");
        return FALSE;
     }

   /* send address */
   WRITE_SDA(LOW);
   DELAY_HALF();
   if (!I2CWrite(HwDeviceExtension, i2c, Address))
     {
        /* ??release the bus?? */
        I2CStop(HwDeviceExtension, i2c);
        WARN_(VIDEOPRT, "I2CStart: Device not found (Address = 0x%x)\n", Address);
        return FALSE;
     }

   INFO_(VIDEOPRT, "I2CStart: SUCCESS!\n");
   return TRUE;
}


STATIC BOOL
I2CRepStart(PVOID HwDeviceExtension, PI2C_CALLBACKS i2c, UCHAR Address)
{
   /* setup lines for repeated start condition */
   WRITE_SCL(LOW);
   DELAY_HALF();
   WRITE_SDA(HIGH);
   DELAY_HALF();
   WRITE_SCL(HIGH);
   DELAY_HALF();

   return I2CStart(HwDeviceExtension, i2c, Address);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

BOOLEAN NTAPI
VideoPortDDCMonitorHelper(
   PVOID HwDeviceExtension,
   PVOID I2CFunctions,
   PUCHAR pEdidBuffer,
   ULONG EdidBufferSize
   )
{
   PDDC_CONTROL ddc = (PDDC_CONTROL)I2CFunctions;
   PI2C_CALLBACKS i2c = &ddc->I2CCallbacks;
   INT Count, i;
   PUCHAR pBuffer = (PUCHAR)pEdidBuffer;
   BOOL Ack;

   TRACE_(VIDEOPRT, "VideoPortDDCMonitorHelper()\n");

   ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);
   if (ddc->Size != sizeof (ddc))
     {
        WARN_(VIDEOPRT, "ddc->Size != %d (%d)\n", sizeof (ddc), ddc->Size);
        return FALSE;
     }

   /* select eeprom */
   if (!I2CStart(HwDeviceExtension, i2c, DDC_EEPROM_ADDRESS | WRITE))
     return FALSE;
   /* set address */
   if (!I2CWrite(HwDeviceExtension, i2c, 0x00))
     return FALSE;
   /* change into read mode */
   if (!I2CRepStart(HwDeviceExtension, i2c, DDC_EEPROM_ADDRESS | READ))
     return FALSE;
   /* read eeprom */
   RtlZeroMemory(pEdidBuffer, EdidBufferSize);
   Count = min(128, EdidBufferSize);
   for (i = 0; i < Count; i++)
     {
        Ack = ((i + 1) < Count);
        pBuffer[i] = I2CRead(HwDeviceExtension, i2c, Ack);
     }
   I2CStop(HwDeviceExtension, i2c);

   /* check EDID header */
   if (pBuffer[0] != 0x00 || pBuffer[1] != 0xff ||
       pBuffer[2] != 0xff || pBuffer[3] != 0xff ||
       pBuffer[4] != 0xff || pBuffer[5] != 0xff ||
       pBuffer[6] != 0xff || pBuffer[7] != 0x00)
     {
        WARN_(VIDEOPRT, "VideoPortDDCMonitorHelper(): Invalid EDID header!\n");
        return FALSE;
     }

   INFO_(VIDEOPRT, "VideoPortDDCMonitorHelper(): EDID version %d rev. %d\n", pBuffer[18], pBuffer[19]);
   INFO_(VIDEOPRT, "VideoPortDDCMonitorHelper() - SUCCESS!\n");
   return TRUE;
}

