-- Add an extra option field "ipb_enable_autoblock" into the ipblocks table. This allows a block to be placed that does not trigger any autoblocks.

ALTER TABLE /*$wgDBprefix*/ipblocks ADD COLUMN ipb_enable_autoblock bool NOT NULL default '1';
