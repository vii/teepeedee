bin_PROGRAMS = teepeedee
bin_SCRIPTS = teepeedee-share

EXTRA_DIST = teepeedee-share


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
	ftpmlstprinter.hh \
	xferlimituser.hh \
	serverssl.hh \
	serverhttps.hh \
	serverftp.hh






teepeedee_LDADD = ../lib/libteepeedee.a -lcrypt
INCLUDES = -I$(srcdir)/../lib
