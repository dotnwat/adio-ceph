AC_DEFUN([ACVT_JAVA],
[
	java_error="no"
	check_java="yes"
	force_java="no"
	have_java="no"

	AC_ARG_ENABLE(java, 
		AC_HELP_STRING([--enable-java],
		[enable Java support, default: enable if JVMTI found by configure]),
	[AS_IF([test x"$enableval" = "xyes"], [force_java="yes"], [check_java="no"])])

	AS_IF([test x"$check_java" = "xyes"],
	[
		AC_MSG_CHECKING([whether we can build shared libraries])
		AS_IF([test x"$enable_shared" = "xyes"],
		[AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no]); java_error="yes"])

		AS_IF([test x"$java_error" = "xno"],
		[
			ACVT_JVMTI
			AS_IF([test x"$have_jvmti" = "xyes"],
			[have_java="yes"], [java_error="yes"])
		])
	])
])

