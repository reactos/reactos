--
-- patch-rc-patrol.sql
-- Adds a row to recentchanges for the patrolling feature
-- 2004-08-09
--

ALTER TABLE /*$wgDBprefix*/recentchanges
	ADD COLUMN rc_patrolled tinyint(3) unsigned NOT NULL default '0';

