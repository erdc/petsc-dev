
#if !defined(__FMDOT_H)
#include <petscsys.h>
#if defined(PETSC_USE_FORTRAN_KERNEL_MDOT)
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define fortranmdot4_      FORTRANMDOT4
#define fortranmdot3_      FORTRANMDOT3
#define fortranmdot2_      FORTRANMDOT2
#define fortranmdot1_      FORTRANMDOT1
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define fortranmdot4_      fortranmdot4
#define fortranmdot3_      fortranmdot3
#define fortranmdot2_      fortranmdot2
#define fortranmdot1_      fortranmdot1
#endif
EXTERN_C_BEGIN
extern void fortranmdot4_(const void*,const void*,const void*,const void*,const void*,PetscInt*,void*,void*,void*,void*);
extern void fortranmdot3_(const void*,const void*,const void*,const void*,PetscInt*,void*,void*,void*);
extern void fortranmdot2_(const void*,const void*,const void*,PetscInt*,void*,void*);
extern void fortranmdot1_(const void*,const void*,PetscInt*,void*);
EXTERN_C_END
#endif
#endif

