-- SQL Dump for the "peoplemap" database

CREATE TABLE `user_locations` (
  `roscms_user_id` bigint(20) UNSIGNED NOT NULL,
  `latitude` float(8,6) NOT NULL,
  `longitude` float(9,6) NOT NULL,
  PRIMARY KEY  (`roscms_user_id`)
);
