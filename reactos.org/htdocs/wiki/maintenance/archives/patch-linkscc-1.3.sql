--
-- linkscc table used to cache link lists in easier to digest form.
-- New schema for 1.3 - removes old lcc_title column.
-- May 2004
--
ALTER TABLE /*$wgDBprefix*/linkscc DROP COLUMN lcc_title;