#define PETSC_DLL
/*
      Code to handle PETSc starting up in debuggers,etc.
*/

#include "petscsys.h"               /*I   "petscsys.h"   I*/
#include <signal.h> 
#if defined(PETSC_HAVE_UNISTD_H)
#include <unistd.h>
#endif 
#if defined(PETSC_HAVE_STDLIB_H)
#include <stdlib.h>
#endif

/*
      These are the debugger and display used if the debugger is started up
*/
static char       Debugger[PETSC_MAX_PATH_LEN];
static char       DebugTerminal[PETSC_MAX_PATH_LEN];
static PetscTruth Xterm = PETSC_TRUE;

#undef __FUNCT__
#define __FUNCT__ "PetscSetDebugTerminal"
/*@C
   PetscSetDebugTerminal - Sets the terminal to use (instead of xterm) for debugging.

   Not Collective

   Input Parameters:
+  terminal - name of terminal and any flags required to execute a program.
              For example "xterm -e", "urxvt -e".

   Options Database Keys:
   -debug_terminal terminal - use this terminal instead of xterm

   Level: developer

   Notes:
   You can start the debugger for all processes in the same GNU screen session.

     mpirun -n 4 ./myapp -start_in_debugger -debug_terminal "screen -X -S debug screen"

   will open 4 windows in the session named "debug".
     
   Fortran Note:
   This routine is not supported in Fortran.

   Concepts: debugger^setting

.seealso: PetscSetDebugger()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscSetDebugTerminal(const char terminal[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscStrcpy(DebugTerminal,terminal);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscSetDebugger" 
/*@C
   PetscSetDebugger - Sets options associated with the debugger.

   Not Collective

   Input Parameters:
+  debugger - name of debugger, which should be in your path,
              usually "dbx", "gdb", "idb", "xxgdb", "kdgb" or "ddd". Also, HP-UX
              supports "xdb", and IBM rs6000 supports "xldb".

-  xterm - flag to indicate debugger window, set to either 1 (to indicate
            debugger should be started in a new xterm) or 0 (to start debugger
            in initial window (the option 0 makes no sense when using more
            than one processor.)

   Level: developer

   Fortran Note:
   This routine is not supported in Fortran.

  Concepts: debugger^setting

.seealso: PetscAttachDebugger(), PetscAttachDebuggerErrorHandler()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscSetDebugger(const char debugger[],PetscTruth xterm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (debugger) {
    ierr = PetscStrcpy(Debugger,debugger);CHKERRQ(ierr);
  }
  Xterm = xterm;

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscSetDefaultDebugger"
/*@
    PetscSetDefaultDebugger - Causes PETSc to use its default 
          debugger.

   Not collective

    Level: advanced

.seealso: PetscSetDebugger(), PetscSetDebuggerFromString()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscSetDefaultDebugger(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
#if defined(PETSC_USE_DBX_DEBUGGER)
  ierr = PetscSetDebugger("dbx",PETSC_TRUE);CHKERRQ(ierr);
#elif defined(PETSC_USE_XDB_DEBUGGER) 
  ierr = PetscSetDebugger("xdb",PETSC_TRUE);CHKERRQ(ierr);
#elif defined(PETSC_USE_IDB_DEBUGGER) 
  ierr = PetscSetDebugger("idb",PETSC_TRUE);CHKERRQ(ierr);
#else  /* Default is gdb */
  ierr = PetscSetDebugger("gdb",PETSC_TRUE);CHKERRQ(ierr);
#endif
  ierr = PetscSetDebugTerminal("xterm -e");CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscCheckDebugger_Private"
static PetscErrorCode PetscCheckDebugger_Private(const char defaultDbg[], const char string[], const char *debugger[])
{
  PetscTruth exists;
  char      *f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscStrstr(string, defaultDbg, &f);CHKERRQ(ierr);
  if (f) {
    ierr = PetscTestFile(string, 'x', &exists);CHKERRQ(ierr);
    if (exists) {
      *debugger = string;
    } else {
      *debugger = defaultDbg;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscSetDebuggerFromString" 
/*@C
    PetscSetDebuggerFromString - Set the complete path for the
       debugger for PETSc to use.

   Not collective
 
   Level: advanced

.seealso: PetscSetDebugger(), PetscSetDefaultDebugger()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscSetDebuggerFromString(char *string)
{
  const char *debugger = PETSC_NULL;
  PetscTruth  xterm    = PETSC_TRUE;
  char       *f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscStrstr(string, "noxterm", &f);CHKERRQ(ierr);
  if (f) xterm = PETSC_FALSE;
  ierr = PetscStrstr(string, "ddd", &f);CHKERRQ(ierr);
  if (f) xterm = PETSC_FALSE;
  ierr = PetscCheckDebugger_Private("xdb",      string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("dbx",      string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("xldb",     string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("gdb",      string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("idb",      string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("xxgdb",    string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("ddd",      string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("kdbg",     string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("ups",      string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("workshop", string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("pgdbg",    string, &debugger);CHKERRQ(ierr);
  ierr = PetscCheckDebugger_Private("pathdb",   string, &debugger);CHKERRQ(ierr);

  ierr = PetscSetDebugger(debugger, xterm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "PetscAttachDebugger" 
/*@
   PetscAttachDebugger - Attaches the debugger to the running process.

   Not Collective

   Level: advanced

   Concepts: debugger^starting from program

.seealso: PetscSetDebugger()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscAttachDebugger(void)
{
#if !defined(PETSC_CANNOT_START_DEBUGGER) 
  int            child=0;
  PetscReal      sleeptime=0;
  PetscErrorCode ierr;
  char           program[PETSC_MAX_PATH_LEN],display[256],hostname[64];
#endif

  PetscFunctionBegin;

#if defined(PETSC_CANNOT_START_DEBUGGER) || !defined(PETSC_HAVE_FORK)
  (*PetscErrorPrintf)("System cannot start debugger\n");
  (*PetscErrorPrintf)("On Cray run program in Totalview debugger\n");
  (*PetscErrorPrintf)("On Windows use Developer Studio(MSDEV)\n");
  MPI_Finalize();
  exit(0);
#else
  ierr = PetscGetDisplay(display,128);CHKERRQ(ierr);
  ierr = PetscGetProgramName(program,PETSC_MAX_PATH_LEN);CHKERRQ(ierr);
  if (ierr) {
    (*PetscErrorPrintf)("Cannot determine program name\n");
    PetscFunctionReturn(1);
  }
  if (!program[0]) {
    (*PetscErrorPrintf)("Cannot determine program name\n");
    PetscFunctionReturn(1);
  }
  child = (int)fork(); 
  if (child < 0) {
    (*PetscErrorPrintf)("Error in fork() attaching debugger\n");
    PetscFunctionReturn(1);
  }

  /*
      Swap role the parent and child. This is (I think) so that control c typed
    in the debugger goes to the correct process.
  */
  if (child) { child = 0; }
  else       { child = (int)getppid(); }

  if (child) { /* I am the parent, will run the debugger */
    const char *args[10];
    char       pid[10];
    PetscInt   j,jj;
    PetscTruth isdbx,isidb,isxldb,isxxgdb,isups,isxdb,isworkshop,isddd,iskdbg;

    ierr = PetscGetHostName(hostname,64);CHKERRQ(ierr);
    /*
         We need to send a continue signal to the "child" process on the 
       alpha, otherwise it just stays off forever
    */
#if defined (PETSC_NEED_KILL_FOR_DEBUGGER)
    kill(child,SIGCONT);
#endif
    sprintf(pid,"%d",child); 

    ierr = PetscStrcmp(Debugger,"xxgdb",&isxxgdb);CHKERRQ(ierr);
    ierr = PetscStrcmp(Debugger,"ddd",&isddd);CHKERRQ(ierr);
    ierr = PetscStrcmp(Debugger,"kdbg",&iskdbg);CHKERRQ(ierr);
    ierr = PetscStrcmp(Debugger,"ups",&isups);CHKERRQ(ierr);
    ierr = PetscStrcmp(Debugger,"xldb",&isxldb);CHKERRQ(ierr);
    ierr = PetscStrcmp(Debugger,"xdb",&isxdb);CHKERRQ(ierr);
    ierr = PetscStrcmp(Debugger,"dbx",&isdbx);CHKERRQ(ierr);
    ierr = PetscStrcmp(Debugger,"idb",&isidb);CHKERRQ(ierr);
    ierr = PetscStrcmp(Debugger,"workshop",&isworkshop);CHKERRQ(ierr);

    if (isxxgdb || isups || isddd ) {
      args[1] = program; args[2] = pid; args[3] = "-display";
      args[0] = Debugger; args[4] = display; args[5] = 0;
      (*PetscErrorPrintf)("PETSC: Attaching %s to %s %s on %s\n",args[0],args[1],pid,hostname);
      if (execvp(args[0],(char**)args)  < 0) {
        perror("Unable to start debugger");
        exit(0);
      }
    } else if (iskdbg) {
      args[1] = "-p"; args[2] = pid; args[3] = program;  args[4] = "-display";
      args[0] = Debugger; args[5] = display; args[6] = 0;
      (*PetscErrorPrintf)("PETSC: Attaching %s to %s %s on %s\n",args[0],args[3],pid,hostname);
      if (execvp(args[0],(char**)args)  < 0) {
        perror("Unable to start debugger");
        exit(0);
      }
    } else if (isxldb) {
      args[1] = "-a"; args[2] = pid; args[3] = program;  args[4] = "-display";
      args[0] = Debugger; args[5] = display; args[6] = 0;
      (*PetscErrorPrintf)("PETSC: Attaching %s to %s %s on %s\n",args[0],args[1],pid,hostname);
      if (execvp(args[0],(char**)args)  < 0) {
        perror("Unable to start debugger");
        exit(0);
      }
    } else if (isworkshop) {
      args[1] = "-s"; args[2] = pid; args[3] = "-D"; args[4] = "-";
      args[0] = Debugger; args[5] = pid; args[6] = "-display"; args[7] = display; args[8] = 0;
      (*PetscErrorPrintf)("PETSC: Attaching %s to %s on %s\n",args[0],pid,hostname);
      if (execvp(args[0],(char**)args)  < 0) {
        perror("Unable to start debugger");
        exit(0);
      }
    } else {
      j = 0;
      if (Xterm) {
        PetscTruth cmp;
        char *tmp,*tmp1;
        ierr = PetscStrncmp(DebugTerminal,"screen",6,&cmp);CHKERRQ(ierr);
        if (cmp) display[0] = 0; /* when using screen, we never pass -display */
        args[j++] = tmp = DebugTerminal;
        if (display[0]) {
          args[j++] = "-display"; args[j++] = display;
        }
        while (*tmp) {
          ierr = PetscStrchr(tmp,' ',&tmp1);CHKERRQ(ierr);
          if (!tmp1) break;
          *tmp1 = 0;
          tmp = tmp1+1;
          args[j++] = tmp;
        }
      }
      args[j++] = Debugger;
      jj = j;
      args[j++] = program; args[j++] = pid; args[j++] = 0;

      if (isidb) {
        j = jj;
        args[j++] = "-pid";
        args[j++] = pid;
        args[j++] = "-gdb";
        args[j++] = program;
        args[j++] = 0;
      }
#if defined(PETSC_USE_P_FOR_DEBUGGER)
      if (isdbx) {
        j = jj;
        args[j++] = "-p";
        args[j++] = pid;
        args[j++] = program;
        args[j++] = 0;
      }
#elif defined(PETSC_USE_LARGEP_FOR_DEBUGGER)
      if (isxdb) {
        j = jj;
        args[j++] = "-l";
        args[j++] = "ALL";
        args[j++] = "-P";
        args[j++] = pid;
        args[j++] = program;
        args[j++] = 0;
      }
#elif defined(PETSC_USE_A_FOR_DEBUGGER)
      if (isdbx) {
        j = jj;
        args[j++] = "-a";
        args[j++] = pid;
        args[j++] = 0;
      }
#elif defined(PETSC_USE_PID_FOR_DEBUGGER)
      if (isdbx) {
        j = jj;
        args[j++] = "-pid";
        args[j++] = pid;
        args[j++] = program;
        args[j++] = 0;
      }
#endif
      if (Xterm) {
        if (display[0]) {
          (*PetscErrorPrintf)("PETSC: Attaching %s to %s of pid %s on display %s on machine %s\n",Debugger,program,pid,display,hostname);
        } else {
          (*PetscErrorPrintf)("PETSC: Attaching %s to %s on pid %s on %s\n",Debugger,program,pid,hostname);
        }
        if (execvp(args[0],(char**)args)  < 0) {
          perror("Unable to start debugger in xterm");
          exit(0);
        }
      } else {
        (*PetscErrorPrintf)("PETSC: Attaching %s to %s of pid %s on %s\n",Debugger,program,pid,hostname);
        if (execvp(args[0],(char**)args)  < 0) {
          perror("Unable to start debugger");
          exit(0);
        }
      }
    }
  } else {   /* I am the child, continue with user code */
    sleeptime = 10; /* default to sleep waiting for debugger */
    ierr = PetscOptionsGetReal(PETSC_NULL,"-debugger_pause",&sleeptime,PETSC_NULL);CHKERRQ(ierr);
    if (sleeptime < 0) sleeptime = -sleeptime;
#if defined(PETSC_NEED_DEBUGGER_NO_SLEEP)
    /*
        HP cannot attach process to sleeping debugger, hence count instead
    */
    { 
      PetscReal x = 1.0;
      int i=10000000;
      while (i--) x++ ; /* cannot attach to sleeper */
    }
#elif defined(PETSC_HAVE_SLEEP_RETURNS_EARLY)
    /*
        IBM sleep may return at anytime, hence must see if there is more time to sleep
    */
    {
      int left = sleeptime;
      while (left > 0) {left = PetscSleep(left) - 1;}
    }
#else
    PetscSleep(sleeptime);
#endif
  }
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscAttachDebuggerErrorHandler" 
/*@C
   PetscAttachDebuggerErrorHandler - Error handler that attaches
   a debugger to a running process when an error is detected.
   This routine is useful for examining variables, etc. 

   Not Collective

   Input Parameters:
+  comm - communicator over which error occurred
.  line - the line number of the error (indicated by __LINE__)
.  fun - function where error occured (indicated by __FUNCT__)
.  file - the file in which the error was detected (indicated by __FILE__)
.  dir - the directory of the file (indicated by __SDIR__)
.  message - an error text string, usually just printed to the screen
.  number - the generic error number
.  p - PETSC_ERROR_INITIAL if error just detected, otherwise PETSC_ERROR_REPEAT
-  ctx - error handler context

   Options Database Keys:
.  -on_error_attach_debugger [noxterm,dbx,xxgdb,xdb,xldb,gdb] [-display name] - Activates
   debugger attachment

   Level: developer

   Notes:
   By default the GNU debugger, gdb, is used.  Alternatives are dbx and
   xxgdb,xldb (on IBM rs6000), xdb (on HP-UX).

   Most users need not directly employ this routine and the other error 
   handlers, but can instead use the simplified interface SETERR, which has 
   the calling sequence
$     SETERRQ(PETSC_COMM_SELF,number,p,message)

   Notes for experienced users:
   Use PetscPushErrorHandler() to set the desired error handler.  The
   currently available PETSc error handlers are
$    PetscTraceBackErrorHandler()
$    PetscAttachDebuggerErrorHandler()
$    PetscAbortErrorHandler()
   or you may write your own.

   Concepts: debugger^error handler
   Concepts: error handler^attach debugger

.seealso:  PetscPushErrorHandler(), PetscTraceBackErrorHandler(), 
           PetscAbortErrorHandler()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscAttachDebuggerErrorHandler(MPI_Comm comm,int line,const char* fun,const char *file,const char* dir,int num,PetscErrorType p,const char* mess,void *ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!fun)  fun = "User provided function";
  if (!dir)  dir = " ";
  if (!mess) mess = " ";

  (*PetscErrorPrintf)("%s() line %d in %s%s %s\n",fun,line,dir,file,mess);

  ierr = PetscAttachDebugger();
  if (ierr) { /* hopeless so get out */
    MPI_Finalize();
    exit(num);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscStopForDebugger" 
/*@C
   PetscStopForDebugger - Prints a message to the screen indicating how to
         attach to the process with the debugger and then waits for the 
         debugger to attach.

   Not Collective

   Level: advanced

   Concepts: debugger^waiting for attachment

.seealso: PetscSetDebugger(), PetscAttachDebugger()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscStopForDebugger(void)
{
  PetscErrorCode ierr;
  PetscInt       sleeptime=0;
  int            err;
#if !defined(PETSC_CANNOT_START_DEBUGGER) 
  int            ppid;
  PetscMPIInt    rank;
  char           program[PETSC_MAX_PATH_LEN],hostname[256];
  PetscTruth     isdbx,isxldb,isxxgdb,isddd,iskdbg,isups,isxdb;
#endif

  PetscFunctionBegin;
#if defined(PETSC_CANNOT_START_DEBUGGER) 
  (*PetscErrorPrintf)("System cannot start debugger; just continuing program\n");
#else
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);
  ierr = PetscGetHostName(hostname,256);
  if (ierr) {
    (*PetscErrorPrintf)("Cannot determine hostname; just continuing program\n");
    PetscFunctionReturn(0);
  }

  ierr = PetscGetProgramName(program,256);
  if (ierr) {
    (*PetscErrorPrintf)("Cannot determine program name; just continuing program\n");
    PetscFunctionReturn(0);
  }
  if (!program[0]) {
    (*PetscErrorPrintf)("Cannot determine program name; just continuing program\n");
    PetscFunctionReturn(0);
  }

  ppid = getpid();

  ierr = PetscStrcmp(Debugger,"xxgdb",&isxxgdb);CHKERRQ(ierr);
  ierr = PetscStrcmp(Debugger,"ddd",&isddd);CHKERRQ(ierr);
  ierr = PetscStrcmp(Debugger,"kdbg",&iskdbg);CHKERRQ(ierr);
  ierr = PetscStrcmp(Debugger,"ups",&isups);CHKERRQ(ierr);
  ierr = PetscStrcmp(Debugger,"xldb",&isxldb);CHKERRQ(ierr);
  ierr = PetscStrcmp(Debugger,"xdb",&isxdb);CHKERRQ(ierr);
  ierr = PetscStrcmp(Debugger,"dbx",&isdbx);CHKERRQ(ierr);

  if (isxxgdb || isups || isddd || iskdbg ) {
    (*PetscErrorPrintf)("[%d]%s>>%s %s %d\n",rank,hostname,Debugger,program,ppid);
  }
#if defined(PETSC_USE_A_FOR_DEBUGGER)
  else if (isxldb) {
    (*PetscErrorPrintf)("{%d]%s>>%s -a %d %s\n",rank,hostname,Debugger,ppid,program);
  }
#endif
#if defined(PETSC_USE_P_FOR_DEBUGGER)
  else if (isdbx) {
    (*PetscErrorPrintf)("[%d]%s>>%s -p %d %s\n",rank,hostname,Debugger,ppid,program);
  }
#elif defined(PETSC_USE_LARGEP_FOR_DEBUGGER)
  else if (isxdb) {
    (*PetscErrorPrintf)("[%d]%s>>%s -l ALL -P %d %s\n",rank,hostname,Debugger,ppid,program);
  }
#elif defined(PETSC_USE_A_FOR_DEBUGGER)
  else if (isdbx) {
    (*PetscErrorPrintf)("[%d]%s>>%s -a %d\n",rank,hostname,Debugger,ppid);
  }
#elif defined(PETSC_USE_PID_FOR_DEBUGGER)
  else if (isdbx) {
    (*PetscErrorPrintf)("[%d]%s>>%s -pid %d %s\n",rank,hostname,Debugger,ppid,program);
  }
#else 
  else {
    (*PetscErrorPrintf)("[%d]%s>>%s %s %d\n",rank,hostname,Debugger,program,ppid);
  }
#endif
#endif /* PETSC_CANNOT_START_DEBUGGER */

  err = fflush(stdout);
  if (err) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SYS,"fflush() failed on stdout");    

  sleeptime = 25; /* default to sleep waiting for debugger */
  ierr = PetscOptionsGetInt(PETSC_NULL,"-debugger_pause",&sleeptime,PETSC_NULL);CHKERRQ(ierr);
  if (sleeptime < 0) sleeptime = -sleeptime;
#if defined(PETSC_NEED_DEBUGGER_NO_SLEEP)
  /*
      HP cannot attach process to sleeping debugger, hence count instead
  */
  { 
    PetscReal x = 1.0;
    int i=10000000;
    while (i--) x++ ; /* cannot attach to sleeper */
  }
#elif defined(PETSC_HAVE_SLEEP_RETURNS_EARLY)
  /*
      IBM sleep may return at anytime, hence must see if there is more time to sleep
  */
  {
    int left = sleeptime;
    while (left > 0) {left = sleep(left) - 1;}
  }
#else
  PetscSleep(sleeptime);
#endif
  PetscFunctionReturn(0);
}



