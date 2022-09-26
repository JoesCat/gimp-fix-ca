# Makefile.am - Top level automakefile for fix-ca

# The braces around ACLOCAL_FLAGS below instead of parentheses are intentional!
# Otherwise autoreconf misparses the line.
ACLOCAL_AMFLAGS=-I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${GIMP_CFLAGS}

AM_CPPFLAGS = -I${builddir} -I${srcdir} @GIMP_CFLAGS@ -I${includedir}

bin_PROGRAMS = fix-ca
bindir = ${GIMP_LIBDIR}/plug-ins

fix_ca_SOURCES = fix-ca.c
fix_ca.$(OBJEXT): fix-ca-config.h
fix_ca_LDADD = ${LIBS} ${GIMP_LIBS}

EXTRA_DIST = README.md fix-ca-config.h.in rpm/gimp-fix-ca.spec.in \
	img-fix-ca/ex-fixed.jpg img-fix-ca/ex-orig.jpg img-fix-ca/ex-zoom.jpg	\
	img-fix-ca/fix-ca-dialog.png img-fix-ca/full-Wat_Pathum_Wanaram.jpg	\
	img-fix-ca/plug-in-browser.png img-fix-ca/Sea_turtle-dialog_before.png	\
	img-fix-ca/Sea_turtle-dialog.png img-fix-ca/Sea_turtle-fixed.jpg	\
	img-fix-ca/Sea_turtle-orig.jpg img-fix-ca/Sea_turtle-zoom.jpg
nodist_EXTRA_DATA = .git .github
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules

strip:
	${STRIP} ${builddir}/fix-ca

# These use gimptool to install/uninstall fix-ca in user directory
install-user:
	${GIMPTOOL} --install-bin ${builddir}/fix-ca

uninstall-user:
	${GIMPTOOL} --uninstall-bin fix-ca
