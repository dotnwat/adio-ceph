#include "nbc.h"

/* only used in this file */
static __inline__ int NBC_Start_round(NBC_Handle *handle);

#ifndef OMPI_COMPONENT
/* the keyval (global) */
static int gkeyval=MPI_KEYVAL_INVALID; 
#endif

#ifndef OMPI_COMPONENT
static int NBC_Key_copy(MPI_Comm oldcomm, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag) {
  /* delete the attribute in the new comm  - it will be created at the
   * first usage */
  *flag = 0;

  return MPI_SUCCESS;
}

static int NBC_Key_delete(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state) {
  NBC_Comminfo *comminfo;

  if(keyval == gkeyval) {
    comminfo=(NBC_Comminfo*)attribute_val;
    free(comminfo);
  } else {
    printf("Got wrong keyval!(%i)\n", keyval); 
  }

  return MPI_SUCCESS;
}
#endif

/* allocates a new schedule array */
int NBC_Sched_create(NBC_Schedule* schedule) {
  
  *schedule=malloc(2*sizeof(int));
  if(*schedule == NULL) { return NBC_OOR; }
  *(int*)*schedule=2*sizeof(int);
  *(((int*)*schedule)+1)=0;

  return NBC_OK;
}

/* frees a schedule array */
int NBC_Free(NBC_Handle* handle) {
  
  /*free(*handle->schedule);*/
  /* the schedule pointer itself is also malloc'd */
  /*free(handle->schedule);*/
  /* if the nbc_I<collective> attached some data */
  if(NULL != handle->tmpbuf) free(handle->tmpbuf);

  return NBC_OK;
}

/* this function puts a send into the schedule */
int NBC_Sched_send(void* buf, char tmpbuf, int count, MPI_Datatype datatype, int dest, NBC_Schedule *schedule) {
  int size;
  NBC_Args_send* send_args;
  
  /* get size of actual schedule */
  NBC_GET_SIZE(*schedule, size);
  /*printf("schedule is %i bytes\n", size);*/
  *schedule = realloc(*schedule, size+sizeof(NBC_Args_send)+sizeof(NBC_Fn_type));
  if(*schedule == NULL) { printf("Error in realloc()\n"); return NBC_OOR; }
  
  /* adjust the function type */
  *(NBC_Fn_type*)((char*)*schedule+size)=SEND;
  
  /* store the passed arguments */
  send_args = (NBC_Args_send*)((char*)*schedule+size+sizeof(NBC_Fn_type));
  send_args->buf=buf;
  send_args->tmpbuf=tmpbuf;
  send_args->count=count;
  send_args->datatype=datatype;
  send_args->dest=dest;

  /* increase number of elements in schedule */
  NBC_INC_NUM_ROUND(*schedule);
  NBC_DEBUG(10, "adding send - ends at byte %i\n", (int)(size+sizeof(NBC_Args_send)+sizeof(NBC_Fn_type)));

  /* increase size of schedule */
  NBC_INC_SIZE(*schedule, sizeof(NBC_Args_send)+sizeof(NBC_Fn_type));

  return NBC_OK;
}

/* this function puts a receive into the schedule */
int NBC_Sched_recv(void* buf, char tmpbuf, int count, MPI_Datatype datatype, int source, NBC_Schedule *schedule) {
  int size;
  NBC_Args_recv* recv_args;
  
  /* get size of actual schedule */
  NBC_GET_SIZE(*schedule, size);
  /*printf("schedule is %i bytes\n", size);*/
  *schedule = realloc(*schedule, size+sizeof(NBC_Args_recv)+sizeof(NBC_Fn_type));
  if(*schedule == NULL) { printf("Error in realloc()\n"); return NBC_OOR; }
  
  /* adjust the function type */
  *(NBC_Fn_type*)((char*)*schedule+size)=RECV;

  /* store the passed arguments */
  recv_args=(NBC_Args_recv*)((char*)*schedule+size+sizeof(NBC_Fn_type));
  recv_args->buf=buf;
  recv_args->tmpbuf=tmpbuf;
  recv_args->count=count;
  recv_args->datatype=datatype;
  recv_args->source=source;

  /* increase number of elements in schedule */
  NBC_INC_NUM_ROUND(*schedule);
  NBC_DEBUG(10, "adding receive - ends at byte %i\n", (int)(size+sizeof(NBC_Args_recv)+sizeof(NBC_Fn_type)));

  /* increase size of schedule */
  NBC_INC_SIZE(*schedule, sizeof(NBC_Args_recv)+sizeof(NBC_Fn_type));

  return NBC_OK;
}

/* this function puts an operation into the schedule */
int NBC_Sched_op(void *buf3, char tmpbuf3, void* buf1, char tmpbuf1, void* buf2, char tmpbuf2, int count, MPI_Datatype datatype, MPI_Op op, NBC_Schedule *schedule) {
  int size;
  NBC_Args_op* op_args;
  
  /* get size of actual schedule */
  NBC_GET_SIZE(*schedule, size);
  /*printf("schedule is %i bytes\n", size);*/
  *schedule = realloc(*schedule, size+sizeof(NBC_Args_op)+sizeof(NBC_Fn_type));
  if(*schedule == NULL) { printf("Error in realloc()\n"); return NBC_OOR; }
  
  /* adjust the function type */
  *(NBC_Fn_type*)((char*)*schedule+size)=OP;

  /* store the passed arguments */
  op_args=(NBC_Args_op*)((char*)*schedule+size+sizeof(NBC_Fn_type));
  op_args->buf1=buf1;
  op_args->buf2=buf2;
  op_args->buf3=buf3;
  op_args->tmpbuf1=tmpbuf1;
  op_args->tmpbuf2=tmpbuf2;
  op_args->tmpbuf3=tmpbuf3;
  op_args->count=count;
  op_args->op=op;
  op_args->datatype=datatype;

  /* increase number of elements in schedule */
  NBC_INC_NUM_ROUND(*schedule);
  NBC_DEBUG(10, "adding op - ends at byte %i\n", (int)(size+sizeof(NBC_Args_op)+sizeof(NBC_Fn_type)));

  /* increase size of schedule */
  NBC_INC_SIZE(*schedule, sizeof(NBC_Args_op)+sizeof(NBC_Fn_type));
  
  return NBC_OK;
}

/* this function puts a copy into the schedule */
int NBC_Sched_copy(void *src, char tmpsrc, int srccount, MPI_Datatype srctype, void *tgt, char tmptgt, int tgtcount, MPI_Datatype tgttype, NBC_Schedule *schedule) {
  int size;
  NBC_Args_copy* copy_args;
  
  /* get size of actual schedule */
  NBC_GET_SIZE(*schedule, size);
  /*printf("schedule is %i bytes\n", size);*/
  *schedule = realloc(*schedule, size+sizeof(NBC_Args_copy)+sizeof(NBC_Fn_type));
  if(*schedule == NULL) { printf("Error in realloc()\n"); return NBC_OOR; }
  
  /* adjust the function type */
  *(NBC_Fn_type*)((char*)*schedule+size)=COPY;
  
  /* store the passed arguments */
  copy_args = (NBC_Args_copy*)((char*)*schedule+size+sizeof(NBC_Fn_type));
  copy_args->src=src;
  copy_args->tmpsrc=tmpsrc;
  copy_args->srccount=srccount;
  copy_args->srctype=srctype;
  copy_args->tgt=tgt;
  copy_args->tmptgt=tmptgt;
  copy_args->tgtcount=tgtcount;
  copy_args->tgttype=tgttype;

  /* increase number of elements in schedule */
  NBC_INC_NUM_ROUND(*schedule);
  NBC_DEBUG(10, "adding copy - ends at byte %i\n", (int)(size+sizeof(NBC_Args_copy)+sizeof(NBC_Fn_type)));

  /* increase size of schedule */
  NBC_INC_SIZE(*schedule, sizeof(NBC_Args_copy)+sizeof(NBC_Fn_type));

  return NBC_OK;
}

/* this function puts a unpack into the schedule */
int NBC_Sched_unpack(void *inbuf, char tmpinbuf, int count, MPI_Datatype datatype, void *outbuf, char tmpoutbuf, NBC_Schedule *schedule) {
  int size;
  NBC_Args_unpack* unpack_args;
  
  /* get size of actual schedule */
  NBC_GET_SIZE(*schedule, size);
  /*printf("schedule is %i bytes\n", size);*/
  *schedule = realloc(*schedule, size+sizeof(NBC_Args_unpack)+sizeof(NBC_Fn_type));
  if(*schedule == NULL) { printf("Error in realloc()\n"); return NBC_OOR; }
  
  /* adjust the function type */
  *(NBC_Fn_type*)((char*)*schedule+size)=UNPACK;
  
  /* store the passed arguments */
  unpack_args = (NBC_Args_unpack*)((char*)*schedule+size+sizeof(NBC_Fn_type));
  unpack_args->inbuf=inbuf;
  unpack_args->tmpinbuf=tmpinbuf;
  unpack_args->count=count;
  unpack_args->datatype=datatype;
  unpack_args->outbuf=outbuf;
  unpack_args->tmpoutbuf=tmpoutbuf;

  /* increase number of elements in schedule */
  NBC_INC_NUM_ROUND(*schedule);
  NBC_DEBUG(10, "adding unpack - ends at byte %i\n", (int)(size+sizeof(NBC_Args_unpack)+sizeof(NBC_Fn_type)));

  /* increase size of schedule */
  NBC_INC_SIZE(*schedule, sizeof(NBC_Args_unpack)+sizeof(NBC_Fn_type));

  return NBC_OK;
}

/* this function ends a round of a schedule */
int NBC_Sched_barrier(NBC_Schedule *schedule) {
  int size;
  
  /* get size of actual schedule */
  NBC_GET_SIZE(*schedule, size);
  /*printf("round terminated at %i bytes\n", size);*/
  *schedule = realloc(*schedule, size+sizeof(char)+sizeof(int));
  if(*schedule == NULL) { printf("Error in realloc()\n"); return NBC_OOR; }
  
  /* add the barrier char (1) because another round follows */
  *(char*)((char*)*schedule+size)=1;
  
  /* set round count elements = 0 for new round */
  *(int*)((char*)*schedule+size+sizeof(char))=0;
  NBC_DEBUG(10, "ending round at byte %i\n", (int)(size+sizeof(char)+sizeof(int)));
  
  /* increase size of schedule */
  NBC_INC_SIZE(*schedule, sizeof(char)+sizeof(int));

  return NBC_OK;
}

/* this function ends a schedule */
int NBC_Sched_commit(NBC_Schedule *schedule) {
  int size;
 
  /* get size of actual schedule */
  NBC_GET_SIZE(*schedule, size);
  /*printf("schedule terminated at %i bytes\n", size);*/
  *schedule = realloc(*schedule, size+sizeof(char));
  if(*schedule == NULL) { printf("Error in realloc()\n"); return NBC_OOR; }
 
  /* add the barrier char (0) because this is the last round */
  *(char*)((char*)*schedule+size)=0;
  NBC_DEBUG(10, "closing schedule %p at byte %i\n", *schedule, (int)(size+sizeof(char)));

  /* increase size of schedule */
  NBC_INC_SIZE(*schedule, sizeof(char));
 
  return NBC_OK;
}

int NBC_Progress(NBC_Handle *handle) {
  int flag, res;
  long size;
  char *delim;

  if((handle->req_count > 0) && (handle->req_array != NULL)) {
    NBC_DEBUG(50, "NBC_Progress: testing for %i requests\n", handle->req_count);
#ifdef OMPI_COMPONENT
    /*res = ompi_request_test_all(handle->req_count, handle->req_array, &flag, MPI_STATUSES_IGNORE);*/
    res = MPI_Testall(handle->req_count, handle->req_array, &flag, MPI_STATUS_IGNORE);
    if(res != OMPI_SUCCESS) { printf("MPI Error in MPI_Testall() (%i)\n", res); return res; }
#endif
#ifdef MPI
    res = MPI_Testall(handle->req_count, handle->req_array, &flag, MPI_STATUS_IGNORE);
    if(res != MPI_SUCCESS) { printf("MPI Error in MPI_Testall() (%i)\n", res); return res; }
#endif
#ifdef IB
    res = IB_Testall(handle->req_count, handle->req_array, &flag);
    if(res != MPI_SUCCESS) { printf("MPI Error in MPI_Testall() (%i)\n", res); return res; }
#endif
  } else { 
    flag = 1; /* we had no open requests -> proceed to next round */
  }

  /* a round is finished */
  if(flag) {
    /* adjust delim to start of current round */
    NBC_DEBUG(10, "NBC_Progress: going in schedule %p to row-offset: %li\n", *handle->schedule, handle->row_offset);
    delim = (char*)*handle->schedule + handle->row_offset;
    NBC_DEBUG(10, "delim: %p\n", delim);
    NBC_GET_ROUND_SIZE(delim, size);
    NBC_DEBUG(10, "size: %li\n", size);
    /* adjust delim to end of current round -> delimiter */
    delim = delim + size;

    if(handle->req_array != NULL) {
      /* free request array */
      free(handle->req_array);
      handle->req_array = NULL;
    }
    handle->req_count = 0;

    if(*delim == 0) {
      /* this was the last round - we're done */
      NBC_DEBUG(5, "NBC_Progress last round finished - we're done\n");
      return NBC_OK;
    } else {
      NBC_DEBUG(5, "NBC_Progress round finished - goto next round\n");
      /* move delim to start of next round */
      delim = delim+1;
      /* initializing handle for new virgin round */
      handle->row_offset = (long)delim - (long)*handle->schedule;
      /* kick it off */
      res = NBC_Start_round(handle);
      if(NBC_OK != res) { printf("Error in NBC_Start_round() (%i)\n", res); return res; }
    }
  }
 
  return NBC_CONTINUE;
}

int NBC_Progress_block(NBC_Handle *handle) {
  int res;
  long size;
  char *delim;

  do {
    if((handle->req_count > 0) && (handle->req_array != NULL)) {
      NBC_DEBUG(50, "NBC_Progress_block: waiting for %i requests\n", handle->req_count);
#ifdef OMPI_COMPONENT
      /*res = ompi_request_wait_all(handle->req_count, handle->req_array, MPI_STATUSES_IGNORE);*/
      res = MPI_Waitall(handle->req_count, handle->req_array, MPI_STATUS_IGNORE);
      if(res != OMPI_SUCCESS) { printf("MPI Error in MPI_Waitall() (%i)\n", res); return res; }
#endif
#ifdef MPI
      res = MPI_Waitall(handle->req_count, handle->req_array, MPI_STATUS_IGNORE);
      if(res != MPI_SUCCESS) { printf("MPI Error in MPI_Waitall() (%i)\n", res); return res; }
#endif
#ifdef IB
      res = IB_Waitall(handle->req_count, handle->req_array);
      if(res != NBC_OK) { printf("MPI Error in MPI_Waitall() (%i)\n", res); return res; }
#endif
    }

    /* a round is finished - adjust delim to start of current round */
    NBC_DEBUG(10, "NBC_Progress_block: going in schedule %p to row-offset: %li\n", *handle->schedule, handle->row_offset);
    delim = (char*)*handle->schedule + handle->row_offset;
    NBC_DEBUG(10, "delim: %p\n", delim);
    NBC_GET_ROUND_SIZE(delim, size);
    NBC_DEBUG(10, "size: %li\n", size);
    /* adjust delim to end of current round -> delimiter */
    delim = delim + size;
    
    if(handle->req_array != NULL) {
      /* free request array */
      free(handle->req_array);
      handle->req_array = NULL;
    }
    handle->req_count = 0;

    if(*delim == 0) {
      /* this was the last round - we're done */
      NBC_DEBUG(5, "NBC_Progress_block: last round finished - we're done\n");
      break;
    } else {
      NBC_DEBUG(5, "NBC_Progress_block: round finished - goto next round\n");
      /* move delim to start of next round */
      delim = delim+1;
      /* initializing handle for new virgin round */
      handle->row_offset = (long)delim - (long)*handle->schedule;
      /* kick it off */
      res = NBC_Start_round(handle);
      if(NBC_OK != res) { printf("Error in NBC_Start_round() (%i)\n", res); return res; }
    }
  } while(1);
 
  return NBC_OK;
}

static __inline__ int NBC_Start_round(NBC_Handle *handle) {
  int *numptr; /* number of operations */
  int i, res;
  NBC_Fn_type *typeptr;
  NBC_Args_send *sendargs; 
  NBC_Args_recv *recvargs; 
  NBC_Args_op *opargs; 
  NBC_Args_copy *copyargs; 
  NBC_Args_unpack *unpackargs; 
  NBC_Schedule myschedule;
  void *buf1, *buf2, *buf3;

  /* get schedule address */
  myschedule = (NBC_Schedule*)((char*)*handle->schedule + handle->row_offset);

  numptr = (int*)myschedule;
  NBC_DEBUG(10, "start_round round at address %p : posting %i operations\n", myschedule, *numptr);

  /* typeptr is increased by sizeof(int) bytes to point to type */
  typeptr = (NBC_Fn_type*)(numptr+1);
  for (i=0; i<*numptr; i++) {
    /* go sizeof op-data forward */
    switch(*typeptr) {
      case SEND:
        NBC_DEBUG(5,"  SEND (offset %li) ", (long)typeptr-(long)myschedule);
        sendargs = (NBC_Args_send*)(typeptr+1);
        NBC_DEBUG(5,"*buf: %p, count: %i, type: %lu, dest: %i, tag: %i)\n", sendargs->buf, sendargs->count, (unsigned long)sendargs->datatype, sendargs->dest, handle->tag);
        typeptr = (NBC_Fn_type*)(((NBC_Args_send*)typeptr)+1);
        /* get an additional request - TODO: req_count NOT thread safe */
        handle->req_count++;
        /* get buffer */
        if(sendargs->tmpbuf) 
          buf1=(char*)handle->tmpbuf+(long)sendargs->buf;
        else
          buf1=sendargs->buf;
#ifdef OMPI_COMPONENT
        handle->req_array = realloc(handle->req_array, (handle->req_count)*sizeof(ompi_request_t*));
        CHECK_NULL(handle->req_array);
        /*res = MCA_PML_CALL(isend_init(buf1, sendargs->count, sendargs->datatype, sendargs->dest, handle->tag, MCA_PML_BASE_SEND_STANDARD, handle->mycomm, handle->req_array+handle->req_count-1));
        printf("MPI_Isend(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long)buf1, sendargs->count, (unsigned long)sendargs->datatype, sendargs->dest, handle->tag, (unsigned long)handle->mycomm, res);*/
        res = MPI_Isend(buf1, sendargs->count, sendargs->datatype, sendargs->dest, handle->tag, handle->mycomm, handle->req_array+handle->req_count-1);
        if(OMPI_SUCCESS != res) { printf("Error in MPI_Isend(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long)buf1, sendargs->count, (unsigned long)sendargs->datatype, sendargs->dest, handle->tag, (unsigned long)handle->mycomm, res); return res; }
#endif
#ifdef MPI
        handle->req_array = realloc(handle->req_array, (handle->req_count)*sizeof(MPI_Request));
        CHECK_NULL(handle->req_array);
        res = MPI_Isend(buf1, sendargs->count, sendargs->datatype, sendargs->dest, handle->tag, handle->mycomm, handle->req_array+handle->req_count-1);
        if(MPI_SUCCESS != res) { printf("Error in MPI_Isend(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long)buf1, sendargs->count, (unsigned long)sendargs->datatype, sendargs->dest, handle->tag, (unsigned long)handle->mycomm, res); return res; }
#endif
#ifdef IB
        handle->req_array = realloc(handle->req_array, (handle->req_count)*sizeof(IB_Request));
        CHECK_NULL(handle->req_array);
        res = IB_Isend(buf1, sendargs->count, sendargs->datatype, sendargs->dest, handle->tag, handle->mycomm, handle->req_array+handle->req_count-1);
        if(NBC_OK != res) { printf("Error in IB_Isend() (%i)\n", res); return res; }
#endif
        break;
      case RECV:
        NBC_DEBUG(5, "  RECV (offset %li) ", (long)typeptr-(long)myschedule);
        recvargs = (NBC_Args_recv*)(typeptr+1);
        NBC_DEBUG(5, "*buf: %p, count: %i, type: %lu, source: %i, tag: %i)\n", recvargs->buf, recvargs->count, (unsigned long)recvargs->datatype, recvargs->source, handle->tag);
        typeptr = (NBC_Fn_type*)(((NBC_Args_recv*)typeptr)+1);
        /* get an additional request - TODO: req_count NOT thread safe */
        handle->req_count++;
        /* get buffer */
        if(recvargs->tmpbuf) {
          buf1=(char*)handle->tmpbuf+(long)recvargs->buf;
        } else {
          buf1=recvargs->buf;
        }
#ifdef OMPI_COMPONENT
        handle->req_array = realloc(handle->req_array, (handle->req_count)*sizeof(ompi_request_t*));
        CHECK_NULL(handle->req_array);
        /*res = MCA_PML_CALL(irecv(buf1, recvargs->count, recvargs->datatype, recvargs->source, handle->tag, handle->mycomm, handle->req_array+handle->req_count-1)); 
        printf("MPI_Irecv(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long)buf1, recvargs->count, (unsigned long)recvargs->datatype, recvargs->source, handle->tag, (unsigned long)handle->mycomm, res); */
        res = MPI_Irecv(buf1, recvargs->count, recvargs->datatype, recvargs->source, handle->tag, handle->mycomm, handle->req_array+handle->req_count-1);
        if(OMPI_SUCCESS != res) { printf("Error in MPI_Irecv(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long)buf1, recvargs->count, (unsigned long)recvargs->datatype, recvargs->source, handle->tag, (unsigned long)handle->mycomm, res); return res; }
#endif
#ifdef MPI
        handle->req_array = realloc(handle->req_array, (handle->req_count)*sizeof(MPI_Request));
        CHECK_NULL(handle->req_array);
        res = MPI_Irecv(buf1, recvargs->count, recvargs->datatype, recvargs->source, handle->tag, handle->mycomm, handle->req_array+handle->req_count-1);
        if(MPI_SUCCESS != res) { printf("Error in MPI_Irecv(%lu, %i, %lu, %i, %i, %lu) (%i)\n", (unsigned long)buf1, recvargs->count, (unsigned long)recvargs->datatype, recvargs->source, handle->tag, (unsigned long)handle->mycomm, res); return res; }
#endif
#ifdef IB
        handle->req_array = realloc(handle->req_array, (handle->req_count)*sizeof(IB_Request));
        CHECK_NULL(handle->req_array);
        res = IB_Irecv(buf1, recvargs->count, recvargs->datatype, recvargs->source, handle->tag, handle->mycomm, handle->req_array+handle->req_count-1);
        if(NBC_OK != res) { printf("Error in MPI_Irecv() (%i)\n", res); return res; }
#endif
        break;
      case OP:
        NBC_DEBUG(5, "  OP   (offset %li) ", (long)typeptr-(long)myschedule);
        opargs = (NBC_Args_op*)(typeptr+1);
        NBC_DEBUG(5, "*buf1: %lu, buf2: %lu, count: %i, type: %lu)\n", (unsigned long)opargs->buf1, (unsigned long)opargs->buf2, opargs->count, (unsigned long)opargs->datatype);
        typeptr = (NBC_Fn_type*)((NBC_Args_op*)typeptr+1);
        /* get buffers */
        if(opargs->tmpbuf1) 
          buf1=(char*)handle->tmpbuf+(long)opargs->buf1;
        else
          buf1=opargs->buf1;
        if(opargs->tmpbuf2) 
          buf2=(char*)handle->tmpbuf+(long)opargs->buf2;
        else
          buf2=opargs->buf2;
        if(opargs->tmpbuf3) 
          buf3=(char*)handle->tmpbuf+(long)opargs->buf3;
        else
          buf3=opargs->buf3;
        res = NBC_Operation(buf3, buf1, buf2, opargs->op, opargs->datatype, opargs->count);
        if(res != NBC_OK) { printf("NBC_Operation() failed (code: %i)\n", res); return res; }
        break;
      case COPY:
        NBC_DEBUG(5, "  COPY   (offset %li) ", (long)typeptr-(long)myschedule);
        copyargs = (NBC_Args_copy*)(typeptr+1);
        NBC_DEBUG(5, "*src: %lu, srccount: %i, srctype: %lu, *tgt: %lu, tgtcount: %i, tgttype: %lu)\n", (unsigned long)copyargs->src, copyargs->srccount, (unsigned long)copyargs->srctype, (unsigned long)copyargs->tgt, copyargs->tgtcount, (unsigned long)copyargs->tgttype);
        typeptr = (NBC_Fn_type*)((NBC_Args_copy*)typeptr+1);
        /* get buffers */
        if(copyargs->tmpsrc) 
          buf1=(char*)handle->tmpbuf+(long)copyargs->src;
        else
          buf1=copyargs->src;
        if(copyargs->tmptgt) 
          buf2=(char*)handle->tmpbuf+(long)copyargs->tgt;
        else
          buf2=copyargs->tgt;
        res = NBC_Copy(buf1, copyargs->srccount, copyargs->srctype, buf2, copyargs->tgtcount, copyargs->tgttype, handle->mycomm);
        if(res != NBC_OK) { printf("NBC_Copy() failed (code: %i)\n", res); return res; }
        break;
      case UNPACK:
        NBC_DEBUG(5, "  UNPACK   (offset %li) ", (long)typeptr-(long)myschedule);
        unpackargs = (NBC_Args_unpack*)(typeptr+1);
        NBC_DEBUG(5, "*src: %lu, srccount: %i, srctype: %lu, *tgt: %lu\n", (unsigned long)unpackargs->inbuf, unpackargs->count, (unsigned long)unpackargs->datatype, (unsigned long)unpackargs->outbuf);
        typeptr = (NBC_Fn_type*)((NBC_Args_unpack*)typeptr+1);
        /* get buffers */
        if(unpackargs->tmpinbuf) 
          buf1=(char*)handle->tmpbuf+(long)unpackargs->inbuf;
        else
          buf1=unpackargs->outbuf;
        if(unpackargs->tmpoutbuf) 
          buf2=(char*)handle->tmpbuf+(long)unpackargs->outbuf;
        else
          buf2=unpackargs->outbuf;
        res = NBC_Unpack(buf1, unpackargs->count, unpackargs->datatype, buf2, handle->mycomm);
        if(res != NBC_OK) { printf("NBC_Unpack() failed (code: %i)\n", res); return res; }
        break;
      default:
        printf("NBC_Start_round: bad type %li at offset %li\n", (long)*typeptr, (long)typeptr-(long)myschedule);
        return NBC_BAD_SCHED;
    }
    /* increase ptr by size of fn_type enum */
    typeptr = (NBC_Fn_type*)((NBC_Fn_type*)typeptr+1);
  }

  /* check if we can make progress - not in the first round, this allows us to leave the
   * initialization faster and to reach more overlap */
  if(handle->row_offset != 4) {
    res = NBC_Progress(handle);
    if((NBC_OK != res) && (NBC_CONTINUE != res)) { printf("Error in NBC_Progress() (%i)\n", res); return res; }
  }
  
  return NBC_OK;
}

__inline__ int NBC_Init_handle(NBC_Handle *handle, MPI_Comm comm) {
  int res, flag;
  NBC_Comminfo *comminfo;
  
  /* create a new state and return handle to it */
  handle->req_array = NULL;
  handle->req_count = 0;
  handle->comm = comm;
  /* first int is the schedule size */
  handle->row_offset = sizeof(int);

  /******************** Do the tag and shadow comm administration ...  ***************/
  
#ifdef OMPI_COMPONENT
  /* the communicator is an ompi_communicator_t with a pointer to
   * c_coll_selected_data where we have our comminfo struct - this was
   * initialized during comm_query */
  {
    ompi_communicator_t *ompicomm;
    
    ompicomm = (ompi_communicator_t*)comm;
    comminfo = (NBC_Comminfo*)ompicomm->c_coll_selected_data;
    flag = 1;
  }
#else
  /* otherwise we have to do the normal attribute stuff :-( */
  /* keyval is not initialized yet, we have to init it */
  if(MPI_KEYVAL_INVALID == gkeyval) {
    res = MPI_Keyval_create(NBC_Key_copy, NBC_Key_delete, &(gkeyval), NULL); 
    if((MPI_SUCCESS != res)) { printf("Error in MPI_Keyval_create() (%i)\n", res); return res; }
  } 

  res = MPI_Attr_get(comm, gkeyval, &comminfo, &flag);
  if((MPI_SUCCESS != res)) { printf("Error in MPI_Attr_get() (%i)\n", res); return res; }
#endif
  if (flag) {
    /* we found it */
    comminfo->tag++;
  } else {
    /* we have to create a new one */
    comminfo = NBC_Init_comm(comm);
    if(comminfo == NULL) { printf("Error in NBC_Init_comm() %i\n", res); return NBC_OOR; }
  }
  handle->tag=comminfo->tag;
  handle->mycomm=comminfo->mycomm;
  /*printf("got comminfo: %lu tag: %i\n", comminfo, comminfo->tag);*/

#if 0
/*#ifdef OMPI_COMPONENT*/
  /* we use negative tags for OMPI */
  if(handle->tag == -50) {
    handle->tag=-32767;
    comminfo->tag=-32767;
    NBC_DEBUG(2,"resetting tags ...\n"); 
  }
#else
#endif
  /* reset counter ... */ 
  if(handle->tag == 32767) {
    handle->tag=1;
    comminfo->tag=1;
    NBC_DEBUG(2,"resetting tags ...\n"); 
  }
/*#endif*/
  
  /******************** end of tag and shadow comm administration ...  ***************/
  handle->comminfo = comminfo;
  
  NBC_DEBUG(3, "got tag %i\n", handle->tag);

  return NBC_OK;
}

__inline__ NBC_Comminfo* NBC_Init_comm(MPI_Comm comm) {
#ifndef OMPI_COMPONENT
  int res;
#endif
  NBC_Comminfo *comminfo;

  comminfo = malloc(sizeof(NBC_Comminfo));
  if(comminfo == NULL) { printf("Error in malloc()\n"); return NULL; }


#ifdef OMPI_COMPONENT
  comminfo->mycomm = comm;
  /* set tag to 1 */
  /*comminfo->tag=-32767;*/
  comminfo->tag=1;
#else
  /* set tag to 1 */
  comminfo->tag=1;
  /* dup and save shadow communicator */
  res = MPI_Comm_dup(comm, &(comminfo->mycomm));
  if((MPI_SUCCESS != res)) { printf("Error in MPI_Comm_dup() (%i)\n", res); return NULL; }
  NBC_DEBUG(1, "created a shadow communicator for %lu ... %lu\n", (unsigned long)comm, (unsigned long)comminfo->mycomm);
#endif

#ifdef NBC_CACHE_SCHEDULE
  /* initialize the NBC_ALLTOALL SchedCache tree */
  comminfo->NBC_Dict[NBC_ALLTOALL] = hb_tree_new((dict_cmp_func)NBC_Alltoall_args_compare, NBC_SchedCache_args_delete_key_dummy, NBC_SchedCache_args_delete);
  if(comminfo->NBC_Dict[NBC_ALLTOALL] == NULL) { printf("Error in hb_tree_new()\n"); return NULL; }
  NBC_DEBUG(1, "added tree at address %lu\n", (unsigned long)comminfo->NBC_Dict[NBC_ALLTOALL]);
  comminfo->NBC_Dict_size[NBC_ALLTOALL] = 0;
  /* initialize the NBC_ALLGATHER SchedCache tree */
  comminfo->NBC_Dict[NBC_ALLGATHER] = hb_tree_new((dict_cmp_func)NBC_Allgather_args_compare, NBC_SchedCache_args_delete_key_dummy, NBC_SchedCache_args_delete);
  if(comminfo->NBC_Dict[NBC_ALLGATHER] == NULL) { printf("Error in hb_tree_new()\n"); return NULL; }
  NBC_DEBUG(1, "added tree at address %lu\n", (unsigned long)comminfo->NBC_Dict[NBC_ALLGATHER]);
  comminfo->NBC_Dict_size[NBC_ALLGATHER] = 0;
  /* initialize the NBC_ALLREDUCE SchedCache tree */
  comminfo->NBC_Dict[NBC_ALLREDUCE] = hb_tree_new((dict_cmp_func)NBC_Allreduce_args_compare, NBC_SchedCache_args_delete_key_dummy, NBC_SchedCache_args_delete);
  if(comminfo->NBC_Dict[NBC_ALLREDUCE] == NULL) { printf("Error in hb_tree_new()\n"); return NULL; }
  NBC_DEBUG(1, "added tree at address %lu\n", (unsigned long)comminfo->NBC_Dict[NBC_ALLREDUCE]);
  comminfo->NBC_Dict_size[NBC_ALLREDUCE] = 0;
  /* initialize the NBC_BARRIER SchedCache tree - is not needed -
   * schedule is hung off directly */
  comminfo->NBC_Dict_size[NBC_BARRIER] = 0;
  /* initialize the NBC_BCAST SchedCache tree */
  comminfo->NBC_Dict[NBC_BCAST] = hb_tree_new((dict_cmp_func)NBC_Bcast_args_compare, NBC_SchedCache_args_delete_key_dummy, NBC_SchedCache_args_delete);
  if(comminfo->NBC_Dict[NBC_BCAST] == NULL) { printf("Error in hb_tree_new()\n"); return NULL; }
  NBC_DEBUG(1, "added tree at address %lu\n", (unsigned long)comminfo->NBC_Dict[NBC_BCAST]);
  comminfo->NBC_Dict_size[NBC_BCAST] = 0;
  /* initialize the NBC_GATHER SchedCache tree */
  comminfo->NBC_Dict[NBC_GATHER] = hb_tree_new((dict_cmp_func)NBC_Gather_args_compare, NBC_SchedCache_args_delete_key_dummy, NBC_SchedCache_args_delete);
  if(comminfo->NBC_Dict[NBC_GATHER] == NULL) { printf("Error in hb_tree_new()\n"); return NULL; }
  NBC_DEBUG(1, "added tree at address %lu\n", (unsigned long)comminfo->NBC_Dict[NBC_GATHER]);
  comminfo->NBC_Dict_size[NBC_GATHER] = 0;
  /* initialize the NBC_REDUCE SchedCache tree */
  comminfo->NBC_Dict[NBC_REDUCE] = hb_tree_new((dict_cmp_func)NBC_Reduce_args_compare, NBC_SchedCache_args_delete_key_dummy, NBC_SchedCache_args_delete);
  if(comminfo->NBC_Dict[NBC_REDUCE] == NULL) { printf("Error in hb_tree_new()\n"); return NULL; }
  NBC_DEBUG(1, "added tree at address %lu\n", (unsigned long)comminfo->NBC_Dict[NBC_REDUCE]);
  comminfo->NBC_Dict_size[NBC_REDUCE] = 0;
  /* initialize the NBC_SCAN SchedCache tree */
  comminfo->NBC_Dict[NBC_SCAN] = hb_tree_new((dict_cmp_func)NBC_Scan_args_compare, NBC_SchedCache_args_delete_key_dummy, NBC_SchedCache_args_delete);
  if(comminfo->NBC_Dict[NBC_SCAN] == NULL) { printf("Error in hb_tree_new()\n"); return NULL; }
  NBC_DEBUG(1, "added tree at address %lu\n", (unsigned long)comminfo->NBC_Dict[NBC_SCAN]);
  comminfo->NBC_Dict_size[NBC_SCAN] = 0;
  /* initialize the NBC_SCATTER SchedCache tree */
  comminfo->NBC_Dict[NBC_SCATTER] = hb_tree_new((dict_cmp_func)NBC_Scatter_args_compare, NBC_SchedCache_args_delete_key_dummy, NBC_SchedCache_args_delete);
  if(comminfo->NBC_Dict[NBC_SCATTER] == NULL) { printf("Error in hb_tree_new()\n"); return NULL; }
  NBC_DEBUG(1, "added tree at address %lu\n", (unsigned long)comminfo->NBC_Dict[NBC_SCATTER]);
  comminfo->NBC_Dict_size[NBC_SCATTER] = 0;
#endif

#ifndef OMPI_COMPONENT
  /* put the new attribute to the comm */
  res = MPI_Attr_put(comm, gkeyval, comminfo); 
  if((MPI_SUCCESS != res)) { printf("Error in MPI_Attr_put() (%i)\n", res); return NULL; }
#endif

  return comminfo;
}

__inline__ int NBC_Start(NBC_Handle *handle, NBC_Schedule *schedule) {
  int res;

  handle->schedule = schedule;
  /* kick off first round */
  res = NBC_Start_round(handle);
  if((NBC_OK != res)) { printf("Error in NBC_Start_round() (%i)\n", res); return res; }

  return NBC_OK;
}

int NBC_Wait_poll(NBC_Handle *handle) {
  int res;

  while(NBC_OK != NBC_Progress(handle));

  res = NBC_Free(handle);
  if((NBC_OK != res)) { printf("Error in NBC_Free() (%i)\n", res); return res; }

  return NBC_OK;
}

int NBC_Wait(NBC_Handle *handle) {
  int res;
  
  NBC_Progress_block(handle);
  
  res = NBC_Free(handle);
  if((NBC_OK != res)) { printf("Error in NBC_Free() (%i)\n", res); return res; }
  
  return NBC_OK;
}

#ifdef NBC_CACHE_SCHEDULE
void NBC_SchedCache_args_delete_key_dummy(void *k) {
    /* do nothing because the key and the data element are identical :-) 
     * both (the single one :) is freed in NBC_<COLLOP>_args_delete() */
}


void NBC_SchedCache_args_delete(void *entry) {
  struct NBC_dummyarg *tmp;
  
  tmp = (struct NBC_dummyarg*)entry;
  /* free taglistentry */
  free(*(tmp->schedule));
  /* the schedule pointer itself is also malloc'd */
  free(tmp->schedule);
  free(tmp);
}
#endif
