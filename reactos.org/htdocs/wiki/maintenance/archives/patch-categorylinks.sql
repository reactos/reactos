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
  -- and dangerous new world in MySQL...
  --
  -- Truncate so that the cl_sortkey key fits in 1000 bytes 
  -- (MyISAM 5 with server_character_set=utf8)
  cl_sortkey varchar(70) binary NOT NULL default '',
  
  -- This isn't really used at present. Provided for an optional
  -- sorting method by approximate addition time.
  cl_timestamp timestamp NOT NULL,
  
  UNIQUE KEY cl_from(cl_from,cl_to),
  
  -- This key is trouble. It's incomplete, AND it's too big
  -- when collation is set to UTF-8. Bleeeacch!
  KEY cl_sortkey(cl_to,cl_sortkey),
  
  -- Not really used?
  KEY cl_timestamp(cl_to,cl_timestamp)

) /*$wgDBTableOptions*/;
