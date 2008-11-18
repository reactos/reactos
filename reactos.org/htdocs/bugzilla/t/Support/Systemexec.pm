# -*- Mode: perl; indent-tabs-mode: nil -*-

package Support::Systemexec;
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(system exec);
@EXPORT_OK = qw();
sub system($$@) {
  1;
}
sub exec($$@) {
  1;
}
1;
