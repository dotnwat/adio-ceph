#! /usr/bin/env bash 
#
# Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#
# This script is run on developer copies of Open MPI -- *not*
# distribution tarballs.

#set -x

##############################################################################
#
# User-definable parameters (search path and minimum supported versions)
# 
# Note: use ';' to separate parameters
##############################################################################

ompi_aclocal_search="aclocal"
if test ! -z "$ACLOCAL"; then
    ompi_aclocal_search="$ACLOCAL"
fi
ompi_autoheader_search="autoheader"
if test ! -z "$AUTOHEADER"; then
    ompi_autoheader_search="$AUTOHEADER"
fi
ompi_autoconf_search="autoconf"
if test ! -z "$AUTOCONF"; then
    ompi_autoconf_search="$AUTOCONF"
fi
ompi_autom4te_search="autom4te"
if test ! -z "$AUTOM4TE"; then
    ompi_autom4te_search="$AUTOM4TE"
fi
ompi_libtoolize_search="libtoolize;glibtoolize"
if test ! -z "$LIBTOOLIZE"; then
    ompi_libtoolize_search="$LIBTOOLIZE"
fi
ompi_automake_search="automake"
if test ! -z "$AUTOMAKE"; then
    ompi_automake_search="$AUTOMAKE"
fi

ompi_automake_version="1.9.6"
ompi_autoconf_version="2.59"
ompi_libtool_version="1.5.16"


##############################################################################
#
# Global variables - should not need to modify defaults
#
##############################################################################

ompi_aclocal_version="$ompi_automake_version"
ompi_autoheader_version="$ompi_autoconf_version"
ompi_libtoolize_version="$ompi_libtool_version"
ompi_autom4te_version="$ompi_autoconf_version"

# program names to execute
ompi_aclocal=""
ompi_autoheader=""
ompi_autoconf=""
ompi_libtoolize=""
ompi_automake=""

mca_no_configure_components_file="config/mca_no_configure_components.m4"
mca_no_config_list_file="mca_no_config_list"
mca_no_config_env_file="mca_no_config_env"
mca_m4_include_file="config/mca_m4_config_include.m4"
mca_m4_config_env_file="mca_m4_config_env"
autogen_subdir_file="autogen.subdirs"
topdir_file="opal/include/opal_config_bottom.h"

if echo a | grep -E '(a|b)' >/dev/null 2>&1; then
  egrep='grep -E'
else
  egrep=egrep
fi


############################################################################
#
# Version check - does major,minor,release check (hopefully ignoring
# beta et al)
#
# INPUT:
#    - minimum version allowable
#    - version we found
#
# OUTPUT:
#    - 0 version is ok
#    - 1 version is not ok
#
# SIDE EFFECTS:
#    none
#
##############################################################################
check_version() {
    local min_version="$1"
    local version="$2"

    local min_major_version="`echo $min_version | cut -f1 -d.`"
    local min_minor_version="`echo $min_version | cut -f2 -d.`"
    local min_release_version="`echo $min_version | cut -f3 -d.`"
    if test "$min_release_version" = "" ; then
        min_release_version=0
    fi

    local major_version="`echo $version | cut -f1 -d.`"
    local minor_version="`echo $version | cut -f2 -d.`"
    local release_version="`echo $version | cut -f3 -d.`"
    if test "$release_version" = "" ; then
        release_version=0
    fi

    if test $min_major_version -lt $major_version ; then
        return 0
    elif test $min_major_version -gt $major_version ; then
        return 1
    fi

    if test $min_minor_version -lt $minor_version ; then
        return 0
    elif test $min_minor_version -gt $minor_version ; then
        return 1
    fi

    if test $min_release_version -gt $release_version ; then
        return 1
    fi

    return 0
}


##############################################################################
#
# find app - find a version of the given application that is new
# enough for use
#
# INPUT:
#    - name of application (eg aclocal)
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#    - sets application_name variable to working executable name
#    - aborts on error finding application
#
##############################################################################
find_app() {
    local app_name="$1"

    local version="0.0.0"
    local min_version="99.99.99"
    local found=0
    local tmpIFS=$IFS

    eval "min_version=\"\$ompi_${app_name}_version\""
    eval "search_path=\"\$ompi_${app_name}_search\""
    IFS=";"
    for i in $search_path ; do
        IFS="$tmpIFS"
        version="`${i} --version 2>&1`"
        if test "$?" != 0 ; then
            IFS=";"
            continue
        fi

        version="`echo $version | cut -f2 -d')'`"
        version="`echo $version | cut -f1 -d' '`"

        if check_version $min_version $version ; then
            eval "ompi_${app_name}=\"${i}\""
            found=1
            break
        fi
    done

    IFS="$tmpIFS"

    if test "$found" = "0" ; then
	cat <<EOF
I could not find a recent enough copy of ${app_name}.
I am gonna abort.  :-(

Please make sure you are using at least the following versions of the
GNU tools:

GNU Autoconf $ompi_autoconf_version
GNU Automake $ompi_automake_version
    NOTE: You may need Automake 1.8.5 (or higher) in order to run
    "make dist" successfully
GNU Libtool  $ompi_libtool_version

EOF
        exit 1
    fi
}


##############################################################################
#
# run_and_check - run the right GNU tool, printing warning on failure
#
# INPUT:
#    - name of application (eg aclocal)
#    - program arguments
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#    - aborts on error running application
#
##############################################################################
run_and_check() {
    local rac_progs="$*"
    echo "[Running] $rac_progs"
    eval $rac_progs
    if test "$?" != 0; then
	cat <<EOF

-------------------------------------------------------------------------
It seems that the execution of "$rac_progs" has failed.  See above for
the specific error message that caused it to abort.
-------------------------------------------------------------------------

EOF
	exit 1
    fi
}

##############################################################################
#
# find_and_delete -  look for standard files in a number of common places
#     (e.g., ./config.guess, config/config.guess, dist/config.guess), and
#     delete it.  If it's not found there, look for AC_CONFIG_AUX_DIR in
#     the configure.in script and try there.  If it's not there, oh well.
#
# INPUT:
#    - file to delete
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#    - files may disappear
#
##############################################################################
find_and_delete() {
    local fad_file="$1"

    local fad_cfile
    local auxdir

    # Look for the file in "standard" places

    if test -f $fad_file; then
	rm -f $fad_file
    elif test -d config/$fad_file; then
	rm -f config/$fad_file
    elif test -d dist/$fad_file; then
	rm -f dist/$fad_file
    else

	# Didn't find it -- look for an AC_CONFIG_AUX_DIR line in
	# configure.[in|ac]

	if test -f configure.in; then
	    fad_cfile=configure.in
	elif test -f configure.ac; then
	    fad_cfile=configure.ac
	else
	    echo "--> Errr... there's no configure.in or configure.ac file!"
	fi
	if test -n "$fad_cfile"; then
	    auxdir="`grep AC_CONFIG_AUX_DIR $fad_cfile | $egrep -v '^dnl' | cut -d\( -f 2 | cut -d\) -f 1`"
	fi
	if test -f "$auxdir/$fad_file"; then
	    rm -f "$auxdir/$fad_file"
	fi
    fi
}


##############################################################################
#
# run_gnu_tools - run the GNU tools in a given directory
#
# INPUT:
#    - OMPI top directory
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#    - assumes that the directory is ready to have the GNU tools run
#      in it (i.e., there's some form of configure.*)
#    - may preprocess the directory before running the GNU tools
#      (e.g., generale Makefile.am's from configure.params, etc.)
#
##############################################################################
run_gnu_tools() {
    rgt_ompi_topdir="$1"

    # Sanity check to ensure that there's a configure.in or
    # configure.ac file here, or if there's a configure.params
    # file and we need to run make_configure.pl.

    if test -f configure.params -a -f configure.stub -a \
	-x "$rgt_ompi_topdir/config/mca_make_configure.pl"; then
        cat <<EOF
*** Found configure.stub
*** Running mca_make_configure.pl
EOF
	"$rgt_ompi_topdir/config/mca_make_configure.pl" \
	    --ompidir "$rgt_ompi_topdir" \
	    --componentdir "`pwd`"
        if test "$?" != "0"; then
           echo "*** autogen.sh failed to complete!"
           exit 1
        fi

        # If we need to make a version header template file, do so

        rgt_abs_dir="`pwd`"
        rgt_component_name=`basename "$rgt_abs_dir"`
        rgt_component_type=`dirname "$rgt_abs_dir"`
        rgt_component_type=`basename "$rgt_component_type"`
        rgt_ver_header="$rgt_abs_dir/$rgt_component_type-$rgt_component_name-version.h"
        rgt_ver_header_base=`basename "$rgt_ver_header"`
        make_version_header_template "$rgt_ver_header_base" "$rgt_component_type" "$rgt_component_name"

	happy=1
	file=configure.ac
    elif test -f configure.in; then
	happy=1
	file=configure.in
    elif test -f configure.ac; then
	happy=1
	file=configure.ac
    else
        cat <<EOF
--> Err... there's no configure.in or configure.ac file in this directory
--> Confused; aborting in despair
EOF
	exit 1
    fi
    unset happy

    # Find and delete the GNU helper script files

    find_and_delete config.guess
    find_and_delete config.sub
    find_and_delete depcomp
    find_and_delete compile
    find_and_delete install-sh
    find_and_delete ltconfig
    find_and_delete ltmain.sh
    find_and_delete missing
    find_and_delete mkinstalldirs
    find_and_delete libtool

    # Run the GNU tools

    echo "*** Running GNU tools"

    if test -f $topdir_file ; then
	cd config
	run_and_check $ompi_autom4te --language=m4sh ompi_get_version.m4sh -o ompi_get_version.sh
	cd ..
    fi

    run_and_check $ompi_aclocal
    if test "`grep AC_CONFIG_HEADER $file`" != "" -o \
	"`grep AM_CONFIG_HEADER $file`" != ""; then
	run_and_check $ompi_autoheader
    fi
    run_and_check $ompi_autoconf

    # We only need the libltdl stuff for the top-level
    # configure, not any of the MCA components.

    if test -f $topdir_file ; then
	rm -rf libltdl opal/libltdl opal/ltdl.h
	run_and_check $ompi_libtoolize --automake --copy --ltdl
        if test -d libltdl; then
            mv libltdl opal
        fi
        if test ! -r opal/libltdl/ltdl.h; then
            cat <<EOF
$ompi_libtoolize --ltdl apparently failed to install libltdl.
Please check that you have the libltdl development files installed.
EOF
            exit 1
        fi

	echo "Adjusting libltdl for OMPI :-("

	echo "  -- patching for argz bugfix in libtool 1.5"
	cd opal/libltdl
        if test "`grep 'while ((before >= *pargz) && (before[-1] != LT_EOS_CHAR))' ltdl.c`" != ""; then
            patch -N -p0 <<EOF
--- ltdl.c.old  2003-11-26 16:42:17.000000000 -0500
+++ ltdl.c      2003-12-03 17:06:27.000000000 -0500
@@ -682,7 +682,7 @@
   /* This probably indicates a programmer error, but to preserve
      semantics, scan back to the start of an entry if BEFORE points
      into the middle of it.  */
-  while ((before >= *pargz) && (before[-1] != LT_EOS_CHAR))
+  while ((before > *pargz) && (before[-1] != LT_EOS_CHAR))
     --before;

   {
EOF
#'
        else
            echo "     ==> your libtool doesn't need this! yay!"
        fi
	cd ../..
	echo "  -- patching configure for broken -c/-o compiler test"
	sed -e 's/chmod -w \./#OMPI\/MPI FIX: chmod -w ./' \
	    configure > configure.new
	mv configure.new configure
	chmod a+x configure
    else
	run_and_check $ompi_libtoolize --automake --copy
    fi
    run_and_check $ompi_automake --foreign -a --copy --include-deps
}



##############################################################################
#
# run_no_configure_component
#   Prepares the non-configure component
#
# INPUT:
#    - OMPI top directory
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#
##############################################################################
run_no_configure_component() {
    noconf_dir="$1"
    noconf_ompi_topdir="$2"
    noconf_project="$3"
    noconf_framework="$4"
    noconf_component="$5"

    # Write out to two files (they're merged at the end)
    noconf_list_file="$noconf_ompi_topdir/$mca_no_config_list_file"
    noconf_env_file="$noconf_ompi_topdir/$mca_no_config_env_file"

    cat >> "$noconf_list_file" <<EOF
dnl ----------------------------------------------------------------

dnl No-configure component: 
dnl    $noconf_dir

EOF

    # Tell configure to add all the PARAM_CONFIG_FILES to
    # the AC_CONFIG_FILES list.
    for file in $PARAM_CONFIG_FILES; do
        echo "AC_CONFIG_FILES([$noconf_dir/$file])" >> "$noconf_list_file"
    done

    cat <<EOF
--> Adding to top-level configure no-configure subdirs:
-->   $noconf_dir
--> Adding to top-level configure AC_CONFIG_FILES list:
-->   $PARAM_CONFIG_FILES
EOF

    echo "$PARAM_CONFIG_PRIORITY $noconf_component" >> "$noconf_env_file"
}


##############################################################################
#
# run_m4_configure_component
#   Prepares the component with an .m4 file that should be used to
#   configure the component.
#
# INPUT:
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#
##############################################################################
run_m4_configure_component() {
    m4conf_dir="$1"
    m4conf_ompi_topdir="$2"
    m4conf_project="$3"
    m4conf_framework="$4"
    m4conf_component="$5"

    # Write out to two files (they're merged at the end)
    m4conf_list_file="$m4conf_ompi_topdir/$mca_no_config_list_file"
    m4conf_env_file="$m4conf_ompi_topdir/$mca_m4_config_env_file"

    cat >> "$m4conf_list_file" <<EOF
dnl ----------------------------------------------------------------

dnl m4-configure component: 
dnl    $m4conf_dir

EOF

    # Tell configure to add all the PARAM_CONFIG_FILES to
    # the AC_CONFIG_FILES list.
    for file in $PARAM_CONFIG_FILES; do
        echo "AC_CONFIG_FILES([$m4conf_dir/$file])" >> "$m4conf_list_file"
    done

    # add the m4_include of the m4 file into the mca .m4 file
    # directly.  It shouldn't be in a macro, so this is fairly safe to
    # do.  By this point, there should already be a header and all
    # that.  m4_includes are relative to the currently included file,
    # so need the .. to get us from config/ to the topsrcdir again.
    echo "m4_include(${m4conf_project}/mca/${m4conf_framework}/${m4conf_component}/configure.m4)" >> "$m4conf_ompi_topdir/$mca_m4_include_file"

    cat <<EOF
--> Adding to top-level configure m4-configure subdirs:
-->   $m4conf_dir
--> Adding to top-level configure AC_CONFIG_FILES list:
-->   $PARAM_CONFIG_FILES
EOF

    echo "$PARAM_CONFIG_PRIORITY $m4conf_component" >> "$m4conf_env_file"
}


##############################################################################
#
# process_dir - look at the files present in a given directory, and do
# one of the following:
#    - skip/ignore it
#    - run custom autogen.sh in it
#    - run the GNU tools in it
#    - get a list of Makefile.am's to add to the top-level configure
#
# INPUT:
#    - directory to run in
#    - OMPI top directory
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#    - skips directories with .ompi_no_gnu .ompi_ignore
#    - uses provided autogen.sh if available
#
##############################################################################
process_dir() {
    pd_dir="$1"
    pd_ompi_topdir="$2"
    pd_project="$3"
    pd_framework="$4"
    pd_component="$5"

    pd_cur_dir="`pwd`"

    # Convert to absolutes

    if test -d "$pd_dir"; then
        cd "$pd_dir"
        pd_abs_dir="`pwd`"
        cd "$pd_cur_dir"
    fi

    if test -d "$pd_ompi_topdir"; then
        cd "$pd_ompi_topdir"
        pd_ompi_topdir="`pwd`"
        cd "$pd_cur_dir"
    fi

    # clean our environment a bit, since we might evaluate a configure.params
    unset PARAM_CONFIG_FILES
    PARAM_CONFIG_PRIORITY="0"

    if test -d "$pd_dir"; then
	cd "$pd_dir"

	# See if the package doesn't want us to set it up

	if test -f .ompi_no_gnu; then
	    cat <<EOF

*** Found .ompi_no_gnu file -- skipping GNU setup in:
***   `pwd`

EOF
	elif test -f .ompi_ignore -a ! -f .ompi_unignore; then

            # Note that if we have an empty (but existant)
            # .ompi_unignore, then we ignore the .ompi_ignore file
            # (and therefore build the component)

	    cat <<EOF

*** Found .ompi_ignore file -- skipping entire tree:
***   `pwd`

EOF

        # Use && instead of -a here for the test conditions because if
        # you use -a, then "test" will execute *all* condition clauses
        # (even if the first one is false), meaning that grep will
        # fail if there is no .ompi_unignore file.  If you use &&,
        # then the latter tests will not be executed if a prior one
        # fails (i.e., grep won't run if .ompi_unignore does not
        # exist).

        elif test -f .ompi_ignore && \
             test -s .ompi_unignore && \
             test -z "`$egrep $USER\$\|$USER@$HOST .ompi_unignore`" ; then

            # If we have a non-empty .ompi_unignore and our username
            # is in there somewhere, we ignore the .ompi_ignore (and
            # therefore build the component).  Otherwise, this
            # condition is true and we don't configure.

	    cat <<EOF

*** Found .ompi_ignore file and .ompi_unignore didn't invalidate -- 
*** skipping entire tree:
***   `pwd`

EOF
	elif test "$pd_abs_dir" != "$pd_ompi_topdir" -a -x autogen.sh; then
	    cat <<EOF

*** Found custom autogen.sh file in:
***   `pwd`

EOF
	    ./autogen.sh
            if test ! $? -eq 0 ; then
                echo "Error running autogen.sh -l in `pwd`.  Aborting."
                exit 1
            fi
        elif test -f configure.params -a -f configure.m4 ; then
            cat <<EOF

*** Found configure.params and configure.m4
***   `pwd`

EOF
            . ./configure.params
            if test -z "$PARAM_CONFIG_FILES"; then
                cat <<EOF
*** No PARAM_CONFIG_FILES!
*** Nothing to do -- skipping this directory
EOF
            else
                # temporary workaround - remove possibly there configure code
                rm -f "configure" "configure.ac*" "acinclude*" "aclocal.m4"

                run_m4_configure_component "$pd_dir" "$pd_ompi_topdir" \
                    "$pd_project" "$pd_framework" "$pd_component"
            fi

        elif test -f configure.ac -o -f configure.in; then
            # If we have configure.ac or configure.in, run the GNU
            # tools here

	    cat <<EOF

*** Found configure.(in|ac)
***   `pwd`

EOF
            run_gnu_tools "$pd_ompi_topdir"

        elif test -f configure.params -a -f configure.stub; then
	    cat <<EOF

*** Found configure.params and configure.stub
***   `pwd`

EOF
            run_gnu_tools "$pd_ompi_topdir"

        elif test -f configure.params; then
	    cat <<EOF

*** Found configure.params
***   `pwd`

EOF
            . ./configure.params
            if test -z "$PARAM_CONFIG_FILES"; then
                cat <<EOF
*** No PARAM_CONFIG_FILES!
*** Nothing to do -- skipping this directory
EOF
            else
                run_no_configure_component "$pd_dir" "$pd_ompi_topdir" \
                    "$pd_project" "$pd_framework" "$pd_component"
            fi
        else
	    cat <<EOF

*** Nothing found; directory skipped
***   `pwd`

EOF
        fi

        # See if there's a file containing additional directories to
        # traverse.  Shell scripts aren't too good at recursion (no
        # local state!), so just have a child autogen.sh do the work.

        if test -f $autogen_subdir_file; then
            pd_subdir_start_dir="`pwd`"
            echo ""
            echo "==> Found $autogen_subdir_file -- sub-traversing..."
            echo ""
            for dir in `cat $autogen_subdir_file`; do
                if test -d "$dir"; then
                    echo "*** Running autogen.sh in $dir"
                    echo "***   (started in $pd_subdir_start_dir)"
                    cd "$dir"
                    $pd_ompi_topdir/autogen.sh -l
                    if test ! $? -eq 0 ; then
                        echo "Error running autogen.sh -l in $dir.  Aborting."
                        exit 1
                    fi
                    cd "$pd_subdir_start_dir"
                    echo ""
                fi
            done
            echo "<== Back in $pd_subdir_start_dir"
            echo "<== autogen.sh continuing..."
        fi

        # Go back to the topdir

        cd "$pd_cur_dir"
    fi
    unset PARAM_CONFIG_FILES PARAM_VERSION_FILE
    unset pd_dir pd_ompi_topdir pd_cur_dir pd_component_type
}


##############################################################################
#
# make_template_version_header -- make a templated version header
# file, but only if we have a PARAM_VERSION_FILE that exists
#
# INPUT:
#    - filename base
#    - component type name
#    - component name
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#
##############################################################################
make_version_header_template() {
    mvht_filename="$1"
    mvht_component_type="$2"
    mvht_component_name="$3"

    # See if we have a VERSION file

    PARAM_CONFIG_FILES_save="$PARAM_CONFIG_FILES"
    . ./configure.params
    if test -z "$PARAM_VERSION_FILE"; then
        if test -f "VERSION"; then
            PARAM_VERSION_FILE="VERSION"
        fi
    else
        if test ! -f "$PARAM_VERSION_FILE"; then
            PARAM_VERSION_FILE=
        fi
    fi

    if test -n "$PARAM_VERSION_FILE" -a -f "$PARAM_VERSION_FILE" -a \
        "$pd_component_type" != "common"; then
        rm -f "$mvht_filename.template.in"
        cat > "$mvht_filename.template.in" <<EOF
/*
 * This file is automatically created by autogen.sh; it should not
 * be edited by hand!!
 *
 * List of version number for this component
 */

#ifndef MCA_${mvht_component_type}_${mvht_component_name}_VERSION_H
#define MCA_${mvht_component_type}_${mvht_component_name}_VERSION_H

#define MCA_${mvht_component_type}_${mvht_component_name}_MAJOR_VERSION @MCA_${mvht_component_type}_${mvht_component_name}_MAJOR_VERSION@
#define MCA_${mvht_component_type}_${mvht_component_name}_MINOR_VERSION @MCA_${mvht_component_type}_${mvht_component_name}_MINOR_VERSION@
#define MCA_${mvht_component_type}_${mvht_component_name}_RELEASE_VERSION @MCA_${mvht_component_type}_${mvht_component_name}_RELEASE_VERSION@
#define MCA_${mvht_component_type}_${mvht_component_name}_GREEK_VERSION "@MCA_${mvht_component_type}_${mvht_component_name}_GREEK_VERSION@"
#define MCA_${mvht_component_type}_${mvht_component_name}_SVN_VERSION "@MCA_${mvht_component_type}_${mvht_component_name}_SVN_VERSION@"
#define MCA_${mvht_component_type}_${mvht_component_name}_VERSION "@MCA_${mvht_component_type}_${mvht_component_name}_VERSION@"

#endif /* MCA_${mvht_component_type}_${mvht_component_name}_VERSION_H */
EOF
    fi
    PARAM_CONFIG_FILES="$PARAM_CONFIG_FILES_save"
    unset PARAM_VERSION_FILE PARAM_CONFIG_FILES_save
}

component_list_sort() {
    cls_filename="$1"

    # why, oh, why can't non-gnu sort support the -s (stable) option?
    # Solaris sort supports -r -n and -u, so we'll assume that works everywhere

    # get the list of priorities
    component_list=
    cls_priority_list=`sort -r -n -u "$cls_filename" | cut -f1 -d' ' | xargs`
    for cls_priority in $cls_priority_list ; do
        component_list="$component_list "`grep "^$cls_priority " "$cls_filename" | cut -f2 -d' ' | xargs`
    done
}


##############################################################################
#
# run_global - run the config in the top OMPI dir and all MCA components
#
# INPUT:
#    none
#
# OUTPUT:
#    none
#
# SIDE EFFECTS:
#
##############################################################################
run_global() {
    # [Re-]Create the mca_component_list file

    rm -f "$mca_no_configure_components_file" "$mca_no_config_list_file" \
        "$mca_no_config_env_file" "$mca_m4_config_env_file" "$mca_m4_include_file"
    touch "$mca_no_configure_components_file" "$mca_no_config_list_file" \
        "$mca_m4_config_env_file" "$mca_m4_include_file"

    # create header for the component m4 include file
    cat > "$mca_m4_include_file" <<EOF
dnl
dnl \$HEADER
dnl

dnl This file is automatically created by autogen.sh; it should not
dnl be edited by hand!!

EOF

    #create header for the component config file
    cat > "$mca_no_configure_components_file" <<EOF
dnl
dnl \$HEADER
dnl

dnl This file is automatically created by autogen.sh; it should not
dnl be edited by hand!!

EOF

    # Now run the config in every directory in <location>/mca/*/*
    # that has a configure.in or configure.ac script
    #
    # In order to deal with components that have .m4 files, we need to
    # build up m4_defined lists along the way.  Unfortunately, there
    # is no good way to do this at the end (stupid sh), so we have to
    # do it as we are going through the lists of frameworks and
    # components.  Use a file to keep the list of components (we don't
    # want every component in a framework included, as the
    # determination about skipping components or whether the component
    # has its own configure script are made later on in the process.

    rg_cwd="`pwd`"
    echo $rg_cwd
    project_list=""
    for project_path in $config_project_list; do 
        project=`basename "$project_path"`
        project_list="$project_list $project"

        framework_list=""
        for framework_path in $project_path/mca/*; do
            framework=`basename "$framework_path"`

	    if test "$framework" != "base" -a \
                -d "$framework_path" ; then
                if test "$framework" = "common" -o \
                    -r "${framework_path}/${framework}.h" ; then
                    framework_list="$framework_list $framework"

                    # Add the framework's configure file into configure,
                    # if there is one
                    if test -r "${framework_path}/configure.m4" ; then
                        echo "m4_include(${framework_path}/configure.m4)" >> "$mca_m4_include_file"
                    fi

                    rm -f "$mca_no_config_env_file" "$mca_m4_config_env_file"
                    touch "$mca_no_config_env_file" "$mca_m4_config_env_file"

                    for component_path in "$framework_path"/*; do
                        if test -d "$component_path"; then
                            if test -f "$component_path/configure.in" -o \
                                -f "$component_path/configure.params" -o \
                                -f "$component_path/configure.ac"; then

                                component=`basename "$component_path"`

                                process_dir "$component_path" "$rg_cwd" \
                                    "$project" "$framework" "$component"
                            fi
                        fi
                    done
                fi

                # make list of components that are "no configure".
                # Sort the list by priority (stable, so things stay in
                # alphabetical order at the same priority), then munge
                # it into form we like
                component_list=
                component_list_sort $mca_no_config_env_file
                component_list_define="m4_define(mca_${framework}_no_config_component_list, ["
                component_list_define_first="1"
                for component in $component_list ; do
                    if test "$component_list_define_first" = "1"; then
                        component_list_define="${component_list_define}${component}"
                        component_list_define_first="0"
                    else
                        component_list_define="${component_list_define}, ${component}"
                    fi
                done
                component_list_define="${component_list_define}])"
                echo "$component_list_define" >> "$mca_no_configure_components_file"

                # make list of components that are "m4 configure"
                component_list=
                component_list_sort $mca_m4_config_env_file
                component_list_define="m4_define(mca_${framework}_m4_config_component_list, ["
                component_list_define_first="1"
                for component in $component_list ; do
                    if test "$component_list_define_first" = "1"; then
                        component_list_define="${component_list_define}${component}"
                        component_list_define_first="0"
                    else
                        component_list_define="${component_list_define}, ${component}"
                    fi
                done
                component_list_define="${component_list_define}])"
                echo "$component_list_define" >> "$mca_no_configure_components_file"
	    fi
        done

        # make list of frameworks for this project
        framework_list_define="m4_define(mca_${project}_framework_list, ["
        framework_list_define_first="1"
        for framework in $framework_list ; do
            if test "$framework_list_define_first" = "1"; then
                framework_list_define="${framework_list_define}${framework}"
                framework_list_define_first="0"
            else
                framework_list_define="${framework_list_define}, ${framework}"
            fi
        done
        framework_list_define="${framework_list_define}])"
        echo "$framework_list_define" >> "$mca_no_configure_components_file"

    done

    # create the m4 defines for the list of projects.  The list of
    # frameworks for each project is already created and in the file.
    project_list_define="m4_define(mca_project_list, ["
    project_list_define_first="1"
    for project in $project_list ; do
        if test "$project_list_define_first" = "1"; then
            project_list_define="${project_list_define}${project}"
            project_list_define_first="0"
        else
            project_list_define="${project_list_define}, ${project}"
        fi
    done
    project_list_define="${project_list_define}])"
    echo "$project_list_define" >> "$mca_no_configure_components_file"


    cat >> "$mca_no_configure_components_file" <<EOF

dnl List all the no-configure components that we found, and AC_DEFINE
dnl their versions

AC_DEFUN([MCA_NO_CONFIG_CONFIG_FILES],[

`cat $mca_no_config_list_file`
])dnl
EOF

    # Remove temp files
    rm -f $mca_no_config_list_file $mca_no_config_env_file $mca_m4_config_env_file

    # Finally, after we found all the no-configure MCA components, run
    # the config in the top-level directory

    process_dir . . "" "" ""

    unset project project_path framework framework_path component component_path
}


##############################################################################
#
# main - do the real work...
#
##############################################################################

# announce
echo "[Checking] command line parameters"

# Check the command line to see if we should run the whole shebang, or
# just in this current directory.

want_local=no
ompidir=
no_ompi=0
no_orte=0
for arg in $*; do
    case $arg in
    -l) want_local=yes ;;
    -ompidir|--ompidir|-ompi|--ompi) ompidir="$2" ;;
    -no-ompi) no_ompi=1 ;;
    -no-orte) no_orte=1 ;;
    *) ;;
    esac
    shift
done

# announce
echo "[Checking] prerequisites"

# sanity check to make sure user isn't being stupid
if test ! -d .svn ; then
    cat <<EOF

This doesn't look like a developer copy of Open MPI.  You probably do not
want to run autogen.sh - it is normally not needed for a release source
tree.  Giving you 5 seconds to reconsider and kill me.

EOF
    sleep 5
fi

# figure out if we're at the top level of the OMPI tree, a component's
# top-level directory, or somewhere else.
if test -f VERSION -a -f configure.ac -a -f $topdir_file ; then
    # locations to look for mca modules
    config_project_list="opal orte ompi"
    if test "$no_ompi" = "1" ; then
        config_project_list="opal orte"
    fi
    if test "$no_orte" = "1" ; then
        config_project_list="opal"
    fi
    echo "Configuring projects: $config_project_list"

    # Top level of OMPI tree
    ompidir="`pwd`"
elif test -f configure.in -o -f configure.ac -o -f configure.params ; then
    # Top level of a component directory
    want_local=yes
    if test -z "$ompidir"; then
        ompidir="../../../.."
    fi
else
    cat <<EOF

You must run this script from either the top level of the OMPI
directory tree or the top-level of an MCA component directory tree.

EOF
    exit 1
fi

# find all the apps we are going to run
find_app "autom4te"
find_app "aclocal"
find_app "autoheader"
find_app "autoconf"
find_app "libtoolize"
find_app "automake"

# do the work
if test "$want_local" = "yes"; then
    process_dir . $ompidir
else
    run_global
fi

# All done
exit 0
