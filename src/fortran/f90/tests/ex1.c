
#include <stdio.h>
#include "petscf90.h"

#ifdef PETSC_HAVE_FORTRAN_CAPS
#define fortran_routine_ FORTRAN_ROUTINE
#define c_routine_ C_ROUTINE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define fortran_routine_ fortran_routine
#define c_routine_ c_routine
#endif

typedef struct {
  int a;
  F90Array1d ptr;
  int c;
} abc;


EXTERN_C_BEGIN

extern void fortran_routine_(abc *);
void c_routine_(abc *x)
{
  double     *data;

  F90Array1dAccess(&(x->ptr),(void **)&data);
  printf("From C: %D %5.2e %D\n",x->a,data[0],x->c);
  fflush(stdout);
  x->a = 2;

  data[0] = 22.0;
  x->c = 222;
  fortran_routine_(x); 
}

EXTERN_C_END
