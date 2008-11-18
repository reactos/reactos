ALTER TABLE /*$wgDBprefix*/interwiki
	ADD COLUMN iw_trans TINYINT NOT NULL DEFAULT 0;
