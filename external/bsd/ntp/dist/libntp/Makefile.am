NULL=
#AUTOMAKE_OPTIONS = ../ansi2knr no-dependencies
AUTOMAKE_OPTIONS = 
BUILT_SOURCES =
CLEANFILES =
noinst_LIBRARIES = libntp.a @MAKE_LIBNTPSIM@
EXTRA_LIBRARIES = libntpsim.a
libntp_a_SRCS =						\
	a_md5encrypt.c					\
	adjtime.c					\
	atoint.c					\
	atolfp.c					\
	atouint.c					\
	audio.c						\
	authkeys.c					\
	authreadkeys.c					\
	authusekey.c					\
	bsd_strerror.c					\
	buftvtots.c					\
	caljulian.c					\
	caltontp.c					\
	calyearstart.c					\
	clocktime.c					\
	clocktypes.c					\
	decodenetnum.c					\
	dofptoa.c					\
	dolfptoa.c					\
	emalloc.c					\
	findconfig.c					\
	fptoa.c						\
	fptoms.c					\
	getopt.c					\
	hextoint.c					\
	hextolfp.c					\
	humandate.c					\
	icom.c						\
	inttoa.c					\
	iosignal.c					\
	lib_strbuf.c					\
	machines.c					\
	memmove.c					\
	mfptoa.c					\
	ntp_lineedit.c					\
	mfptoms.c					\
	mktime.c					\
	modetoa.c					\
	mstolfp.c					\
	msutotsf.c					\
	msyslog.c					\
	netof.c						\
	ntp_rfc2553.c					\
	numtoa.c					\
	numtohost.c					\
	octtoint.c					\
	prettydate.c					\
	ntp_random.c					\
	recvbuff.c					\
	refnumtoa.c					\
	snprintf.c					\
	socktoa.c					\
	socktohost.c					\
	ssl_init.c					\
	statestr.c					\
	strdup.c					\
	strstr.c					\
	syssignal.c					\
	tsftomsu.c					\
	tstotv.c					\
	tvtots.c					\
	uglydate.c					\
	uinttoa.c					\
	ymd2yd.c					\
	$(srcdir)/../lib/isc/assertions.c		\
	$(srcdir)/../lib/isc/buffer.c			\
	$(srcdir)/../lib/isc/nothreads/condition.c	\
	$(srcdir)/../lib/isc/unix/dir.c			\
	$(srcdir)/../lib/isc/error.c			\
	$(srcdir)/../lib/isc/unix/errno2result.c	\
	$(srcdir)/../lib/isc/event.c			\
	$(srcdir)/../lib/isc/unix/file.c		\
	$(srcdir)/../lib/isc/inet_ntop.c		\
	$(srcdir)/../lib/isc/inet_pton.c		\
	$(srcdir)/../lib/isc/unix/interfaceiter.c	\
	$(srcdir)/../lib/isc/lib.c			\
	$(srcdir)/../lib/isc/log.c			\
	$(srcdir)/../lib/isc/md5.c			\
	$(srcdir)/../lib/isc/nls/msgcat.c		\
	$(srcdir)/../lib/isc/unix/net.c			\
	$(srcdir)/../lib/isc/netaddr.c			\
	$(srcdir)/../lib/isc/netscope.c			\
	$(srcdir)/../lib/isc/ondestroy.c		\
	$(srcdir)/../lib/isc/random.c			\
	$(srcdir)/../lib/isc/result.c			\
	$(srcdir)/../lib/isc/unix/stdio.c		\
	$(srcdir)/../lib/isc/unix/stdtime.c		\
	$(srcdir)/../lib/isc/unix/strerror.c		\
	$(srcdir)/../lib/isc/task.c			\
	$(srcdir)/../lib/isc/nothreads/thread.c		\
	$(srcdir)/../lib/isc/unix/time.c		\
	$(srcdir)/../lib/isc/sockaddr.c			\
	$(NULL)
libntp_a_SOURCES = systime.c $(libntp_a_SRCS)
libntpsim_a_SOURCES = systime_s.c $(libntp_a_SRCS)
EXTRA_libntp_a_SOURCES = adjtimex.c
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/isc/include \
	-I$(top_srcdir)/lib/isc/nothreads/include \
	-I$(top_srcdir)/lib/isc/unix/include
ETAGS_ARGS = Makefile.am

EXTRA_DIST = README

include $(top_srcdir)/depsver.mf
