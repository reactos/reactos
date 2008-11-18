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
  
  UNIQUE KEY tl_from(tl_from,tl_namespace,tl_title),
  KEY (tl_namespace,tl_title)

) /*$wgDBTableOptions*/;

