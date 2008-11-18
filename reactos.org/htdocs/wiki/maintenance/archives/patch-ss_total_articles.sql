-- Faster statistics, as of 1.4.3

ALTER TABLE /*$wgDBprefix*/site_stats
  ADD ss_total_pages bigint default -1,
  ADD ss_users bigint default -1,
  ADD ss_admins int default -1;
