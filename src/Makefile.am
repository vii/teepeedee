sbin_PROGRAMS = teepeedee



teepeedee_SOURCES = main.cc \
	ftpcontrol.cc \
	user.cc \
	ftpdatalistener.cc \
	ftplistlong.cc \
	serverftp.cc \
	serverhttp.cc \
	server.cc \
	httpcontrol.cc \
	httplist.cc




noinst_HEADERS = ftpcontrol.hh \
	user.hh \
	ftpdatalistener.hh \
	ftplist.hh \
	ftplistlong.hh \
	serverftp.hh \
	serverhttp.hh \
	server.hh \
	httpcontrol.hh \
	httplist.hh \
	ftplistmlsd.hh \
	ftpmlstprinter.hh





teepeedee_LDADD = ../lib/libteepeedee.a -lcrypt
INCLUDES = -I$(srcdir)/../lib
