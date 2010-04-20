# Copyright (c) 2007-2010 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

INCLUDE(list_subdirs)

# there are several steps and issues for checking mca components:
#
#   1.   go through each framwork dir, add the top-level headers
#        to the source list.
#
#   2.   go through each component dir, the framwork base dir is checked
#        also in this setp, and is added to the source list.
#
#    2a.   if a .windows file exists in a framwork base dir, that means there
#          might be properties that have to be checked for this dir, e.g. files
#           need to be excluded.
#
#    2b.   if a .windows file exists in a component dir, this component
#          should be included in the solution. There could be properties that
#          need to check for this component. If it's a static build or not a
#          DSO shared build, just add necessary sources; if it is a DSO build,
#          generate a proper CMakeLists.txt file for each component, so that
#          this component will be compiled separately.
#
#   3.   Generate static-components.h file with available mca components.
#
#
#   Available properties in .windows files:
#
#       exclude_list:       files that need to be excluded from the solution.
#
#       required_check:     a CMake module has to be run to check the libraries/headers
#                           that needed by this component.
#
#       not_single_shared_lib:   this component should not be built separately, it's not 
#                                a single mca shared library.
#
#       mca_link_libraries: this component has to be linked with other targets or libraries,
#                           e.g. Ws2_32.lib


SET(MCA_FRAMEWORK_LIST "")
CHECK_SUBDIRS("${PROJECT_SOURCE_DIR}/mca" MCA_FRAMEWORK_LIST)
#MESSAGE("MCA_FRAMEWORK_LIST:${MCA_FRAMEWORK_LIST}")


FILE(GLOB ${PROJECT_NAME}_MCA_HEADER_FILES "mca/*.C" "mca/*.h")
SET(${PROJECT_NAME}_MCA_FILES ${${PROJECT_NAME}_MCA_FILES} ${${PROJECT_NAME}_MCA_HEADER_FILES})
SOURCE_GROUP(mca FILES ${${PROJECT_NAME}_MCA_HEADER_FILES})

# clear the variables first
SET(MCA_FRAMEWORK "")
SET(MCA_FRAMEWORK_FILES "")
SET(MCA_FILES "")

# parse each mca subdir
FOREACH (MCA_FRAMEWORK ${MCA_FRAMEWORK_LIST})

  IF(NOT ${MCA_FRAMEWORK} STREQUAL "CMakeFiles" AND NOT ${MCA_FRAMEWORK} STREQUAL "svn")
    #SET(CURRENT_PATH "mca/${${PROJECT_NAME}_MCA_SUBDIR}")
    FILE(GLOB MCA_FRAMEWORK_FILES "mca/${MCA_FRAMEWORK}/*.c" "mca/${MCA_FRAMEWORK}/*.h"
                                  "mca/${MCA_FRAMEWORK}/*.cc" "mca/${MCA_FRAMEWORK}/*.cpp")
    SET(MCA_FILES ${MCA_FILES} ${MCA_FRAMEWORK_FILES})
    SOURCE_GROUP(mca\\${MCA_FRAMEWORK} FILES ${MCA_FRAMEWORK_FILES})
    
    SET(COMPONENT_LIST "")
    CHECK_SUBDIRS("${PROJECT_SOURCE_DIR}/mca/${MCA_FRAMEWORK}" COMPONENT_LIST)

    SET(CURRENT_COMPONENT_PRIORITY 0)
    SET(BEST_COMPONENT_PRIORITY 0)

    # parse each component subdir of current mca framework
    FOREACH (MCA_COMPONENT ${COMPONENT_LIST})

      IF(${MCA_COMPONENT} STREQUAL "base")

        SET(CURRENT_PATH "${PROJECT_SOURCE_DIR}/mca/${MCA_FRAMEWORK}/base")
        FILE(GLOB MCA_FRAMEWORK_BASE_FILES "${CURRENT_PATH}/*.c" "${CURRENT_PATH}/*.h"
                                           "${CURRENT_PATH}/*.cc" "${CURRENT_PATH}/*.cpp")

        IF(EXISTS "${CURRENT_PATH}/.windows")

          #MESSAGE("MCA_FRAMEWORK_BASE_FILES:${MCA_FRAMEWORK_BASE_FILES}")
          SET(EXCLUDE_LIST "")
          FILE(STRINGS ${CURRENT_PATH}/.windows EXCLUDE_LIST REGEX "^exclude_list=")

          IF(NOT EXCLUDE_LIST STREQUAL "")
            STRING(REPLACE "exclude_list=" "" EXCLUDE_LIST ${EXCLUDE_LIST})
          ENDIF(NOT EXCLUDE_LIST STREQUAL "")

          # remove the files in the exclude list
          FOREACH(FILE ${EXCLUDE_LIST})
            LIST(REMOVE_ITEM MCA_FRAMEWORK_BASE_FILES "${CURRENT_PATH}/${FILE}")
          ENDFOREACH(FILE)

        ENDIF(EXISTS "${CURRENT_PATH}/.windows")

        SET_SOURCE_FILES_PROPERTIES(${PROJECT_BINARY_DIR}/mca/${MCA_FRAMEWORK}/base/static-components.h 
          PROPERTIES GENERATED true)
        SET(MCA_FRAMEWORK_BASE_FILES ${MCA_FRAMEWORK_BASE_FILES}
          ${PROJECT_BINARY_DIR}/mca/${MCA_FRAMEWORK}/base/static-components.h)
        SET(MCA_FILES ${MCA_FILES} ${MCA_FRAMEWORK_BASE_FILES})

        SOURCE_GROUP(mca\\${MCA_FRAMEWORK}\\base FILES ${MCA_FRAMEWORK_BASE_FILES})

        # Install help files if they are here.
        INSTALL(DIRECTORY ${CURRENT_PATH}/ DESTINATION share/openmpi/
          FILES_MATCHING PATTERN "*.txt"
          PATTERN ".svn" EXCLUDE
          PATTERN ".hg" EXCLUDE)

      ELSEIF(EXISTS "${PROJECT_SOURCE_DIR}/mca/${MCA_FRAMEWORK}/${MCA_COMPONENT}/.windows")

        SET(COMPONENT_FILES "")
        SET(CURRENT_PATH ${PROJECT_SOURCE_DIR}/mca/${MCA_FRAMEWORK}/${MCA_COMPONENT})
        FILE(GLOB COMPONENT_FILES "${CURRENT_PATH}/*.C" "${CURRENT_PATH}/*.h"
          "${CURRENT_PATH}/*.cc" "${CURRENT_PATH}/*.cpp")

        # by default, build this component.
        SET(BUILD_COMPONENT TRUE)

        # do we have to run a check module first?
        SET(REQUIRED_CHECK "")
        FILE(STRINGS ${CURRENT_PATH}/.windows REQUIRED_CHECK REGEX "^required_check=")
        
        SET(EXTRA_INCLUDE_PATH "")
        IF(NOT REQUIRED_CHECK STREQUAL "")
          STRING(REPLACE "required_check=" "" REQUIRED_CHECK ${REQUIRED_CHECK})
          UNSET(RESULT_INCLUDE_PATH)
          INCLUDE(${REQUIRED_CHECK})
          IF(RESULT)
            SET(EXTRA_INCLUDE_PATH ${RESULT_INCLUDE_PATH})
          ELSE(RESULT)
            # Required check failed, don't build this component.
            SET(BUILD_COMPONENT FALSE)
          ENDIF(RESULT)
        ENDIF(NOT REQUIRED_CHECK STREQUAL "")

        IF(BUILD_COMPONENT)

          # check the library build type
          FILE(STRINGS ${CURRENT_PATH}/.windows
                VALUE REGEX "^not_single_shared_lib=")
          IF(NOT VALUE STREQUAL "")
            STRING(REPLACE "not_single_shared_lib=" "" NOT_SINGLE_SHARED_LIB ${VALUE})
          ENDIF(NOT VALUE STREQUAL "")

          # check out if we have to exclude some source files.
          SET(EXCLUDE_LIST "")
          FILE(STRINGS ${CURRENT_PATH}/.windows EXCLUDE_LIST REGEX "^exclude_list=")
        
          IF(NOT EXCLUDE_LIST STREQUAL "")
            STRING(REPLACE "exclude_list=" "" EXCLUDE_LIST ${EXCLUDE_LIST})
          ENDIF(NOT EXCLUDE_LIST STREQUAL "")
        
          # remove the files in the exclude list
          FOREACH(FILE ${EXCLUDE_LIST})
            LIST(REMOVE_ITEM COMPONENT_FILES "${CURRENT_PATH}/${FILE}")
          ENDFOREACH(FILE)

          IF(NOT OPAL_WANT_LIBLTDL OR NOT_SINGLE_SHARED_LIB STREQUAL "1")
            SET(NOT_SINGLE_SHARED_LIB "")
            # add sources for static build or for the shared build when this is not a stand along library.
            SET(MCA_FILES ${MCA_FILES} ${COMPONENT_FILES})
            SOURCE_GROUP(mca\\${MCA_FRAMEWORK}\\${MCA_COMPONENT} FILES ${COMPONENT_FILES})

            INCLUDE_DIRECTORIES(${EXTRA_INCLUDE_PATH})

            IF(EXISTS "${CURRENT_PATH}/configure.params")
              FILE(STRINGS "${CURRENT_PATH}/configure.params" 
                CURRENT_COMPONENT_PRIORITY REGEX "PRIORITY")
              IF(NOT CURRENT_COMPONENT_PRIORITY STREQUAL "")
                STRING(REGEX REPLACE "[A-Z_]+=" "" CURRENT_COMPONENT_PRIORITY ${CURRENT_COMPONENT_PRIORITY})
              ENDIF(NOT CURRENT_COMPONENT_PRIORITY STREQUAL "")
            ENDIF(EXISTS "${CURRENT_PATH}/configure.params")

            IF(CURRENT_COMPONENT_PRIORITY GREATER BEST_COMPONENT_PRIORITY)
              # I have a higher priority for this mca, put me at the very beginning.
              SET (OUTFILE_EXTERN
                "extern const mca_base_component_t mca_${MCA_FRAMEWORK}_${MCA_COMPONENT}_component"  
                "\n${OUTFILE_EXTERN}")
              SET(FRAMEWORK_STRUCT_DEF
                "&mca_${MCA_FRAMEWORK}_${MCA_COMPONENT}_component,\n"  
                ${FRAMEWORK_STRUCT_DEF})
              SET(BEST_COMPONENT_PRIORITY ${CURRENT_COMPONENT_PRIORITY})
            ELSE(CURRENT_COMPONENT_PRIORITY GREATER BEST_COMPONENT_PRIORITY)
              SET (OUTFILE_EXTERN ${OUTFILE_EXTERN}
                "\nextern const mca_base_component_t mca_${MCA_FRAMEWORK}_${MCA_COMPONENT}_component;")
              SET(FRAMEWORK_STRUCT_DEF ${FRAMEWORK_STRUCT_DEF}
                "&mca_${MCA_FRAMEWORK}_${MCA_COMPONENT}_component,\n")
            ENDIF(CURRENT_COMPONENT_PRIORITY GREATER BEST_COMPONENT_PRIORITY)


          ELSE(NOT OPAL_WANT_LIBLTDL OR NOT_SINGLE_SHARED_LIB STREQUAL "1")

            # get the libraries required for this component.
            SET(MCA_LINK_LIBRARIES "")
            FILE(STRINGS ${CURRENT_PATH}/.windows VALUE REGEX "^mca_link_libraries=")
            IF(NOT VALUE STREQUAL "")
              STRING(REPLACE "mca_link_libraries=" "" MCA_LINK_LIBRARIES ${VALUE})
            ENDIF(NOT VALUE STREQUAL "")

            # the mca_common_* libraries should be installed into bin,
            # this will avoid the runtime open module failure.
            IF("${MCA_FRAMEWORK}" STREQUAL "common")
              SET(LIB_NAME_PREFIX "lib")
              SET(INSTALL_DEST "RUNTIME DESTINATION bin
                                LIBRARY DESTINATION lib
                                ARCHIVE DESTINATION lib")
              SET(PDB_DEST "bin")
            ELSE("${MCA_FRAMEWORK}" STREQUAL "common")
              SET(LIB_NAME_PREFIX "")
              IF(OMPI_DEBUG_BUILD)
                SET(INSTALL_DEST "RUNTIME DESTINATION lib/openmpi/debug
                                  LIBRARY DESTINATION lib/openmpi/debug
                                  ARCHIVE DESTINATION lib/openmpi/debug")
                SET(PDB_DEST "lib/openmpi/debug")
              ELSE(OMPI_DEBUG_BUILD)
                SET(INSTALL_DEST "RUNTIME DESTINATION lib/openmpi
                                  LIBRARY DESTINATION lib/openmpi
                                  ARCHIVE DESTINATION lib/openmpi")
              ENDIF(OMPI_DEBUG_BUILD)
            ENDIF("${MCA_FRAMEWORK}" STREQUAL "common")
            

            # generate CMakeLists.txt for each component for DSO build.
            FILE (WRITE "${PROJECT_BINARY_DIR}/mca/${MCA_FRAMEWORK}/${MCA_COMPONENT}/CMakeLists.txt"
              "
#
# Copyright (c) 2007-2008 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

# make new project for shared build

INCLUDE_DIRECTORIES(\${EXTRA_INCLUDE_PATH})

ADD_LIBRARY(${LIB_NAME_PREFIX}mca_${MCA_FRAMEWORK}_${MCA_COMPONENT} SHARED 
            \${COMPONENT_FILES})

SET_TARGET_PROPERTIES(${LIB_NAME_PREFIX}mca_${MCA_FRAMEWORK}_${MCA_COMPONENT}
                      PROPERTIES COMPILE_FLAGS \"-D_USRDLL -DOPAL_IMPORTS -DOMPI_IMPORTS -DORTE_IMPORTS /TP\")

TARGET_LINK_LIBRARIES (${LIB_NAME_PREFIX}mca_${MCA_FRAMEWORK}_${MCA_COMPONENT}
  libopen-pal ${MCA_LINK_LIBRARIES})

INSTALL(TARGETS ${LIB_NAME_PREFIX}mca_${MCA_FRAMEWORK}_${MCA_COMPONENT} ${INSTALL_DEST})
IF (OMPI_DEBUG_BUILD)
  INSTALL(FILES ${OpenMPI_BINARY_DIR}/Debug/${LIB_NAME_PREFIX}mca_${MCA_FRAMEWORK}_${MCA_COMPONENT}${CMAKE_DEBUG_POSTFIX}.pdb
    DESTINATION ${PDB_DEST})
ENDIF (OMPI_DEBUG_BUILD)
          ")

            ADD_SUBDIRECTORY (${PROJECT_BINARY_DIR}/mca/${MCA_FRAMEWORK}/${MCA_COMPONENT} mca/${MCA_FRAMEWORK}/${MCA_COMPONENT})
          ENDIF(NOT OPAL_WANT_LIBLTDL OR NOT_SINGLE_SHARED_LIB STREQUAL "1")

          # Install help files if they are here.
          INSTALL(DIRECTORY ${CURRENT_PATH}/ DESTINATION share/openmpi/
            FILES_MATCHING PATTERN "*.txt"
            PATTERN ".svn" EXCLUDE
            PATTERN ".hg" EXCLUDE)

        ENDIF(BUILD_COMPONENT)        
      ENDIF(${MCA_COMPONENT} STREQUAL "base")

    ENDFOREACH(MCA_COMPONENT)

    STRING(LENGTH "${FRAMEWORK_STRUCT_DEF}" STRUCT_STRING_LENTH)
    IF(STRUCT_STRING_LENTH GREATER 0)
      STRING (REPLACE ";" "" OUTFILE_STRUCT ${FRAMEWORK_STRUCT_DEF})
    ENDIF(STRUCT_STRING_LENTH GREATER 0)
    # write out static-component.h for this mca.
    FILE(WRITE "${PROJECT_BINARY_DIR}/mca/${MCA_FRAMEWORK}/base/static-components.h" 
      "/*
 * $HEADER$
 */
#if defined(c_plusplus) || defined(__cplusplus)
  extern \"C\" {
#endif

${OUTFILE_EXTERN}

const mca_base_component_t *mca_${MCA_FRAMEWORK}_base_static_components[] = {
  ${OUTFILE_STRUCT}
  NULL
};

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
  ")

  SET(OUTFILE_EXTERN "")
  SET(OUTFILE_STRUCT "")
  SET(FRAMEWORK_STRUCT_DEF "")
  ENDIF(NOT ${MCA_FRAMEWORK} STREQUAL "CMakeFiles" AND NOT ${MCA_FRAMEWORK} STREQUAL "svn")
ENDFOREACH (MCA_FRAMEWORK)
