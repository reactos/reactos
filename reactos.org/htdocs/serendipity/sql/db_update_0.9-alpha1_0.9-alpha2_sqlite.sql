CREATE TABLE {PREFIX}permalinks (
    permalink varchar(255) not null default '',
    entry_id int(10) {UNSIGNED} not null default '0',
    type varchar(200) not null default '',
    data text
);

CREATE INDEX pl_idx ON {PREFIX}permalinks (permalink);
CREATE INDEX ple_idx ON {PREFIX}permalinks (entry_id);
CREATE INDEX plt_idx ON {PREFIX}permalinks (type);
CREATE INDEX plcomb_idx ON {PREFIX}permalinks (permalink, type);

CREATE INDEX commentry_idx ON {PREFIX}comments (entry_id);
CREATE INDEX commpentry_idx ON {PREFIX}comments (parent_id);
CREATE INDEX commtype_idx ON {PREFIX}comments (type);
CREATE INDEX commstat_idx ON {PREFIX}comments (status);

CREATE INDEX edraft_idx ON {PREFIX}entries (isdraft);
CREATE INDEX eauthor_idx ON {PREFIX}entries (authorid);

CREATE INDEX refentry_idx ON {PREFIX}references (entry_id);

CREATE INDEX exits_idx ON {PREFIX}exits (entry_id,day);

CREATE INDEX referrers_idx ON {PREFIX}referrers (entry_id,day);
CREATE INDEX urllast_idx on {PREFIX}suppress (last);
CREATE INDEX pluginplace_idx ON {PREFIX}plugins (placement);

CREATE INDEX categorya_idx ON {PREFIX}category (authorid);
CREATE INDEX categoryp_idx ON {PREFIX}category (parentid);
CREATE INDEX categorylr_idx ON {PREFIX}category (category_left, category_right);
