
#include "petsc.h"           /*I "petsc.h" I*/


#undef __FUNCT__  
#define __FUNCT__ "PetscIgnoreErrorHandler" 
/*@C
   PetscIgnoreErrorHandler - Ignores the error, allows program to continue as if error did not occure

   Not Collective

   Input Parameters:
+  line - the line number of the error (indicated by __LINE__)
.  func - the function where error is detected (indicated by __FUNCT__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  dir - the directory of the file (indicated by __SDIR__)
.  mess - an error text string, usually just printed to the screen
.  n - the generic error number
.  p - specific error number
-  ctx - error handler context

   Level: developer

   Notes:
   Most users need not directly employ this routine and the other error 
   handlers, but can instead use the simplified interface SETERRQ, which has 
   the calling sequence
$     SETERRQ(number,p,mess)

   Notes for experienced users:
   Use PetscPushErrorHandler() to set the desired error handler.  The
   currently available PETSc error handlers include PetscTraceBackErrorHandler(),
   PetscAttachDebuggerErrorHandler(), PetscAbortErrorHandler(), and PetscStopErrorHandler()

   Concepts: error handler^traceback
   Concepts: traceback^generating

.seealso:  PetscPushErrorHandler(), PetscAttachDebuggerErrorHandler(), 
          PetscAbortErrorHandler(), PetscTraceBackErrorHandler()
 @*/
int PetscIgnoreErrorHandler(int line,const char *fun,const char* file,const char *dir,int n,int p,const char *mess,void *ctx)
{
  PetscFunctionBegin;
  PetscFunctionReturn(n);
}


#undef __FUNCT__  
#define __FUNCT__ "PetscTraceBackErrorHandler" 
/*@C

   PetscTraceBackErrorHandler - Default error handler routine that generates
   a traceback on error detection.

   Not Collective

   Input Parameters:
+  line - the line number of the error (indicated by __LINE__)
.  func - the function where error is detected (indicated by __FUNCT__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  dir - the directory of the file (indicated by __SDIR__)
.  mess - an error text string, usually just printed to the screen
.  n - the generic error number
.  p - specific error number
-  ctx - error handler context

   Level: developer

   Notes:
   Most users need not directly employ this routine and the other error 
   handlers, but can instead use the simplified interface SETERRQ, which has 
   the calling sequence
$     SETERRQ(number,p,mess)

   Notes for experienced users:
   Use PetscPushErrorHandler() to set the desired error handler.  The
   currently available PETSc error handlers include PetscTraceBackErrorHandler(),
   PetscAttachDebuggerErrorHandler(), PetscAbortErrorHandler(), and PetscStopErrorHandler()

   Concepts: error handler^traceback
   Concepts: traceback^generating

.seealso:  PetscPushErrorHandler(), PetscAttachDebuggerErrorHandler(), 
          PetscAbortErrorHandler()
 @*/
int PetscTraceBackErrorHandler(int line,const char *fun,const char* file,const char *dir,int n,int p,const char *mess,void *ctx)
{
  PetscLogDouble    mem,rss;
  PetscTruth        flg1,flg2;

  PetscFunctionBegin;

  (*PetscErrorPrintf)("%s() line %d in %s%s\n",fun,line,dir,file);
  if (p == 1) {
    if (n == PETSC_ERR_MEM) {
      (*PetscErrorPrintf)("Out of memory. This could be due to allocating\n");
      (*PetscErrorPrintf)("too large an object or bleeding by not properly\n");
      (*PetscErrorPrintf)("destroying unneeded objects.\n");
      PetscTrSpace(&mem,PETSC_NULL,PETSC_NULL);
      PetscGetResidentSetSize(&rss);
      PetscOptionsHasName(PETSC_NULL,"-trdump",&flg1);
      PetscOptionsHasName(PETSC_NULL,"-trmalloc_log",&flg2);
      if (flg2) {
        PetscTrLogDump(stdout);
      } else if (flg1) {
        (*PetscErrorPrintf)("Memory allocated %d Memory used by process %d\n",(int)mem,(int)rss);
        PetscTrDump(stdout);
      } else {
        (*PetscErrorPrintf)("Memory allocated %d Memory used by process %d\n",(int)mem,(int)rss);
        (*PetscErrorPrintf)("Try running with -trdump or -trmalloc_log for info.\n");
      }
    } else {
        const char *text;
        PetscErrorMessage(n,&text,PETSC_NULL);
        if (text) (*PetscErrorPrintf)("%s!\n",text);
    }
    if (mess) {
      (*PetscErrorPrintf)("%s!\n",mess);
    }
  }
  PetscFunctionReturn(n);
}

