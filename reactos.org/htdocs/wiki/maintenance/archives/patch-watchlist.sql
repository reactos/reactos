-- Convert watchlists to new new format ;)

-- Ids just aren't convenient when what we want is to
-- treat article and talk pages as equivalent.
-- Better to use namespace (drop the 1 bit!) and title

-- 2002-12-17 by Brion Vibber <brion@pobox.com>
-- affects, affected by changes to SpecialWatchlist.php, User.php,
-- Article.php, Title.php, SpecialRecentchanges.php

DROP TABLE IF EXISTS watchlist2;
CREATE TABLE watchlist2 (
  wl_user int unsigned NOT NULL,
  wl_namespace int unsigned NOT NULL default '0',
  wl_title varchar(255) binary NOT NULL default '',
  UNIQUE KEY (wl_user, wl_namespace, wl_title)
) /*$wgDBTableOptions*/;

INSERT INTO watchlist2 (wl_user,wl_namespace,wl_title)
  SELECT DISTINCT wl_user,(cur_namespace | 1) - 1,cur_title
  FROM watchlist,cur WHERE wl_page=cur_id;

ALTER TABLE watchlist RENAME TO oldwatchlist;
ALTER TABLE watchlist2 RENAME TO watchlist;

-- Check that the new one is correct, then:
-- DROP TABLE oldwatchlist;

-- Also should probably drop the ancient and now unused:
ALTER TABLE user DROP COLUMN user_watch;
