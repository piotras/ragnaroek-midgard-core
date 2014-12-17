dnl $Id: configure.ac 26432 2010-06-22 08:35:36Z piotras $
dnl
dnl Process this file with autoconf to produce a configure script.
dnl
dnl The Midgard version number is changed from here. First edit
dnl this file, then write a ChangeLog entry about the new version,
dnl and finally commit the source tree and give it a tag.
dnl
dnl If the version contains user visible changes then you should
dnl have updated the NEWS file. Remember also to test the release
dnl before committing.
dnl
AC_INIT([midgard-core],[8.10.0])
AC_PREREQ(2.50)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR(src/midgard.c)

dnl Automake is needed for the build system.
dnl
AM_INIT_AUTOMAKE([-Wno-portability -Wall])
AM_MAINTAINER_MODE

LT_INIT

AC_PROG_CC(gcc cc)
AC_LANG(C)

AM_PROG_LEX
AC_PROG_YACC

dnl Checks for programs.
AM_PROG_LIBTOOL

dnl Checks for libraries.
PKG_CHECK_MODULES(MIDGARD, glib-2.0 gobject-2.0 libxml-2.0 dbus-1 dbus-glib-1)

AM_GLIB_GNU_GETTEXT
LIBS="$INTLLIBS $LIBS"

PATH="${PATH}:${prefix}/bin"

_MIDGARD_DBUS_SUPPORT=yes
MIDGARD_DBUS_SUPPORT=1
AC_MSG_CHECKING([whether to compile dbus support (enabled by default)])
AC_ARG_WITH(dbus-support,
	[  --with-dbus-support    compile with dbus support ],
	_MIDGARD_DBUS_SUPPORT=[$]withval, _MIDGARD_DBUS_SUPPORT="yes")
AC_MSG_RESULT($_MIDGARD_DBUS_SUPPORT)
if test "$_MIDGARD_DBUS_SUPPORT" == "no"; then
	MIDGARD_DBUS_SUPPORT=0
fi

dnl Checks for header files.
AC_HEADER_STDC

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl Checks for library functions.
AC_SEARCH_LIBS(crypt, crypt,,AC_MSG_ERROR(You need libcrypt))
AC_CHECK_HEADERS(zlib.h,,AC_MSG_ERROR(You need zlib.h))
AC_CHECK_HEADERS(crypt.h)
AC_CHECK_HEADERS(security/pam_appl.h pam/pam_appl.h)
AC_SEARCH_LIBS(compress, z,,AC_MSG_ERROR(You need libz))
AC_SEARCH_LIBS(gzopen, z,,AC_MSG_ERROR(You need libz))
AC_SEARCH_LIBS(floor, m,,AC_MSG_ERROR(You need libmath))

LFLAGS="${LFLAGS} -Pmgdlib -olex.yy.c"

dnl accomodate Solaris
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(htonl, socket)

dnl search PAM
AC_CHECK_LIB(pam, pam_start)

dnl PP: Keep those till code is not cleaned
USE_MIDGARD_PAGELINKS=0
USE_MIDGARD_PREPARSER_FUDGE_ARRAYS=0
USE_CACHE_FIX=1
USE_MIDGARD_MULTILANG=1
USE_MIDGARD_PAGE_OWNER=1
USE_MIDGARD_VC=1
USE_MIDGARD_QUOTA=1

dnl Find MySQL
AC_ARG_WITH([mysql],
            AS_HELP_STRING(--with-mysql,Deprecated. Use --with-mysql-config instead.),
            [AC_MSG_WARN([Ignoring deprecated option --with-mysql])])
AC_ARG_WITH([mysql-config],
            AS_HELP_STRING(--with-mysql-config=/path/to/mysql_config,Location of the mysql_config program),
            [MYSQL_CONFIG="$withval"],
            [AC_PATH_PROG([MYSQL_CONFIG],["mysql_config"],[""])])
AC_MSG_CHECKING([MYSQL_CFLAGS])
MYSQL_CFLAGS=`"$MYSQL_CONFIG" --cflags`
AC_MSG_RESULT([$MYSQL_CFLAGS])
AC_SUBST([MYSQL_CFLAGS])
AC_MSG_CHECKING([MYSQL_LIBS])
MYSQL_LIBS=`"$MYSQL_CONFIG" --libs`
AC_MSG_RESULT([$MYSQL_LIBS])
AC_SUBST([MYSQL_LIBS])

AC_MSG_CHECKING([MySQL version])

case `"$MYSQL_CONFIG" --version` in
	(3.*|4.0.*)
	AC_MSG_ERROR([MySQL version is less than 4.1!. You should upgrade MySQL])
	;;
esac

AC_MSG_RESULT( > 4.0 OK)

dnl Piotras , I try here to compile and use mysql_get_server_version
dnl which is new in MySQL 4.1
dnl This macro seems to always return success, even if there is syntax error
dnl AC_COMPILE_IFELSE(
dnl	AC_LANG_PROGRAM(
dnl	[[#include <mysql.h>]],
dnl	[[
dnl	int main()
dnl	{
dnl		mysql_get_server_version();
dnl	}
dnl	]]),
dnl	mysql_version_is_ok=no)
	
dnl Piotras: I have no idea how to return success with this one.
dnl AC_RUN_IFELSE([AC_LANG_PROGRAM([[
dnl	#include <mysql.h>
dnl	]]
dnl	, 
dnl	[[int main()
dnl	{
dnl	#if MYSQL_VERSION_ID > 40100
dnl		exit(0);
dnl	#else
dnl		exit(1);
dnl	#endif
dnl	}
dnl	]])]
dnl	,
dnl	[mysql_version_is_ok=yes],[mysql_version_is_ok=no])

MIDGARD_PARSER_DEBUG=0
AC_ARG_ENABLE(parser-logging,
[  --enable-parser-logging Have midgard parser emit log information],
[
  if test "$enableval" = "yes" ; then
    MIDGARD_PARSER_DEBUG=1
  elif test "$enableval" != "no" ; then
    AC_MSG_ERROR(only 'yes' or 'no' are valid answers)
  fi
])
AC_MSG_CHECKING(whether the Midgard parser should spew out log information)
if test "$MIDGARD_PARSER_DEBUG" = "0" ; then
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT(yes)
fi

MIDGARD_PHP_REQUEST_CONFIG_BUG_WORKAROUND=1
dnl PHP request_config bug, temporary workaround
AC_MSG_CHECKING(whether work around the PHP request_config bug)
AC_ARG_WITH(php-rc-bug,
[
Hacks and workarounds:

  --with-php-rc-bug       Work around the PHP request_config bug.],
[
  if test "$withval" = "no" ; then
    MIDGARD_PHP_REQUEST_CONFIG_BUG_WORKAROUND=0
  elif test "$withval" = "yes" ; then
    MIDGARD_PHP_REQUEST_CONFIG_BUG_WORKAROUND=1
  else
    AC_MSG_ERROR(only 'yes' or 'no' are valid answers)
  fi
])
if test "$MIDGARD_PHP_REQUEST_CONFIG_BUG_WORKAROUND" = "0" ; then
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT(yes)
fi

MIDGARD_PARSER_HACK_EVAL=0
AC_MSG_CHECKING([whether to hack eval into mgd_eval (discouraged, only for backwards compatibility)])
AC_ARG_WITH(eval-hack,
[  --with-eval-hack        Hack to change eval into mgd_eval on the fly.
                          Discouraged, but necesary for backwards compatibility,
                          disabled by default.],
[
  if test "$withval" = "no" ; then
    MIDGARD_PARSER_HACK_EVAL=0
  elif test "$withval" = "yes" ; then
    MIDGARD_PARSER_HACK_EVAL=1
  else
    AC_MSG_ERROR(only 'yes' or 'no' are valid answers)
  fi
])
if test "$MIDGARD_PARSER_HACK_EVAL" = "0" ; then
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT(yes)
fi

dnl 'Intelligent' array formatting detection
AC_MSG_CHECKING(whether to enable preparser array formatting with the old dot syntax)
AC_ARG_WITH(preparser-array-detection,
[  --with-preparser-array-detection
                          Enable preparser array formatting with the old
                          dot syntax (discouraged)],
[
  if test "$withval" = "no"; then
    AC_MSG_RESULT(no)
    USE_MIDGARD_PREPARSER_FUDGE_ARRAYS=0
  else
    AC_MSG_RESULT(yes)
    USE_MIDGARD_PREPARSER_FUDGE_ARRAYS=1
  fi
],[
  if test "$USE_MIDGARD_PREPARSER_FUDGE_ARRAYS" = "0" ; then
    AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT(yes)
  fi
])

if test "$prefix" = "/usr"; then
	sysconfdir="/etc/midgard"
elif test "$prefix" = "NONE"; then
	sysconfdir="/usr/local/etc/midgard"
else 
	sysconfdir="$prefix/etc/midgard"
fi

AC_SUBST(LIBS)
AC_SUBST(MIDGARD_PHP_REQUEST_CONFIG_BUG_WORKAROUND)
AC_SUBST(USE_MIDGARD_PAGELINKS)
AC_SUBST(USE_CACHE_FIX)
AC_SUBST(USE_MIDGARD_MULTILANG)
AC_SUBST(USE_MIDGARD_PAGE_OWNER)
AC_SUBST(USE_MIDGARD_VC)
AC_SUBST(USE_MIDGARD_QUOTA)
AC_SUBST(USE_MIDGARD_PREPARSER_FUDGE_ARRAYS)
AC_SUBST(VERSION)
AC_SUBST(VERSION_STRING)
AC_SUBST(LFLAGS)
AC_SUBST(MIDGARD_PARSER_DEBUG)
AC_SUBST(MIDGARD_PARSER_HACK_EVAL)
AC_SUBST(MIDGARD_DBUS_SUPPORT)

AC_CONFIG_FILES([Makefile Doxyfile midgard/midgard_config_auto.h midgard.pc])
AC_OUTPUT

dnl HACK!
sed -e 's/-D_FILE_OFFSET_BITS=64//g' midgard.pc > midgard.pc.tmp.midgard  
mv midgard.pc.tmp.midgard midgard.pc
