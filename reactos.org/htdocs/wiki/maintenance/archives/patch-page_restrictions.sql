--- Used for storing page restrictions (i.e. protection levels)
CREATE TABLE /*$wgDBprefix*/page_restrictions (
	-- Page to apply restrictions to (Foreign Key to page).
	pr_page int NOT NULL,
	-- The protection type (edit, move, etc)
	pr_type varbinary(60) NOT NULL,
	-- The protection level (Sysop, autoconfirmed, etc)
	pr_level varbinary(60) NOT NULL,
	-- Whether or not to cascade the protection down to pages transcluded.
	pr_cascade tinyint NOT NULL,
	-- Field for future support of per-user restriction.
	pr_user int NULL,
	-- Field for time-limited protection.
	pr_expiry varbinary(14) NULL,

	PRIMARY KEY pr_pagetype (pr_page,pr_type),
	KEY pr_typelevel (pr_type,pr_level),
	KEY pr_level (pr_level),
	KEY pr_cascade (pr_cascade)
) /*$wgDBTableOptions*/;
