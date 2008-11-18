-- Add indexes to the mime types in image for use on Special:MIMEsearch,
-- changes a query like
--
-- SELECT img_name FROM image WHERE img_major_mime = "image" AND img_minor_mime = "svg";
-- from:
-- +-------+------+---------------+------+---------+------+------+-------------+
-- | table | type | possible_keys | key  | key_len | ref  | rows | Extra       |
-- +-------+------+---------------+------+---------+------+------+-------------+
-- | image | ALL  | NULL          | NULL |    NULL | NULL |  194 | Using where |
-- +-------+------+---------------+------+---------+------+------+-------------+
-- to:
-- +-------+------+-------------------------------+----------------+---------+-------+------+-------------+
-- | table | type | possible_keys                 | key            | key_len | ref   | rows | Extra       |
-- +-------+------+-------------------------------+----------------+---------+-------+------+-------------+
-- | image | ref  | img_major_mime,img_minor_mime | img_minor_mime |      32 | const |    4 | Using where |
-- +-------+------+-------------------------------+----------------+---------+-------+------+-------------+

ALTER TABLE /*$wgDBprefix*/image
	ADD INDEX img_major_mime (img_major_mime);
ALTER TABLE /*$wgDBprefix*/image
	ADD INDEX img_minor_mime (img_minor_mime);

