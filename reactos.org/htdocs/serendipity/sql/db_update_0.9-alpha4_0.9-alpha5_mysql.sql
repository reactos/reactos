create table {PREFIX}access (
  groupid int(10) {UNSIGNED} not null default '0',
  artifact_id int(10) {UNSIGNED} not null default '0',
  artifact_type varchar(64) NOT NULL default '',
  artifact_mode varchar(64) NOT NULL default '',
  artifact_index varchar(64) NOT NULL default ''
);

CREATE INDEX accessgroup_idx ON {PREFIX}access(groupid);
CREATE INDEX accessgroupT_idx ON {PREFIX}access(artifact_id,artifact_type,artifact_mode);
CREATE INDEX accessforeign_idx ON {PREFIX}access(artifact_id);

