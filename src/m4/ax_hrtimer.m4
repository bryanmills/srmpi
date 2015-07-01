AU_ALIAS([ACX_HRTIMER], [AX_HRTIMER])
AC_DEFUN([AX_HRTIMER], [
	AC_MSG_CHECKING( [resolution of MPI_Wtime()] )

	AC_RUN_IFELSE(AC_LANG_PROGRAM[
		#include <mpi.h>
	AC_ARG_VAR(WRAP, [PMPI wrapper generator script])
	AC_PATH_PROG(WRAP, wrap.py,,[$PATH:$PWD])
	AC_SUBST(WRAP) 
	
	# Execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
	if test x = x"$WRAP"; then
		$2
		:
	else
		ifelse(
			[$1],
			,
			[ AC_DEFINE(	HAVE_WRAP, 
					1, 
					[Define is you have the wrap PMPI wrapper generator]
			)],
			[$1]
		)
		:
fi
])dnl AX_WRAP
