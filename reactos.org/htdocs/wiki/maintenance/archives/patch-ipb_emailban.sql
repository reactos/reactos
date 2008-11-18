-- Add row for email blocks --

ALTER TABLE /*$wgDBprefix*/ipblocks
	ADD ipb_block_email tinyint NOT NULL default '0';
