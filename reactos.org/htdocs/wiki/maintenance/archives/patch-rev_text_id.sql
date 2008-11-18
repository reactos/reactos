--
-- Adds rev_text_id field to revision table.
-- This is a key to text.old_id, so that revisions can be stored
-- for non-save operations without duplicating text, and so that
-- a back-end storage system can provide its own numbering system
-- if necessary.
--
-- rev.rev_id and text.old_id are no longer assumed to be the same.
--
-- 2005-03-28
--

ALTER TABLE /*$wgDBprefix*/revision
  ADD rev_text_id int unsigned NOT NULL;

UPDATE /*$wgDBprefix*/revision
  SET rev_text_id=rev_id;
