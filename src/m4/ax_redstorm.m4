#
# AX_REDSTORM([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#

AU_ALIAS([ACX_REDSTORM], [AX_REDSTORM])
AC_DEFUN([AX_REDSTORM], [
	AC_PREREQ(2.50)

	AC_LANG_CASE(
		[C],
		[
			AC_REQUIRE([AC_PROG_CC])

			if test x = x"$ax_portals_save_CC"; then
				AC_MSG_ERROR([QKMPICC must be defined])
			fi

			ax_restorm_save_CC="${CC}"
			ax_restorm_save_CFLAGS="${CFLAGS}"

			CC="${ax_portals_save_CC}"
			CFLAGS="${ax_portals_save_CFLAGS} ${CFLAGS}"

			AC_MSG_CHECKING([if building for Redstorm])
			AC_COMPILE_IFELSE(
				[AC_LANG_PROGRAM(
				[[
					#include <catamount/catmalloc.h>
				]])],
				[ build_redstorm=yes ],
				[ build_redstorm=no ]
			)
			AC_MSG_RESULT($build_redstorm)

			AC_CHECK_FUNCS([killrank])
	
			CC="${ax_restorm_save_CC}"
			CFLAGS="${ax_restorm_save_CFLAGS}"
		]
	)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
	if test "xno" = "x$build_redstorm"; then
		$2
		:
	else
		ifelse(
			[$1],,
			[ AC_DEFINE(
				[HAVE_REDSTORM],
				1,
				[Define if we are building for Redstorm]
			)],
			[$1]
		)
		:
	fi
])dnl AX_REDSTORM
