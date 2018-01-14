AM_CFLAGS = $(VARNISHAPI_CFLAGS) -Wall -Werror
AM_LDFLAGS = $(VARNISHAPI_LIBS) $(VMOD_LDFLAGS)

vmod_LTLIBRARIES = libvmod_shell.la

libvmod_shell_la_SOURCES = \
	vmod_shell.c

nodist_libvmod_shell_la_SOURCES = \
	vcc_shell_if.c \
	vcc_shell_if.h

dist_man_MANS = vmod_shell.3

@BUILD_VMOD_SHELL@

AM_TESTS_ENVIRONMENT = \
	PATH="$(VMOD_TEST_PATH)" \
	LD_LIBRARY_PATH="$(VARNISH_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = varnishtest -v
AM_VTC_LOG_FLAGS = -Dvmod_shell="$(VMOD_SHELL)"

TESTS = @VMOD_TESTS@

EXTRA_DIST = \
	vmod_shell.vcc \
	$(VMOD_TESTS)
