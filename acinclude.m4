dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2004-2005 The Trustees of Indiana University.
dnl                         All rights reserved.
dnl Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
dnl                         All rights reserved.
dnl Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
dnl                         University of Stuttgart.  All rights reserved.
dnl Copyright (c) 2004-2005 The Regents of the University of California.
dnl                         All rights reserved.
dnl $COPYRIGHT$
dnl 
dnl Additional copyrights may follow
dnl 
dnl $HEADER$
dnl


#
# Open MPI-specific tests
#

sinclude(config/c_get_alignment.m4)
sinclude(config/c_weak_symbols.m4)

sinclude(config/cxx_find_template_parameters.m4)
sinclude(config/cxx_find_template_repository.m4)
sinclude(config/cxx_have_exceptions.m4)
sinclude(config/cxx_find_exception_flags.m4)

sinclude(config/f77_check_type.m4)
sinclude(config/f77_find_ext_symbol_convention.m4)
sinclude(config/f77_get_alignment.m4)
sinclude(config/f77_get_fortran_handle_max.m4)
sinclude(config/f77_get_sizeof.m4)

sinclude(config/f90_check_type.m4)
sinclude(config/f90_find_module_include_flag.m4)
sinclude(config/f90_get_alignment.m4)
sinclude(config/f90_get_precision.m4)
sinclude(config/f90_get_range.m4)
sinclude(config/f90_get_sizeof.m4)

sinclude(config/ompi_try_assemble.m4)
sinclude(config/ompi_config_asm.m4)

sinclude(config/ompi_case_sensitive_fs_setup.m4)
sinclude(config/ompi_check_optflags.m4)
sinclude(config/ompi_config_subdir.m4)
sinclude(config/ompi_config_subdir_args.m4)
sinclude(config/ompi_configure_options.m4)
sinclude(config/ompi_find_type.m4)
sinclude(config/ompi_functions.m4)
sinclude(config/ompi_get_version.m4)
sinclude(config/ompi_get_libtool_linker_flags.m4)
sinclude(config/ompi_mca.m4)
sinclude(config/ompi_setup_cc.m4)
sinclude(config/ompi_setup_cxx.m4)
sinclude(config/ompi_setup_f77.m4)
sinclude(config/ompi_setup_f90.m4)
sinclude(config/ompi_setup_libevent.m4)

sinclude(config/ompi_check_pthread_pids.m4)
sinclude(config/ompi_config_pthreads.m4)
sinclude(config/ompi_config_solaris_threads.m4)
sinclude(config/ompi_config_threads.m4)

#
# The config/mca_no_configure_components.m4 file is generated by
# autogen.sh
#

sinclude(config/mca_no_configure_components.m4)
