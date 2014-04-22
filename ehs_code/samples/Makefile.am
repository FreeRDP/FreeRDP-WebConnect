AM_CPPFLAGS = -I$(top_srcdir)

noinst_HEADERS = btexception.h common.h base64.h sha1.h \
	wscommon.h wsendpoint.h wsframe.h wsutf8.h
	

# Sample programs to be built but not installed
#
EXTRA_PROGRAMS = ehs_formtest ehs_test ehs_testharness ehs_uploader \
				  ehs_simple ehs_mirror ehs_https ehs_privport \
				  ehs_exception ehs_chunktest ehs_basicauth bindhelper \
				  ehs_wsgate

noinst_PROGRAMS = ehs_formtest ehs_test ehs_testharness ehs_uploader \
				  ehs_simple ehs_mirror ehs_https ehs_privport \
				  ehs_exception ehs_chunktest ehs_basicauth bindhelper \
				  ehs_wsgate

bin_PROGRAMS = $(INSTALL_SAMPLES)

ehs_basicauth_SOURCES = ehs_basicauth.cpp base64.cpp
ehs_basicauth_LDADD = $(top_builddir)/libehs.la

ehs_chunktest_SOURCES = ehs_chunktest.cpp
ehs_chunktest_LDADD = $(top_builddir)/libehs.la

ehs_formtest_SOURCES = ehs_formtest.cpp
ehs_formtest_LDADD = $(top_builddir)/libehs.la

ehs_test_SOURCES = ehs_test.cpp
ehs_test_LDADD = $(top_builddir)/libehs.la

ehs_testharness_SOURCES = ehs_testharness.cpp
ehs_testharness_LDADD=$(top_builddir)/libehs.la

ehs_uploader_SOURCES = ehs_uploader.cpp
ehs_uploader_LDADD = $(top_builddir)/libehs.la

ehs_simple_SOURCES = ehs_simple.cpp
ehs_simple_LDADD = $(top_builddir)/libehs.la

ehs_mirror_SOURCES = ehs_mirror.cpp
ehs_mirror_LDADD = $(top_builddir)/libehs.la

ehs_https_SOURCES = ehs_https.cpp
ehs_https_LDADD = $(top_builddir)/libehs.la

ehs_privport_SOURCES = ehs_privport.cpp
ehs_privport_LDADD = $(top_builddir)/libehs.la

ehs_exception_SOURCES = ehs_exception.cpp btexception.cpp
ehs_exception_CPPFLAGS = $(AM_CPPFLAGS) $(TRACE_CPPFLAGS)
ehs_exception_LDADD = $(top_builddir)/libehs.la $(TRACE_LIBS)

ehs_wsgate_SOURCES = ehs_wsgate.cpp btexception.cpp base64.cpp sha1.cpp
ehs_wsgate_CPPFLAGS = $(AM_CPPFLAGS) $(TRACE_CPPFLAGS)
ehs_wsgate_LDADD = $(top_builddir)/libehs.la $(TRACE_LIBS) $(BOOST_SYSTEM_LIBS)

bindhelper_SOURCES = bindhelper.c

EXTRA_DIST = sha1-license.txt wstest.html
MAINTAINERCLEANFILES = Makefile.in
