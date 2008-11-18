-- Adds a user,timestamp index to the archive table
-- Used for browsing deleted contributions and renames
ALTER TABLE /*$wgDBprefix*/archive 
	ADD INDEX usertext_timestamp ( ar_user_text , ar_timestamp );
