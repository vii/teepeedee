sbin_PROGRAMS = teepeedee



teepeedee_SOURCES = main.cc \
	user.cc \
	serverhttp.cc \
	server.cc \
	httpcontrol.cc \
	httplist.cc \
	control.cc \
	serverhttps.cc \
	serverftp.cc \
	ftpcontrol.cc \
	ftpdatalistener.cc \
	ftplistlong.cc




noinst_HEADERS =	user.hh \
	serverhttp.hh \
	server.hh \
	httpcontrol.hh \
	httplist.hh \
	control.hh \
	serverregistration.hh \
	serverhttps.hh \
	ftpcontrol.hh \
	ftpdatalistener.hh \
	ftplist.hh \
	ftplistlong.hh \
	ftplistmlsd.hh \
	ftpmlstprinter.hh





teepeedee_LDADD = ../lib/libteepeedee.a -lcrypt
INCLUDES = -I$(srcdir)/../lib
