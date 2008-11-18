ALTER TABLE `{PREFIX}entries` ADD `moderate_comments` ENUM('true','false') DEFAULT 'false' NOT NULL AFTER `allow_comments`;
ALTER TABLE `{PREFIX}comments` ADD `status` VARCHAR( 50 ) NOT NULL AFTER `subscribed` ;
UPDATE {PREFIX}comments SET status = 'approved';
