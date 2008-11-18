--
-- linkscc table used to cache link lists in easier to digest form
-- November 2003
--
-- Format later updated.
--

CREATE TABLE /*$wgDBprefix*/linkscc (
  lcc_pageid INT UNSIGNED NOT NULL UNIQUE KEY,
  lcc_cacheobj MEDIUMBLOB NOT NULL

) /*$wgDBTableOptions*/;
