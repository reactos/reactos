-- Break fulltext search index out to separate table from cur
-- This is being done mainly to allow us to use InnoDB tables
-- for the main db while keeping the MyISAM fulltext index for
-- search.

-- 2002-12-16, 2003-01-25 Brion VIBBER <brion@pobox.com>

-- Creating searchindex table...
DROP TABLE IF EXISTS /*$wgDBprefix*/searchindex;
CREATE TABLE /*$wgDBprefix*/searchindex (
  -- Key to page_id
  si_page int unsigned NOT NULL,
  
  -- Munged version of title
  si_title varchar(255) NOT NULL default '',
  
  -- Munged version of body text
  si_text mediumtext NOT NULL,
  
  UNIQUE KEY (si_page)

) ENGINE=MyISAM;

-- Copying data into new table...
INSERT INTO /*$wgDBprefix*/searchindex
  (si_page,si_title,si_text)
  SELECT
    cur_id,cur_ind_title,cur_ind_text
    FROM /*$wgDBprefix*/cur;


-- Creating fulltext index...
ALTER TABLE /*$wgDBprefix*/searchindex
  ADD FULLTEXT si_title (si_title),
  ADD FULLTEXT si_text (si_text);

-- Dropping index columns from cur table.
ALTER TABLE /*$wgDBprefix*/cur
  DROP COLUMN cur_ind_title,
  DROP COLUMN cur_ind_text;
