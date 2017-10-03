
Sdb

Sdb files are Shim Databases.
They contain information about bad applications, and about the fixes that can be applied to them.


Shims

A shim is a piece of code that influences the process it's applied to.
This can be done by calling certain api's, or by changing the behavior of api's.
'DisableThemes' is an example of a shim that changes behavior by calling 'SetThemeAppProperties'.
'Win95VersionLie' is an example of a shim that changes behavior, by intercepting calls to GetVersion[Ex].

Layers

A layer is a collection (1..n) of shims.
Layers are used to reference a collection of shims by name.


