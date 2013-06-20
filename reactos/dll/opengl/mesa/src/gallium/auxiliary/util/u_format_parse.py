#!/usr/bin/env python

'''
/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
'''


VOID, UNSIGNED, SIGNED, FIXED, FLOAT = range(5)

SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W, SWIZZLE_0, SWIZZLE_1, SWIZZLE_NONE, = range(7)

PLAIN = 'plain'

RGB = 'rgb'
SRGB = 'srgb'
YUV = 'yuv'
ZS = 'zs'


def is_pot(x):
   return (x & (x - 1)) == 0


VERY_LARGE = 99999999999999999999999


class Channel:
    '''Describe the channel of a color channel.'''
    
    def __init__(self, type, norm, pure, size, name = ''):
        self.type = type
        self.norm = norm
        self.pure = pure
        self.size = size
        self.sign = type in (SIGNED, FIXED, FLOAT)
        self.name = name

    def __str__(self):
        s = str(self.type)
        if self.norm:
            s += 'n'
        if self.pure:
            s += 'p'
        s += str(self.size)
        return s

    def __eq__(self, other):
        return self.type == other.type and self.norm == other.norm and self.pure == other.pure and self.size == other.size

    def max(self):
        '''Maximum representable number.'''
        if self.type == FLOAT:
            return VERY_LARGE
        if self.type == FIXED:
            return (1 << (self.size/2)) - 1
        if self.norm:
            return 1
        if self.type == UNSIGNED:
            return (1 << self.size) - 1
        if self.type == SIGNED:
            return (1 << (self.size - 1)) - 1
        assert False
    
    def min(self):
        '''Minimum representable number.'''
        if self.type == FLOAT:
            return -VERY_LARGE
        if self.type == FIXED:
            return -(1 << (self.size/2))
        if self.type == UNSIGNED:
            return 0
        if self.norm:
            return -1
        if self.type == SIGNED:
            return -(1 << (self.size - 1))
        assert False


class Format:
    '''Describe a pixel format.'''

    def __init__(self, name, layout, block_width, block_height, channels, swizzles, colorspace):
        self.name = name
        self.layout = layout
        self.block_width = block_width
        self.block_height = block_height
        self.channels = channels
        self.swizzles = swizzles
        self.name = name
        self.colorspace = colorspace

    def __str__(self):
        return self.name

    def short_name(self):
        '''Make up a short norm for a format, suitable to be used as suffix in
        function names.'''

        name = self.name
        if name.startswith('PIPE_FORMAT_'):
            name = name[len('PIPE_FORMAT_'):]
        name = name.lower()
        return name

    def block_size(self):
        size = 0
        for channel in self.channels:
            size += channel.size
        return size

    def nr_channels(self):
        nr_channels = 0
        for channel in self.channels:
            if channel.size:
                nr_channels += 1
        return nr_channels

    def is_array(self):
        if self.layout != PLAIN:
            return False
        ref_channel = self.channels[0]
        for channel in self.channels[1:]:
            if channel.size and (channel.size != ref_channel.size or channel.size % 8):
                return False
        return True

    def is_mixed(self):
        if self.layout != PLAIN:
            return False
        ref_channel = self.channels[0]
        if ref_channel.type == VOID:
           ref_channel = self.channels[1]
        for channel in self.channels[1:]:
            if channel.type != VOID:
                if channel.type != ref_channel.type:
                    return True
                if channel.norm != ref_channel.norm:
                    return True
                if channel.pure != ref_channel.pure:
                    return True
        return False

    def is_pot(self):
        return is_pot(self.block_size())

    def is_int(self):
        if self.layout != PLAIN:
            return False
        for channel in self.channels:
            if channel.type not in (VOID, UNSIGNED, SIGNED):
                return False
        return True

    def is_float(self):
        if self.layout != PLAIN:
            return False
        for channel in self.channels:
            if channel.type not in (VOID, FLOAT):
                return False
        return True

    def is_bitmask(self):
        if self.layout != PLAIN:
            return False
        if self.block_size() not in (8, 16, 32):
            return False
        for channel in self.channels:
            if channel.type not in (VOID, UNSIGNED, SIGNED):
                return False
        return True

    def inv_swizzles(self):
        '''Return an array[4] of inverse swizzle terms'''
        '''Only pick the first matching value to avoid l8 getting blue and i8 getting alpha'''
        inv_swizzle = [None]*4
        for i in range(4):
            swizzle = self.swizzles[i]
            if swizzle < 4 and inv_swizzle[swizzle] == None:
                inv_swizzle[swizzle] = i
        return inv_swizzle

    def stride(self):
        return self.block_size()/8


_type_parse_map = {
    '':  VOID,
    'x': VOID,
    'u': UNSIGNED,
    's': SIGNED,
    'h': FIXED,
    'f': FLOAT,
}

_swizzle_parse_map = {
    'x': SWIZZLE_X,
    'y': SWIZZLE_Y,
    'z': SWIZZLE_Z,
    'w': SWIZZLE_W,
    '0': SWIZZLE_0,
    '1': SWIZZLE_1,
    '_': SWIZZLE_NONE,
}

def parse(filename):
    '''Parse the format descrition in CSV format in terms of the 
    Channel and Format classes above.'''

    stream = open(filename)
    formats = []
    for line in stream:
        try:
            comment = line.index('#')
        except ValueError:
            pass
        else:
            line = line[:comment]
        line = line.strip()
        if not line:
            continue

        fields = [field.strip() for field in line.split(',')]
        
        name = fields[0]
        layout = fields[1]
        block_width, block_height = map(int, fields[2:4])

        swizzles = [_swizzle_parse_map[swizzle] for swizzle in fields[8]]
        colorspace = fields[9]
        
        if layout == PLAIN:
            names = ['']*4
            if colorspace in (RGB, SRGB):
                for i in range(4):
                    swizzle = swizzles[i]
                    if swizzle < 4:
                        names[swizzle] += 'rgba'[i]
            elif colorspace == ZS:
                for i in range(4):
                    swizzle = swizzles[i]
                    if swizzle < 4:
                        names[swizzle] += 'zs'[i]
            else:
                assert False
            for i in range(4):
                if names[i] == '':
                    names[i] = 'x'
        else:
            names = ['x', 'y', 'z', 'w']

        channels = []
        for i in range(0, 4):
            field = fields[4 + i]
            if field:
                type = _type_parse_map[field[0]]
                if field[1] == 'n':
                    norm = True
                    pure = False
                    size = int(field[2:])
                elif field[1] == 'p':
                    pure = True
                    norm = False
                    size = int(field[2:])
                else:
                    norm = False
                    pure = False
                    size = int(field[1:])
            else:
                type = VOID
                norm = False
                pure = False
                size = 0
            channel = Channel(type, norm, pure, size, names[i])
            channels.append(channel)

        format = Format(name, layout, block_width, block_height, channels, swizzles, colorspace)
        formats.append(format)
    return formats

