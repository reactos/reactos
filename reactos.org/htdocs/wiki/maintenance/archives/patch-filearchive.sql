--
-- Record of deleted file data
--
CREATE TABLE /*$wgDBprefix*/filearchive (
  -- Unique row id
  fa_id int not null auto_increment,
  
  -- Original base filename; key to image.img_name, page.page_title, etc
  fa_name varchar(255) binary NOT NULL default '',
  
  -- Filename of archived file, if an old revision
  fa_archive_name varchar(255) binary default '',
  
  -- Which storage bin (directory tree or object store) the file data
  -- is stored in. Should be 'deleted' for files that have been deleted;
  -- any other bin is not yet in use.
  fa_storage_group varbinary(16),
  
  -- SHA-1 of the file contents plus extension, used as a key for storage.
  -- eg 8f8a562add37052a1848ff7771a2c515db94baa9.jpg
  --
  -- If NULL, the file was missing at deletion time or has been purged
  -- from the archival storage.
  fa_storage_key varbinary(64) default '',
  
  -- Deletion information, if this file is deleted.
  fa_deleted_user int,
  fa_deleted_timestamp binary(14) default '',
  fa_deleted_reason text,
  
  -- Duped fields from image
  fa_size int unsigned default '0',
  fa_width int  default '0',
  fa_height int  default '0',
  fa_metadata mediumblob,
  fa_bits int  default '0',
  fa_media_type ENUM("UNKNOWN", "BITMAP", "DRAWING", "AUDIO", "VIDEO", "MULTIMEDIA", "OFFICE", "TEXT", "EXECUTABLE", "ARCHIVE") default NULL,
  fa_major_mime ENUM("unknown", "application", "audio", "image", "text", "video", "message", "model", "multipart") default "unknown",
  fa_minor_mime varbinary(32) default "unknown",
  fa_description tinyblob,
  fa_user int unsigned default '0',
  fa_user_text varchar(255) binary default '',
  fa_timestamp binary(14) default '',
  
  PRIMARY KEY (fa_id),
  INDEX (fa_name, fa_timestamp),             -- pick out by image name
  INDEX (fa_storage_group, fa_storage_key),  -- pick out dupe files
  INDEX (fa_deleted_timestamp),              -- sort by deletion time
  INDEX (fa_deleted_user)                    -- sort by deleter

) /*$wgDBTableOptions*/;
