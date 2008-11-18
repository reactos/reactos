-- Creates interwiki prefix<->url mapping table
-- used from 2003-08-21 dev version.
-- Import the default mappings from maintenance/interwiki.sql

CREATE TABLE /*$wgDBprefix*/interwiki (
  -- The interwiki prefix, (e.g. "Meatball", or the language prefix "de")
  iw_prefix varchar(32) NOT NULL,
  
  -- The URL of the wiki, with "$1" as a placeholder for an article name.
  -- Any spaces in the name will be transformed to underscores before
  -- insertion.
  iw_url blob NOT NULL,
  
  -- A boolean value indicating whether the wiki is in this project
  -- (used, for example, to detect redirect loops)
  iw_local BOOL NOT NULL,
  
  UNIQUE KEY iw_prefix (iw_prefix)

) /*$wgDBTableOptions*/;
