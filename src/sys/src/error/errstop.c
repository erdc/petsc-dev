
#include "petsc.h"           /*I "petsc.h" I*/

#undef __FUNCT__  
#define __FUNCT__ "PetscStopErrorHandler" 
/*@C
   PetscStopErrorHandler - Calls MPI_abort() and exists.

   Not Collective

   Input Parameters:
+  line - the line number of the error (indicated by __LINE__)
.  fun - the function where the error occurred (indicated by __FUNCT__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  dir - the directory of the file (indicated by __SDIR__)
.  mess - an error text string, usually just printed to the screen
.  n - the generic error number
.  p - the specific error number
-  ctx - error handler context

   Level: developer

   Notes:
   Most users need not directly employ this routine and the other error 
   handlers, but can instead use the simplified interface SETERRQ, which has 
   the calling sequence
$     SETERRQ(n,p,mess)

   Notes for experienced users:
   Use PetscPushErrorHandler() to set the desired error handler.  The
   currently available PETSc error handlers include PetscTraceBackErrorHandler(),
   PetscStopErrorHandler(), PetscAttachDebuggerErrorHandler(), and PetscAbortErrorHandler().

   Concepts: error handler^stopping

.seealso:  PetscPushErrorHandler(), PetscAttachDebuggerErrorHandler(), 
           PetscAbortErrorHandler(), PetscTraceBackErrorHandler()
 @*/
int PetscStopErrorHandler(int line,const char *fun,const char *file,const char *dir,int n,int p,const char *mess,void *ctx)
{
  PetscTruth     flg1,flg2;
  PetscLogDouble mem,rss;

  PetscFunctionBegin;
  if (!mess) mess = " ";

  if (n == PETSC_ERR_MEM) {
    (*PetscErrorPrintf)("%s() line %d in %s%s\n",fun,line,dir,file);
    (*PetscErrorPrintf)("Out of memory. This could be due to allocating\n");
    (*PetscErrorPrintf)("too large an object or bleeding by not properly\n");
    (*PetscErrorPrintf)("destroying unneeded objects.\n");
    PetscTrSpace(&mem,PETSC_NULL,PETSC_NULL); PetscGetResidentSetSize(&rss);
    PetscOptionsHasName(PETSC_NULL,"-trdump",&flg1);
    PetscOptionsHasName(PETSC_NULL,"-trmalloc_log",&flg2);
    if (flg2) {
      PetscTrLogDump(stdout);
    } else if (flg1) {
      (*PetscErrorPrintf)("Memory allocated %d Memory used by process %d\n",(int)mem,(int)rss);
      PetscTrDump(stdout);
    }  else {
      (*PetscErrorPrintf)("Memory allocated %d Memory used by process %d\n",(int)mem,(int)rss);
      (*PetscErrorPrintf)("Try running with -trdump or -trmalloc_log for info.\n");
    }
  } else if (n == PETSC_ERR_SUP) {
    (*PetscErrorPrintf)("%s() line %d in %s%s\n",fun,line,dir,file);
    (*PetscErrorPrintf)("No support for this operation for this object type!\n");
    (*PetscErrorPrintf)("%s\n",mess);
  } else if (n == PETSC_ERR_SIG) {
    (*PetscErrorPrintf)("%s() line %d in %s%s %s\n",fun,line,dir,file,mess);
  } else {
    (*PetscErrorPrintf)("%s() line %d in %s%s\n    %s\n",fun,line,dir,file,mess);
  }
  MPI_Abort(PETSC_COMM_WORLD,n);
  PetscFunctionReturn(0);
}

