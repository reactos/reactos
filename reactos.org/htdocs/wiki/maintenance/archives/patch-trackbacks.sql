CREATE TABLE /*$wgDBprefix*/trackbacks (
	tb_id		INTEGER AUTO_INCREMENT PRIMARY KEY,
	tb_page		INTEGER REFERENCES page(page_id) ON DELETE CASCADE,
	tb_title	VARCHAR(255) NOT NULL,
	tb_url		BLOB NOT NULL,
	tb_ex		TEXT,
	tb_name		VARCHAR(255),

	INDEX (tb_page)
);
