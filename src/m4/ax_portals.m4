#
# AX_PORTALS([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# FIXME: Currently only work for C programs, add C++, and maybe fortran?
# FIXME: this is ugly and will only really work for redstorm
#

AU_ALIAS([ACX_PORTALS], [AX_PORTALS])

AC_DEFUN([AX_PORTALS],[
	AC_ARG_WITH(
		[redstorm-dist],
		[ AS_HELP_STRING(
			[--with-redstorm-dist],
			[location of Redstorm distribution top-level directory]
		)],

		[
			AC_PATH_PROGS( QKMPICC, mpicc, ${CC},
			    [${withval}/install/mpich2-64/GP2/bin:$PATH]
			)
			
			QKCFLAGS="-I${withval}/install/include/portals"
			PORTALS_CFLAGS="-I${withval}/install/include/portals"
			MPIIMP_CFLAGS="-I${withval}/pe/computelibs/mpich2/src/include -I${withval}/pe/computelibs/BGP2-64/src/include -I${withval}/pe/computelibs/mpich2/src/mpid/portals32/include -I${withval}/pe/computelibs/BGP2-64/src/mpid/portals32/include -I${withval}/pe/computelibs/mpich2/src/mpid/common/datatype -I${withval}/pe/computelibs/mpich2/src/mpi/topo"
		],

		[
			AC_ARG_VAR(QKMPICC, [QK MPI C compiler])
			AC_PATH_PROGS(QKMPICC, mpicc, ${CC})
		]
	)

	ax_portals_save_CC="${CC}"
	ax_portals_save_CFLAGS="${CFLAGS}"

	CC="${QKMPICC}"
	CFLAGS="${ax_portals_save_CFLAGS} ${PORTALS_CFLAGS}"

	AC_SUBST(QKMPICC)
	AC_SUBST(QKCFLAGS)
	AC_SUBST(PORTALS_CFLAGS)
	AC_SUBST(MPIIMP_CFLAGS)

	AC_MSG_CHECKING([for portals3.h])
	AC_TRY_COMPILE(
		[
			#include <portals3.h>
		],
		[],
		[
			AC_MSG_RESULT(yes)

			AC_CHECK_LIB(
				[portals],
				 PtlInit,
				[PTL_LIBS="-lportals"],
				[AC_MSG_WARN([No Portals Support])]
			)
		],
		[
			AC_MSG_RESULT(no)
			PTL_LIBS=""
		]
	)

	CC="${ax_portals_save_CC}"
	CFLAGS="${ax_portals_save_CFLAGS}"

	ax_portals_save_CC="${QKMPICC}"
        ax_portals_save_CFLAGS="${QKCFLAGS}"

	AC_SUBST(PTL_LIBS)

####### Finally, execute IF-FOUND/NOT-FOUND
	if test x = x"$PTL_LIBS"; then
		$2
		:
	else
		ifelse(
			[$1],,
			[AC_DEFINE(
				[HAVE_PTLS],
				1,
				[Define if we have Portals support])
			],
			[$1]
		)
		:
	fi
])dnl AX_PORTALS
