-- Patch for email authentication T.Gries/M.Arndt 27.11.2004
-- Table user_newtalk is dropped, as the table watchlist is now also used for storing user_talk-page notifications
DROP TABLE /*$wgDBprefix*/user_newtalk;
