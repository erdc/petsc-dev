#ifdef PETSC_RCS_HEADER
"$Id: petscconf.h,v 1.3 2000/11/28 17:26:31 bsmith Exp $"
"Defines the configuration for this machine"
#endif

#if !defined(INCLUDED_PETSCCONF_H)
#define INCLUDED_PETSCCONF_H

#define PARCH_beos
#define PETSC_ARCH_NAME "beos"

#define HAVE_SLEEP
#define HAVE_POPEN
#define HAVE_SYS_WAIT_H
#define HAVE_VPRINTF
#define RETSIGTYPE void
#define STDC_HEADERS
#define SIZEOF_VOID_P 4
#define SIZEOF_INT 4
#define SIZEOF_DOUBLE 8
#define HAVE_DRAND48
#define HAVE_GETCWD
#define HAVE_GETHOSTNAME
#define HAVE_GETTIMEOFDAY
#define HAVE_MEMMOVE
#define HAVE_RAND
#define HAVE_READLINK
#define HAVE_SIGACTION
#define HAVE_SIGNAL
#define HAVE_SOCKET
#define HAVE_STRSTR
#define HAVE_UNAME
#define HAVE_FCNTL_H
#define HAVE_LIMITS_H
#define HAVE_MALLOC_H
#define HAVE_PWD_H 
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_SYS_RESOURCE_H
#define HAVE_SYS_TIME_H 
#define HAVE_UNISTD_H

#define BITS_PER_BYTE 8

#define PETSC_HAVE_FORTRAN_UNDERSCORE 
#define PETSC_HAVE_FORTRAN_UNDERSCORE_UNDERSCORE

#define HAVE_DOUBLE_ALIGN_MALLOC
#define PETSC_CANNOT_START_DEBUGGER
#define PETSC_HAVE_NO_GETRUSAGE
#define PETSC_PRINTF_FORMAT_CHECK(a,b) __attribute__ ((format (printf, a,b)))

#define MISSING_SIGBUS
#define MISSING_SIGQUIT
#define MISSING_SIGSYS

#endif
