-- Page length field (in bytes) for current revision of page.
-- Since page text is now stored separately, it may be compressed
-- or otherwise difficult to calculate. Additionally, the field
-- can be indexed for handy 'long' and 'short' page lists.
--
-- Added 2005-03-12

ALTER TABLE /*$wgDBprefix*/page
  ADD page_len int unsigned NOT NULL,
  ADD INDEX (page_len);

-- Not accurate if upgrading from intermediate
-- 1.5 alpha and have revision compression on.
UPDATE /*$wgDBprefix*/page, /*$wgDBprefix*/text
  SET page_len=LENGTH(old_text)
  WHERE page_latest=old_id;
