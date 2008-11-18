-- SQL to create the initial tables for the MediaWiki database.
-- This is read and executed by the install script; you should
-- not have to run it by itself unless doing a manual install.
-- This is the Oracle version (based on PostgreSQL schema).
-- For information about each table, please see the notes in maintenance/tables.sql

CREATE SEQUENCE user_user_id_seq MINVALUE 0 START WITH 0;

CREATE TABLE mwuser ( -- replace reserved word 'user'
  user_id                   INTEGER  NOT NULL  PRIMARY KEY,
  user_name                 VARCHAR(255)     NOT NULL  UNIQUE,
  user_real_name            CLOB,
  user_password             CLOB,
  user_newpassword          CLOB,
  user_newpass_time         TIMESTAMP WITH TIME ZONE,
  user_token                CHAR(32),
  user_email                CLOB,
  user_email_token          CHAR(32),
  user_email_token_expires  TIMESTAMP WITH TIME ZONE,
  user_email_authenticated  TIMESTAMP WITH TIME ZONE,
  user_options              CLOB,
  user_touched              TIMESTAMP WITH TIME ZONE,
  user_registration         TIMESTAMP WITH TIME ZONE,
  user_editcount            INTEGER
);
CREATE INDEX user_email_token_idx ON mwuser (user_email_token);

-- Create a dummy user to satisfy fk contraints especially with revisions
INSERT INTO mwuser
  VALUES (user_user_id_seq.nextval,'Anonymous','',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, '', current_timestamp, current_timestamp, 0);

CREATE TABLE user_groups (
  ug_user   INTEGER      NULL  REFERENCES mwuser(user_id) ON DELETE CASCADE,
  ug_group  CHAR(16)     NOT NULL
);
CREATE UNIQUE INDEX user_groups_unique ON user_groups (ug_user, ug_group);

CREATE TABLE user_newtalk (
  user_id  INTEGER NOT NULL  REFERENCES mwuser(user_id) ON DELETE CASCADE,
  user_ip  VARCHAR(40)        NULL
);
CREATE INDEX user_newtalk_id_idx ON user_newtalk (user_id);
CREATE INDEX user_newtalk_ip_idx ON user_newtalk (user_ip);

CREATE SEQUENCE page_page_id_seq;
CREATE TABLE page (
  page_id            INTEGER        NOT NULL PRIMARY KEY,
  page_namespace     SMALLINT       NOT NULL,
  page_title         VARCHAR(255)           NOT NULL,
  page_restrictions  CLOB,
  page_counter       INTEGER         DEFAULT 0 NOT NULL,
  page_is_redirect   CHAR           DEFAULT 0 NOT NULL,
  page_is_new        CHAR           DEFAULT 0 NOT NULL,
  page_random        NUMERIC(15,14) NOT NULL,
  page_touched       TIMESTAMP WITH TIME ZONE,
  page_latest        INTEGER        NOT NULL, -- FK?
  page_len           INTEGER        NOT NULL
);
CREATE UNIQUE INDEX page_unique_name ON page (page_namespace, page_title);
CREATE INDEX page_random_idx         ON page (page_random);
CREATE INDEX page_len_idx            ON page (page_len);

CREATE TRIGGER page_set_random BEFORE INSERT ON page
        FOR EACH ROW WHEN (new.page_random IS NULL)
        BEGIN
                SELECT dbms_random.value INTO :new.page_random FROM dual;
        END;
/

CREATE SEQUENCE rev_rev_id_val;
CREATE TABLE revision (
  rev_id          INTEGER      NOT NULL PRIMARY KEY,
  rev_page        INTEGER          NULL  REFERENCES page (page_id) ON DELETE CASCADE,
  rev_text_id     INTEGER          NULL, -- FK
  rev_comment     CLOB,
  rev_user        INTEGER      NOT NULL  REFERENCES mwuser(user_id),
  rev_user_text   VARCHAR(255)         NOT NULL,
  rev_timestamp   TIMESTAMP WITH TIME ZONE  NOT NULL,
  rev_minor_edit  CHAR         DEFAULT '0' NOT NULL,
  rev_deleted     CHAR         DEFAULT '0' NOT NULL,
  rev_len         INTEGER          NULL,
  rev_parent_id   INTEGER      	   DEFAULT NULL
);
CREATE UNIQUE INDEX revision_unique ON revision (rev_page, rev_id);
CREATE INDEX rev_text_id_idx        ON revision (rev_text_id);
CREATE INDEX rev_timestamp_idx      ON revision (rev_timestamp);
CREATE INDEX rev_user_idx           ON revision (rev_user);
CREATE INDEX rev_user_text_idx      ON revision (rev_user_text);


CREATE SEQUENCE text_old_id_val;
CREATE TABLE pagecontent ( -- replaces reserved word 'text'
  old_id     INTEGER  NOT NULL PRIMARY KEY,
  old_text   CLOB,
  old_flags  CLOB
);


CREATE SEQUENCE pr_id_val;
CREATE TABLE page_restrictions (
  pr_id      INTEGER      NOT NULL UNIQUE,
  pr_page    INTEGER          NULL  REFERENCES page (page_id) ON DELETE CASCADE,
  pr_type    VARCHAR(255)         NOT NULL,
  pr_level   VARCHAR(255)         NOT NULL,
  pr_cascade SMALLINT     NOT NULL,
  pr_user    INTEGER          NULL,
  pr_expiry  TIMESTAMP WITH TIME ZONE      NULL
);
ALTER TABLE page_restrictions ADD CONSTRAINT page_restrictions_pk PRIMARY KEY (pr_page,pr_type);

CREATE TABLE archive (
  ar_namespace   SMALLINT     NOT NULL,
  ar_title       VARCHAR(255)         NOT NULL,
  ar_text        CLOB,
  ar_comment     CLOB,
  ar_user        INTEGER          NULL  REFERENCES mwuser(user_id) ON DELETE SET NULL,
  ar_user_text   CLOB         NOT NULL,
  ar_timestamp   TIMESTAMP WITH TIME ZONE  NOT NULL,
  ar_minor_edit  CHAR         DEFAULT '0' NOT NULL,
  ar_flags       CLOB,
  ar_rev_id      INTEGER,
  ar_text_id     INTEGER,
  ar_deleted     INTEGER      DEFAULT '0' NOT NULL
);
CREATE INDEX archive_name_title_timestamp ON archive (ar_namespace,ar_title,ar_timestamp);

CREATE TABLE redirect (
  rd_from       INTEGER  NOT NULL  REFERENCES page(page_id) ON DELETE CASCADE,
  rd_namespace  SMALLINT NOT NULL,
  rd_title      VARCHAR(255)     NOT NULL
);
CREATE INDEX redirect_ns_title ON redirect (rd_namespace,rd_title,rd_from);


CREATE TABLE pagelinks (
  pl_from       INTEGER   NOT NULL  REFERENCES page(page_id) ON DELETE CASCADE,
  pl_namespace  SMALLINT  NOT NULL,
  pl_title      VARCHAR(255)      NOT NULL
);
CREATE UNIQUE INDEX pagelink_unique ON pagelinks (pl_from,pl_namespace,pl_title);

CREATE TABLE templatelinks (
  tl_from       INTEGER  NOT NULL  REFERENCES page(page_id) ON DELETE CASCADE,
  tl_namespace  INTEGER     NOT NULL,
  tl_title      VARCHAR(255)     NOT NULL
);
CREATE UNIQUE INDEX templatelinks_unique ON templatelinks (tl_namespace,tl_title,tl_from);

CREATE TABLE imagelinks (
  il_from  INTEGER  NOT NULL  REFERENCES page(page_id) ON DELETE CASCADE,
  il_to    VARCHAR(255)     NOT NULL
);
CREATE UNIQUE INDEX il_from ON imagelinks (il_to,il_from);

CREATE TABLE categorylinks (
  cl_from       INTEGER      NOT NULL  REFERENCES page(page_id) ON DELETE CASCADE,
  cl_to         VARCHAR(255)         NOT NULL,
  cl_sortkey    VARCHAR(86),
  cl_timestamp  TIMESTAMP WITH TIME ZONE  NOT NULL
);
CREATE UNIQUE INDEX cl_from ON categorylinks (cl_from, cl_to);
CREATE INDEX cl_sortkey     ON categorylinks (cl_to, cl_sortkey);

CREATE TABLE externallinks (
  el_from   INTEGER  NOT NULL  REFERENCES page(page_id) ON DELETE CASCADE,
  el_to     VARCHAR(2048) NOT NULL,
  el_index  CLOB     NOT NULL
);
-- XXX CREATE INDEX externallinks_from_to ON externallinks (el_from,el_to);
-- XXX CREATE INDEX externallinks_index   ON externallinks (el_index);

CREATE TABLE langlinks (
  ll_from    INTEGER  NOT NULL  REFERENCES page (page_id) ON DELETE CASCADE,
  ll_lang    VARCHAR(10),
  ll_title   VARCHAR(255)
);
CREATE UNIQUE INDEX langlinks_unique ON langlinks (ll_from,ll_lang);
CREATE INDEX langlinks_lang_title    ON langlinks (ll_lang,ll_title);


CREATE TABLE site_stats (
  ss_row_id         INTEGER  NOT NULL  UNIQUE,
  ss_total_views    INTEGER            DEFAULT 0,
  ss_total_edits    INTEGER            DEFAULT 0,
  ss_good_articles  INTEGER            DEFAULT 0,
  ss_total_pages    INTEGER            DEFAULT -1,
  ss_users          INTEGER            DEFAULT -1,
  ss_admins         INTEGER            DEFAULT -1,
  ss_images         INTEGER            DEFAULT 0
);

CREATE TABLE hitcounter (
  hc_id  INTEGER  NOT NULL
);


CREATE SEQUENCE ipblocks_ipb_id_val;
CREATE TABLE ipblocks (
  ipb_id                INTEGER      NOT NULL PRIMARY KEY,
  ipb_address           VARCHAR(255)     NULL,
  ipb_user              INTEGER          NULL  REFERENCES mwuser(user_id) ON DELETE SET NULL,
  ipb_by                INTEGER      NOT NULL  REFERENCES mwuser(user_id) ON DELETE CASCADE,
  ipb_reason            VARCHAR(255)         NOT NULL,
  ipb_timestamp         TIMESTAMP WITH TIME ZONE  NOT NULL,
  ipb_auto              CHAR         DEFAULT '0' NOT NULL,
  ipb_anon_only         CHAR         DEFAULT '0' NOT NULL,
  ipb_create_account    CHAR         DEFAULT '1' NOT NULL,
  ipb_enable_autoblock  CHAR         DEFAULT '1' NOT NULL,
  ipb_expiry            TIMESTAMP WITH TIME ZONE  NOT NULL,
  ipb_range_start       CHAR(8),
  ipb_range_end         CHAR(8),
  ipb_deleted           INTEGER      DEFAULT '0' NOT NULL
);
CREATE INDEX ipb_address ON ipblocks (ipb_address);
CREATE INDEX ipb_user    ON ipblocks (ipb_user);
CREATE INDEX ipb_range   ON ipblocks (ipb_range_start,ipb_range_end);


CREATE TABLE image (
  img_name         VARCHAR(255)      NOT NULL  PRIMARY KEY,
  img_size         INTEGER   NOT NULL,
  img_width        INTEGER   NOT NULL,
  img_height       INTEGER   NOT NULL,
  img_metadata     CLOB,
  img_bits         SMALLINT,
  img_media_type   CLOB,
  img_major_mime   CLOB                DEFAULT 'unknown',
  img_minor_mime   CLOB                DEFAULT 'unknown',
  img_description  CLOB,
  img_user         INTEGER       NULL  REFERENCES mwuser(user_id) ON DELETE SET NULL,
  img_user_text    CLOB      NOT NULL,
  img_timestamp    TIMESTAMP WITH TIME ZONE
);
CREATE INDEX img_size_idx      ON image (img_size);
CREATE INDEX img_timestamp_idx ON image (img_timestamp);

CREATE TABLE oldimage (
  oi_name          VARCHAR(255)         NOT NULL  REFERENCES image(img_name),
  oi_archive_name  VARCHAR(255),
  oi_size          INTEGER      NOT NULL,
  oi_width         INTEGER      NOT NULL,
  oi_height        INTEGER      NOT NULL,
  oi_bits          SMALLINT     NOT NULL,
  oi_description   CLOB,
  oi_user          INTEGER          NULL  REFERENCES mwuser(user_id) ON DELETE SET NULL,
  oi_user_text     CLOB         NOT NULL,
  oi_timestamp     TIMESTAMP WITH TIME ZONE  NOT NULL,
  oi_metadata      CLOB, 	 
  oi_media_type    VARCHAR(10) DEFAULT NULL,
  oi_major_mime    VARCHAR(11) DEFAULT 'unknown',
  oi_minor_mime    VARCHAR(32) DEFAULT 'unknown',
  oi_deleted       INTEGER DEFAULT 0 NOT NULL
);
CREATE INDEX oi_name_timestamp ON oldimage (oi_name,oi_timestamp);
CREATE INDEX oi_name_archive_name ON oldimage (oi_name,oi_archive_name);

CREATE SEQUENCE filearchive_fa_id_seq;
CREATE TABLE filearchive (
  fa_id                 INTEGER       NOT NULL  PRIMARY KEY,
  fa_name               VARCHAR(255)         NOT NULL,
  fa_archive_name       VARCHAR(255),
  fa_storage_group      VARCHAR(16),
  fa_storage_key        CHAR(64),
  fa_deleted_user       INTEGER          NULL  REFERENCES mwuser(user_id) ON DELETE SET NULL,
  fa_deleted_timestamp  TIMESTAMP WITH TIME ZONE  NOT NULL,
  fa_deleted_reason     CLOB,
  fa_size               SMALLINT     NOT NULL,
  fa_width              SMALLINT     NOT NULL,
  fa_height             SMALLINT     NOT NULL,
  fa_metadata           CLOB,
  fa_bits               SMALLINT,
  fa_media_type         CLOB,
  fa_major_mime         CLOB                   DEFAULT 'unknown',
  fa_minor_mime         CLOB                   DEFAULT 'unknown',
  fa_description        CLOB         NOT NULL,
  fa_user               INTEGER          NULL  REFERENCES mwuser(user_id) ON DELETE SET NULL,
  fa_user_text          CLOB         NOT NULL,
  fa_timestamp          TIMESTAMP WITH TIME ZONE,
  fa_deleted            INTEGER      DEFAULT '0' NOT NULL
);
CREATE INDEX fa_name_time ON filearchive (fa_name, fa_timestamp);
CREATE INDEX fa_dupe      ON filearchive (fa_storage_group, fa_storage_key);
CREATE INDEX fa_notime    ON filearchive (fa_deleted_timestamp);
CREATE INDEX fa_nouser    ON filearchive (fa_deleted_user);


CREATE SEQUENCE rc_rc_id_seq;
CREATE TABLE recentchanges (
  rc_id              INTEGER      NOT NULL PRIMARY KEY,
  rc_timestamp       TIMESTAMP WITH TIME ZONE  NOT NULL,
  rc_cur_time        TIMESTAMP WITH TIME ZONE  NOT NULL,
  rc_user            INTEGER          NULL  REFERENCES mwuser(user_id) ON DELETE SET NULL,
  rc_user_text       CLOB         NOT NULL,
  rc_namespace       SMALLINT     NOT NULL,
  rc_title           VARCHAR(255)         NOT NULL,
  rc_comment         VARCHAR(255),
  rc_minor           CHAR         DEFAULT '0' NOT NULL,
  rc_bot             CHAR         DEFAULT '0' NOT NULL,
  rc_new             CHAR         DEFAULT '0' NOT NULL,
  rc_cur_id          INTEGER          NULL  REFERENCES page(page_id) ON DELETE SET NULL,
  rc_this_oldid      INTEGER      NOT NULL,
  rc_last_oldid      INTEGER      NOT NULL,
  rc_type            CHAR         DEFAULT '0' NOT NULL,
  rc_moved_to_ns     SMALLINT,
  rc_moved_to_title  CLOB,
  rc_patrolled       CHAR         DEFAULT '0' NOT NULL,
  rc_ip              VARCHAR(15),
  rc_old_len         INTEGER,
  rc_new_len         INTEGER,
  rc_deleted         INTEGER      DEFAULT '0' NOT NULL,
  rc_logid           INTEGER      DEFAULT '0' NOT NULL,
  rc_log_type      	 CLOB,
  rc_log_action      CLOB,
  rc_params      	 CLOB
);
CREATE INDEX rc_timestamp       ON recentchanges (rc_timestamp);
CREATE INDEX rc_namespace_title ON recentchanges (rc_namespace, rc_title);
CREATE INDEX rc_cur_id          ON recentchanges (rc_cur_id);
CREATE INDEX new_name_timestamp ON recentchanges (rc_new, rc_namespace, rc_timestamp);
CREATE INDEX rc_ip              ON recentchanges (rc_ip);


CREATE TABLE watchlist (
  wl_user                   INTEGER     NOT NULL  REFERENCES mwuser(user_id) ON DELETE CASCADE,
  wl_namespace              SMALLINT    DEFAULT 0 NOT NULL,
  wl_title                  VARCHAR(255)        NOT NULL,
  wl_notificationtimestamp  TIMESTAMP WITH TIME ZONE
);
CREATE UNIQUE INDEX wl_user_namespace_title ON watchlist (wl_namespace, wl_title, wl_user);


CREATE TABLE math (
  math_inputhash              VARCHAR(16)      NOT NULL  UNIQUE,
  math_outputhash             VARCHAR(16)      NOT NULL,
  math_html_conservativeness  SMALLINT  NOT NULL,
  math_html                   CLOB,
  math_mathml                 CLOB
);


CREATE TABLE interwiki (
  iw_prefix  VARCHAR(32)   NOT NULL  UNIQUE,
  iw_url     VARCHAR(127)  NOT NULL,
  iw_local   CHAR  NOT NULL,
  iw_trans   CHAR  DEFAULT '0' NOT NULL
);

CREATE TABLE querycache (
  qc_type       CHAR(32)      NOT NULL,
  qc_value      SMALLINT  NOT NULL,
  qc_namespace  SMALLINT  NOT NULL,
  qc_title      CHAR(255)      NOT NULL
);
CREATE INDEX querycache_type_value ON querycache (qc_type, qc_value);

CREATE TABLE querycache_info (
  qci_type       VARCHAR(32)              UNIQUE,
  qci_timestamp  TIMESTAMP WITH TIME ZONE NULL
);

CREATE TABLE querycachetwo (
  qcc_type          CHAR(32)     NOT NULL,
  qcc_value         SMALLINT DEFAULT 0 NOT NULL,
  qcc_namespace     INTEGER  DEFAULT 0 NOT NULL,
  qcc_title         CHAR(255)     DEFAULT '' NOT NULL,
  qcc_namespacetwo  INTEGER  DEFAULT 0 NOT NULL,
  qcc_titletwo      CHAR(255)     DEFAULT '' NOT NULL
);
CREATE INDEX querycachetwo_type_value ON querycachetwo (qcc_type, qcc_value);
CREATE INDEX querycachetwo_title      ON querycachetwo (qcc_type,qcc_namespace,qcc_title);
CREATE INDEX querycachetwo_titletwo   ON querycachetwo (qcc_type,qcc_namespacetwo,qcc_titletwo);


CREATE TABLE objectcache (
  keyname  CHAR(255)              UNIQUE,
  value    BLOB,
  exptime  TIMESTAMP WITH TIME ZONE  NOT NULL
);
CREATE INDEX objectcacache_exptime ON objectcache (exptime);

CREATE TABLE transcache (
  tc_url       VARCHAR(255)         NOT NULL  UNIQUE,
  tc_contents  CLOB         NOT NULL,
  tc_time      TIMESTAMP WITH TIME ZONE  NOT NULL
);


CREATE SEQUENCE log_log_id_seq;
CREATE TABLE logging (
  log_type        VARCHAR(10)         NOT NULL,
  log_action      VARCHAR(10)         NOT NULL,
  log_timestamp   TIMESTAMP WITH TIME ZONE  NOT NULL,
  log_user        INTEGER                REFERENCES mwuser(user_id) ON DELETE SET NULL,
  log_namespace   SMALLINT     NOT NULL,
  log_title       VARCHAR(255)         NOT NULL,
  log_comment     VARCHAR(255),
  log_params      CLOB,
  log_deleted     INTEGER      DEFAULT '0' NOT NULL,
  log_id          INTEGER      NOT NULL PRIMARY KEY
);
CREATE INDEX logging_type_name ON logging (log_type, log_timestamp);
CREATE INDEX logging_user_time ON logging (log_timestamp, log_user);
CREATE INDEX logging_page_time ON logging (log_namespace, log_title, log_timestamp);

CREATE SEQUENCE trackbacks_tb_id_seq;
CREATE TABLE trackbacks (
  tb_id     INTEGER   NOT NULL PRIMARY KEY,
  tb_page   INTEGER            REFERENCES page(page_id) ON DELETE CASCADE,
  tb_title  VARCHAR(255)     NOT NULL,
  tb_url    VARCHAR(255)     NOT NULL,
  tb_ex     CLOB,
  tb_name   VARCHAR(255) 
);
CREATE INDEX trackback_page ON trackbacks (tb_page);

CREATE SEQUENCE job_job_id_seq;
CREATE TABLE job (
  job_id         INTEGER   NOT NULL PRIMARY KEY,
  job_cmd        VARCHAR(255)      NOT NULL,
  job_namespace  SMALLINT  NOT NULL,
  job_title      VARCHAR(255)      NOT NULL,
  job_params     CLOB      NOT NULL
);
CREATE INDEX job_cmd_namespace_title ON job (job_cmd, job_namespace, job_title);

-- This table is not used unless profiling is turned on
--CREATE TABLE profiling (
--  pf_count   INTEGER         DEFAULT 0 NOT NULL,
--  pf_time    NUMERIC(18,10)  DEFAULT 0 NOT NULL,
--  pf_name    CLOB            NOT NULL,
--  pf_server  CLOB            NULL
--);
--CREATE UNIQUE INDEX pf_name_server ON profiling (pf_name, pf_server);

CREATE TABLE searchindex (
  si_page	INTEGER UNIQUE NOT NULL,
  si_title	VARCHAR(255) DEFAULT '' NOT NULL,
  si_text	CLOB NOT NULL
);


CREATE INDEX si_title_idx ON searchindex(si_title) INDEXTYPE IS ctxsys.context;
CREATE INDEX si_text_idx ON searchindex(si_text) INDEXTYPE IS ctxsys.context;
