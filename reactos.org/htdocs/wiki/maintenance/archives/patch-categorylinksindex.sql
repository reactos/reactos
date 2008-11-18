-- 
-- patch-categorylinksindex.sql
-- 
-- Per bug 10280 / http://bugzilla.wikimedia.org/show_bug.cgi?id=10280
--
-- Improve enum continuation performance of the what pages belong to a category query
-- 

ALTER TABLE /*$wgDBprefix*/categorylinks
   DROP INDEX cl_sortkey,
   ADD INDEX cl_sortkey(cl_to, cl_sortkey, cl_from);
