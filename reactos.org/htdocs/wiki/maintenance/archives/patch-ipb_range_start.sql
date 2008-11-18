-- Add the range handling fields
ALTER TABLE /*$wgDBprefix*/ipblocks 
  ADD ipb_range_start tinyblob NOT NULL default '',
  ADD ipb_range_end tinyblob NOT NULL default '',
  ADD INDEX ipb_range (ipb_range_start(8), ipb_range_end(8));


-- Initialise fields
-- Only range blocks match ipb_address LIKE '%/%', this fact is used in the code already
UPDATE /*$wgDBprefix*/ipblocks 
  SET 
    ipb_range_start = LPAD(HEX( 
        (SUBSTRING_INDEX(ipb_address, '.', 1) << 24)
      + (SUBSTRING_INDEX(SUBSTRING_INDEX(ipb_address, '.', 2), '.', -1) << 16)
      + (SUBSTRING_INDEX(SUBSTRING_INDEX(ipb_address, '.', 3), '.', -1) << 24)
      + (SUBSTRING_INDEX(SUBSTRING_INDEX(ipb_address, '/', 1), '.', -1)) ), 8, '0' ),

    ipb_range_end = LPAD(HEX( 
        (SUBSTRING_INDEX(ipb_address, '.', 1) << 24)
      + (SUBSTRING_INDEX(SUBSTRING_INDEX(ipb_address, '.', 2), '.', -1) << 16)
      + (SUBSTRING_INDEX(SUBSTRING_INDEX(ipb_address, '.', 3), '.', -1) << 24)
      + (SUBSTRING_INDEX(SUBSTRING_INDEX(ipb_address, '/', 1), '.', -1))
      + ((1 << (32 - SUBSTRING_INDEX(ipb_address, '/', -1))) - 1) ), 8, '0' )

  WHERE ipb_address LIKE '%/%';
