/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
/*
 * File         : mpi_Intercomm.c
 * Headerfile   : mpi_Intercomm.h 
 * Author       : Xinying Li
 * Created      : Thu Apr  9 12:22:15 1998
 * Revision     : $Revision: 1.3 $
 * Updated      : $Date: 2003/01/16 16:39:34 $
 * Copyright: Northeast Parallel Architectures Center
 *            at Syracuse University 1998
 */

#include "ompi_config.h"

#ifdef HAVE_TARGETCONDITIONALS_H
#include <TargetConditionals.h>
#endif

#include "mpi.h"
#include "mpi_Intercomm.h"
#include "mpiJava.h"


/*
 * Class:     mpi_Intercomm
 * Method:    Remote_size
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_mpi_Intercomm_Remote_1size(JNIEnv *env, jobject jthis)
{
    int size;

    ompi_java_clearFreeList(env) ;

    MPI_Comm_remote_size((MPI_Comm)((*env)->GetLongField(env,jthis,ompi_java.CommhandleID)),
                         &size);
    return size;
}
/*
 * Class:     mpi_Intercomm
 * Method:    remote_group
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_mpi_Intercomm_remote_1group(JNIEnv *env, jobject jthis)
{
    MPI_Group group;

    ompi_java_clearFreeList(env) ;

    MPI_Comm_remote_group((MPI_Comm)((*env)->GetLongField(env,jthis,ompi_java.CommhandleID)),
                          &group);
    return (jlong)group;
}

/*
 * Class:     mpi_Intercomm
 * Method:    merge
 * Signature: (Z)Lmpi/Intracomm;
 */
JNIEXPORT jlong JNICALL Java_mpi_Intercomm_merge(JNIEnv *env, jobject jthis, jboolean high)
{
    MPI_Comm newintracomm;

    ompi_java_clearFreeList(env) ;

    MPI_Intercomm_merge((MPI_Comm)((*env)->GetLongField(env,jthis,ompi_java.CommhandleID)), high,
                        &newintracomm);
    return (jlong)newintracomm;
}
