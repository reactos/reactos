--
-- Create the new pagelinks table to merge links and brokenlinks data,
-- and populate it.
-- 
-- Unlike the old links and brokenlinks, these records will not need to be
-- altered when target pages are created, deleted, or renamed. This should
-- reduce the amount of severe database frustration that happens when widely-
-- linked pages are altered.
--
-- Fixups for brokenlinks to pages in namespaces need to be run after this;
-- this is done by updaters.inc if run through the regular update scripts.
--
-- 2005-05-26
--

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
  
  UNIQUE KEY pl_from(pl_from,pl_namespace,pl_title),
  KEY (pl_namespace,pl_title)

) /*$wgDBTableOptions*/;


-- Import existing-page links
INSERT
  INTO /*$wgDBprefix*/pagelinks (pl_from,pl_namespace,pl_title)
  SELECT l_from,page_namespace,page_title
    FROM /*$wgDBprefix*/links, /*$wgDBprefix*/page
    WHERE l_to=page_id;

-- import brokenlinks
-- NOTE: We'll have to fix up individual entries that aren't in main NS
INSERT INTO /*$wgDBprefix*/pagelinks (pl_from,pl_namespace,pl_title)
  SELECT bl_from, 0, bl_to
  FROM /*$wgDBprefix*/brokenlinks;

-- For each namespace do something like:
--
-- UPDATE /*$wgDBprefix*/pagelinks
--   SET pl_namespace=$ns,
--       pl_title=TRIM(LEADING '$prefix:' FROM pl_title)
-- WHERE pl_namespace=0
--   AND pl_title LIKE '$likeprefix:%'";
--
