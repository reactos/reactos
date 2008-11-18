-- New field in archive table to preserve text source IDs across undeletion.
--
-- Older entries containing NULL in this field will contain text in the
-- ar_text and ar_flags fields, and will cause the (re)creation of a new
-- text record upon undeletion.
--
-- Newer ones will reference a text.old_id with this field, and the existing
-- entries will be used as-is; only a revision record need be created.
--
-- Added 2005-05-01

ALTER TABLE /*$wgDBprefix*/archive
  ADD
    ar_text_id int unsigned;
