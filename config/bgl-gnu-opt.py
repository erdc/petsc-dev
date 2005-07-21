#!/usr/bin/env python
#
# BGL has broken 'libc' dependencies. The option 'LIBS' is 
# used to workarround this problem. Another workarround is to
# modify mpicc/mpif77 scripts and make them link with these
# additional libraries.
#
# Also the default fortran namemangling changed - so the usage
# of iarg_()/getargc_() internal compiler symbols does not work
# without a minor manual hack to zstart.c sourcefile
#
configure_options = [
  '-LIBS=-lc -lnss_files -lnss_dns -lresolv',
  '--with-cc=mpicc',
  '--with-fc=mpif77',

  '--with-blas-lapack-dir=/home/balay/software/fblaslapack/gnu-O3',
  '--with-shared=0',
  
  '-COPTFLAGS=-O3',
  '-FOPTFLAGS=-O3',
  '--with-debugging=0',
  '--with-fortran-kernels=1',
  
  '--can-execute=0',
  '--sizeof_void_p=4',
  '--sizeof_short=2',
  '--sizeof_int=4',
  '--sizeof_long=4',
  '--sizeof_long_long=8',
  '--sizeof_float=4',
  '--sizeof_double=8',
  '--bits_per_byte=8',
  '--sizeof_MPI_Comm=4',
  '--sizeof_MPI_Fint=4'
  ]

if __name__ == '__main__':
  import configure
  configure.petsc_configure(configure_options)

# Extra options used for testing locally
test_options = []
    
