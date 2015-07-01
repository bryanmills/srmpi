AU_ALIAS([ACX_WRAP], [AX_WRAP])
AC_DEFUN([AX_WRAP], [
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
