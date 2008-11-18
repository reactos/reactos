
-- Jobs performed by parallel apache threads or a command-line daemon
CREATE TABLE /*$wgDBprefix*/job (
  job_id int unsigned NOT NULL auto_increment,
  
  -- Command name, currently only refreshLinks is defined
  job_cmd varbinary(60) NOT NULL default '',

  -- Namespace and title to act on
  -- Should be 0 and '' if the command does not operate on a title
  job_namespace int NOT NULL,
  job_title varchar(255) binary NOT NULL,

  -- Any other parameters to the command
  -- Presently unused, format undefined
  job_params blob NOT NULL,

  PRIMARY KEY job_id (job_id),
  KEY (job_cmd, job_namespace, job_title)
) /*$wgDBTableOptions*/;
