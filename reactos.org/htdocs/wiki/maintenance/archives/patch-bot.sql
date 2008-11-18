-- Add field to recentchanges for easy filtering of bot entries
-- edits by a user with 'bot' in user.user_rights should be
-- marked 1 in rc_bot.

-- Change made 2002-12-15 by Brion VIBBER <brion@pobox.com>
-- this affects code in Article.php, User.php SpecialRecentchanges.php
-- column also added to buildTables.inc

ALTER TABLE /*$wgDBprefix*/recentchanges
  ADD COLUMN rc_bot tinyint unsigned NOT NULL default '0'
  AFTER rc_minor;
