-- user_token patch
-- 2004-09-23

ALTER TABLE /*$wgDBprefix*/user ADD user_token  binary(32) NOT NULL default '';

UPDATE /*$wgDBprefix*/user SET user_token = concat(
	substring(rand(),3,4),
	substring(rand(),3,4),
	substring(rand(),3,4),
	substring(rand(),3,4),
	substring(rand(),3,4),
	substring(rand(),3,4),
	substring(rand(),3,4),
	substring(rand(),3,4)
);
