-- patch-random-dateindex.sql
-- 2003-02-09
--
-- This patch does two things:
--  * Adds cur_random column to replace random table
--    (Requires change to SpecialRandom.php)
--    random table no longer needs refilling
--    Note: short-term duplicate results *are* possible, but very unlikely on large wiki
--
--  * Adds inverse_timestamp columns to cur and old and indexes
--    to allow descending timestamp sort in history, contribs, etc
--    (Requires changes to Article.php, DatabaseFunctions.php,
--     ... )
--                       cur_timestamp  inverse_timestamp
--     99999999999999 - 20030209222556 = 79969790777443
--     99999999999999 - 20030211083412 = 79969788916587
--
--    We won't need this on MySQL 4; there will be a removal patch later.

-- Indexes:
-- cur needs (cur_random) for random sort
-- cur and old need (namespace,title,timestamp) index for history,watchlist,rclinked
-- cur and old need (user,timestamp) index for contribs
-- cur and old need (user_text,timestamp) index for contribs

ALTER TABLE /*$wgDBprefix*/cur
  DROP INDEX cur_user,
  DROP INDEX cur_user_text,
  ADD COLUMN cur_random real unsigned NOT NULL,
  ADD COLUMN inverse_timestamp char(14) binary NOT NULL default '',
  ADD INDEX (cur_random),
  ADD INDEX name_title_timestamp (cur_namespace,cur_title,inverse_timestamp),
  ADD INDEX user_timestamp (cur_user,inverse_timestamp),
  ADD INDEX usertext_timestamp (cur_user_text,inverse_timestamp);

UPDATE /*$wgDBprefix*/cur SET
  inverse_timestamp=99999999999999-cur_timestamp,
  cur_random=RAND();

ALTER TABLE /*$wgDBprefix*/old
  DROP INDEX old_user,
  DROP INDEX old_user_text,
  ADD COLUMN inverse_timestamp char(14) binary NOT NULL default '',
  ADD INDEX name_title_timestamp (old_namespace,old_title,inverse_timestamp),
  ADD INDEX user_timestamp (old_user,inverse_timestamp),
  ADD INDEX usertext_timestamp (old_user_text,inverse_timestamp);

UPDATE /*$wgDBprefix*/old SET
  inverse_timestamp=99999999999999-old_timestamp;

-- If leaving wiki publicly accessible in read-only mode during
-- the upgrade, comment out the below line; leave 'random' table
-- in place until the new software is installed.
DROP TABLE /*$wgDBprefix*/random;
