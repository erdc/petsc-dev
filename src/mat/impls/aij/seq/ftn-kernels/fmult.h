
#if !defined(__FMULT_H)
#include "petsc.h"
#if defined(PETSC_USE_FORTRAN_KERNEL_MULTAIJ)
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define fortranmultaij_                FORTRANMULTAIJ
#define fortranmulttransposeaddaij_    FORTRANMULTTRANSPOSEADDAIJ
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define fortranmultaij_                fortranmultaij
#define fortranmulttransposeaddaij_    fortranmulttransposeaddaij
#endif
EXTERN void fortranmultaij_(PetscInt*,const PetscScalar*,const PetscInt*,const PetscInt*,const MatScalar*,PetscScalar*);
EXTERN void fortranmulttransposeaddaij_(PetscInt*,void*,PetscInt*,PetscInt*,void*,void*);
#endif
#endif

