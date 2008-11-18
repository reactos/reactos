--
-- patch-rc_len.sql
-- Adds two rows to recentchanges to hold the text size befor and after the edit
-- 2006-12-03
--

ALTER TABLE /*$wgDBprefix*/recentchanges
	ADD COLUMN rc_old_len int, ADD COLUMN rc_new_len int;

