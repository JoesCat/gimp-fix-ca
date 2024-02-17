# -*- Autoconf -*-
dnl Process this file with "autoreconf -i;automake" to produce a configure script.

# Copyright (C) 2022 by Joe Da Silva

AC_PREREQ([2.68])
#--------------------------------------------------------------------------
# Setup variables before running AC_INIT()
#
# Making point releases:
#   fix_ca_major_version += 0;
#   fix_ca_minor_version += 1; (patches or added function(s))
#
# If any new functions have been added:
#   fix_ca_major_version += 0;
#   fix_ca_minor_version += 1; (added function(s))
#
# If backwards compatibility has been broken:
#   fix_ca_major_version += 1;
#   fix_ca_minor_version = 0;
#
m4_define([fix_ca_major_version], [3.0])
m4_define([fix_ca_minor_version], [4])
m4_define([fix_ca_version],[fix_ca_major_version.fix_ca_minor_version])
m4_define([fix_ca_package_name], [gimp-fix-ca])

#--------------------------------------------------------------------------
AC_INIT([fix-ca],[fix_ca_version],[https://github.com/JoesCat/gimp-fix-ca/issues],
	[fix_ca_package_name],[https://github.com/JoesCat/gimp-fix-ca])
#--------------------------------------------------------------------------
AC_CONFIG_SRCDIR([fix-ca.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign -Wall])

#--------------------------------------------------------------------------
# automake 1.12 needs AM_PROG_AR but automake < 1.11.2 doesn't recognize it
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_INIT
AC_SUBST([LIBTOOL_DEPS])

#--------------------------------------------------------------------------
# Checks for programs.
AC_PROG_CC
AC_PROG_SED
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PATH_PROG([STRIP],[strip],[:])
AC_PATH_PROG([GIMPTOOL],[gimptool-2.0],[:])
AM_CONFIG_HEADER(fix-ca-config.h)
AC_PROG_INSTALL
AC_PROG_MAKE_SET

#--------------------------------------------------------------------------
# Enable silent build rules by default, this requires atleast Automake-1.11
# Disable by passing --disable-silent-rules to configure or using make V=1
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],[AC_SUBST([AM_DEFAULT_VERBOSITY],[1])])

#--------------------------------------------------------------------------
# The following is for benefit of links using paths relative to top_srcdir.
AC_SUBST([CPPFLAGS],["${CPPFLAGS}"])

#--------------------------------------------------------------------------
# Compiler and Linker flags to override auto detection and insertion.
# Use CFLAGS if you want to add more in addition to autodetect.
AC_ARG_VAR([FCA_CFLAGS],[C compiler flags overriding automatic detection])
AC_ARG_VAR([FCA_LIB],[Linker flags overriding automatic detection])

# Auto detect and add usable compiler warnings (skip if FCA_CFLAGS="").
WCFLAGS=""
AC_LANG_PUSH([C])
AX_CHECK_COMPILE_FLAG([-Wall],[WCFLAGS="$WCFLAGS -Wall"])
AX_CHECK_COMPILE_FLAG([-Wextra],[WCFLAGS="$WCFLAGS -Wextra"])
AX_CHECK_COMPILE_FLAG([-Wcast-align],[WCFLAGS="$WCFLAGS -Wcast-align"])
AX_CHECK_COMPILE_FLAG([-Wbad-function-cast],[WCFLAGS="$WCFLAGS -Wbad-function-cast"])
AX_CHECK_COMPILE_FLAG([-Wc++-compat],[WCFLAGS="$WCFLAGS -Wc++-compat"])
AX_CHECK_COMPILE_FLAG([-Wmissing-prototypes],[WCFLAGS="$WCFLAGS -Wmissing-prototypes"])
AX_CHECK_COMPILE_FLAG([-Wunused],[WCFLAGS="$WCFLAGS -Wunused"])
AX_CHECK_COMPILE_FLAG([-Wdeprecated-declarations],[WCFLAGS="$WCFLAGS -Wdeprecated-declarations"])
AX_CHECK_COMPILE_FLAG([-Wconversion],[WCFLAGS="$WCFLAGS -Wconversion"])
AX_CHECK_COMPILE_FLAG([-Wsign-conversion],[WCFLAGS="$WCFLAGS -Wsign-conversion"])
AX_CHECK_COMPILE_FLAG([-Wformat=2],[WCFLAGS="$WCFLAGS -Wformat=2"])
AX_CHECK_COMPILE_FLAG([-Wformat-security],[WCFLAGS="$WCFLAGS -Wformat-security"])
AX_CHECK_COMPILE_FLAG([-fno-common],[WCFLAGS="$WCFLAGS -fno-common"])
AX_CHECK_COMPILE_FLAG([-Wmissing-declarations],[WCFLAGS="$WCFLAGS -Wmissing-declarations"])
AX_CHECK_COMPILE_FLAG([-Wstrict-prototypes],[WCFLAGS="$WCFLAGS -Wstrict-prototypes"])
AX_CHECK_COMPILE_FLAG([-fPIC],[WCFLAGS="$WCFLAGS -fPIC"])
AX_CHECK_COMPILE_FLAG([-Wstrict-overflow],[WCFLAGS="$WCFLAGS -Wstrict-overflow"])
AX_CHECK_COMPILE_FLAG([-Wpointer-arith],[WCFLAGS="$WCFLAGS -Wpointer-arith"])
AX_CHECK_COMPILE_FLAG([-Wcast-qual],[WCFLAGS="$WCFLAGS -Wcast-qual"])
AX_CHECK_COMPILE_FLAG([-Wcast-align],[WCFLAGS="$WCFLAGS -Wcast-align"])
AX_CHECK_COMPILE_FLAG([-Wpadded],[WCFLAGS="$WCFLAGS -Wpadded"])
AX_CHECK_COMPILE_FLAG([-Woverlength-strings],[WCFLAGS="$WCFLAGS -Woverlength-strings"])
# ARM likely has this, x86 not likely for "half or "_Float16"
AX_CHECK_COMPILE_FLAG([-mfp16-format],[WCFLAGS="$WCFLAGS -mfp16-format"])
# Use the following two combined for clang (if working with "half" or "_Float16"
AX_CHECK_COMPILE_FLAG([-fnative-half-type],[WCFLAGS="$WCFLAGS -fnative-half-type"])
AX_CHECK_COMPILE_FLAG([-fallow-half-arguments-and-returns],[WCFLAGS="$WCFLAGS -fallow-half-arguments-and-returns"])
AC_LANG_POP
# Skip if replacing with FCA_LIB instead.
WLSLIB=""
if test -z $FCA_LIB ; then
   WLSLIB="${WCFLAGS}"
fi
if test "${FCA_CFLAGS}"x != x; then
   WCFLAGS=""
fi

#--------------------------------------------------------------------------
# Check for math.h include and math library (some OSes have -lm built-in).
have_libm=maybe
AC_CHECK_HEADER([math.h],
    AC_SEARCH_LIBS([cos],[m],[have_libm=yes]))
if test x"${have_libm}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the Math library and math.h],[1])
fi

# Check for libgimp/gimp.h include file and libgimp library. LGPL
GIMP_CFLAGS=
GIMP_LIBS=
have_libgimp=no
PKG_CHECK_MODULES([GIMP],[gimp-2.0 >= 2.10.0 gimpui-2.0 >= 2.10.0],[have_libgimp=yes])
if test x"${have_libgimp}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the developer version of libgimp.],[1])
fi
AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)
GIMP_LIBDIR=`${PKG_CONFIG} --variable=gimplibdir gimp-2.0`
AC_SUBST(GIMP_LIBDIR)
GIMP_DATADIR=`${PKG_CONFIG} --variable=gimpdatadir gimp-2.0`
AC_SUBST(GIMP_DATADIR)

AM_CONDITIONAL([HAVEGIMPTOOL],[test "${GIMPTOOL}"x != x])

#--------------------------------------------------------------------------
# Enable debug_time mode
AC_ARG_ENABLE([debugtime],
  [AS_HELP_STRING([--enable-debugtime],
    [Enable debugging time printf mode on commandline @<:@default=no@:>@])],
  [],[enable_debugtime=no])
if test "x$enable_debugtime" = xyes || test "x$enable_debugtime" = xtrue ; then
  AC_DEFINE([DEBUG_TIME],1,[Define if debugging time using printf.])
fi

#--------------------------------------------------------------------------
# Pass variables to fix-ca-config.h
AC_DEFINE([FIX_CA_MAJOR_VERSION],["fix_ca_major_version"],[gimp-fix-ca-plugin major version])
AC_DEFINE([FIX_CA_MINOR_VERSION],["fix_ca_minor_version"],[gimp-fix-ca-plugin minor version])

#--------------------------------------------------------------------------
# Pass variables to MAKEFILE.AM
AC_SUBST([CPPFLAGS],["${CPPFLAGS}"])
AC_SUBST([LS_CFLAGS])
AC_SUBST([LS_LIB])
AC_SUBST([WCFLAGS])
AC_SUBST([WLSLIB])
AC_SUBST([FIX_CA_MAJOR_VERSION],[fix_ca_major_version])
AC_SUBST([FIX_CA_MINOR_VERSION],[fix_ca_minor_version])
AC_SUBST([FIX_CA_VERSION],[fix_ca_version])
AC_SUBST([FIX_CA_PACKAGE_NAME],[fix_ca_package_name])
AC_SUBST([HOST],["$host"])

dnl Do this here since automake can't process 'if/else/endif in Makefile.am
SNIPPET='
noinst_PROGRAMS =
ifeq ($(shell id -u),0)
  bindir = $(GIMP_LIBDIR)/plug-ins/fix-ca
else
  noinst_PROGRAMS += fix-ca
endif
'
AC_SUBST([SNIPPET])
AM_SUBST_NOTMAKE([SNIPPET])

#--------------------------------------------------------------------------
# Put ifndef wrapper on fix-ca-config.h so we don't call it repeatedly.
AH_TOP([#ifndef FIX_CA_CONFIG_H
#define FIX_CA_CONFIG_H 1])
AH_BOTTOM([

#endif])

#--------------------------------------------------------------------------
AC_CONFIG_FILES([
Makefile
rpm/gimp-fix-ca.spec
])
AC_OUTPUT
AC_MSG_NOTICE([

Configuration:

  Source code location	${srcdir}
  Build code location	${builddir}
  docs root dir		${datarootdir}
  Compiler		${CC}
  CPPFLAGS		${CPPFLAGS}
  Config auto WCFLAGS	${WCFLAGS}
  Config FCA_CFLAGS	${FCA_CFLAGS}
  Config FCA_LIB	${FCA_LIB}
  GIMP_CFLAGS		${GIMP_CFLAGS}
  GIMP_LIBS		${GIMP_LIBS}
  LIBS			${LIBS}
])
