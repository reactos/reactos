/*
 * Copyright 2019 Philip Rebohle
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

RWTexture1DArray<uint4> dst;

struct
{
    uint4 clear_value;
    int2 dst_offset;
    int2 dst_extent;
} u_info;

[numthreads(64, 1, 1)]
void main(int3 thread_id : SV_DispatchThreadID)
{
    if (thread_id.x < u_info.dst_extent.x)
        dst[int2(u_info.dst_offset.x + thread_id.x, thread_id.y)] = u_info.clear_value;
}
