/*
 * Unit tests for power management functions
 *
 * Copyright (c) 2019 Alex Henrie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/test.h"

void test_GetSystemPowerStatus(void)
{
    SYSTEM_POWER_STATUS ps;
    BOOL ret;
    BYTE capacity_flags, expected_capacity_flags;

    if (0) /* crashes */
        GetSystemPowerStatus(NULL);

    memset(&ps, 0x23, sizeof(ps));
    ret = GetSystemPowerStatus(&ps);
    ok(ret == TRUE, "expected TRUE\n");

    if (ps.BatteryFlag == BATTERY_FLAG_UNKNOWN)
    {
        skip("GetSystemPowerStatus not implemented or not working\n");
        return;
    }
    else if (ps.BatteryFlag != BATTERY_FLAG_NO_BATTERY)
    {
        trace("battery detected\n");
        expected_capacity_flags = 0;
        if (ps.BatteryLifePercent > 66)
            expected_capacity_flags |= BATTERY_FLAG_HIGH;
        if (ps.BatteryLifePercent < 33)
            expected_capacity_flags |= BATTERY_FLAG_LOW;
        if (ps.BatteryLifePercent < 5)
            expected_capacity_flags |= BATTERY_FLAG_CRITICAL;
        capacity_flags = (ps.BatteryFlag & ~BATTERY_FLAG_CHARGING);
        ok(capacity_flags == expected_capacity_flags,
           "expected %u%%-charged battery to have capacity flags 0x%02x, got 0x%02x\n",
           ps.BatteryLifePercent, expected_capacity_flags, capacity_flags);
        ok(ps.BatteryLifeTime <= ps.BatteryFullLifeTime,
           "expected BatteryLifeTime %lu to be less than or equal to BatteryFullLifeTime %lu\n",
           ps.BatteryLifeTime, ps.BatteryFullLifeTime);
        if (ps.BatteryFlag & BATTERY_FLAG_CHARGING)
        {
            ok(ps.BatteryLifeTime == BATTERY_LIFE_UNKNOWN,
               "expected BatteryLifeTime to be -1 when charging, got %lu\n", ps.BatteryLifeTime);
            ok(ps.BatteryFullLifeTime == BATTERY_LIFE_UNKNOWN,
               "expected BatteryFullLifeTime to be -1 when charging, got %lu\n", ps.BatteryFullLifeTime);
        }
    }
    else
    {
        trace("no battery detected\n");
        ok(ps.ACLineStatus == AC_LINE_ONLINE,
           "expected ACLineStatus to be 1, got %u\n", ps.ACLineStatus);
        ok(ps.BatteryLifePercent == BATTERY_PERCENTAGE_UNKNOWN,
           "expected BatteryLifePercent to be -1, got %u\n", ps.BatteryLifePercent);
        ok(ps.BatteryLifeTime == BATTERY_LIFE_UNKNOWN,
           "expected BatteryLifeTime to be -1, got %lu\n", ps.BatteryLifeTime);
        ok(ps.BatteryFullLifeTime == BATTERY_LIFE_UNKNOWN,
           "expected BatteryFullLifeTime to be -1, got %lu\n", ps.BatteryFullLifeTime);
    }
}

START_TEST(power)
{
    test_GetSystemPowerStatus();
}
