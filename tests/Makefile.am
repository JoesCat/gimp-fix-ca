scripttests = test1.sh

AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${GIMP_CFLAGS} ${GTK_CFLAGS} ${WCFLAGS} ${FCA_CFLAGS} -I${top_srcdir} -I${top_builddir}

AM_CPPFLAGS = -I${top_srcdir} -I${top_builddir} ${GIMP_CFLAGS} -I${includedir}

EXTRA_DIST = test-fix-ca.c test.scm test1.md5
nodist_bin_SCRIPTS = test1.sh

noinst_PROGRAMS     = test-fix-ca
test_fix_ca_name    = test-fix-ca
test_fix_ca_SOURCES = test-fix-ca.c
test_fix_ca.$(OBJEXT): fix-ca-config.h
test_fix_ca_LDADD   = ${LIBS} ${GIMP_LIBS} ${GTK_LIBS} ${WSLIB} ${FCA_LIB}

update-tests: $(scripttests)

$(scripttests):
	echo "#!/bin/sh" > ${builddir}/test1.sh; \
	echo "rm -f ${builddir}/test1.tga" >> ${builddir}/test1.sh; \
	echo "${GIMPTOOL} --install-script ${srcdir}/test.scm" >> ${builddir}/test1.sh; \
	echo "${GIMPTOOL} --install-bin ${builddir}/test-fix-ca" >> ${builddir}/test1.sh; \
	echo "gimp --verbose --console-messages -i -b '(test \"${top_srcdir}/img-fix-ca/full-branches.jpg\" \"${builddir}/test1.tga\" 6.0 -2.4 658 1280 1 0.0 0.0 0.0 0.0)' -b '(gimp-quit 0)'" >> ${builddir}/test1.sh; \
	echo "${GIMPTOOL} --uninstall-bin test-fix-ca" >> ${builddir}/test1.sh; \
	echo "${GIMPTOOL} --uninstall-script test.scm" >> ${builddir}/test1.sh; \
	echo "${MD5SUM} -c ${top_srcdir}/tests/test1.md5" >> ${builddir}/test1.sh; \
	${CHMOD} +x ${builddir}/test1.sh

TESTS = ${builddir}/test1.sh

clean-local:
	rm -f ${builddir}/test?.sh ${builddir}/test?.tga

.PHONY: update-tests
