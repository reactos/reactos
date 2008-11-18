-- Add a sort-key to page_restrictions table.
-- First immediate use of this is as a sort-key for coming modifications
-- of Special:Protectedpages.
-- Andrew Garrett, February 2007

ALTER TABLE /*$wgDBprefix*/page_restrictions
	ADD COLUMN pr_id int unsigned not null auto_increment,
	ADD UNIQUE KEY pr_id (pr_id);
