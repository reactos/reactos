-- More statistics, for version 1.6

ALTER TABLE /*$wgDBprefix*/site_stats ADD ss_images int default '0';
SELECT @images := COUNT(*) FROM /*$wgDBprefix*/image;
UPDATE /*$wgDBprefix*/site_stats SET ss_images=@images;
