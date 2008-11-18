-- Blobs table for external storage

CREATE TABLE /*$wgDBprefix*/blobs (
	blob_id integer UNSIGNED NOT NULL AUTO_INCREMENT,
	blob_text longblob,
	PRIMARY KEY  (blob_id)
) ENGINE=MyISAM MAX_ROWS=1000000 AVG_ROW_LENGTH=1000000;

