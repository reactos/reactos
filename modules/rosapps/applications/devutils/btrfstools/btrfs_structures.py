# PROJECT:     Python tools for traversing BTRFS structures
# LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
# PURPOSE:     Classes and structures for BTRFS on-disk layout
# COPYRIGHT:   Copyright 2018 Victor Perevertkin (victor@perevertkin.ru)

# some code was taken from https://github.com/knorrie/python-btrfs

from btrfs_constants import *
import struct
from collections import namedtuple, OrderedDict
import collections.abc
import copy
import datetime
import os
import uuid
import crc32c

ULLONG_MAX = (1 << 64) - 1
ULONG_MAX = (1 << 32) - 1


def ULL(n):
  return n & ULLONG_MAX


ROOT_TREE_OBJECTID = 1
EXTENT_TREE_OBJECTID = 2
CHUNK_TREE_OBJECTID = 3
DEV_TREE_OBJECTID = 4
FS_TREE_OBJECTID = 5
ROOT_TREE_DIR_OBJECTID = 6
CSUM_TREE_OBJECTID = 7
QUOTA_TREE_OBJECTID = 8
UUID_TREE_OBJECTID = 9
FREE_SPACE_TREE_OBJECTID = 10

DEV_STATS_OBJECTID = 0
BALANCE_OBJECTID = ULL(-4)
ORPHAN_OBJECTID = ULL(-5)
TREE_LOG_OBJECTID = ULL(-6)
TREE_LOG_FIXUP_OBJECTID = ULL(-7)
TREE_RELOC_OBJECTID = ULL(-8)
DATA_RELOC_TREE_OBJECTID = ULL(-9)
EXTENT_CSUM_OBJECTID = ULL(-10)
FREE_SPACE_OBJECTID = ULL(-11)
FREE_INO_OBJECTID = ULL(-12)
MULTIPLE_OBJECTIDS = ULL(-255)

FIRST_FREE_OBJECTID = 256
LAST_FREE_OBJECTID = ULL(-256)
FIRST_CHUNK_TREE_OBJECTID = 256

DEV_ITEMS_OBJECTID = 1

BTRFS_SYSTEM_CHUNK_ARRAY_SIZE = 2048


INODE_ITEM_KEY = 1
INODE_REF_KEY = 12
INODE_EXTREF_KEY = 13
XATTR_ITEM_KEY = 24
ORPHAN_ITEM_KEY = 48
DIR_LOG_ITEM_KEY = 60
DIR_LOG_INDEX_KEY = 72
DIR_ITEM_KEY = 84
DIR_INDEX_KEY = 96
EXTENT_DATA_KEY = 108
EXTENT_CSUM_KEY = 128
ROOT_ITEM_KEY = 132
ROOT_BACKREF_KEY = 144
ROOT_REF_KEY = 156
EXTENT_ITEM_KEY = 168
METADATA_ITEM_KEY = 169
TREE_BLOCK_REF_KEY = 176
EXTENT_DATA_REF_KEY = 178
SHARED_BLOCK_REF_KEY = 182
SHARED_DATA_REF_KEY = 184
BLOCK_GROUP_ITEM_KEY = 192
FREE_SPACE_INFO_KEY = 198
FREE_SPACE_EXTENT_KEY = 199
FREE_SPACE_BITMAP_KEY = 200
DEV_EXTENT_KEY = 204
DEV_ITEM_KEY = 216
CHUNK_ITEM_KEY = 228
QGROUP_STATUS_KEY = 240
QGROUP_INFO_KEY = 242
QGROUP_LIMIT_KEY = 244
QGROUP_RELATION_KEY = 246
BALANCE_ITEM_KEY = 248
DEV_STATS_KEY = 249
DEV_REPLACE_KEY = 250
UUID_KEY_SUBVOL = 251
UUID_KEY_RECEIVED_SUBVOL = 252
STRING_ITEM_KEY = 253

BLOCK_GROUP_SINGLE = 0
BLOCK_GROUP_DATA = 1 << 0
BLOCK_GROUP_SYSTEM = 1 << 1
BLOCK_GROUP_METADATA = 1 << 2
BLOCK_GROUP_RAID0 = 1 << 3
BLOCK_GROUP_RAID1 = 1 << 4
BLOCK_GROUP_DUP = 1 << 5
BLOCK_GROUP_RAID10 = 1 << 6
BLOCK_GROUP_RAID5 = 1 << 7
BLOCK_GROUP_RAID6 = 1 << 8

BLOCK_GROUP_TYPE_MASK = (
  BLOCK_GROUP_DATA |
  BLOCK_GROUP_SYSTEM |
  BLOCK_GROUP_METADATA
)

BLOCK_GROUP_PROFILE_MASK = (
  BLOCK_GROUP_RAID0 |
  BLOCK_GROUP_RAID1 |
  BLOCK_GROUP_RAID5 |
  BLOCK_GROUP_RAID6 |
  BLOCK_GROUP_DUP |
  BLOCK_GROUP_RAID10
)

AVAIL_ALLOC_BIT_SINGLE = 1 << 48 # used in balance_args
SPACE_INFO_GLOBAL_RSV = 1 << 49


_block_group_flags_str_map = {
  BLOCK_GROUP_DATA: 'DATA',
  BLOCK_GROUP_METADATA: 'METADATA',
  BLOCK_GROUP_SYSTEM: 'SYSTEM',
  BLOCK_GROUP_RAID0: 'RAID0',
  BLOCK_GROUP_RAID1: 'RAID1',
  BLOCK_GROUP_DUP: 'DUP',
  BLOCK_GROUP_RAID10: 'RAID10',
  BLOCK_GROUP_RAID5: 'RAID5',
  BLOCK_GROUP_RAID6: 'RAID6',
}

_balance_args_profiles_str_map = {
  BLOCK_GROUP_RAID0: 'RAID0',
  BLOCK_GROUP_RAID1: 'RAID1',
  BLOCK_GROUP_DUP: 'DUP',
  BLOCK_GROUP_RAID10: 'RAID10',
  BLOCK_GROUP_RAID5: 'RAID5',
  BLOCK_GROUP_RAID6: 'RAID6',
  AVAIL_ALLOC_BIT_SINGLE: 'SINGLE',
}

QGROUP_LEVEL_SHIFT = 48

EXTENT_FLAG_DATA = 1 << 0
EXTENT_FLAG_TREE_BLOCK = 1 << 1
BLOCK_FLAG_FULL_BACKREF = 1 << 8

_extent_flags_str_map = {
  EXTENT_FLAG_DATA: 'DATA',
  EXTENT_FLAG_TREE_BLOCK: 'TREE_BLOCK',
  BLOCK_FLAG_FULL_BACKREF: 'FULL_BACKREF',
}

INODE_NODATASUM = 1 << 0
INODE_NODATACOW = 1 << 1
INODE_READONLY = 1 << 2
INODE_NOCOMPRESS = 1 << 3
INODE_PREALLOC = 1 << 4
INODE_SYNC = 1 << 5
INODE_IMMUTABLE = 1 << 6
INODE_APPEND = 1 << 7
INODE_NODUMP = 1 << 8
INODE_NOATIME = 1 << 9
INODE_DIRSYNC = 1 << 10
INODE_COMPRESS = 1 << 11

_inode_flags_str_map = {
  INODE_NODATASUM: 'NODATASUM',
  INODE_READONLY: 'READONLY',
  INODE_NOCOMPRESS: 'NOCOMPRESS',
  INODE_PREALLOC: 'PREALLOC',
  INODE_SYNC: 'SYNC',
  INODE_IMMUTABLE: 'IMMUTABLE',
  INODE_APPEND: 'APPEND',
  INODE_NODUMP: 'NODUMP',
  INODE_NOATIME: 'NOATIME',
  INODE_DIRSYNC: 'DIRSYNC',
  INODE_COMPRESS: 'COMPRESS',
}

ROOT_SUBVOL_RDONLY = 1 << 0

_root_flags_str_map = {
  ROOT_SUBVOL_RDONLY: 'RDONLY',
}

FT_UNKNOWN = 0
FT_REG_FILE = 1
FT_DIR = 2
FT_CHRDEV = 3
FT_BLKDEV = 4
FT_FIFO = 5
FT_SOCK = 6
FT_SYMLINK = 7
FT_XATTR = 8
FT_MAX = 9

_dir_item_type_str_map = {
  FT_UNKNOWN: 'UNKNOWN',
  FT_REG_FILE: 'FILE',
  FT_DIR: 'DIR',
  FT_CHRDEV: 'CHRDEV',
  FT_BLKDEV: 'BLKDEV',
  FT_FIFO: 'FIFO',
  FT_SOCK: 'SOCK',
  FT_SYMLINK: 'SYMLINK',
  FT_XATTR: 'XATTR',
}

COMPRESS_NONE = 0
COMPRESS_ZLIB = 1
COMPRESS_LZO = 2
COMPRESS_ZSTD = 3

_compress_type_str_map = {
  COMPRESS_NONE: 'none',
  COMPRESS_ZLIB: 'zlib',
  COMPRESS_LZO: 'lzo',
  COMPRESS_ZSTD: 'zstd',
}

FILE_EXTENT_INLINE = 0
FILE_EXTENT_REG = 1
FILE_EXTENT_PREALLOC = 2

_file_extent_type_str_map = {
  FILE_EXTENT_INLINE: 'inline',
  FILE_EXTENT_REG: 'regular',
  FILE_EXTENT_PREALLOC: 'prealloc',
}


def qgroup_level(objectid):
  return objectid >> QGROUP_LEVEL_SHIFT


def qgroup_subvid(objectid):
  return objectid & ((1 << QGROUP_LEVEL_SHIFT) - 1)


_key_objectid_str_map = {
  ROOT_TREE_OBJECTID: 'ROOT_TREE',
  EXTENT_TREE_OBJECTID: 'EXTENT_TREE',
  CHUNK_TREE_OBJECTID: 'CHUNK_TREE',
  DEV_TREE_OBJECTID: 'DEV_TREE',
  FS_TREE_OBJECTID: 'FS_TREE',
  ROOT_TREE_DIR_OBJECTID: 'ROOT_TREE_DIR',
  CSUM_TREE_OBJECTID: 'CSUM_TREE',
  QUOTA_TREE_OBJECTID: 'QUOTA_TREE',
  UUID_TREE_OBJECTID: 'UUID_TREE',
  FREE_SPACE_TREE_OBJECTID: 'FREE_SPACE_TREE',
  BALANCE_OBJECTID: 'BALANCE',
  ORPHAN_OBJECTID: 'ORPHAN',
  TREE_LOG_OBJECTID: 'TREE_LOG',
  TREE_LOG_FIXUP_OBJECTID: 'TREE_LOG_FIXUP',
  TREE_RELOC_OBJECTID: 'TREE_RELOC',
  DATA_RELOC_TREE_OBJECTID: 'DATA_RELOC_TREE',
  EXTENT_CSUM_OBJECTID: 'EXTENT_CSUM',
  FREE_SPACE_OBJECTID: 'FREE_SPACE',
  FREE_INO_OBJECTID: 'FREE_INO',
  MULTIPLE_OBJECTIDS: 'MULTIPLE',
}


def key_objectid_str(objectid, _type):
  if _type == DEV_EXTENT_KEY:
    return str(objectid)
  if _type == QGROUP_RELATION_KEY:
    return "{}/{}".format(qgroup_level(objectid), qgroup_subvid(objectid))
  if _type == UUID_KEY_SUBVOL or _type == UUID_KEY_RECEIVED_SUBVOL:
    return "0x{:0>16x}".format(objectid)

  if objectid == ROOT_TREE_OBJECTID and _type == DEV_ITEM_KEY:
    return 'DEV_ITEMS'
  if objectid == DEV_STATS_OBJECTID and _type == DEV_STATS_KEY:
    return 'DEV_STATS'
  if objectid == FIRST_CHUNK_TREE_OBJECTID and _type == CHUNK_ITEM_KEY:
    return 'FIRST_CHUNK_TREE'
  if objectid == ULLONG_MAX:
    return '-1'

  return _key_objectid_str_map.get(objectid, str(objectid))


_key_type_str_map = {
  INODE_ITEM_KEY: 'INODE_ITEM',
  INODE_REF_KEY: 'INODE_REF',
  INODE_EXTREF_KEY: 'INODE_EXTREF',
  XATTR_ITEM_KEY: 'XATTR_ITEM',
  ORPHAN_ITEM_KEY: 'ORPHAN_ITEM',
  DIR_LOG_ITEM_KEY: 'DIR_LOG_ITEM',
  DIR_LOG_INDEX_KEY: 'DIR_LOG_INDEX',
  DIR_ITEM_KEY: 'DIR_ITEM',
  DIR_INDEX_KEY: 'DIR_INDEX',
  EXTENT_DATA_KEY: 'EXTENT_DATA',
  EXTENT_CSUM_KEY: 'EXTENT_CSUM',
  ROOT_ITEM_KEY: 'ROOT_ITEM',
  ROOT_BACKREF_KEY: 'ROOT_BACKREF',
  ROOT_REF_KEY: 'ROOT_REF',
  EXTENT_ITEM_KEY: 'EXTENT_ITEM',
  METADATA_ITEM_KEY: 'METADATA_ITEM',
  TREE_BLOCK_REF_KEY: 'TREE_BLOCK_REF',
  EXTENT_DATA_REF_KEY: 'EXTENT_DATA_REF',
  SHARED_BLOCK_REF_KEY: 'SHARED_BLOCK_REF',
  SHARED_DATA_REF_KEY: 'SHARED_DATA_REF',
  BLOCK_GROUP_ITEM_KEY: 'BLOCK_GROUP_ITEM',
  FREE_SPACE_INFO_KEY: 'FREE_SPACE_INFO',
  FREE_SPACE_EXTENT_KEY: 'FREE_SPACE_EXTENT',
  FREE_SPACE_BITMAP_KEY: 'FREE_SPACE_BITMAP',
  DEV_EXTENT_KEY: 'DEV_EXTENT',
  DEV_ITEM_KEY: 'DEV_ITEM',
  CHUNK_ITEM_KEY: 'CHUNK_ITEM',
  QGROUP_STATUS_KEY: 'QGROUP_STATUS',
  QGROUP_INFO_KEY: 'QGROUP_INFO',
  QGROUP_LIMIT_KEY: 'QGROUP_LIMIT',
  QGROUP_RELATION_KEY: 'QGROUP_RELATION',
  BALANCE_ITEM_KEY: 'BALANCE_ITEM',
  DEV_STATS_KEY: 'DEV_STATS',
  DEV_REPLACE_KEY: 'DEV_REPLACE',
  UUID_KEY_SUBVOL: 'UUID_SUBVOL',
  UUID_KEY_RECEIVED_SUBVOL: 'RECEIVED_SUBVOL',
  STRING_ITEM_KEY: 'STRING_ITEM',
}

# === Helper functions

def key_type_str(_type):
  return _key_type_str_map.get(_type, str(_type))


def key_offset_str(offset, _type):
  if _type == QGROUP_RELATION_KEY or _type == QGROUP_INFO_KEY or _type == QGROUP_LIMIT_KEY:
    return "{}/{}".format(qgroup_level(offset), qgroup_subvid(offset))
  if _type == UUID_KEY_SUBVOL or _type == UUID_KEY_RECEIVED_SUBVOL:
    return "0x{:0>16x}".format(offset)
  if _type == ROOT_ITEM_KEY:
    return _key_objectid_str_map.get(offset, str(offset))
  if offset == ULLONG_MAX:
    return '-1'

  return str(offset)


def flags_str(flags, flags_str_map):
  ret = []
  for flag in sorted(flags_str_map.keys()):
    if flags & flag:
      ret.append(flags_str_map[flag])
  if len(ret) == 0:
    ret.append("none")
    return '|'.join(ret)


def embedded_text_for_str(text):
  try:
    return "utf-8 {}".format(text.decode('utf-8'))
  except UnicodeDecodeError:
   return "raw {}".format(repr(text))


# === Basic structures


class TimeSpec(object):
  sstruct = struct.Struct('<QL')

  @staticmethod
  def from_values(sec, nsec):
    t = TimeSpec.__new__(TimeSpec)
    t.sec = sec
    t.nsec = nsec
    return t

  def __init__(self, data):
    self.sec, self.nsec = TimeSpec.sstruct.unpack_from(data)

  @property
  def iso8601(self):
    return datetime.datetime.utcfromtimestamp(
      float("{self.sec}.{self.nsec}".format(self=self))
    ).isoformat()

  def __str__(self):
   return "{self.sec}.{self.nsec} ({self.iso8601})".format(self=self)


class Key(object):
 def __init__(self, objectid, _type, offset):
  self._objectid = objectid
  self._type = _type
  self._offset = offset
  self._pack()

 @property
 def objectid(self):
  return self._objectid

 @objectid.setter
 def objectid(self, _objectid):
  self._objectid = _objectid
  self._pack()

 @property
 def type(self):
  return self._type

 @type.setter
 def type(self, _type):
  self._type = _type
  self._pack()

 @property
 def offset(self):
  return self._offset

 @offset.setter
 def offset(self, _offset):
  self._offset = _offset
  self._pack()

 @property
 def key(self):
  return self._key

 @key.setter
 def key(self, _key):
  self._key = _key
  self._unpack()

 def _pack(self):
  self._key = (self.objectid << 72) + (self._type << 64) + self.offset

 def _unpack(self):
  self._objectid = self._key >> 72
  self._type = (self._key & ((1 << 72) - 1)) >> 64
  self._offset = (self._key & ((1 << 64) - 1))

 def __lt__(self, other):
  if isinstance(other, Key):
   return self._key < other._key
  return self._key < other

 def __le__(self, other):
  if isinstance(other, Key):
   return self._key <= other._key
  return self._key <= other

 def __eq__(self, other):
  if isinstance(other, Key):
   return self._key == other._key
  return self._key == other

 def __ge__(self, other):
  if isinstance(other, Key):
   return self._key >= other._key
  return self._key >= other

 def __gt__(self, other):
  if isinstance(other, Key):
   return self._key > other._key
  return self._key > other

 def __str__(self):
  return "({} {} {})".format(
   key_objectid_str(self._objectid, self._type),
   key_type_str(self._type),
   key_offset_str(self._offset, self._type),
  )

 def __add__(self, amount):
  new_key = copy.copy(self)
  new_key.key += amount
  return new_key

 def __sub__(self, amount):
  new_key = copy.copy(self)
  new_key.key -= amount
  return new_key

class DiskKey(Key):
  sstruct = struct.Struct('<QBQ')

  def __init__(self, data):
   super(DiskKey, self).__init__(*DiskKey.sstruct.unpack_from(data))

class InnerKey(Key):
  sstruct = struct.Struct('<QBQQQ')

  def __init__(self, data):
   unpacked_data = InnerKey.sstruct.unpack_from(data)
   super().__init__(*unpacked_data[:3])
   self.block_num = unpacked_data[3]
   self.generation = unpacked_data[4]

  def __str__(self):
   return "(inner_key {} {} {} block_num {}, generation {})".format(
    key_objectid_str(self._objectid, self._type),
    key_type_str(self._type),
    key_offset_str(self._offset, self._type),
    self.block_num,
    self.generation,
   )

class LeafKey(Key):
  sstruct = struct.Struct('<QBQLL')

  def __init__(self, data):
   unpacked_data = LeafKey.sstruct.unpack_from(data)
   super().__init__(*unpacked_data[:3])
   self.data_offset = unpacked_data[3]
   self.data_size = unpacked_data[4]

  def __str__(self):
   return "(leaf_key {} {} {} data_offset {:#x} data_size {})".format(
    key_objectid_str(self._objectid, self._type),
    key_type_str(self._type),
    key_offset_str(self._offset, self._type),
    self.data_offset,
    self.data_size,
   )

class ItemData(object):
  def __init__(self, key):
   self.key = key

  def setattr_from_key(self, objectid_attr=None, type_attr=None, offset_attr=None):
    if objectid_attr is not None:
      setattr(self, objectid_attr, self.key.objectid)
    if type_attr is not None:
      setattr(self, type_attr, self.key.type)
    if offset_attr is not None:
      setattr(self, offset_attr, self.key.offset)
    self._key_attrs = objectid_attr, type_attr, offset_attr

  @property
  def key_attrs(self):
    try:
      return self._key_attrs
    except AttributeError:
      return None, None, None

  def __lt__(self, other):
   return self.key < other.key


superblock = struct.Struct('<32x16s2Q8s9Q5L4QH2B611x2048s')
# NOTE: the structure is not complete
# FS UUID
# Physical block address
# Flags
# Signature (_BHRfS_M)
# generation
# Log. address of root of tree roots
# Log. address of chunk tree root
# Log. address of log tree root
# log_root_transid
# total_bytes
# bytes_used
# root_dir_objectid (usually 6)
# num_devices
# sectorsize
# nodesize
# __unused_leafsize
# stripesize
# sys_chunk_array_size
# chunk_root_generation
# compat_flags
# compat_ro_flags
# incompat_flags
# csum_type
# root_level 23
# chunk_root_level 24
# ---
# sys_chunk_array


_node_header_struct = struct.Struct('<32x16sQQ16sQQLB')
NodeHeader = namedtuple('NodeHeader', 'FS_UUID node_addr flags chunk_tree_uuid generation tree_id items_num level')

# === Items

class InodeItem(ItemData):
  _inode_item = [
    struct.Struct('<5Q4L3Q32x'),
    TimeSpec.sstruct,
    TimeSpec.sstruct,
    TimeSpec.sstruct,
    TimeSpec.sstruct,
  ]
  sstruct = struct.Struct('<' + ''.join([s.format[1:].decode() for s in _inode_item]))

  def __init__(self, key, data):
    super().__init__(key)
    self.generation, self.transid, self.size, self.nbytes, self.block_group, \
      self.nlink, self.uid, self.gid, self.mode, self.rdev, self.flags, self.sequence = \
      InodeItem._inode_item[0].unpack_from(data)
    pos = InodeItem._inode_item[0].size
    next_pos = pos + TimeSpec.sstruct.size
    self.atime = TimeSpec(data[pos:next_pos])
    pos, next_pos = next_pos, next_pos + TimeSpec.sstruct.size
    self.ctime = TimeSpec(data[pos:next_pos])
    pos, next_pos = next_pos, next_pos + TimeSpec.sstruct.size
    self.mtime = TimeSpec(data[pos:next_pos])
    pos, next_pos = next_pos, next_pos + TimeSpec.sstruct.size
    self.otime = TimeSpec(data[pos:next_pos])

  @property
  def flags_str(self):
    return flags_str(self.flags, _inode_flags_str_map)

  def __str__(self):
    return "inode generation {self.generation} transid {self.transid} size {self.size} " \
      "nbytes {self.nbytes} block_group {self.block_group} mode {self.mode:05o} " \
      "nlink {self.nlink} uid {self.uid} gid {self.gid} rdev {self.rdev} " \
      "flags {self.flags:#x}({self.flags_str})".format(self=self)


class RootItem(ItemData):
  _root_item = [
    InodeItem.sstruct,
    struct.Struct('<7QL'),
    DiskKey.sstruct,
    struct.Struct('<BBQ16s16s16s4Q'),
    TimeSpec.sstruct,
    TimeSpec.sstruct,
    TimeSpec.sstruct,
    TimeSpec.sstruct,
  ]
  sstruct = struct.Struct('<' + ''.join([s.format[1:].decode() for s in _root_item]))

  def __init__(self, key, data):
    super().__init__(key)
    self.inode = InodeItem(None, data[:InodeItem.sstruct.size])
    pos = InodeItem.sstruct.size
    self.generation, self.dirid, self.bytenr, self.byte_limit, self.bytes_used, \
      self.last_snapshot, self.flags, self.refs = \
      RootItem._root_item[1].unpack_from(data, pos)
    pos += RootItem._root_item[1].size
    self.drop_progress = DiskKey(data[pos:pos+DiskKey.sstruct.size])
    pos += DiskKey.sstruct.size
    self.drop_level, self.level, self.generation_v2, uuid_bytes, parent_uuid_bytes, \
      received_uuid_bytes, self.ctransid, self.otransid, self.stransid, self.rtransid = \
      RootItem._root_item[3].unpack_from(data, pos)
    self.uuid = uuid.UUID(bytes=uuid_bytes)
    self.parent_uuid = uuid.UUID(bytes=parent_uuid_bytes)
    self.received_uuid = uuid.UUID(bytes=received_uuid_bytes)
    pos += RootItem._root_item[3].size
    next_pos = pos + TimeSpec.sstruct.size
    self.ctime = TimeSpec(data[pos:next_pos])
    pos, next_pos = next_pos, next_pos + TimeSpec.sstruct.size
    self.otime = TimeSpec(data[pos:next_pos])
    pos, next_pos = next_pos, next_pos + TimeSpec.sstruct.size
    self.stime = TimeSpec(data[pos:next_pos])
    pos, next_pos = next_pos, next_pos + TimeSpec.sstruct.size
    self.rtime = TimeSpec(data[pos:next_pos])

  @property
  def flags_str(self):
    return flags_str(self.flags, _root_flags_str_map)

  def __str__(self):
    return "root {self.key.objectid} uuid {self.uuid} " \
      "generation {self.generation} last_snapshot {self.last_snapshot} " \
      "bytenr {self.bytenr:#x} level {self.level} " \
      "flags {self.flags:#x}({self.flags_str})".format(self=self)


class Chunk(ItemData):
  sstruct = struct.Struct('<4Q3L2H')

  def __init__(self, key, data):
    super().__init__(key)
    self.setattr_from_key(offset_attr='vaddr')
    self.length, self.owner, self.stripe_len, self.type, self.io_align, \
      self.io_width, self.sector_size, self.num_stripes, self.sub_stripes = \
      Chunk.sstruct.unpack_from(data)
    self.stripes = []
    pos = Chunk.sstruct.size
    for i in range(self.num_stripes):
      next_pos = pos + Stripe.sstruct.size
      self.stripes.append(Stripe(data[pos:next_pos]))
      pos = next_pos

  @property
  def size(self):
   return Chunk.sstruct.size + self.num_stripes * Stripe.sstruct.size

  @property
  def type_str(self):
    return flags_str(self.type, _block_group_flags_str_map)

  def __str__(self):
    return "chunk vaddr {self.vaddr:#x} type {self.type_str} length {self.length} " \
      "num_stripes {self.num_stripes}".format(self=self)


class Stripe(object):
  sstruct = struct.Struct('<2Q16s')

  def __init__(self, data):
    self.devid, self.offset, uuid_bytes = Stripe.sstruct.unpack(data)
    self.uuid = uuid.UUID(bytes=uuid_bytes)

  def __str__(self):
    return "stripe devid {self.devid} offset {self.offset:#x}".format(self=self)


class InodeRefList(ItemData, collections.abc.MutableSequence):
  def __init__(self, header, data):
    super().__init__(header)
    self._list = []
    pos = 0
    while pos < header.len:
      inode_ref = InodeRef(data, pos)
      self._list.append(inode_ref)
      pos += len(inode_ref)

  def __getitem__(self, index):
    return self._list[index]

  def __setitem__(self, index, value):
    self._list[index] = value

  def __delitem__(self, index):
    del self._list[index]

  def __len__(self):
    return len(self._list)

  def insert(self, index, value):
    self._list.insert(index, value)

  def __str__(self):
    return "inode ref list size {}".format(len(self))


class InodeRef(ItemData):
  sstruct = struct.Struct('<QH')

  def __init__(self, key, data):
   super().__init__(key)
   self.index, self.name_len = InodeRef.sstruct.unpack_from(data)
   self.name, = struct.Struct('<{}s'.format(self.name_len)).unpack_from(data, InodeRef.sstruct.size)
   self._len = InodeRef.sstruct.size + self.name_len

  @property
  def name_str(self):
    return embedded_text_for_str(self.name)

  def __len__(self):
    return self._len

  def __str__(self):
   return "inode ref index {self.index} name {self.name_str}".format(self=self)


class DirItemList(ItemData, collections.abc.MutableSequence):
  def __init__(self, key, data):
    super().__init__(key)
    self._list = []
    pos = 0
    while pos < key.data_size:
      cls = {DIR_ITEM_KEY: DirItem, XATTR_ITEM_KEY: XAttrItem}
      dir_item = cls[self.key.type](data, pos)
      self._list.append(dir_item)
      pos += len(dir_item)

  def __getitem__(self, index):
    return self._list[index]

  def __setitem__(self, index, value):
    self._list[index] = value

  def __delitem__(self, index):
    del self._list[index]

  def __len__(self):
    return len(self._list)

  def insert(self, index, value):
    self._list.insert(index, value)

  def __str__(self):
   return "dir item list hash {self.key.offset} size {}".format(len(self), self=self)


class XAttrItemList(DirItemList):
  def __str__(self):
    return "xattr item list hash {self.key.offset} size {}".format(len(self), self=self)


class DirItem(object):
  _dir_item = [
    DiskKey.sstruct,
    struct.Struct('<QHHB')
  ]
  sstruct = struct.Struct('<' + ''.join([s.format[1:].decode() for s in _dir_item]))

  def __init__(self, data, pos):
   next_pos = pos + DiskKey.sstruct.size
   self.location = DiskKey(data[pos:next_pos])
   pos = next_pos
   self.transid, self.data_len, self.name_len, self.type = \
     DirItem._dir_item[1].unpack_from(data, pos)
   pos += DirItem._dir_item[1].size
   self.name, = struct.Struct('<{}s'.format(self.name_len)).unpack_from(data, pos)
   pos += self.name_len
   self.data, = struct.Struct('<{}s'.format(self.data_len)).unpack_from(data, pos)
   pos += self.data_len
   self._len = DirItem.sstruct.size + self.name_len + self.data_len

  @property
  def type_str(self):
    return _dir_item_type_str_map[self.type]

  @property
  def name_str(self):
    return embedded_text_for_str(self.name)

  @property
  def data_str(self):
    return embedded_text_for_str(self.data)

  def __len__(self):
    return self._len

  def __str__(self):
    return "dir item location {self.location} type {self.type_str} " \
      "name {self.name_str}".format(self=self)


class XAttrItem(DirItem):
  def __str__(self):
    return "xattr item name {self.name_str} data {self.data_str}".format(self=self)


class DirIndex(ItemData):
  def __init__(self, header, data):
    super().__init__(header)
    self.location = DiskKey(data[:DiskKey.sstruct.size])
    pos = DiskKey.sstruct.size
    self.transid, self.data_len, self.name_len, self.type = \
      DirItem._dir_item[1].unpack_from(data, pos)
    pos += DirItem._dir_item[1].size
    self.name, = struct.Struct('<{}s'.format(self.name_len)).unpack_from(data, pos)

  @property
  def type_str(self):
    return _dir_item_type_str_map[self.type]

  @property
  def name_str(self):
    return embedded_text_for_str(self.name)

  def __str__(self):
    return "dir index {self.key.offset} location {self.location} type {self.type_str} " \
     "name {self.name_str}".format(self=self)


class FileExtentItem(ItemData):
  _file_extent_item = [
    struct.Struct('<QQBB2xB'),
    struct.Struct('<4Q'),
  ]
  sstruct = struct.Struct('<' + ''.join([s.format[1:].decode()
                          for s in _file_extent_item]))

  def __init__(self, key, data):
    super().__init__(key)
    self.logical_offset = key.offset
    self.generation, self.ram_bytes, self.compression, self.encryption, self.type = \
      FileExtentItem._file_extent_item[0].unpack_from(data)
    if self.type != FILE_EXTENT_INLINE:
      # These are confusing, so they deserve a comment in the code:
      # (disk_bytenr EXTENT_ITEM disk_num_bytes) is the tree key of
      # the extent item storing the actual data.
      #
      # The third one, offset is the offset inside that extent where the
      # data we need starts. num_bytes is the amount of bytes to be used
      # from that offset onwards.
      #
      # Remember that these numbers always be multiples of disk block
      # sizes, because that's how it gets cowed. We don't just use 1 or 2
      # bytes from another extent.
      pos = FileExtentItem._file_extent_item[0].size
      self.disk_bytenr, self.disk_num_bytes, self.offset, self.num_bytes = \
        FileExtentItem._file_extent_item[1].unpack_from(data, pos)
    else:
      self._inline_encoded_nbytes = key.data_size - FileExtentItem._file_extent_item[0].size

  @property
  def compression_str(self):
    return _compress_type_str_map.get(self.compression, 'unknown')

  @property
  def type_str(self):
    return _file_extent_type_str_map.get(self.type, 'unknown')

  def __str__(self):
    ret = ["extent data at {self.logical_offset} generation {self.generation} "
        "ram_bytes {self.ram_bytes} "
        "compression {self.compression_str} type {self.type_str}".format(self=self)]
    if self.type != FILE_EXTENT_INLINE:
      ret.append("disk_bytenr {self.disk_bytenr} disk_num_bytes {self.disk_num_bytes} "
            "offset {self.offset} num_bytes {self.num_bytes}".format(self=self))
    else:
      ret.append("inline_encoded_nbytes {self._inline_encoded_nbytes}".format(self=self))
    return ' '.join(ret)


class ExtentItem(ItemData):
  sstruct = struct.Struct('<3Q')
  extent_inline_ref = struct.Struct('<BQ')

  def __init__(self, header, data, load_data_refs=True, load_metadata_refs=True):
    super().__init__(header)
    self.setattr_from_key(objectid_attr='vaddr', offset_attr='length')
    pos = 0
    self.refs, self.generation, self.flags = ExtentItem.sstruct.unpack_from(data, pos)
    pos += ExtentItem.sstruct.size
    if self.flags == EXTENT_FLAG_DATA and load_data_refs:
      self.extent_data_refs = []
      self.shared_data_refs = []
      while pos < len(data):
        inline_ref_type, inline_ref_offset = \
          ExtentItem.extent_inline_ref.unpack_from(data, pos)
        if inline_ref_type == EXTENT_DATA_REF_KEY:
          pos += 1
          next_pos = pos + InlineExtentDataRef.sstruct.size
          self.extent_data_refs.append(InlineExtentDataRef(data[pos:next_pos]))
          pos = next_pos
        elif inline_ref_type == SHARED_DATA_REF_KEY:
          pos += 1
          next_pos = pos + InlineSharedDataRef.inline_shared_data_ref.size
          self.shared_data_refs.append(InlineSharedDataRef(data[pos:next_pos]))
          pos = next_pos
    elif self.flags & EXTENT_FLAG_TREE_BLOCK and load_metadata_refs:
      next_pos = pos + TreeBlockInfo.tree_block_info.size
      self.tree_block_info = TreeBlockInfo(data[pos:next_pos])
      pos = next_pos
      self.tree_block_refs = []
      self.shared_block_refs = []
      while pos < len(data):
        inline_ref_type, inline_ref_offset = \
          ExtentItem.extent_inline_ref.unpack_from(data, pos)
        if inline_ref_type == TREE_BLOCK_REF_KEY:
          self.tree_block_refs.append(InlineTreeBlockRef(inline_ref_offset))
        elif inline_ref_type == SHARED_BLOCK_REF_KEY:
          self.shared_block_refs.append(InlineSharedBlockRef(inline_ref_offset))
        else:
          raise Exception("BUG: expected inline TREE_BLOCK_REF or SHARED_BLOCK_REF_KEY "
                  "but got inline_ref_type {}".format(inline_ref_type))
        pos += ExtentItem.extent_inline_ref.size

  def append_extent_data_ref(self, ref):
    self.extent_data_refs.append(ref)

  def append_shared_data_ref(self, ref):
    self.shared_data_refs.append(ref)

  def append_tree_block_ref(self, ref):
    self.tree_block_refs.append(ref)

  def append_shared_block_ref(self, ref):
    self.shared_block_refs.append(ref)

  @property
  def flags_str(self):
    return flags_str(self.flags, _extent_flags_str_map)

  def __str__(self):
    return "extent vaddr {self.vaddr} length {self.length} refs {self.refs} " \
      "gen {self.generation} flags {self.flags_str}".format(self=self)


class ExtentDataRef(ItemData):
  sstruct = struct.Struct('<3QL')

  def __init__(self, header, data):
    super().__init__(header)
    self.root, self.objectid, self.offset, self.count = \
      ExtentDataRef.sstruct.unpack(data)

  def __str__(self):
    return "extent data backref root {self.root} objectid {self.objectid} " \
      "offset {self.offset} count {self.count}".format(self=self)


class InlineExtentDataRef(ExtentDataRef):
  sstruct = ExtentDataRef.sstruct

  def __init__(self, data):
    self.root, self.objectid, self.offset, self.count = \
      InlineExtentDataRef.sstruct.unpack(data)

  def __str__(self):
    return "inline extent data backref root {self.root} objectid {self.objectid} " \
      "offset {self.offset} count {self.count}".format(self=self)


class SharedDataRef(ItemData):
  sstruct = struct.Struct('<L')

  def __init__(self, header, data):
    super().__init__(header)
    self.setattr_from_key(offset_attr='parent')
    self.count, = SharedDataRef.sstruct.unpack(data)

  def __str__(self):
    return "shared data backref parent {self.parent} count {self.count}".format(self=self)


class InlineSharedDataRef(SharedDataRef):
  sstruct = struct.Struct('<QL')

  def __init__(self, data):
    self.parent, self.count = InlineSharedDataRef.sstruct.unpack(data)

  def __str__(self):
    return "inline shared data backref parent {self.parent} " \
     "count {self.count}".format(self=self)


class TreeBlockInfo(object):
  sstruct = struct.Struct('<QBQB')

  def __init__(self, data):
    tb_objectid, tb_type, tb_offset, self.level = \
      TreeBlockInfo.sstruct.unpack(data)
    self.key = Key(tb_objectid, tb_type, tb_offset)

  def __str__(self):
   return "tree block key {self.key} level {self.level}".format(self=self)

class TreeBlockRef(ItemData):
  def __init__(self, header):
    super().__init__(header)
    self.setattr_from_key(offset_attr='root')

  def __str__(self):
    return "tree block backref root {}".format(key_objectid_str(self.root, None))


class InlineTreeBlockRef(TreeBlockRef):
  def __init__(self, root):
    self.root = root

  def __str__(self):
    return "inline tree block backref root {}".format(key_objectid_str(self.root, None))


class SharedBlockRef(ItemData):
  def __init__(self, header):
    super().__init__(header)
    self.setattr_from_key(offset_attr='parent')

  def __str__(self):
    return "shared block backref parent {}".format(self.parent)


class InlineSharedBlockRef(SharedBlockRef):
  def __init__(self, parent):
    self.parent = parent

  def __str__(self):
   return "inline shared block backref parent {}".format(self.parent)

# === Main FileSystem class

def key_bin_search(fd, base_offset, item_size, cmp_item, min, max):
  low = min
  high = max

  while low < high:
    mid = (low + high) // 2
    offset = base_offset + mid * item_size

    fd.seek(offset)
    key1 = DiskKey(fd.read(item_size))
    if key1 > cmp_item:
      high = mid
    elif key1 < cmp_item:
      low = mid + 1
    else:
      return True, mid

  return False, low


chunk_map_item = namedtuple('chunk_map_item', 'logical physical length devid')


class FileSystem(object):
  def __init__(self, path, part_offset):
    self._chunk_map = OrderedDict()
    self.path = path
    self.part_offset = part_offset
    self.fd = open(path, 'rb')
    self.fd.seek(part_offset + 0x10000) # going to superblock

    sb_bytes = self.fd.read(superblock.size)
    sb_tuple = superblock.unpack(sb_bytes)

    if sb_tuple[3] != b'_BHRfS_M':
      raise "No signature found"

    # setting base FS information
    self.fsid = sb_tuple[0]
    self.nodesize = sb_tuple[14]
    self.sectorsize = sb_tuple[13]

    self._chunk_root = sb_tuple[6]
    self._chunk_root_level = sb_tuple[24]
    self._tree_roots_root = sb_tuple[5]
    self._tree_roots_root_level = sb_tuple[23]

    # setting chunk map
    sys_chunk_array_size = sb_tuple[17]
    sys_chunk = sb_tuple[25][:sys_chunk_array_size]

    pos = 0
    while pos < sys_chunk_array_size:
      key = DiskKey(sys_chunk[pos:])
      pos += DiskKey.sstruct.size
      chunk = Chunk(key, sys_chunk[pos:])
      for st in chunk.stripes:
        self._insert_chunk(chunk_map_item(chunk.vaddr, st.offset, chunk.length, st.devid))
      pos += chunk.size

    # setting tree roots
    _, fs_tree_root_item = self.search_tree(self._tree_roots_root_level, self._tree_roots_root, Key(FS_TREE_OBJECTID, ROOT_ITEM_KEY, 0))
    _, extent_tree_root_item = self.search_tree(self._tree_roots_root_level, self._tree_roots_root, Key(EXTENT_TREE_OBJECTID, ROOT_ITEM_KEY, 0))
    self._fs_root_level = fs_tree_root_item.level
    self._fs_root = fs_tree_root_item.bytenr
    self._extent_root_level = extent_tree_root_item.level
    self._extent_root = extent_tree_root_item.bytenr

  @property
  def chunk_root(self):
    return self._chunk_root_level, self._chunk_root

  @property
  def tree_roots_root(self):
    return self._tree_roots_root_level, self._tree_roots_root

  @property
  def fs_root(self):
    return self._fs_root_level, self._fs_root

  @property
  def extent_root(self):
    return self._extent_root_level, self._extent_root

  def logical_to_physical(self, log):
    cur_logical = next(iter(self._chunk_map)) # first item

    for logical, cmi in self._chunk_map.items():
      if logical > log:
        break
      cur_logical = logical

    # if there is no address in chunk_map, searching in chunk_tree
    if cur_logical + self._chunk_map[cur_logical].length < log:
      def process_func(header, offset):
        for i in range(header.items_num):
          self.fd.seek(offset + i * LeafKey.sstruct.size)
          k = LeafKey(self.fd.read(LeafKey.sstruct.size))
          self.fd.seek(offset + k.data_offset)
          if k.type == CHUNK_ITEM_KEY:
            item = _key_type_class_map[k.type](k, self.fd.read(k.data_size))
            for st in item.stripes:
              self._insert_chunk(chunk_map_item(item.vaddr, st.offset, item.length, st.devid))

      self.search_tree(self._chunk_root_level, self._chunk_root, Key(FIRST_CHUNK_TREE_OBJECTID, CHUNK_ITEM_KEY, log), process_func)

      cur_logical = next(iter(self._chunk_map)) # first item
      if cur_logical > log:
        raise Exception(f'Cannot translate address {log:#x}')

      for logical, cmi in self._chunk_map.items():
        if logical > log:
          break
        cur_logical = logical

      if cur_logical + self._chunk_map[cur_logical].length < log:
        raise Exception(f'Cannot translate address {log:#x}')

    print('address translation: {:#x} -> {:#x}'.format(log, self._chunk_map[cur_logical].physical + log - cur_logical))
    return self.part_offset + self._chunk_map[cur_logical].physical + log - cur_logical

  def search_tree(self, level, root_offset, key, process_node_func = None):
    for lvl in range(level, 0, -1):
      # inner node
      root_offset = self.logical_to_physical(root_offset)
      self.fd.seek(root_offset)
      header = NodeHeader._make(_node_header_struct.unpack(self.fd.read(_node_header_struct.size)))
      if header.level != lvl:
        raise Exception('Invalid inner node level')

      found, itemnr = key_bin_search(
        self.fd,
        root_offset + _node_header_struct.size,
        InnerKey.sstruct.size,
        key,
        0,
        header.items_num
        )

      # TODO: better understand this
      if not found and itemnr > 0:
        itemnr -= 1

      self.fd.seek(root_offset + _node_header_struct.size + itemnr * InnerKey.sstruct.size)
      k = InnerKey(self.fd.read(InnerKey.sstruct.size))
      root_offset = k.block_num
    else:
      # we are in leaf node
      root_offset = self.logical_to_physical(root_offset)
      self.fd.seek(root_offset)
      header = NodeHeader._make(_node_header_struct.unpack(self.fd.read(_node_header_struct.size)))
      if header.level != 0:
        raise Exception('Invalid leaf level')

      if process_node_func:
        process_node_func(header, root_offset + _node_header_struct.size)
        return root_offset

      found, itemnr = key_bin_search(
        self.fd,
        root_offset + _node_header_struct.size,
        LeafKey.sstruct.size,
        key,
        0,
        header.items_num
        )

      self.fd.seek(root_offset + _node_header_struct.size + itemnr * LeafKey.sstruct.size)
      k = LeafKey(self.fd.read(LeafKey.sstruct.size))
      self.fd.seek(root_offset + _node_header_struct.size + k.data_offset)

      if k.type in _key_type_class_map:
        return k, _key_type_class_map[k.type](k, self.fd.read(k.data_size))
      else:
        return k, False

  def print_node(self, header, offset):
    print(header)
    root_paddr = self.logical_to_physical(header.node_addr)
    key_size = InnerKey.sstruct.size if header.level > 0 else LeafKey.sstruct.size
    key_struct = InnerKey if header.level > 0 else LeafKey
    for i in range(header.items_num):
      self.fd.seek(root_paddr + _node_header_struct.size + i * key_size)
      k = key_struct(self.fd.read(key_size))
      print(k)
      self.fd.seek(root_paddr + _node_header_struct.size + k.data_offset)
      if k.type in _key_type_class_map and header.level == 0:
        item = _key_type_class_map[k.type](k, self.fd.read(k.data_size))
        print(item)
        if k.type == DIR_ITEM_KEY:
          for it in item:
            print(it)
      print('============================')

  def print_chunk_map(self):
    print('=== chunk map ===')
    for logical, cmi in self._chunk_map.items():
      print(f'{cmi.logical:#x}..{cmi.logical+cmi.length:#x} -> {cmi.physical:#x}..{cmi.physical+cmi.length:#x}')
    print('=================')

  def _insert_chunk(self, chunk):
    if not chunk.logical in self._chunk_map:
      cm = dict(self._chunk_map)
      cm[chunk.logical] = chunk
      self._chunk_map = OrderedDict(sorted(cm.items()))


_key_type_class_map = {
  INODE_ITEM_KEY: InodeItem,
  INODE_REF_KEY: InodeRef,
  DIR_ITEM_KEY: DirItemList,
  DIR_INDEX_KEY: DirIndex,
  EXTENT_DATA_KEY: FileExtentItem,
  ROOT_ITEM_KEY: RootItem,
  EXTENT_ITEM_KEY: ExtentItem,
  CHUNK_ITEM_KEY: Chunk,
}
