--
-- Add rev_deleted flag to revision table.
-- Deleted revisions can thus continue to be listed in history
-- and user contributions, and their text storage doesn't have
-- to be disturbed.
--
-- 2005-03-31
--

ALTER TABLE /*$wgDBprefix*/revision
  ADD rev_deleted tinyint unsigned NOT NULL default '0';
