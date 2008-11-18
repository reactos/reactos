-- Moving img_exif to img_metadata, so the name won't be so confusing when we
-- Use it for Ogg metadata or something like that.

ALTER TABLE /*$wgDBprefix*/image ADD (
  img_metadata mediumblob NOT NULL
);
