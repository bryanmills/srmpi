AC_PREREQ([2.61])
AC_INIT([mills-optimized-collectives], [0.1], [bmills@cs.pitt.edu])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CONFIG_SRCDIR([lib/])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_LIBOBJ_DIR([lib])

AC_MSG_CHECKING([whether to include debug support])
AC_ARG_ENABLE( debug,
       	[ AS_HELP_STRING( [--enable-debug],
			  [Enable library debug support] )
	],
	[ case "${enableval}" in
                yes | no ) DEBUG="${enableval}" ;;
                * ) AC_MSG_ERROR( [bad value ${enableval} for --enable-debug] ) ;;
         esac],
        [ DEBUG="no" ]
)
AM_CONDITIONAL( [WITH_DEBUG], [test "x$DEBUG" = "xyes"] )
if test "x$DEBUG" = "xyes"; then
	AC_DEFINE( [WITH_DEBUG], [], [Build with debug support] )
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([whether to include sanity check support])
AC_ARG_ENABLE( sanity,
       	[ AS_HELP_STRING( [--enable-sanity],
			  [Enable library sanity check support] )
	],
	[ case "${enableval}" in
                yes | no ) SANITY="${enableval}" ;;
                * ) AC_MSG_ERROR( [bad value ${enableval} for --enable-sanity] ) ;;
         esac],
        [ SANITY="no" ]
)
AM_CONDITIONAL( [WITH_SANITY], [test "x$SANITY" = "xyes"] )
if test "x$SANITY" = "xyes"; then
	AC_DEFINE( [WITH_SANITY], [], [Build with sanity check support] )
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([for collective implementation to use])
AC_ARG_ENABLE( collectives,
        [ AS_HELP_STRING( [--enable-collectives = linear | optimized | none ],
                          [Use specified collective implementation] )
        ],
        [ case "${enableval}" in
                linear | optimized | none ) COLLECTIVES="${enableval}" ;;
                * ) AC_MSG_ERROR( [ bad value <${enableval}> for --enable-collectives ] ) ;;
          esac ],
        [ COLLECTIVES="linear" ]
)
if test "x$COLLECTIVES" = "xnone"; then
        AC_DEFINE( [NATIVE_COLLECTIVES], [], [Use MPI native collectives] )
elif test "x$COLLECTIVES" = "xlinear"; then
        AC_DEFINE( [P2P_COLLECTIVES], [], [Use LINEAR collective implementation] )
elif test "x$COLLECTIVES" = "xoptimized"; then
        AC_DEFINE( [P2P_COLLECTIVES], [], [Use OPTIMIZED collective implementation] )
else
        AC_MSG_ERROR( [why are we here, we should never be here] )
fi

AM_CONDITIONAL( [NATIVE_COLLECTIVES], [ test "x${COLLECTIVES}" = "xnative" ] )
AM_CONDITIONAL( [LINEAR_COLLECTIVES], [ test "x${COLLECTIVES}" = "xlinear" ] )
AM_CONDITIONAL( [OPTIMIZED_COLLECTIVES], [ test "x${COLLECTIVES}" = "xoptimized" ] )

AC_DEFINE_UNQUOTED( 	COLLECTIVE_NAME,
			[${COLLECTIVES}],
			[Collective implementation name])

AC_MSG_RESULT( [${COLLECTIVES}] )

AC_MSG_CHECKING([for replica node use])
AC_ARG_ENABLE( replica,
        [ AS_HELP_STRING( [--enable-replica = simple | none ],
                          [Use specified replica node implementation] )
        ],
        [ case "${enableval}" in
                simple | none ) REPLICA="${enableval}" ;;
                * ) AC_MSG_ERROR( [ bad value <${enableval}> for --enable-replica ] ) ;;
          esac ],
        [ REPLICA="none" ]
)
if test "x$REPLICA" = "xnone"; then
        AC_DEFINE( [NO_REPLICA_NODES], [], [No Replica] )
elif test "x$REPLICA" = "xsimple"; then
        AC_DEFINE( [SIMPLE_REPLICA_NODES], [], [Use SIMPLE replica implementation] )
else
        AC_MSG_ERROR( [why are we here, we should never be here] )
fi
AC_MSG_RESULT( [${REPLICA}] )

AM_CONDITIONAL( [NO_REPLICA], [ test "x${REPLICA}" = "xnone" ] )
AM_CONDITIONAL( [SIMPLE_REPLICA], [ test "x${REPLICA}" = "xsimple" ] )

AC_DEFINE_UNQUOTED( 	REPLICA_TYPE,
			[${REPLICA}],
			[Replica implementation name])



# Checks for programs.

AC_F77_WRAPPERS
AC_PROG_CC
AC_PROG_CC_C_O
AM_PROG_CC_C_O

AC_PROG_LIBTOOL

AX_MPI(
        [
         	have_mpi="yes";
         	AC_DEFINE(
            		[HAVE_MPI],
                	1,
                	[Define if you have the MPI library.]
         	)
        ],
        [ 
		have_mpi="no";
	  	AC_MSG_ERROR( [MPI support needed for this PMPI library ] ) 
	]
)

AM_CONDITIONAL( [HAVE_MPI], [test "x$have_mpi" = "xyes"] )

##AX_WRAP(
##[
##	have_wrap="yes";
##	AC_DEFINE( 
##		[HAVE_WRAP], 1, [Define if you have PMPI wrap generator] )
##],
##[
##	have_wrap="no";
##	AC_MSG_ERROR( [wrap PMPI wrapper generator needed for library] )
##])

AX_PYTHON

AX_CFLAGS_GCC_OPTION([-pedantic], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-Wall], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-std=gnu99])
AX_CFLAGS_GCC_OPTION([-Wshadow], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-Wcast-align], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-Wcast-qual], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-Wstrict-prototype], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-Wpointer-arith], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-D_GNU_SOURCE], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-g], [HOST_CFLAGS])
AX_CFLAGS_GCC_OPTION([-I../include/], [HOST_CFLAGS])
AC_SUBST([HOST_CFLAGS])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h execinfo.h libgen.h float.h inttypes.h])

# Checks for typedefs, structures, and compiler characteristics.

AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AX_C___ATTRIBUTE__

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset backtrace backtrace_symbols readlink basename])

# Check for /proc/self/exe
AC_CHECK_FILE(  [/proc/self/exe],
                AC_DEFINE( [HAVE_PROC_SELF_EXE], 1,
                           [Define if /proc/self/exe exists] ),
                AC_MSG_WARN([Cannot find /proc/self/exe.  May not be able to determine executable name]) )

AC_CONFIG_FILES([Makefile
         lib/Makefile
		 tests/Makefile])
AC_OUTPUT
