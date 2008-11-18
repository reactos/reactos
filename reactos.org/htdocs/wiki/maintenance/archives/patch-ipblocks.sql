-- For auto-expiring blocks --

ALTER TABLE /*$wgDBprefix*/ipblocks
	ADD ipb_auto tinyint NOT NULL default '0',
	ADD ipb_id int NOT NULL auto_increment,
	ADD PRIMARY KEY (ipb_id);
