cd "$HOME/AAA_DATI_21042026/progetti/myPdfSearch-beta-version" || exit 1

cat > configure.ac <<'EOF'
AC_PREREQ([2.69])

AC_INIT([mypdfsearch], [0.1.0], [vincenzolocicero68@gmail.com])
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_FILES([Makefile])

AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_HEADERS([zlib.h openssl/evp.h], [],
    [AC_MSG_ERROR([Required development header not found])])

AC_CHECK_LIB([z], [inflate], [],
    [AC_MSG_ERROR([zlib library not found])])

AC_CHECK_LIB([crypto], [EVP_sha256], [],
    [AC_MSG_ERROR([OpenSSL libcrypto library not found])])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [build with debugging symbols and without optimizations])],
    [enable_debug="$enableval"],
    [enable_debug=no])

AS_IF([test "x$enable_debug" != "xyes" &&
       test "x$enable_debug" != "xno"],
    [AC_MSG_ERROR([--enable-debug accepts only yes or no])])

AC_ARG_ENABLE([mytime],
    [AS_HELP_STRING([--enable-mytime],
        [enable the MYTIME execution-time instrumentation])],
    [enable_mytime="$enableval"],
    [enable_mytime=no])

AS_IF([test "x$enable_mytime" != "xyes" &&
       test "x$enable_mytime" != "xno"],
    [AC_MSG_ERROR([--enable-mytime accepts only yes or no])])

dnl Set optimization defaults only when the user did not supply CFLAGS.
AS_IF([test "x$ac_env_CFLAGS_set" != "xset"], [
    AS_IF([test "x$enable_debug" = "xyes"],
        [CFLAGS="-O0 -g"],
        [CFLAGS="-O3"])
])

AM_CONDITIONAL([DEBUG_BUILD], [test "x$enable_debug" = "xyes"])
AM_CONDITIONAL([SHOW_TIME], [test "x$enable_mytime" = "xyes"])

AC_OUTPUT
