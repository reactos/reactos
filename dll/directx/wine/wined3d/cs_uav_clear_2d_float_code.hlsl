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

RWTexture2D<float4> dst;

struct
{
    float4 clear_value;
    int2 dst_offset;
    int2 dst_extent;
} u_info;

[numthreads(8, 8, 1)]
void main(int3 thread_id : SV_DispatchThreadID)
{
    if (all(thread_id.xy < u_info.dst_extent.xy))
        dst[u_info.dst_offset.xy + thread_id.xy] = u_info.clear_value;
}
