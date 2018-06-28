This package consists of two programs that illustrate the Interprocess
communication between a master process and a number of background
processes. Each background process is connected to a different database.

This package has been built with IBM's CSET/2 compiler and using
beta versions of DB2/2.

The code will only work with OS/2 version 2 since it uses APIs only
available on that platform.

sqlprep worker.sqc sample /b /p
sqlbind worker.bnd dale
sqlbind worker.bnd turbo
icc -c worker.c
link386 worker.obj,worker.exe,/NOI /ST:32000,sql_dyn.lib;
