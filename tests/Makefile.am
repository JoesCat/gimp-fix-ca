AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${GIMP_CFLAGS} ${GTK_CFLAGS} ${WCFLAGS} ${FCA_CFLAGS} -I${top_srcdir} -I${top_builddir}

AM_CPPFLAGS = -I${top_srcdir} -I${top_builddir} ${GIMP_CFLAGS} -I${includedir}

EXTRA_DIST = test-fix-ca.c test-fix-ca.scm test1.md5 test2.md5 test3.md5

noinst_PROGRAMS     = test-fix-ca
test_fix_ca_name    = test-fix-ca
test_fix_ca_SOURCES = test-fix-ca.c
test_fix_ca.$(OBJEXT): fix-ca-config.h
test_fix_ca_LDADD   = ${LIBS} ${GIMP_LIBS} ${GTK_LIBS} ${WSLIB} ${FCA_LIB}

update-tests1:
	echo "#!/bin/sh" > ${builddir}/test1.sh; \
	echo "rm -f ${builddir}/test1.bmp" >> ${builddir}/test1.sh; \
	echo "${GIMPTOOL} --install-script ${srcdir}/test-fix-ca.scm" >> ${builddir}/test1.sh; \
	echo "${GIMPTOOL} --install-bin ${builddir}/test-fix-ca" >> ${builddir}/test1.sh; \
	echo "gimp --verbose --console-messages -i -b '(test \"${top_srcdir}/img-fix-ca/full-branches.jpg\" \"${builddir}/test1.bmp\" 6.0 -2.4 658 1280 1 0.0 0.0 0.0 0.0)' -b '(gimp-quit 0)'" >> ${builddir}/test1.sh; \
	echo "${GIMPTOOL} --uninstall-bin test-fix-ca" >> ${builddir}/test1.sh; \
	echo "${GIMPTOOL} --uninstall-script test-fix-ca.scm" >> ${builddir}/test1.sh; \
	echo "${MD5SUM} -c ${top_srcdir}/tests/test1.md5" >> ${builddir}/test1.sh; \
	${CHMOD} +x ${builddir}/test1.sh

update-tests2:
	echo "#!/bin/sh" > ${builddir}/test2.sh; \
	echo "rm -f ${builddir}/test2.tga" >> ${builddir}/test2.sh; \
	echo "${GIMPTOOL} --install-script ${srcdir}/test-fix-ca.scm" >> ${builddir}/test2.sh; \
	echo "${GIMPTOOL} --install-bin ${builddir}/test-fix-ca" >> ${builddir}/test2.sh; \
	echo "gimp --verbose --console-messages -i -b '(test \"${top_srcdir}/img-fix-ca/full-branches.jpg\" \"${builddir}/test2.tga\" 6.0 -2.4 658 1280 1 0.0 0.0 0.0 0.0)' -b '(gimp-quit 0)'" >> ${builddir}/test2.sh; \
	echo "${GIMPTOOL} --uninstall-bin test-fix-ca" >> ${builddir}/test2.sh; \
	echo "${GIMPTOOL} --uninstall-script test-fix-ca.scm" >> ${builddir}/test2.sh; \
	echo "${MD5SUM} -c ${top_srcdir}/tests/test2.md5" >> ${builddir}/test2.sh; \
	${CHMOD} +x ${builddir}/test2.sh

update-tests3:
	echo "#!/bin/sh" > ${builddir}/test3.sh; \
	echo "rm -f ${builddir}/test3.xpm" >> ${builddir}/test3.sh; \
	echo "${GIMPTOOL} --install-script ${srcdir}/test-fix-ca.scm" >> ${builddir}/test3.sh; \
	echo "${GIMPTOOL} --install-bin ${builddir}/test-fix-ca" >> ${builddir}/test3.sh; \
	echo "gimp --verbose --console-messages -i -b '(test \"${top_srcdir}/img-fix-ca/full-branches.jpg\" \"${builddir}/test3.xpm\" 6.0 -2.4 658 1280 1 0.0 0.0 0.0 0.0)' -b '(gimp-quit 0)'" >> ${builddir}/test3.sh; \
	echo "${GIMPTOOL} --uninstall-bin test-fix-ca" >> ${builddir}/test3.sh; \
	echo "${GIMPTOOL} --uninstall-script test-fix-ca.scm" >> ${builddir}/test3.sh; \
	echo "${MD5SUM} -c ${top_srcdir}/tests/test3.md5" >> ${builddir}/test3.sh; \
	${CHMOD} +x ${builddir}/test3.sh

TESTS = ${builddir}/test1.sh ${builddir}/test2.sh ${builddir}/test3.sh

test1.sh:
	make update-tests1

test2.sh:
	make update-tests2

test3.sh:
	make update-tests3

clean-local:
	rm -f ${builddir}/test?.sh ${builddir}/test?.bmp ${builddir}/test?.tga ${builddir}/test?.xpm

.PHONY: update-tests1 update-tests2 update-tests3
