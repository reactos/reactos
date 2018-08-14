# PROJECT:     Python tools for traversing BTRFS structures
# LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
# PURPOSE:     Script for obtaining freeldr.sys from BTRFS disk image
# COPYRIGHT:   Copyright 2018 Victor Perevertkin (victor@perevertkin.ru)

from btrfs_structures import *
import crc32c

fs = FileSystem('btrfs-big.bin', 0x7e00)

fs.print_chunk_map()


freeldr_dir_key = Key(256, DIR_ITEM_KEY, crc32c.name_hash('freeldr.sys')) # 256 - root dir objectid crc32c.name_hash('freeldr.sys')
print(freeldr_dir_key)

print('!!!!!!!!!!!!!!!!!!!! fs tree 1')
fs_level, fs_root = fs.fs_root
freeldr_dir_key, freeldr_dir_item = fs.search_tree(fs_level, fs_root, freeldr_dir_key)
fs.search_tree(fs_level, fs_root, freeldr_dir_key, fs.print_node)

freeldr_item, = (x for x in freeldr_dir_item if x.name.decode('utf-8') == 'freeldr.sys')
freeldr_extent_data_key = Key(freeldr_item.location.objectid, EXTENT_DATA_KEY, 0)

print('!!!!!!!!!!!!!!!!!!!! fs tree 2')
freeldr_extent_data_key, freeldr_extent_data_item = fs.search_tree(fs_level, fs_root, freeldr_extent_data_key)
fs.search_tree(fs_level, fs_root, freeldr_extent_data_key, fs.print_node)

# # exploring extent tree
print('!!!!!!!!!!!!!!!!!!!! extent tree')
extent_level, extent_root = fs.extent_root
exkey, extent_item = fs.search_tree(extent_level, extent_root, Key(freeldr_extent_data_item.disk_bytenr, EXTENT_ITEM_KEY, freeldr_extent_data_item.disk_num_bytes))

print(freeldr_extent_data_item)
fs.fd.seek(fs.logical_to_physical(extent_item.vaddr))
freeldr = fs.fd.read(freeldr_extent_data_item.num_bytes)

file = open("readed_freeldr.sys", "wb")
file.write(freeldr)
print(crc32c.name_hash('freeldr.sys'))
