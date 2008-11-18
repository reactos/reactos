--
-- Optional tables for parserTests recording mode
-- With --record option, success data will be saved to these tables,
-- and comparisons of what's changed from the previous run will be
-- displayed at the end of each run.
--
-- These tables currently require MySQL 5 (or maybe 4.1?) for subselects.
--

drop table if exists /*$wgDBprefix*/testitem;
drop table if exists /*$wgDBprefix*/testrun;

create table /*$wgDBprefix*/testrun (
  tr_id int not null auto_increment,
  
  tr_date char(14) binary,
  tr_mw_version blob,
  tr_php_version blob,
  tr_db_version blob,
  tr_uname blob,
  
  primary key (tr_id)
) engine=InnoDB;

create table /*$wgDBprefix*/testitem (
  ti_run int not null,
  ti_name varchar(255),
  ti_success bool,
  
  unique key (ti_run, ti_name),
  key (ti_run, ti_success),
  
  foreign key (ti_run) references /*$wgDBprefix*/testrun(tr_id)
    on delete cascade
) engine=InnoDB;
