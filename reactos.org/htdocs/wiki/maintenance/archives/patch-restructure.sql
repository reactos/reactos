-- The Great Restructuring of October 2004
-- Creates 'page', 'revision' tables and transforms the classic
-- cur+old into a separate page+revision+text structure.
--
-- The pre-conversion 'old' table is renamed to 'text' and used
-- without internal restructuring to avoid rebuilding the entire
-- table. (This can be done separately if desired.)
--
-- The pre-conversion 'cur' table is now redundant and can be
-- discarded when done.

CREATE TABLE /*$wgDBprefix*/page (
  page_id int unsigned NOT NULL auto_increment,
  page_namespace tinyint NOT NULL,
  page_title varchar(255) binary NOT NULL,
  page_restrictions tinyblob NOT NULL,
  page_counter bigint unsigned NOT NULL default '0',
  page_is_redirect tinyint unsigned NOT NULL default '0',
  page_is_new tinyint unsigned NOT NULL default '0',
  page_random real unsigned NOT NULL,
  page_touched binary(14) NOT NULL default '',
  page_latest int unsigned NOT NULL,
  page_len int unsigned NOT NULL,

  PRIMARY KEY page_id (page_id),
  UNIQUE INDEX name_title (page_namespace,page_title),
  INDEX (page_random),
  INDEX (page_len)
);

CREATE TABLE /*$wgDBprefix*/revision (
  rev_id int unsigned NOT NULL auto_increment,
  rev_page int unsigned NOT NULL,
  rev_comment tinyblob NOT NULL,
  rev_user int unsigned NOT NULL default '0',
  rev_user_text varchar(255) binary NOT NULL default '',
  rev_timestamp binary(14) NOT NULL default '',
  rev_minor_edit tinyint unsigned NOT NULL default '0',
  rev_deleted tinyint unsigned NOT NULL default '0',

  
  PRIMARY KEY rev_page_id (rev_page, rev_id),
  UNIQUE INDEX rev_id (rev_id),
  INDEX rev_timestamp (rev_timestamp),
  INDEX page_timestamp (rev_page,rev_timestamp),
  INDEX user_timestamp (rev_user,rev_timestamp),
  INDEX usertext_timestamp (rev_user_text,rev_timestamp)
);

-- If creating new 'text' table it would look like this:
--
-- CREATE TABLE /*$wgDBprefix*/text (
--   old_id int(8) unsigned NOT NULL auto_increment,
--   old_text mediumtext NOT NULL,
--   old_flags tinyblob NOT NULL,
--   
--   PRIMARY KEY old_id (old_id)
-- );


-- Lock!
LOCK TABLES /*$wgDBprefix*/page WRITE, /*$wgDBprefix*/revision WRITE, /*$wgDBprefix*/old WRITE, /*$wgDBprefix*/cur WRITE;

-- Save the last old_id value for later
SELECT (@maxold:=MAX(old_id)) FROM /*$wgDBprefix*/old;

-- First, copy all current entries into the old table.
INSERT
  INTO /*$wgDBprefix*/old
    (old_namespace,
    old_title,
    old_text,
    old_comment,
    old_user,
    old_user_text,
    old_timestamp,
    old_minor_edit,
    old_flags)
  SELECT
    cur_namespace,
    cur_title,
    cur_text,
    cur_comment,
    cur_user,
    cur_user_text,
    cur_timestamp,
    cur_minor_edit,
    ''
  FROM /*$wgDBprefix*/cur;

-- Now, copy all old data except the text into revisions
INSERT
  INTO /*$wgDBprefix*/revision
    (rev_id,
    rev_page,
    rev_comment,
    rev_user,
    rev_user_text,
    rev_timestamp,
    rev_minor_edit)
  SELECT
    old_id,
    cur_id,
    old_comment,
    old_user,
    old_user_text,
    old_timestamp,
    old_minor_edit
  FROM /*$wgDBprefix*/old,/*$wgDBprefix*/cur
  WHERE old_namespace=cur_namespace
    AND old_title=cur_title;

-- And, copy the cur data into page
INSERT
  INTO /*$wgDBprefix*/page
    (page_id,
    page_namespace,
    page_title,
    page_restrictions,
    page_counter,
    page_is_redirect,
    page_is_new,
    page_random,
    page_touched,
    page_latest)
  SELECT
    cur_id,
    cur_namespace,
    cur_title,
    cur_restrictions,
    cur_counter,
    cur_is_redirect,
    cur_is_new,
    cur_random,
    cur_touched,
    rev_id
  FROM /*$wgDBprefix*/cur,/*$wgDBprefix*/revision
  WHERE cur_id=rev_page
    AND rev_timestamp=cur_timestamp
    AND rev_id > @maxold;

UNLOCK TABLES;

-- Keep the old table around as the text store.
-- Its extra fields will be ignored, but trimming them is slow
-- so we won't bother doing it for now.
ALTER TABLE /*$wgDBprefix*/old RENAME TO /*$wgDBprefix*/text;
