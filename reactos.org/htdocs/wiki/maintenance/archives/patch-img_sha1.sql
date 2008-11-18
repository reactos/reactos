-- Add img_sha1, oi_sha1 and related indexes
ALTER TABLE /*$wgDBprefix*/image
  ADD COLUMN img_sha1 varbinary(32) NOT NULL default '',
  ADD INDEX img_sha1 (img_sha1);

ALTER TABLE /*$wgDBprefix*/oldimage
  ADD COLUMN oi_sha1 varbinary(32) NOT NULL default '',
  ADD INDEX oi_sha1 (oi_sha1);
