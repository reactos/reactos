--
-- patch-rc-newindex.sql
-- Adds an index to recentchanges to optimize Special:Newpages
-- 2004-01-25
--

ALTER TABLE /*$wgDBprefix*/recentchanges
	ADD INDEX new_name_timestamp(rc_new,rc_namespace,rc_timestamp);

