dnl -*- shell-script -*-
dnl
dnl $HEADER$
dnl

AC_DEFUN([LAM_MCA],[

# Find which modules should be built as run-time loadable modules
# Acceptable combinations:
#
# --with-modules
# --with-modules=[.+,]*MODULE_TYPE[.+,]*
# --with-modules=[.+,]*MODULE_TYPE-MODULE_NAME[.+,]*
# --without-modules
#

AC_MSG_CHECKING([which modules should be run-time loadable])
AC_ARG_WITH(modules,
    AC_HELP_STRING([--with-modules=LIST],
		   [comma-separated list of types and/or type-module pairs of modules that will be built as run-time loadable modules (as opposed to statically linked in LAM/MPI (if supported on this platform).  This directly implies "--enable-shared=LIST and --disable-static=LIST".]))

if test "$with_modules" = "" -o "$with_modules" = "no"; then
    LOADABLE_MODULE_all=0
    msg=none
elif test "$with_modules" = "yes"; then
    LOADABLE_MODULE_all=1
    msg=all
else
    LOADABLE_MODULE_all=0
    ifs_save="$IFS"
    IFS="${IFS}$PATH_SEPARATOR,"
    msg=
    for module in $with_modules; do
	str="`echo LOADABLE_MODULE_$module=1 | sed s/-/_/g`"
	eval $str
	msg="$module $msg"
    done
    IFS="$ifs_save"
fi
AC_MSG_RESULT([$msg])
unset msg

# The list of MCA types (it's fixed)

AC_MSG_CHECKING([for MCA types])
found_types="oob pcm registry coll io one pml ptl topo"
AC_MSG_RESULT([$found_types])

# Get the list of all the non-configure MCA modules that were found by
# autogen.sh.

# config/mca_no_configure_modules.m4
MCA_FIND_NO_CONFIGURE_MODULES

# Now determine the configurable modules in each of the types.  This
# is a little redundant and could be combined into the loop above, but
# we separate it out for clarity.  The extern statements and array of
# pointers to the module global structs are written to a file for each
# type that is #include'd in the flue file for each type.

for type in $found_types; do
    all_modules=
    static_modules=
    dynamic_modules=
    static_ltlibs=

    # Ensure that the directory where the #include file is to live
    # exists.  Need to do this for VPATH builds, because the directory
    # may not exist yet.

    outdir=src/mca/$type/base
    total_dir="."
    for dir_part in `IFS='/\\'; set X $outdir; shift; echo "$[@]"`; do
	total_dir=$total_dir/$dir_part
	test -d "$total_dir" ||
	mkdir "$total_dir" ||
	AC_MSG_ERROR([cannot create $total_dir])
    done

    # Also ensure that the dynamic-mca base directory exists

    total_dir="."
    dyndir=src/mca/dynamic/$type
    for dir_part in `IFS='/\\'; set X $dyndir; shift; echo "$[@]"`; do
	total_dir=$total_dir/$dir_part
	test -d "$total_dir" ||
	mkdir "$total_dir" ||
	AC_MSG_ERROR([cannot create $total_dir])
    done

    # Remove any previous generated #include files

    outfile=$outdir/static-modules.h
    rm -f $outfile $outfile.struct $outfile.extern \
	$outfile.all $outfile.static $outfile.dyanmic
    touch $outfile.struct $outfile.extern \
	$outfile.all $outfile.static $outfile.dynamic

    # Manual conversion of $type to its generic name (e.g., crmpi->cr,
    # crlam->cr).
    # JMS Fix this

    case "$type" in
    crmpi)
	generic_type="cr"
	;;
    crlam)
	generic_type="cr"
	;;
    *)
	generic_type="$type"
	;;
    esac

    # Iterate through the list of no-configure modules

    foo="found_modules=\$MCA_${type}_NO_CONFIGURE_SUBDIRS"
    eval $foo

    for module in $found_modules; do
        m=`basename "$module"`

        if test -d $srcdir/$module -a ! -f $srcdir/$module/.lam_ignore; then
            lam_show_subtitle "MCA module $type:$m (no configure script)"

	    # Remove any possible sym link in the mca-dynamic tree
            
	    rm -f src/mca/dyanmic/$type/$m

            # Now process the module

            MCA_PROCESS_MODULE(1, 1, $type, $m)

            # Note that the AM_CONDITIONAL for this module is set in
            # config/mca_module_list.m4 -- which is generated by
            # autogen.sh because we cannot have a variable
            # AM_CONDITIONAL name (which we need here).  Since
            # autogen.sh knows the name that is necessary, it just
            # generated the AM_CONDITIONAL directly.  Here, we fill in
            # the variable that is used in that AM_CONDITIONAL.

            if test "$compile_mode" = "dynamic"; then
                value=1
            else
                value=0
            fi
            foo="BUILD_${type}_${m}_LOADABLE_MODULE=$value"
            eval $foo
        fi
    done

    # Find all configureable modules, run their configure scripts,
    # etc.

    for module in $srcdir/src/mca/$type/*; do
	FOUND=0
	HAPPY=0
	m="`basename $module`"
	if test -d $module -a -x $module/configure -a \
	    ! -f $module/.lam_ignore; then
            lam_show_subtitle "MCA module $type:$m (need to configure)"

	    # We found one!

	    FOUND=1

	    # Remove any possible sym link in the mca-dynamic tree

	    rm -f src/mca/dyanmic/$type/$m

            # Configure the module subdirectory

            LAM_CONFIG_SUBDIR([src/mca/$type/$m],
				  [$lam_subdir_args], 
				  [HAPPY=1], [HAPPY=0])
	fi

        # Process this module

        MCA_PROCESS_MODULE($FOUND, $HAPPY, $type, $m)
    done

    # m4 weirdness: must also do the echo after the sort, or we get a
    # string with newlines in it

    all_modules="`sort $outfile.all`"
    all_modules="`echo $all_modules`"
    static_modules="`sort $outfile.static`"
    static_modules="`echo $static_modules`"
    dynamic_modules="`sort $outfile.dynamic`"
    dynamic_modules="`echo $dynamic_modules`"
    rm -f $outfile $outfile.all $outfile.static $outfile.dynamic

    # Create the final .h file that will be included in the type's
    # top-level glue.  This lists all the static modules.

    cat > $outfile <<EOF
/*
 * \$HEADER\$
 */

`cat $outfile.extern`

const mca_base_module_t *mca_${type}_base_static_modules[[]] = {
`cat $outfile.struct`
  NULL
};
EOF
    rm -f $outfile.struct $outfile.extern 

    # Save the results for the Makefile.am's.  Note the whacky shell
    # script escaping that is necessary because $modules may be
    # multiple words, but we also need to substitute on ${type}...

    foo="MCA_${type}_ALL_SUBDIRS"'="$all_modules"'
    eval "$foo"
    foo="MCA_${type}_STATIC_SUBDIRS"'="$static_modules"'
    eval "$foo"
    foo="MCA_${type}_DYNAMIC_SUBDIRS"'="$dynamic_modules"'
    eval "$foo"
    foo="MCA_${type}_STATIC_LTLIBS"'="$static_ltlibs"'
    eval "$foo"
done
unset foo type m modules structs outfile outdir total_dir file \
    all_modules static_modules dynamic_modules static_ltlibs

# Grumble.  It seems that AC_SUBST and AC_DEFINE don't let you
# substitue on a variable name that contains a variable (e.g.,
# LAM_MCA_$type_SUBDIRS).  So we have to do this manually.  :-(

# LAM types

AC_SUBST(MCA_oob_ALL_SUBDIRS)
AC_SUBST(MCA_oob_STATIC_SUBDIRS)
AC_SUBST(MCA_oob_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_oob_STATIC_LTLIBS)

AC_SUBST(MCA_pcm_ALL_SUBDIRS)
AC_SUBST(MCA_pcm_STATIC_SUBDIRS)
AC_SUBST(MCA_pcm_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_pcm_STATIC_LTLIBS)

AC_SUBST(MCA_registry_ALL_SUBDIRS)
AC_SUBST(MCA_registry_STATIC_SUBDIRS)
AC_SUBST(MCA_registry_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_registry_STATIC_LTLIBS)

# MPI types

AC_SUBST(MCA_coll_ALL_SUBDIRS)
AC_SUBST(MCA_coll_STATIC_SUBDIRS)
AC_SUBST(MCA_coll_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_coll_STATIC_LTLIBS)

AC_SUBST(MCA_io_ALL_SUBDIRS)
AC_SUBST(MCA_io_STATIC_SUBDIRS)
AC_SUBST(MCA_io_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_io_STATIC_LTLIBS)

AC_SUBST(MCA_one_ALL_SUBDIRS)
AC_SUBST(MCA_one_STATIC_SUBDIRS)
AC_SUBST(MCA_one_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_one_STATIC_LTLIBS)

AC_SUBST(MCA_pml_ALL_SUBDIRS)
AC_SUBST(MCA_pml_STATIC_SUBDIRS)
AC_SUBST(MCA_pml_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_pml_STATIC_LTLIBS)

AC_SUBST(MCA_ptl_ALL_SUBDIRS)
AC_SUBST(MCA_ptl_STATIC_SUBDIRS)
AC_SUBST(MCA_ptl_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_ptl_STATIC_LTLIBS)

AC_SUBST(MCA_topo_ALL_SUBDIRS)
AC_SUBST(MCA_topo_STATIC_SUBDIRS)
AC_SUBST(MCA_topo_DYNAMIC_SUBDIRS)
AC_SUBST(MCA_topo_STATIC_LTLIBS)

# Finally, now that we've filled in all the test variables, get all
# the AM_CONDITIONALs that indicate whether to build modules as shared
# or static.

# config/mca_no_configure_modules.m4
MCA_AMC_NO_CONFIGURE_MODULES])

dnl -----------------------------------------------------------------------
dnl -----------------------------------------------------------------------
dnl -----------------------------------------------------------------------

AC_DEFUN([MCA_PROCESS_MODULE],[
HAPPY=$1
FOUND=$2
type=$3
m=$4

# See if it dropped an output file for us to pick up some
# shell variables in.  

infile="src/mca/$type/$m/post_configure.sh"

# Did we find a valid module, and did its configure run
# successfully?

if test "$HAPPY" = "1"; then

    # Add this subdir to the mast list of all MCA module subdirs

    echo $m >> $outfile.all

    # Is this module going to built staic or shared?

    str="SHARED_TYPE=\$LOADABLE_MODULE_$type"
    eval $str
    str="SHARED_GENERIC_TYPE=\$LOADABLE_MODULE_$generic_type"
    eval $str
    str="SHARED_MODULE=\$LOADABLE_MODULE_${type}_$m"
    eval $str

    shared_mode_override=static

    # Setup for either shared or static

    if test "$shared_mode_override" = "dynamic" -o \
	"$SHARED_TYPE" = "1" -o \
	"$SHARED_GENERIC_TYPE" = "1" -o \
	"$SHARED_MODULE" = "1" -o \
	"$LOADABLE_MODULE_all" = "1"; then
	compile_mode="dynamic"
	echo $m >> $outfile.dynamic
	ls -l "src/mca/dynamic/$type/$m"
	$LN_S "$LAM_TOP_BUILDDIR/src/mca/$type/$m" \
	    "src/mca/dynamic/$type/$m"
	ls -l "src/mca/dynamic/$type/$m"
    else
	static_ltlibs="$m/libmca_${library}_${type}_${m}.la $static_ltlibs"
	echo "extern const mca_base_module_t mca_${type}_${m}_module;" >> $outfile.extern
	echo "  &mca_${type}_${m}_module, " >> $outfile.struct
	compile_mode="static"
	echo $m >> $outfile.static
    fi

    # Output pretty results

    AC_MSG_CHECKING([if MCA module $type:$m can compile])
    AC_MSG_RESULT([yes])
    AC_MSG_CHECKING([for MCA module $type:$m compile mode])
    AC_MSG_RESULT([$compile_mode])

    # If there's an output file, add the values to
    # scope_EXTRA_flags.

    if test -f $infile; then

	# First check for the ABORT tag

	line="`grep ABORT= $infile | cut -d= -f2-`"
	if test -n "$line" -a "$line" != "no"; then
	    AC_MSG_WARN([MCA module configure script told me to abort])
	    AC_MSG_ERROR([cannot continue])
	fi

	# Now check for the rest of the tags

	for scope in LIBLAM LIBMPI WRAPPER; do
	    for flags in CFLAGS CXXFLAGS FFLAGS LDFLAGS LIBS; do
		var="${scope}_EXTRA_${flags}"
		line="`grep $var= $infile | cut -d= -f2-`"
		if test -n "$line"; then
		    str="$var="'"$'"$var $line"'"'
		    eval $str
		fi
	    done
	done
    fi
elif test "$FOUND" = "1"; then
    AC_MSG_CHECKING([if MCA module $type:$m can compile])
    AC_MSG_RESULT([no])

    # If this module was requested as the default for this
    # type, then abort.

    str="foo="'"$'"with_$type"'"'
    eval $str
    str="bar="'"$'"with_$generic_type"'"'
    eval $str
    if test "$foo" = "$m" -o "$bar" = "$m"; then
	AC_MSG_WARN([MCA module "$m" failed to configure properly])
	AC_MSG_WARN([This module was selected as the default])
	AC_MSG_ERROR([Cannot continue])
	exit 1
    fi
fi
])
