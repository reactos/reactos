-- Adding colomn with username of blocker and sets it.
-- Required for crosswiki blocks.

ALTER TABLE /*$wgDBprefix*/ipblocks
	ADD ipb_by_text varchar(255) binary NOT NULL default '';

UPDATE /*$wgDBprefix*/ipblocks 
	JOIN /*$wgDBprefix*/user ON ipb_by = user_id
	SET ipb_by_text = user_name
	WHERE ipb_by != 0;