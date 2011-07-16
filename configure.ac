#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([qpi-stress], [0.1], [fangdingjun@gmail.com])
AC_CONFIG_SRCDIR([src/worker.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(1.0)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([alarm gettimeofday memset])
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT 
