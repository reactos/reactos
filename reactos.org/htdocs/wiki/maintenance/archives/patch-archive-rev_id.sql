-- New field in archive table to preserve revision IDs across undeletion.
-- Added 2005-03-10

ALTER TABLE /*$wgDBprefix*/archive
  ADD
    ar_rev_id int unsigned;
