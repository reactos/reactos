-- 
-- patch-backlinkindexes.sql
-- 
-- Per bug 6440 / http://bugzilla.wikimedia.org/show_bug.cgi?id=6440
--
-- Improve performance of the "what links here"-type queries
-- 

ALTER TABLE /*$wgDBprefix*/pagelinks
   DROP INDEX pl_namespace,
   ADD INDEX pl_namespace(pl_namespace, pl_title, pl_from);

ALTER TABLE /*$wgDBprefix*/templatelinks
   DROP INDEX tl_namespace,
   ADD INDEX tl_namespace(tl_namespace, tl_title, tl_from);
   
ALTER TABLE /*$wgDBprefix*/imagelinks
   DROP INDEX il_to,
   ADD INDEX il_to(il_to, il_from);
