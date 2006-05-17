<!--
 ...........................................................................
 Copyright (c) 2004-2006 The Regents of the University of California.
                         All rights reserved.
 $COPYRIGHT$
 
 Additional copyrights may follow
 
 $HEADER$
 ...........................................................................
 -->

<!--
  - chasm-mpi.f90.xsl: creates F90 functions that call MPI equivalent
  -
  - Output should be directed to the file, Method_f90.f90
  -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:import href="common.xsl"/>
<xsl:import href="common-C.xsl"/>
<xsl:import href="common-f90.xsl"/>

<xsl:output method="text"/>

<!-- global variables -->

<xsl:param name="test_function" select="unknown_function"/>

<xsl:param name="interface_size" select="medium"/>

<xsl:variable name="filename">
  <xsl:call-template name="lower-case">
    <xsl:with-param name="symbol" select="/library/scope/@name"/>
  </xsl:call-template>
  <xsl:text>_C.c</xsl:text>
</xsl:variable>

<xsl:variable name="header_file">
  <xsl:call-template name="lower-case">
    <xsl:with-param name="symbol" select="/library/scope/@name"/>
  </xsl:call-template>
  <xsl:text>_C.h</xsl:text>
</xsl:variable>


<!--
  - root level
  -->
<xsl:template match="/">
  <xsl:call-template name="openFile">
    <xsl:with-param name="filename" select="$filename"/>
  </xsl:call-template>

  <!-- output all include files -->

  <xsl:call-template name="include-files">
    <xsl:with-param name="filename" select="$filename"/>
  </xsl:call-template>

  <!-- define C bridging functions -->

  <xsl:apply-templates select="/library/scope/method[@name=$test_function]"/>

  <!--xsl:for-each select="library/scope">
    <xsl:call-template name="defineFunctions"/>
  </xsl:for-each-->

  <xsl:call-template name="closeFile">
    <xsl:with-param name="filename" select="$filename"/>
  </xsl:call-template>
</xsl:template>


<!--
  - method level: define program to call Fortran procedures>
  -->
<xsl:template match="method">
  <xsl:param name="module" select="../@name"/>
  <xsl:value-of select="$nl"/>
  <xsl:if test="@kind != 'No_F90'">
    <xsl:if test="@template = 'yes'">
      <xsl:call-template name="defineFunction"/>
    </xsl:if>
  </xsl:if>
</xsl:template>


<!--
  - defineFunctions: define functions to call Fortran procedures <scope>
  -->
<xsl:template name="defineFunctions">
  <xsl:param name="module" select="@name"/>
  <xsl:value-of select="$nl"/>
  <xsl:for-each select="method">
    <xsl:if test="@kind != 'No_F90'">
      <xsl:if test="@template = 'yes'">
        <xsl:call-template name="defineFunction"/>
      </xsl:if>
    </xsl:if>
  </xsl:for-each>
</xsl:template>


<!--
  - defineFunction: define function to call Fortran procedures <method>
  -->
<xsl:template name="defineFunction">
  <xsl:param name="method" select="@name"/>

    <xsl:text>
output() {
    procedure=$1
    rank=$2
    type=$4
    proc="$1$2D$3"

    cat &lt;&lt;EOF

</xsl:text>

      <xsl:call-template name="defineArrayFunctionBody"/>

<xsl:text>
EOF
}

for rank in $allranks
do
  case "$rank" in  0)  dim=''  ;  esac
  case "$rank" in  1)  dim=', dimension(:)'  ;  esac
  case "$rank" in  2)  dim=', dimension(:,:)'  ;  esac
  case "$rank" in  3)  dim=', dimension(:,:,:)'  ;  esac
  case "$rank" in  4)  dim=', dimension(:,:,:,:)'  ;  esac
  case "$rank" in  5)  dim=', dimension(:,:,:,:,:)'  ;  esac
  case "$rank" in  6)  dim=', dimension(:,:,:,:,:,:)'  ;  esac
  case "$rank" in  7)  dim=', dimension(:,:,:,:,:,:,:)'  ;  esac

</xsl:text>

    <xsl:text>  output </xsl:text> <xsl:value-of select="@name"/>
    <xsl:text> ${rank} CH "character${dim}"</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>  output </xsl:text> <xsl:value-of select="@name"/>
    <xsl:text> ${rank} L "logical${dim}"</xsl:text>
    <xsl:value-of select="$nl"/>

    <xsl:text>  for kind in $ikinds</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>  do</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>    output </xsl:text> <xsl:value-of select="@name"/>
    <xsl:text> ${rank} I${kind} "integer*${kind}${dim}"</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>  done</xsl:text>
    <xsl:value-of select="$nl"/>

    <xsl:text>  for kind in $rkinds</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>  do</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>    output </xsl:text> <xsl:value-of select="@name"/>
    <xsl:text> ${rank} R${kind} "real*${kind}${dim}"</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>  done</xsl:text>
    <xsl:value-of select="$nl"/>

    <xsl:text>  for kind in $ckinds</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>  do</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>    output </xsl:text> <xsl:value-of select="@name"/>
    <xsl:text> ${rank} C${kind} "complex*${kind}${dim}"</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>  done</xsl:text>
    <xsl:value-of select="$nl"/>

    <xsl:text>done</xsl:text>
    <xsl:value-of select="$nl"/>

</xsl:template>


<!--
  - defineArrayFunctionBody
  -->
<xsl:template name="defineArrayFunctionBody">

    <xsl:text>subroutine ${proc}(</xsl:text>
    <xsl:call-template name="arg-list"/> <xsl:text>)</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>  include "mpif.h"</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:call-template name="decl-construct-list">
      <xsl:with-param name="ws" select="''"/>
      <xsl:with-param name="void_type" select="'${type}'"/>
      <xsl:with-param name="void_kind" select="''"/>
      <xsl:with-param name="has_dim" select="0"/>
    </xsl:call-template>

    <xsl:for-each select="return[1]">
      <xsl:if test="@name = 'ierr'">
       <xsl:text>  integer, intent(out) :: ierr</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:value-of select="$nl"/>
    <xsl:text>  call ${procedure}(</xsl:text>
    <xsl:call-template name="arg-list"/> <xsl:text>)</xsl:text>
    <xsl:value-of select="$nl"/>
    <xsl:text>end subroutine ${proc}</xsl:text>
    <xsl:value-of select="$nl"/>

</xsl:template>


<!--
  - arg-list <method>
  -->
<xsl:template name="arg-list">
  <xsl:for-each select="arg">
    <xsl:value-of select="@name"/>
    <xsl:if test="position() != last()">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <xsl:if test="position() = 5">
      <xsl:text>&amp;</xsl:text>
      <xsl:value-of select="concat($nl, '        ')"/>
    </xsl:if>
  </xsl:for-each>
  <xsl:for-each select="return[1]">
    <xsl:if test="@name = 'ierr'">
      <xsl:if test="../arg[1]">
        <xsl:text>, </xsl:text>
      </xsl:if>
      <xsl:value-of select="@name"/>
    </xsl:if>
  </xsl:for-each>
</xsl:template>


<!--
  - decl-construct-list <method>
  -->
<xsl:template name="decl-construct-list">
  <xsl:param name="ws" select="'  '"/>
  <xsl:param name="void_type"/>
  <xsl:param name="void_kind"/>
  <xsl:param name="has_dim" select="1"/>
  <xsl:for-each select="arg">
    <xsl:call-template name="type-decl-stmt">
      <xsl:with-param name="ws" select="$ws"/>
      <xsl:with-param name="void_type" select="$void_type"/>
      <xsl:with-param name="void_kind" select="$void_kind"/>
      <xsl:with-param name="has_dim" select="$has_dim"/>
    </xsl:call-template>
  </xsl:for-each>
</xsl:template>


<!--
  - type-decl-stmt <arg>
  -->
<xsl:template name="type-decl-stmt">
  <xsl:param name="ws" select="'  '"/>
  <xsl:param name="void_type"/>
  <xsl:param name="void_kind"/>
  <xsl:param name="has_dim" select="1"/>

  <xsl:value-of select="$ws"/>
  <xsl:text>  </xsl:text>
  <xsl:for-each select="type[1]">
    <xsl:call-template name="decl-type-spec">
      <xsl:with-param name="void_type" select="$void_type"/>
      <xsl:with-param name="void_kind" select="$void_kind"/>
      <xsl:with-param name="has_dim" select="$has_dim"/>
    </xsl:call-template>
  </xsl:for-each>

  <xsl:if test="type/@kind != 'void'">
    <xsl:value-of select="concat(', intent(', @intent, ')')"/>
  </xsl:if>
  <xsl:value-of select="concat(' :: ', @name, $nl)"/>

</xsl:template>


<!--
  - decl-type-spec <arg>
  -->
<xsl:template name="decl-type-spec">
  <xsl:param name="void_type"/>
  <xsl:param name="void_kind"/>
  <xsl:param name="has_dim" select="1"/>

  <xsl:choose>

    <!-- C++ types -->

    <xsl:when test="@kind = 'void'">
      <xsl:value-of select="$void_type"/>
    </xsl:when>
    <xsl:when test="@kind = 'bool'">
      <xsl:text>logical</xsl:text>
    </xsl:when>
    <xsl:when test="@kind = 'int'">
      <xsl:choose>
        <xsl:when test="@ikind = 'int'">
          <xsl:text>integer</xsl:text>
        </xsl:when>
        <xsl:when test="@ikind = 'char'">
          <xsl:text>character</xsl:text>
          <xsl:if test="../../@kind = 'ptr'">
            <xsl:text>(len=*)</xsl:text>
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>UNSUPPORTED</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="@kind = 'float'">
      <xsl:text>UNSUPPORTED</xsl:text>
    </xsl:when>
    <xsl:when test="@kind = 'ptr'">
      <xsl:for-each select="indirect[1]/type[1]">
        <xsl:call-template name="decl-type-spec">
          <xsl:with-param name="void_type" select="$void_type"/>
          <xsl:with-param name="void_kind" select="$void_kind"/>
          <xsl:with-param name="has_dim" select="$has_dim"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:when>
    <xsl:when test="@kind = 'array'">
      <xsl:for-each select="array[1]/type[1]">
        <xsl:call-template name="decl-type-spec"/>
      </xsl:for-each>
      <xsl:text>, dimension(</xsl:text>
      <xsl:for-each select="array[1]/dimension">
        <xsl:value-of select="@extent"/>
        <xsl:if test="position() != last()">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
    </xsl:when>
    <xsl:when test="@kind = 'ref'">
      <xsl:for-each select="indirect/type">
        <xsl:call-template name="type-spec">
          <xsl:with-param name="depth" select="../@depth"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:when>
    <xsl:when test="@kind = 'usertype'">
      <xsl:choose>
        <xsl:when test="@usertype = 'default_integer'">
          <xsl:text>integer</xsl:text>
        </xsl:when>
        <xsl:when test="@usertype = 'MPI_Aint'">
          <xsl:text>integer(kind=MPI_ADDRESS_KIND)</xsl:text>
        </xsl:when>
        <xsl:when test="@usertype = 'int64_t'">
          <xsl:text>integer(kind=MPI_OFFSET_KIND)</xsl:text>
        </xsl:when>
        <xsl:when test="@usertype = 'MPI_Status'">
          <xsl:text>integer, dimension(MPI_STATUS_SIZE)</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:param name="prefix" select="substring-before(@usertype, '_')"/>
          <xsl:if test="$prefix = 'MPI'">
            <xsl:text>integer</xsl:text>
          </xsl:if>
          <xsl:if test="$prefix != 'MPI'">
            <xsl:text>UNSUPPORTED</xsl:text>
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>UNSUPPORTED</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
  - copyright: print copyright
  -->
<xsl:template name="copyright">

<xsl:text>
#
# Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#
</xsl:text>

</xsl:template>


<!--
  - info: print information about file
  -->
<xsl:template name="info">

<xsl:text>
#
# This file generates a Fortran code to bridge between an explicit F90
# generic interface and the F77 implementation.
#
# This file is automatically generated by either of the scripts
#   ../xml/create_mpi_f90_medium.f90.sh or
#   ../xml/create_mpi_f90_large.f90.sh
#

</xsl:text>

</xsl:template>


<!--
  - openFile: print file header information
  -->
<xsl:template name="openFile">
  <xsl:param name="filename" select="''"/>
  <xsl:text>#! /bin/sh</xsl:text>
  <xsl:value-of select="$nl"/>
  <xsl:call-template name="copyright"/>
  <xsl:call-template name="info"/>
  <xsl:text>. "$1/fortran_kinds.sh"</xsl:text>
  <xsl:value-of select="$nl"/>
  <xsl:value-of select="$nl"/>

  <xsl:text># This entire file is only generated in medium/large </xsl:text>
  <xsl:text>modules.  So if</xsl:text>
  <xsl:value-of select="$nl"/>
  <xsl:text># we're not at least medium, bail now.</xsl:text>
  <xsl:value-of select="$nl"/>
  <xsl:value-of select="$nl"/>

  <xsl:text>check_size </xsl:text> <xsl:value-of select="$interface_size"/>
  <xsl:value-of select="$nl"/>
  <xsl:text>if test "$output" = "0"; then</xsl:text>
  <xsl:value-of select="$nl"/>
  <xsl:text>    exit 0</xsl:text>
  <xsl:value-of select="$nl"/>
  <xsl:text>fi</xsl:text>
  <xsl:value-of select="$nl"/>
  <xsl:value-of select="$nl"/>

  <xsl:text># Ok, we should continue.</xsl:text>
  <xsl:value-of select="$nl"/>
  <xsl:value-of select="$nl"/>

  <xsl:text>allranks="0 $ranks"</xsl:text>
  <xsl:value-of select="$nl"/>
</xsl:template>


<!--
  - closeFile: finish up
  -->
<xsl:template name="closeFile">
  <xsl:param name="filename" select="''"/>
</xsl:template>


</xsl:stylesheet>
