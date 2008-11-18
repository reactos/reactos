-- Adding index to sort by uploader
ALTER TABLE /*$wgDBprefix*/filearchive 
  ADD INDEX fa_user_timestamp (fa_user_text,fa_timestamp),
  -- Remove useless, incomplete index
  DROP INDEX fa_deleted_user;
