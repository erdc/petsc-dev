#ifdef PETSC_RCS_HEADER
"$Id: petscconf.h,v 1.7 2001/02/09 19:40:53 bsmith Exp $"
"Defines the configuration for this machine"
#endif

#if !defined(INCLUDED_PETSCCONF_H)
#define INCLUDED_PETSCCONF_H

#define PARCH_linux
#define PETSC_ARCH_NAME "linux"

#define PETSC_HAVE_SYS_WAIT_H 1
#define TIME_WITH_SYS_TIME 1
#define PETSC_HAVE_FORTRAN_UNDERSCORE 1
#define PETSC_HAVE_DRAND48 1
#define PETSC_HAVE_GETCWD 1
#define PETSC_HAVE_GETHOSTNAME 1
#define PETSC_HAVE_GETTIMEOFDAY 1
#define PETSC_HAVE_MEMMOVE 1
#define PETSC_HAVE_RAND 1
#define PETSC_HAVE_READLINK 1
#define PETSC_HAVE_REALPATH 1
#define PETSC_HAVE_SIGACTION 1
#define PETSC_HAVE_SIGNAL 1
#define PETSC_HAVE_SIGSET 1
#define PETSC_HAVE_SOCKET 1
#define PETSC_HAVE_STRSTR 1
#define PETSC_HAVE_UNAME 1
#define PETSC_HAVE_FCNTL_H 1
#define PETSC_HAVE_LIMITS_H 1
#define PETSC_HAVE_MALLOC_H 1
#define PETSC_HAVE_PWD_H 1
#define PETSC_HAVE_SEARCH_H 1
#define PETSC_HAVE_STDLIB_H 1
#define PETSC_HAVE_STRING_H 1
#define PETSC_HAVE_STRINGS_H 1
#define PETSC_HAVE_STROPTS_H 1
#define PETSC_HAVE_SYS_PROCFS_H 1
#define PETSC_HAVE_SYS_RESOURCE_H 1
#define PETSC_HAVE_SYS_TIME_H 1
#define PETSC_HAVE_UNISTD_H 1
#define PETSC_HAVE_LIBNSL 1

#define PETSC_USE_KBYTES_FOR_SIZE
#define PETSC_HAVE_POPEN
#define PETSC_HAVE_GETDOMAINNAME  
#define PETSC_USE_DBX_DEBUGGER

#define SIZEOF_VOID_P 8
#define SIZEOF_INT 4
#define SIZEOF_DOUBLE 8
#define BITS_PER_BYTE 8

#define PETSC_NEED_SOCKET_PROTO

#define PETSC_NEED_KILL_FOR_DEBUGGER
#define PETSC_USE_PID_FOR_DEBUGGER
#define PETSC_HAVE_TEMPLATED_COMPLEX

#define PETSC_HAVE_F90_H "f90impl/f90_alpha.h"
#define PETSC_HAVE_F90_C "src/sys/src/f90/f90_alpha.c"

#define PETSC_MISSING_SIGSYS

#endif
