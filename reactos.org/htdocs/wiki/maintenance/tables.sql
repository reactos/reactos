-- SQL to create the initial tables for the MediaWiki database.
-- This is read and executed by the install script; you should
-- not have to run it by itself unless doing a manual install.

--
-- General notes:
--
-- If possible, create tables as InnoDB to benefit from the
-- superior resiliency against crashes and ability to read
-- during writes (and write during reads!)
--
-- Only the 'searchindex' table requires MyISAM due to the
-- requirement for fulltext index support, which is missing
-- from InnoDB.
--
--
-- The MySQL table backend for MediaWiki currently uses
-- 14-character BINARY or VARBINARY fields to store timestamps.
-- The format is YYYYMMDDHHMMSS, which is derived from the
-- text format of MySQL's TIMESTAMP fields.
--
-- Historically TIMESTAMP fields were used, but abandoned
-- in early 2002 after a lot of trouble with the fields
-- auto-updating.
--
-- The Postgres backend uses DATETIME fields for timestamps,
-- and we will migrate the MySQL definitions at some point as
-- well.
--
--
-- The /*$wgDBprefix*/ comments in this and other files are
-- replaced with the defined table prefix by the installer
-- and updater scripts. If you are installing or running
-- updates manually, you will need to manually insert the
-- table prefix if any when running these scripts.
--


--
-- The user table contains basic account information,
-- authentication keys, etc.
--
-- Some multi-wiki sites may share a single central user table
-- between separate wikis using the $wgSharedDB setting.
--
-- Note that when a external authentication plugin is used,
-- user table entries still need to be created to store
-- preferences and to key tracking information in the other
-- tables.
--
CREATE TABLE /*$wgDBprefix*/user (
  user_id int unsigned NOT NULL auto_increment,
  
  -- Usernames must be unique, must not be in the form of
  -- an IP address. _Shouldn't_ allow slashes or case
  -- conflicts. Spaces are allowed, and are _not_ converted
  -- to underscores like titles. See the User::newFromName() for
  -- the specific tests that usernames have to pass.
  user_name varchar(255) binary NOT NULL default '',
  
  -- Optional 'real name' to be displayed in credit listings
  user_real_name varchar(255) binary NOT NULL default '',
  
  -- Password hashes, normally hashed like so:
  -- MD5(CONCAT(user_id,'-',MD5(plaintext_password))), see
  -- wfEncryptPassword() in GlobalFunctions.php
  user_password tinyblob NOT NULL,
  
  -- When using 'mail me a new password', a random
  -- password is generated and the hash stored here.
  -- The previous password is left in place until
  -- someone actually logs in with the new password,
  -- at which point the hash is moved to user_password
  -- and the old password is invalidated.
  user_newpassword tinyblob NOT NULL,
  
  -- Timestamp of the last time when a new password was
  -- sent, for throttling purposes
  user_newpass_time binary(14),

  -- Note: email should be restricted, not public info.
  -- Same with passwords.
  user_email tinytext NOT NULL,
  
  -- Newline-separated list of name=value defining the user
  -- preferences
  user_options blob NOT NULL,
  
  -- This is a timestamp which is updated when a user
  -- logs in, logs out, changes preferences, or performs
  -- some other action requiring HTML cache invalidation
  -- to ensure that the UI is updated.
  user_touched binary(14) NOT NULL default '',
  
  -- A pseudorandomly generated value that is stored in
  -- a cookie when the "remember password" feature is
  -- used (previously, a hash of the password was used, but
  -- this was vulnerable to cookie-stealing attacks)
  user_token binary(32) NOT NULL default '',
  
  -- Initially NULL; when a user's e-mail address has been
  -- validated by returning with a mailed token, this is
  -- set to the current timestamp.
  user_email_authenticated binary(14),
  
  -- Randomly generated token created when the e-mail address
  -- is set and a confirmation test mail sent.
  user_email_token binary(32),
  
  -- Expiration date for the user_email_token
  user_email_token_expires binary(14),
  
  -- Timestamp of account registration.
  -- Accounts predating this schema addition may contain NULL.
  user_registration binary(14),
  
  -- Count of edits and edit-like actions.
  --
  -- *NOT* intended to be an accurate copy of COUNT(*) WHERE rev_user=user_id
  -- May contain NULL for old accounts if batch-update scripts haven't been
  -- run, as well as listing deleted edits and other myriad ways it could be
  -- out of sync.
  --
  -- Meant primarily for heuristic checks to give an impression of whether
  -- the account has been used much.
  --
  user_editcount int,

  PRIMARY KEY user_id (user_id),
  UNIQUE INDEX user_name (user_name),
  INDEX (user_email_token)

) /*$wgDBTableOptions*/;

--
-- User permissions have been broken out to a separate table;
-- this allows sites with a shared user table to have different
-- permissions assigned to a user in each project.
--
-- This table replaces the old user_rights field which used a
-- comma-separated blob.
--
CREATE TABLE /*$wgDBprefix*/user_groups (
  -- Key to user_id
  ug_user int unsigned NOT NULL default '0',
  
  -- Group names are short symbolic string keys.
  -- The set of group names is open-ended, though in practice
  -- only some predefined ones are likely to be used.
  --
  -- At runtime $wgGroupPermissions will associate group keys
  -- with particular permissions. A user will have the combined
  -- permissions of any group they're explicitly in, plus
  -- the implicit '*' and 'user' groups.
  ug_group varbinary(16) NOT NULL default '',
  
  PRIMARY KEY (ug_user,ug_group),
  KEY (ug_group)
) /*$wgDBTableOptions*/;

-- Stores notifications of user talk page changes, for the display
-- of the "you have new messages" box
CREATE TABLE /*$wgDBprefix*/user_newtalk (
  -- Key to user.user_id
  user_id int NOT NULL default '0',
  -- If the user is an anonymous user their IP address is stored here
  -- since the user_id of 0 is ambiguous
  user_ip varbinary(40) NOT NULL default '',
  -- The highest timestamp of revisions of the talk page viewed
  -- by this user
  user_last_timestamp binary(14) NOT NULL default '',
  INDEX user_id (user_id),
  INDEX user_ip (user_ip)

) /*$wgDBTableOptions*/;


--
-- Core of the wiki: each page has an entry here which identifies
-- it by title and contains some essential metadata.
--
CREATE TABLE /*$wgDBprefix*/page (
  -- Unique identifier number. The page_id will be preserved across
  -- edits and rename operations, but not deletions and recreations.
  page_id int unsigned NOT NULL auto_increment,
  
  -- A page name is broken into a namespace and a title.
  -- The namespace keys are UI-language-independent constants,
  -- defined in includes/Defines.php
  page_namespace int NOT NULL,
  
  -- The rest of the title, as text.
  -- Spaces are transformed into underscores in title storage.
  page_title varchar(255) binary NOT NULL,
  
  -- Comma-separated set of permission keys indicating who
  -- can move or edit the page.
  page_restrictions tinyblob NOT NULL,
  
  -- Number of times this page has been viewed.
  page_counter bigint unsigned NOT NULL default '0',
  
  -- 1 indicates the article is a redirect.
  page_is_redirect tinyint unsigned NOT NULL default '0',
  
  -- 1 indicates this is a new entry, with only one edit.
  -- Not all pages with one edit are new pages.
  page_is_new tinyint unsigned NOT NULL default '0',
  
  -- Random value between 0 and 1, used for Special:Randompage
  page_random real unsigned NOT NULL,
  
  -- This timestamp is updated whenever the page changes in
  -- a way requiring it to be re-rendered, invalidating caches.
  -- Aside from editing this includes permission changes,
  -- creation or deletion of linked pages, and alteration
  -- of contained templates.
  page_touched binary(14) NOT NULL default '',

  -- Handy key to revision.rev_id of the current revision.
  -- This may be 0 during page creation, but that shouldn't
  -- happen outside of a transaction... hopefully.
  page_latest int unsigned NOT NULL,
  
  -- Uncompressed length in bytes of the page's current source text.
  page_len int unsigned NOT NULL,

  PRIMARY KEY page_id (page_id),
  UNIQUE INDEX name_title (page_namespace,page_title),
  
  -- Special-purpose indexes
  INDEX (page_random),
  INDEX (page_len)

) /*$wgDBTableOptions*/;

--
-- Every edit of a page creates also a revision row.
-- This stores metadata about the revision, and a reference
-- to the text storage backend.
--
CREATE TABLE /*$wgDBprefix*/revision (
  rev_id int unsigned NOT NULL auto_increment,
  
  -- Key to page_id. This should _never_ be invalid.
  rev_page int unsigned NOT NULL,
  
  -- Key to text.old_id, where the actual bulk text is stored.
  -- It's possible for multiple revisions to use the same text,
  -- for instance revisions where only metadata is altered
  -- or a rollback to a previous version.
  rev_text_id int unsigned NOT NULL,
  
  -- Text comment summarizing the change.
  -- This text is shown in the history and other changes lists,
  -- rendered in a subset of wiki markup by Linker::formatComment()
  rev_comment tinyblob NOT NULL,
  
  -- Key to user.user_id of the user who made this edit.
  -- Stores 0 for anonymous edits and for some mass imports.
  rev_user int unsigned NOT NULL default '0',
  
  -- Text username or IP address of the editor.
  rev_user_text varchar(255) binary NOT NULL default '',
  
  -- Timestamp
  rev_timestamp binary(14) NOT NULL default '',
  
  -- Records whether the user marked the 'minor edit' checkbox.
  -- Many automated edits are marked as minor.
  rev_minor_edit tinyint unsigned NOT NULL default '0',
  
  -- Not yet used; reserved for future changes to the deletion system.
  rev_deleted tinyint unsigned NOT NULL default '0',
  
  -- Length of this revision in bytes
  rev_len int unsigned,

  -- Key to revision.rev_id
  -- This field is used to add support for a tree structure (The Adjacency List Model)
  rev_parent_id int unsigned default NULL,

  PRIMARY KEY rev_page_id (rev_page, rev_id),
  UNIQUE INDEX rev_id (rev_id),
  INDEX rev_timestamp (rev_timestamp),
  INDEX page_timestamp (rev_page,rev_timestamp),
  INDEX user_timestamp (rev_user,rev_timestamp),
  INDEX usertext_timestamp (rev_user_text,rev_timestamp)

) /*$wgDBTableOptions*/ MAX_ROWS=10000000 AVG_ROW_LENGTH=1024;
-- In case tables are created as MyISAM, use row hints for MySQL <5.0 to avoid 4GB limit

--
-- Holds text of individual page revisions.
--
-- Field names are a holdover from the 'old' revisions table in
-- MediaWiki 1.4 and earlier: an upgrade will transform that
-- table into the 'text' table to minimize unnecessary churning
-- and downtime. If upgrading, the other fields will be left unused.
--
CREATE TABLE /*$wgDBprefix*/text (
  -- Unique text storage key number.
  -- Note that the 'oldid' parameter used in URLs does *not*
  -- refer to this number anymore, but to rev_id.
  --
  -- revision.rev_text_id is a key to this column
  old_id int unsigned NOT NULL auto_increment,
  
  -- Depending on the contents of the old_flags field, the text
  -- may be convenient plain text, or it may be funkily encoded.
  old_text mediumblob NOT NULL,
  
  -- Comma-separated list of flags:
  -- gzip: text is compressed with PHP's gzdeflate() function.
  -- utf8: text was stored as UTF-8.
  --       If $wgLegacyEncoding option is on, rows *without* this flag
  --       will be converted to UTF-8 transparently at load time.
  -- object: text field contained a serialized PHP object.
  --         The object either contains multiple versions compressed
  --         together to achieve a better compression ratio, or it refers
  --         to another row where the text can be found.
  old_flags tinyblob NOT NULL,
  
  PRIMARY KEY old_id (old_id)

) /*$wgDBTableOptions*/ MAX_ROWS=10000000 AVG_ROW_LENGTH=10240;
-- In case tables are created as MyISAM, use row hints for MySQL <5.0 to avoid 4GB limit

--
-- Holding area for deleted articles, which may be viewed
-- or restored by admins through the Special:Undelete interface.
-- The fields generally correspond to the page, revision, and text
-- fields, with several caveats.
--
CREATE TABLE /*$wgDBprefix*/archive (
  ar_namespace int NOT NULL default '0',
  ar_title varchar(255) binary NOT NULL default '',
  
  -- Newly deleted pages will not store text in this table,
  -- but will reference the separately existing text rows.
  -- This field is retained for backwards compatibility,
  -- so old archived pages will remain accessible after
  -- upgrading from 1.4 to 1.5.
  -- Text may be gzipped or otherwise funky.
  ar_text mediumblob NOT NULL,
  
  -- Basic revision stuff...
  ar_comment tinyblob NOT NULL,
  ar_user int unsigned NOT NULL default '0',
  ar_user_text varchar(255) binary NOT NULL,
  ar_timestamp binary(14) NOT NULL default '',
  ar_minor_edit tinyint NOT NULL default '0',
  
  -- See ar_text note.
  ar_flags tinyblob NOT NULL,
  
  -- When revisions are deleted, their unique rev_id is stored
  -- here so it can be retained after undeletion. This is necessary
  -- to retain permalinks to given revisions after accidental delete
  -- cycles or messy operations like history merges.
  -- 
  -- Old entries from 1.4 will be NULL here, and a new rev_id will
  -- be created on undeletion for those revisions.
  ar_rev_id int unsigned,
  
  -- For newly deleted revisions, this is the text.old_id key to the
  -- actual stored text. To avoid breaking the block-compression scheme
  -- and otherwise making storage changes harder, the actual text is
  -- *not* deleted from the text table, merely hidden by removal of the
  -- page and revision entries.
  --
  -- Old entries deleted under 1.2-1.4 will have NULL here, and their
  -- ar_text and ar_flags fields will be used to create a new text
  -- row upon undeletion.
  ar_text_id int unsigned,

  -- rev_deleted for archives
  ar_deleted tinyint unsigned NOT NULL default '0',

  -- Length of this revision in bytes
  ar_len int unsigned,

  -- Reference to page_id. Useful for sysadmin fixing of large pages 
  -- merged together in the archives, or for cleanly restoring a page
  -- at its original ID number if possible.
  --
  -- Will be NULL for pages deleted prior to 1.11.
  ar_page_id int unsigned,
  
  -- Original previous revision
  ar_parent_id int unsigned default NULL,
  
  KEY name_title_timestamp (ar_namespace,ar_title,ar_timestamp),
  KEY usertext_timestamp (ar_user_text,ar_timestamp)

) /*$wgDBTableOptions*/;


--
-- Track page-to-page hyperlinks within the wiki.
--
CREATE TABLE /*$wgDBprefix*/pagelinks (
  -- Key to the page_id of the page containing the link.
  pl_from int unsigned NOT NULL default '0',
  
  -- Key to page_namespace/page_title of the target page.
  -- The target page may or may not exist, and due to renames
  -- and deletions may refer to different page records as time
  -- goes by.
  pl_namespace int NOT NULL default '0',
  pl_title varchar(255) binary NOT NULL default '',
  
  UNIQUE KEY pl_from (pl_from,pl_namespace,pl_title),
  KEY (pl_namespace,pl_title,pl_from)

) /*$wgDBTableOptions*/;


--
-- Track template inclusions.
--
CREATE TABLE /*$wgDBprefix*/templatelinks (
  -- Key to the page_id of the page containing the link.
  tl_from int unsigned NOT NULL default '0',
  
  -- Key to page_namespace/page_title of the target page.
  -- The target page may or may not exist, and due to renames
  -- and deletions may refer to different page records as time
  -- goes by.
  tl_namespace int NOT NULL default '0',
  tl_title varchar(255) binary NOT NULL default '',
  
  UNIQUE KEY tl_from (tl_from,tl_namespace,tl_title),
  KEY (tl_namespace,tl_title,tl_from)

) /*$wgDBTableOptions*/;

--
-- Track links to images *used inline*
-- We don't distinguish live from broken links here, so
-- they do not need to be changed on upload/removal.
--
CREATE TABLE /*$wgDBprefix*/imagelinks (
  -- Key to page_id of the page containing the image / media link.
  il_from int unsigned NOT NULL default '0',
  
  -- Filename of target image.
  -- This is also the page_title of the file's description page;
  -- all such pages are in namespace 6 (NS_IMAGE).
  il_to varchar(255) binary NOT NULL default '',
  
  UNIQUE KEY il_from (il_from,il_to),
  KEY (il_to,il_from)

) /*$wgDBTableOptions*/;

--
-- Track category inclusions *used inline*
-- This tracks a single level of category membership
-- (folksonomic tagging, really).
--
CREATE TABLE /*$wgDBprefix*/categorylinks (
  -- Key to page_id of the page defined as a category member.
  cl_from int unsigned NOT NULL default '0',
  
  -- Name of the category.
  -- This is also the page_title of the category's description page;
  -- all such pages are in namespace 14 (NS_CATEGORY).
  cl_to varchar(255) binary NOT NULL default '',
  
  -- The title of the linking page, or an optional override
  -- to determine sort order. Sorting is by binary order, which
  -- isn't always ideal, but collations seem to be an exciting
  -- and dangerous new world in MySQL... The sortkey is updated
  -- if no override exists and cl_from is renamed.
  --
  -- Truncate so that the cl_sortkey key fits in 1000 bytes 
  -- (MyISAM 5 with server_character_set=utf8)
  cl_sortkey varchar(70) binary NOT NULL default '',
  
  -- This isn't really used at present. Provided for an optional
  -- sorting method by approximate addition time.
  cl_timestamp timestamp NOT NULL,
  
  UNIQUE KEY cl_from (cl_from,cl_to),
  
  -- We always sort within a given category...
  KEY cl_sortkey (cl_to,cl_sortkey,cl_from),
  
  -- Not really used?
  KEY cl_timestamp (cl_to,cl_timestamp)

) /*$wgDBTableOptions*/;

-- 
-- Track all existing categories.  Something is a category if 1) it has an en-
-- try somewhere in categorylinks, or 2) it once did.  Categories might not
-- have corresponding pages, so they need to be tracked separately.
--
CREATE TABLE /*$wgDBprefix*/category (
  -- Primary key
  cat_id int unsigned NOT NULL auto_increment,

  -- Name of the category, in the same form as page_title (with underscores).
  -- If there is a category page corresponding to this category, by definition,
  -- it has this name (in the Category namespace).
  cat_title varchar(255) binary NOT NULL,

  -- The numbers of member pages (including categories and media), subcatego-
  -- ries, and Image: namespace members, respectively.  These are signed to
  -- make underflow more obvious.  We make the first number include the second
  -- two for better sorting: subtracting for display is easy, adding for order-
  -- ing is not.
  cat_pages int signed NOT NULL default 0,
  cat_subcats int signed NOT NULL default 0,
  cat_files int signed NOT NULL default 0,

  -- Reserved for future use
  cat_hidden tinyint unsigned NOT NULL default 0,
  
  PRIMARY KEY (cat_id),
  UNIQUE KEY (cat_title),

  -- For Special:Mostlinkedcategories
  KEY (cat_pages)
) /*$wgDBTableOptions*/;

--
-- Track links to external URLs
--
CREATE TABLE /*$wgDBprefix*/externallinks (
  -- page_id of the referring page
  el_from int unsigned NOT NULL default '0',

  -- The URL
  el_to blob NOT NULL,

  -- In the case of HTTP URLs, this is the URL with any username or password
  -- removed, and with the labels in the hostname reversed and converted to 
  -- lower case. An extra dot is added to allow for matching of either
  -- example.com or *.example.com in a single scan.
  -- Example: 
  --      http://user:password@sub.example.com/page.html
  --   becomes
  --      http://com.example.sub./page.html
  -- which allows for fast searching for all pages under example.com with the
  -- clause: 
  --      WHERE el_index LIKE 'http://com.example.%'
  el_index blob NOT NULL,
  
  KEY (el_from, el_to(40)),
  KEY (el_to(60), el_from),
  KEY (el_index(60))
) /*$wgDBTableOptions*/;

-- 
-- Track interlanguage links
--
CREATE TABLE /*$wgDBprefix*/langlinks (
  -- page_id of the referring page
  ll_from int unsigned NOT NULL default '0',
  
  -- Language code of the target
  ll_lang varbinary(20) NOT NULL default '',

  -- Title of the target, including namespace
  ll_title varchar(255) binary NOT NULL default '',

  UNIQUE KEY (ll_from, ll_lang),
  KEY (ll_lang, ll_title)
) /*$wgDBTableOptions*/;

--
-- Contains a single row with some aggregate info
-- on the state of the site.
--
CREATE TABLE /*$wgDBprefix*/site_stats (
  -- The single row should contain 1 here.
  ss_row_id int unsigned NOT NULL,
  
  -- Total number of page views, if hit counters are enabled.
  ss_total_views bigint unsigned default '0',
  
  -- Total number of edits performed.
  ss_total_edits bigint unsigned default '0',
  
  -- An approximate count of pages matching the following criteria:
  -- * in namespace 0
  -- * not a redirect
  -- * contains the text '[['
  -- See Article::isCountable() in includes/Article.php
  ss_good_articles bigint unsigned default '0',
  
  -- Total pages, theoretically equal to SELECT COUNT(*) FROM page; except faster
  ss_total_pages bigint default '-1',

  -- Number of users, theoretically equal to SELECT COUNT(*) FROM user;
  ss_users bigint default '-1',

  -- Deprecated, no longer updated as of 1.5
  ss_admins int default '-1',

  -- Number of images, equivalent to SELECT COUNT(*) FROM image
  ss_images int default '0',

  UNIQUE KEY ss_row_id (ss_row_id)

) /*$wgDBTableOptions*/;

--
-- Stores an ID for every time any article is visited;
-- depending on $wgHitcounterUpdateFreq, it is
-- periodically cleared and the page_counter column
-- in the page table updated for the all articles
-- that have been visited.)
--
CREATE TABLE /*$wgDBprefix*/hitcounter (
  hc_id int unsigned NOT NULL
) ENGINE=HEAP MAX_ROWS=25000;


--
-- The internet is full of jerks, alas. Sometimes it's handy
-- to block a vandal or troll account.
--
CREATE TABLE /*$wgDBprefix*/ipblocks (
  -- Primary key, introduced for privacy.
  ipb_id int NOT NULL auto_increment,
  
  -- Blocked IP address in dotted-quad form or user name.
  ipb_address tinyblob NOT NULL,
  
  -- Blocked user ID or 0 for IP blocks.
  ipb_user int unsigned NOT NULL default '0',
  
  -- User ID who made the block.
  ipb_by int unsigned NOT NULL default '0',
  
  -- User name of blocker
  ipb_by_text varchar(255) binary NOT NULL default '',
  
  -- Text comment made by blocker.
  ipb_reason tinyblob NOT NULL,
  
  -- Creation (or refresh) date in standard YMDHMS form.
  -- IP blocks expire automatically.
  ipb_timestamp binary(14) NOT NULL default '',
  
  -- Indicates that the IP address was banned because a banned
  -- user accessed a page through it. If this is 1, ipb_address
  -- will be hidden, and the block identified by block ID number.
  ipb_auto bool NOT NULL default 0,

  -- If set to 1, block applies only to logged-out users
  ipb_anon_only bool NOT NULL default 0,

  -- Block prevents account creation from matching IP addresses
  ipb_create_account bool NOT NULL default 1,

  -- Block triggers autoblocks
  ipb_enable_autoblock bool NOT NULL default '1',
  
  -- Time at which the block will expire.
  -- May be "infinity"
  ipb_expiry varbinary(14) NOT NULL default '',
  
  -- Start and end of an address range, in hexadecimal
  -- Size chosen to allow IPv6
  ipb_range_start tinyblob NOT NULL,
  ipb_range_end tinyblob NOT NULL,

  -- Flag for entries hidden from users and Sysops
  ipb_deleted bool NOT NULL default 0,

  -- Block prevents user from accessing Special:Emailuser
  ipb_block_email bool NOT NULL default 0,
  
  PRIMARY KEY ipb_id (ipb_id),

  -- Unique index to support "user already blocked" messages
  -- Any new options which prevent collisions should be included
  UNIQUE INDEX ipb_address (ipb_address(255), ipb_user, ipb_auto, ipb_anon_only),

  INDEX ipb_user (ipb_user),
  INDEX ipb_range (ipb_range_start(8), ipb_range_end(8)),
  INDEX ipb_timestamp (ipb_timestamp),
  INDEX ipb_expiry (ipb_expiry)

) /*$wgDBTableOptions*/;


--
-- Uploaded images and other files.
--
CREATE TABLE /*$wgDBprefix*/image (
  -- Filename.
  -- This is also the title of the associated description page,
  -- which will be in namespace 6 (NS_IMAGE).
  img_name varchar(255) binary NOT NULL default '',
  
  -- File size in bytes.
  img_size int unsigned NOT NULL default '0',
  
  -- For images, size in pixels.
  img_width int NOT NULL default '0',
  img_height int NOT NULL default '0',
  
  -- Extracted EXIF metadata stored as a serialized PHP array.
  img_metadata mediumblob NOT NULL,
  
  -- For images, bits per pixel if known.
  img_bits int NOT NULL default '0',
  
  -- Media type as defined by the MEDIATYPE_xxx constants
  img_media_type ENUM("UNKNOWN", "BITMAP", "DRAWING", "AUDIO", "VIDEO", "MULTIMEDIA", "OFFICE", "TEXT", "EXECUTABLE", "ARCHIVE") default NULL,
  
  -- major part of a MIME media type as defined by IANA
  -- see http://www.iana.org/assignments/media-types/
  img_major_mime ENUM("unknown", "application", "audio", "image", "text", "video", "message", "model", "multipart") NOT NULL default "unknown",
  
  -- minor part of a MIME media type as defined by IANA
  -- the minor parts are not required to adher to any standard
  -- but should be consistent throughout the database
  -- see http://www.iana.org/assignments/media-types/
  img_minor_mime varbinary(32) NOT NULL default "unknown",
  
  -- Description field as entered by the uploader.
  -- This is displayed in image upload history and logs.
  img_description tinyblob NOT NULL,
  
  -- user_id and user_name of uploader.
  img_user int unsigned NOT NULL default '0',
  img_user_text varchar(255) binary NOT NULL,
  
  -- Time of the upload.
  img_timestamp varbinary(14) NOT NULL default '',
  
  -- SHA-1 content hash in base-36
  img_sha1 varbinary(32) NOT NULL default '',

  PRIMARY KEY img_name (img_name),
  
  INDEX img_usertext_timestamp (img_user_text,img_timestamp),
  -- Used by Special:Imagelist for sort-by-size
  INDEX img_size (img_size),
  -- Used by Special:Newimages and Special:Imagelist
  INDEX img_timestamp (img_timestamp),
  -- Used in API and duplicate search
  INDEX img_sha1 (img_sha1)


) /*$wgDBTableOptions*/;

--
-- Previous revisions of uploaded files.
-- Awkwardly, image rows have to be moved into
-- this table at re-upload time.
--
CREATE TABLE /*$wgDBprefix*/oldimage (
  -- Base filename: key to image.img_name
  oi_name varchar(255) binary NOT NULL default '',
  
  -- Filename of the archived file.
  -- This is generally a timestamp and '!' prepended to the base name.
  oi_archive_name varchar(255) binary NOT NULL default '',
  
  -- Other fields as in image...
  oi_size int unsigned NOT NULL default 0,
  oi_width int NOT NULL default 0,
  oi_height int NOT NULL default 0,
  oi_bits int NOT NULL default 0,
  oi_description tinyblob NOT NULL,
  oi_user int unsigned NOT NULL default '0',
  oi_user_text varchar(255) binary NOT NULL,
  oi_timestamp binary(14) NOT NULL default '',

  oi_metadata mediumblob NOT NULL,
  oi_media_type ENUM("UNKNOWN", "BITMAP", "DRAWING", "AUDIO", "VIDEO", "MULTIMEDIA", "OFFICE", "TEXT", "EXECUTABLE", "ARCHIVE") default NULL,
  oi_major_mime ENUM("unknown", "application", "audio", "image", "text", "video", "message", "model", "multipart") NOT NULL default "unknown",
  oi_minor_mime varbinary(32) NOT NULL default "unknown",
  oi_deleted tinyint unsigned NOT NULL default '0',
  oi_sha1 varbinary(32) NOT NULL default '',
  
  INDEX oi_usertext_timestamp (oi_user_text,oi_timestamp),
  INDEX oi_name_timestamp (oi_name,oi_timestamp),
  -- oi_archive_name truncated to 14 to avoid key length overflow
  INDEX oi_name_archive_name (oi_name,oi_archive_name(14)),
  INDEX oi_sha1 (oi_sha1)

) /*$wgDBTableOptions*/;

--
-- Record of deleted file data
--
CREATE TABLE /*$wgDBprefix*/filearchive (
  -- Unique row id
  fa_id int NOT NULL auto_increment,
  
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
  fa_width int default '0',
  fa_height int default '0',
  fa_metadata mediumblob,
  fa_bits int default '0',
  fa_media_type ENUM("UNKNOWN", "BITMAP", "DRAWING", "AUDIO", "VIDEO", "MULTIMEDIA", "OFFICE", "TEXT", "EXECUTABLE", "ARCHIVE") default NULL,
  fa_major_mime ENUM("unknown", "application", "audio", "image", "text", "video", "message", "model", "multipart") default "unknown",
  fa_minor_mime varbinary(32) default "unknown",
  fa_description tinyblob,
  fa_user int unsigned default '0',
  fa_user_text varchar(255) binary,
  fa_timestamp binary(14) default '',

  -- Visibility of deleted revisions, bitfield
  fa_deleted tinyint unsigned NOT NULL default '0',
  
  PRIMARY KEY (fa_id),
  INDEX (fa_name, fa_timestamp),             -- pick out by image name
  INDEX (fa_storage_group, fa_storage_key),  -- pick out dupe files
  INDEX (fa_deleted_timestamp),              -- sort by deletion time
  INDEX fa_user_timestamp (fa_user_text,fa_timestamp) -- sort by uploader

) /*$wgDBTableOptions*/;

--
-- Primarily a summary table for Special:Recentchanges,
-- this table contains some additional info on edits from
-- the last few days, see Article::editUpdates()
--
CREATE TABLE /*$wgDBprefix*/recentchanges (
  rc_id int NOT NULL auto_increment,
  rc_timestamp varbinary(14) NOT NULL default '',
  rc_cur_time varbinary(14) NOT NULL default '',
  
  -- As in revision
  rc_user int unsigned NOT NULL default '0',
  rc_user_text varchar(255) binary NOT NULL,
  
  -- When pages are renamed, their RC entries do _not_ change.
  rc_namespace int NOT NULL default '0',
  rc_title varchar(255) binary NOT NULL default '',
  
  -- as in revision...
  rc_comment varchar(255) binary NOT NULL default '',
  rc_minor tinyint unsigned NOT NULL default '0',
  
  -- Edits by user accounts with the 'bot' rights key are
  -- marked with a 1 here, and will be hidden from the
  -- default view.
  rc_bot tinyint unsigned NOT NULL default '0',
  
  rc_new tinyint unsigned NOT NULL default '0',
  
  -- Key to page_id (was cur_id prior to 1.5).
  -- This will keep links working after moves while
  -- retaining the at-the-time name in the changes list.
  rc_cur_id int unsigned NOT NULL default '0',
  
  -- rev_id of the given revision
  rc_this_oldid int unsigned NOT NULL default '0',
  
  -- rev_id of the prior revision, for generating diff links.
  rc_last_oldid int unsigned NOT NULL default '0',
  
  -- These may no longer be used, with the new move log.
  rc_type tinyint unsigned NOT NULL default '0',
  rc_moved_to_ns tinyint unsigned NOT NULL default '0',
  rc_moved_to_title varchar(255) binary NOT NULL default '',
  
  -- If the Recent Changes Patrol option is enabled,
  -- users may mark edits as having been reviewed to
  -- remove a warning flag on the RC list.
  -- A value of 1 indicates the page has been reviewed.
  rc_patrolled tinyint unsigned NOT NULL default '0',
  
  -- Recorded IP address the edit was made from, if the
  -- $wgPutIPinRC option is enabled.
  rc_ip varbinary(40) NOT NULL default '',
  
  -- Text length in characters before
  -- and after the edit
  rc_old_len int,
  rc_new_len int,

  -- Visibility of deleted revisions, bitfield
  rc_deleted tinyint unsigned NOT NULL default '0',

  -- Value corresonding to log_id, specific log entries
  rc_logid int unsigned NOT NULL default '0',
  -- Store log type info here, or null
  rc_log_type varbinary(255) NULL default NULL,
  -- Store log action or null
  rc_log_action varbinary(255) NULL default NULL,
  -- Log params
  rc_params blob NULL,
  
  PRIMARY KEY rc_id (rc_id),
  INDEX rc_timestamp (rc_timestamp),
  INDEX rc_namespace_title (rc_namespace, rc_title),
  INDEX rc_cur_id (rc_cur_id),
  INDEX new_name_timestamp (rc_new,rc_namespace,rc_timestamp),
  INDEX rc_ip (rc_ip),
  INDEX rc_ns_usertext (rc_namespace, rc_user_text),
  INDEX rc_user_text (rc_user_text, rc_timestamp)

) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/watchlist (
  -- Key to user.user_id
  wl_user int unsigned NOT NULL,
  
  -- Key to page_namespace/page_title
  -- Note that users may watch pages which do not exist yet,
  -- or existed in the past but have been deleted.
  wl_namespace int NOT NULL default '0',
  wl_title varchar(255) binary NOT NULL default '',
  
  -- Timestamp when user was last sent a notification e-mail;
  -- cleared when the user visits the page.
  wl_notificationtimestamp varbinary(14),
  
  UNIQUE KEY (wl_user, wl_namespace, wl_title),
  KEY namespace_title (wl_namespace, wl_title)

) /*$wgDBTableOptions*/;


--
-- Used by the math module to keep track
-- of previously-rendered items.
--
CREATE TABLE /*$wgDBprefix*/math (
  -- Binary MD5 hash of the latex fragment, used as an identifier key.
  math_inputhash varbinary(16) NOT NULL,
  
  -- Not sure what this is, exactly...
  math_outputhash varbinary(16) NOT NULL,
  
  -- texvc reports how well it thinks the HTML conversion worked;
  -- if it's a low level the PNG rendering may be preferred.
  math_html_conservativeness tinyint NOT NULL,
  
  -- HTML output from texvc, if any
  math_html text,
  
  -- MathML output from texvc, if any
  math_mathml text,
  
  UNIQUE KEY math_inputhash (math_inputhash)

) /*$wgDBTableOptions*/;

--
-- When using the default MySQL search backend, page titles
-- and text are munged to strip markup, do Unicode case folding,
-- and prepare the result for MySQL's fulltext index.
--
-- This table must be MyISAM; InnoDB does not support the needed
-- fulltext index.
--
CREATE TABLE /*$wgDBprefix*/searchindex (
  -- Key to page_id
  si_page int unsigned NOT NULL,
  
  -- Munged version of title
  si_title varchar(255) NOT NULL default '',
  
  -- Munged version of body text
  si_text mediumtext NOT NULL,
  
  UNIQUE KEY (si_page),
  FULLTEXT si_title (si_title),
  FULLTEXT si_text (si_text)

) ENGINE=MyISAM;

--
-- Recognized interwiki link prefixes
--
CREATE TABLE /*$wgDBprefix*/interwiki (
  -- The interwiki prefix, (e.g. "Meatball", or the language prefix "de")
  iw_prefix varchar(32) NOT NULL,
  
  -- The URL of the wiki, with "$1" as a placeholder for an article name.
  -- Any spaces in the name will be transformed to underscores before
  -- insertion.
  iw_url blob NOT NULL,
  
  -- A boolean value indicating whether the wiki is in this project
  -- (used, for example, to detect redirect loops)
  iw_local bool NOT NULL,
  
  -- Boolean value indicating whether interwiki transclusions are allowed.
  iw_trans tinyint NOT NULL default 0,
  
  UNIQUE KEY iw_prefix (iw_prefix)

) /*$wgDBTableOptions*/;

--
-- Used for caching expensive grouped queries
--
CREATE TABLE /*$wgDBprefix*/querycache (
  -- A key name, generally the base name of of the special page.
  qc_type varbinary(32) NOT NULL,
  
  -- Some sort of stored value. Sizes, counts...
  qc_value int unsigned NOT NULL default '0',
  
  -- Target namespace+title
  qc_namespace int NOT NULL default '0',
  qc_title varchar(255) binary NOT NULL default '',
  
  KEY (qc_type,qc_value)

) /*$wgDBTableOptions*/;

--
-- For a few generic cache operations if not using Memcached
--
CREATE TABLE /*$wgDBprefix*/objectcache (
  keyname varbinary(255) NOT NULL default '',
  value mediumblob,
  exptime datetime,
  UNIQUE KEY (keyname),
  KEY (exptime)

) /*$wgDBTableOptions*/;

--
-- Cache of interwiki transclusion
--
CREATE TABLE /*$wgDBprefix*/transcache (
  tc_url varbinary(255) NOT NULL,
  tc_contents text,
  tc_time int NOT NULL,
  UNIQUE INDEX tc_url_idx (tc_url)
) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/logging (
  -- Log ID, for referring to this specific log entry, probably for deletion and such.
  log_id int unsigned NOT NULL auto_increment,

  -- Symbolic keys for the general log type and the action type
  -- within the log. The output format will be controlled by the
  -- action field, but only the type controls categorization.
  log_type varbinary(10) NOT NULL default '',
  log_action varbinary(10) NOT NULL default '',
  
  -- Timestamp. Duh.
  log_timestamp binary(14) NOT NULL default '19700101000000',
  
  -- The user who performed this action; key to user_id
  log_user int unsigned NOT NULL default 0,
  
  -- Key to the page affected. Where a user is the target,
  -- this will point to the user page.
  log_namespace int NOT NULL default 0,
  log_title varchar(255) binary NOT NULL default '',
  
  -- Freeform text. Interpreted as edit history comments.
  log_comment varchar(255) NOT NULL default '',
  
  -- LF separated list of miscellaneous parameters
  log_params blob NOT NULL,

  -- rev_deleted for logs
  log_deleted tinyint unsigned NOT NULL default '0',

  PRIMARY KEY log_id (log_id),
  KEY type_time (log_type, log_timestamp),
  KEY user_time (log_user, log_timestamp),
  KEY page_time (log_namespace, log_title, log_timestamp),
  KEY times (log_timestamp)

) /*$wgDBTableOptions*/;

CREATE TABLE /*$wgDBprefix*/trackbacks (
  tb_id int auto_increment,
  tb_page int REFERENCES /*$wgDBprefix*/page(page_id) ON DELETE CASCADE,
  tb_title varchar(255) NOT NULL,
  tb_url blob NOT NULL,
  tb_ex text,
  tb_name varchar(255),

  PRIMARY KEY (tb_id),
  INDEX (tb_page)
) /*$wgDBTableOptions*/;


-- Jobs performed by parallel apache threads or a command-line daemon
CREATE TABLE /*$wgDBprefix*/job (
  job_id int unsigned NOT NULL auto_increment,
  
  -- Command name
  -- Limited to 60 to prevent key length overflow
  job_cmd varbinary(60) NOT NULL default '',

  -- Namespace and title to act on
  -- Should be 0 and '' if the command does not operate on a title
  job_namespace int NOT NULL,
  job_title varchar(255) binary NOT NULL,

  -- Any other parameters to the command
  -- Presently unused, format undefined
  job_params blob NOT NULL,

  PRIMARY KEY job_id (job_id),
  KEY (job_cmd, job_namespace, job_title)
) /*$wgDBTableOptions*/;


-- Details of updates to cached special pages
CREATE TABLE /*$wgDBprefix*/querycache_info (

  -- Special page name
  -- Corresponds to a qc_type value
  qci_type varbinary(32) NOT NULL default '',

  -- Timestamp of last update
  qci_timestamp binary(14) NOT NULL default '19700101000000',

  UNIQUE KEY ( qci_type )

) /*$wgDBTableOptions*/;

-- For each redirect, this table contains exactly one row defining its target
CREATE TABLE /*$wgDBprefix*/redirect (
  -- Key to the page_id of the redirect page
  rd_from int unsigned NOT NULL default '0',

  -- Key to page_namespace/page_title of the target page.
  -- The target page may or may not exist, and due to renames
  -- and deletions may refer to different page records as time
  -- goes by.
  rd_namespace int NOT NULL default '0',
  rd_title varchar(255) binary NOT NULL default '',

  PRIMARY KEY rd_from (rd_from),
  KEY rd_ns_title (rd_namespace,rd_title,rd_from)
) /*$wgDBTableOptions*/;

-- Used for caching expensive grouped queries that need two links (for example double-redirects)
CREATE TABLE /*$wgDBprefix*/querycachetwo (
  -- A key name, generally the base name of of the special page.
  qcc_type varbinary(32) NOT NULL,
  
  -- Some sort of stored value. Sizes, counts...
  qcc_value int unsigned NOT NULL default '0',
  
  -- Target namespace+title
  qcc_namespace int NOT NULL default '0',
  qcc_title varchar(255) binary NOT NULL default '',
  
  -- Target namespace+title2
  qcc_namespacetwo int NOT NULL default '0',
  qcc_titletwo varchar(255) binary NOT NULL default '',

  KEY qcc_type (qcc_type,qcc_value),
  KEY qcc_title (qcc_type,qcc_namespace,qcc_title),
  KEY qcc_titletwo (qcc_type,qcc_namespacetwo,qcc_titletwo)

) /*$wgDBTableOptions*/;

-- Used for storing page restrictions (i.e. protection levels)
CREATE TABLE /*$wgDBprefix*/page_restrictions (
  -- Page to apply restrictions to (Foreign Key to page).
  pr_page int NOT NULL,
  -- The protection type (edit, move, etc)
  pr_type varbinary(60) NOT NULL,
  -- The protection level (Sysop, autoconfirmed, etc)
  pr_level varbinary(60) NOT NULL,
  -- Whether or not to cascade the protection down to pages transcluded.
  pr_cascade tinyint NOT NULL,
  -- Field for future support of per-user restriction.
  pr_user int NULL,
  -- Field for time-limited protection.
  pr_expiry varbinary(14) NULL,
  -- Field for an ID for this restrictions row (sort-key for Special:ProtectedPages)
  pr_id int unsigned NOT NULL auto_increment,

  PRIMARY KEY pr_pagetype (pr_page,pr_type),

  UNIQUE KEY pr_id (pr_id),
  KEY pr_typelevel (pr_type,pr_level),
  KEY pr_level (pr_level),
  KEY pr_cascade (pr_cascade)
) /*$wgDBTableOptions*/;

-- Protected titles - nonexistent pages that have been protected
CREATE TABLE /*$wgDBprefix*/protected_titles (
  pt_namespace int NOT NULL,
  pt_title varchar(255) binary NOT NULL,
  pt_user int unsigned NOT NULL,
  pt_reason tinyblob,
  pt_timestamp binary(14) NOT NULL,
  pt_expiry varbinary(14) NOT NULL default '',
  pt_create_perm varbinary(60) NOT NULL,
  PRIMARY KEY (pt_namespace,pt_title),
  KEY pt_timestamp (pt_timestamp)
) /*$wgDBTableOptions*/;

-- Name/value pairs indexed by page_id
CREATE TABLE /*$wgDBprefix*/page_props (
  pp_page int NOT NULL,
  pp_propname varbinary(60) NOT NULL,
  pp_value blob NOT NULL,

  PRIMARY KEY (pp_page,pp_propname)
) /*$wgDBTableOptions*/;

-- A table to log updates, one text key row per update.
CREATE TABLE /*$wgDBprefix*/updatelog (
  ul_key varchar(255) NOT NULL,
  PRIMARY KEY (ul_key)
) /*$wgDBTableOptions*/;

-- vim: sw=2 sts=2 et
