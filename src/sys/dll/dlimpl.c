#define PETSC_DLL
/*
   Low-level routines for managing dynamic link libraries (DLLs).
*/

#include "petsc.h"
#include "petscfix.h"

/* XXX Should be in charge of BuildSystem configure !!!*/
#define PETSC_HAVE_DYNAMIC_LIBRARIES

#if !defined(PETSC_HAVE_DYNAMIC_LIBRARIES)
/* XXX Should be done better !!!*/
#undef PETSC_HAVE_WINDOWS_H
#undef PETSC_HAVE_DLFCN_H
#endif

#if defined(PETSC_HAVE_WINDOWS_H)
#include <windows.h>
#endif

#if defined(PETSC_HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#if 0
#undef PETSC_HAVE_WINDOWS_H
#undef PETSC_HAVE_DLFCN_H
#elif 0
#undef  PETSC_HAVE_DLFCN_H
#define PETSC_HAVE_WINDOWS_H
#define PETSC_HAVE_GETPROCADDRESS
#define HMODULE int
extern HMODULE LoadLibrary(const char*);
extern HMODULE GetCurrentProcess(void);
extern void*   GetProcAddress(HMODULE,const char*);
#endif

#if defined(PETSC_HAVE_WINDOWS_H)
typedef HMODULE dlhandle_t;
#elif defined(PETSC_HAVE_DLFCN_H)
typedef void* dlhandle_t;
#else
typedef void* dlhandle_t;
#endif

#undef __FUNCT__  
#define __FUNCT__ "PetscDLOpen"
/*@C
   PetscDLOpen - 
@*/
PetscErrorCode PETSC_DLLEXPORT PetscDLOpen(const char *name, PetscDLFlags flags, PetscDLHandle *handle)
{
  const char *dlname;
  int        dlflags;
  dlhandle_t dlhandle;

  PetscFunctionBegin;
  PetscValidCharPointer(name, 1);
  PetscValidPointer(handle, 3);

  dlname   = name;
  dlflags  = PETSC_DL_DEFAULT;
  dlhandle = PETSC_NULL;

  /* --- LoadLibrary ---*/  
#if defined(PETSC_HAVE_WINDOWS_H)
  dlhandle = LoadLibrary(dlname);
  if (!dlhandle) {
#if defined(PETSC_HAVE_GETLASTERROR)
    PetscErrorCode ierr;
    DWORD          erc;
    char           *buff;
    erc  = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		  NULL,erc,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPSTR)&buff,0,NULL);
    ierr = PetscError(__LINE__,__FUNCT__,__FILE__,__SDIR__,PETSC_ERR_FILE_OPEN,1,
		      "Unable to open dynamic library:\n  %s\n  Error message from LoadLibrary() %s\n",dlname,buff);
    LocalFree(buff);
    PetscFunctionReturn(ierr);
#else
    SETERRQ2(PETSC_ERR_FILE_OPEN,
	     "Unable to open dynamic library:\n  %s\n  Error message from LoadLibrary() %s\n",dlname,"unavailable");
#endif
  }
  /* --- dlopen ---*/  
#elif defined(PETSC_HAVE_DLFCN_H)
  /*
      Mode indicates symbols required by symbol loaded with dlsym() 
     are only loaded when required (not all together) also indicates
     symbols required can be contained in other libraries also opened
     with dlopen()
  */
  dlflags = RTLD_LAZY;
#if defined(PETSC_HAVE_RTLD_NOW)
  if (flags & PETSC_DL_NOW)  dlflags = RTLD_NOW;
#endif
#if defined(PETSC_HAVE_RTLD_GLOBAL)
  if (flags & PETSC_DL_GLOBAL) dlflags |= RTLD_GLOBAL;
#endif
  dlhandle = dlopen(dlname,flags); 
  if (!dlhandle) {
#if defined(PETSC_HAVE_DLERROR)
    const char *errmsg = dlerror();
#else
    const char *errmsg = "unavailable";
#endif
    SETERRQ2(PETSC_ERR_FILE_OPEN,
	     "Unable to open dynamic library:\n  %s\n  Error message from dlopen() %s\n", dlname, errmsg)
  }
  /* --- unimplemented ---*/  
#else
  SETERRQ(PETSC_ERR_SUP_SYS, "cannot open dynamic libraries");
#endif

  *handle = (PetscDLHandle) dlhandle;

  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "PetscDLClose"
/*@C
   PetscDLClose - 
@*/
PetscErrorCode PETSC_DLLEXPORT PetscDLClose(PetscDLHandle *handle)
{
  dlhandle_t dlhandle;

  PetscFunctionBegin;
  PetscValidPointer(handle, 1);

  dlhandle = (dlhandle_t) *handle;

  /* --- FreeLibrary ?? ---*/  
#if defined(PETSC_HAVE_WINDOWS_H)
  /* XXXX Implement !!!*/

  /* --- dclose ---*/  
#elif defined(PETSC_HAVE_DLFCN_H)
  if (dlclose(dlhandle) < 0) {
#if defined(PETSC_HAVE_DLERROR)
    const char *errmsg = dlerror();
#else
    const char *errmsg = "unavailable";
#endif
    SETERRQ1(PETSC_ERR_LIB, /* XXX Better error code ? */
	     "Unable to close dynamic library:\n  Error message from dlclose() %s\n", errmsg);
  }
  /* --- unimplemented ---*/  
#else
  SETERRQ(PETSC_ERR_SUP_SYS, "cannot close dynamic libraries");
#endif

  *handle = PETSC_NULL;

  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "PetscDLSym"
/*@C
   PetscDLSym - 
@*/
PetscErrorCode PETSC_DLLEXPORT PetscDLSym(PetscDLHandle handle, const char *symbol, void **value)
{
  dlhandle_t dlhandle;
  void       *dlvalue;

  PetscFunctionBegin;
  PetscValidCharPointer(symbol, 2);
  PetscValidPointer(value, 3);

  dlhandle = (dlhandle_t) handle;

#if defined(PETSC_HAVE_WINDOWS_H) && defined(PETSC_HAVE_GETPROCADDRESS)
  if (!dlhandle) dlhandle = GetCurrentProcess();
  dlvalue = GetProcAddress(dlhandle,symbol);
#elif defined(PETSC_HAVE_DLFCN_H)
  if (!dlhandle) dlhandle = (dlhandle_t)0;
  dlvalue = dlsym(dlhandle,symbol);
#else
  SETERRQ(PETSC_ERR_SUP_SYS, "cannot load symbols from dynamic libraries");
#endif

  *value = dlvalue;

  PetscFunctionReturn(0);
}
