
#if !defined(__BAIJ_H)
#define __BAIJ_H
#include "private/matimpl.h"
#include "../src/mat/impls/aij/seq/aij.h"
#include "../src/mat/impls/baij/seq/ftn-kernels/fsolvebaij.h"

/*  
  MATSEQBAIJ format - Block compressed row storage. The i[] and j[] 
  arrays start at 0.
*/

/* This header is shared by the SeqSBAIJ matrix */
#define SEQBAIJHEADER \
  PetscInt         bs2;              /*  square of block size */                                     \
  PetscInt         mbs,nbs;          /* rows/bs, columns/bs */                                       \
  PetscScalar      *mult_work;       /* work array for matrix vector product*/                       \
  MatScalar        *saved_values;                                                                    \
                                                                                                     \
  Mat              sbaijMat;         /* mat in sbaij format */                                       \
                                                                                                     \
  PetscTruth       pivotinblocks;    /* pivot inside factorization of each diagonal block */         \
                                                                                                     \
  MatScalar        *idiag;           /* inverse of block diagonal  */                                \
  PetscTruth       idiagvalid       /* if above has correct/current values */


typedef struct {
  SEQAIJHEADER(MatScalar);
  SEQBAIJHEADER;
} Mat_SeqBAIJ;

EXTERN_C_BEGIN
EXTERN PetscErrorCode MatSeqBAIJSetPreallocation_SeqBAIJ(Mat,PetscInt,PetscInt,PetscInt*);
EXTERN_C_END
EXTERN PetscErrorCode MatILUFactorSymbolic_SeqBAIJ(Mat,Mat,IS,IS,const MatFactorInfo*);
EXTERN PetscErrorCode MatICCFactorSymbolic_SeqBAIJ(Mat,Mat,IS,const MatFactorInfo*);
EXTERN PetscErrorCode MatCholeskyFactorSymbolic_SeqBAIJ(Mat,Mat,IS,const MatFactorInfo*);
EXTERN PetscErrorCode MatCholeskyFactorNumeric_SeqBAIJ_N(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatCholeskyFactorNumeric_SeqBAIJ_N_NaturalOrdering(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatDuplicate_SeqBAIJ(Mat,MatDuplicateOption,Mat*);
EXTERN PetscErrorCode MatMissingDiagonal_SeqBAIJ(Mat,PetscTruth*,PetscInt*);
EXTERN PetscErrorCode MatMarkDiagonal_SeqBAIJ(Mat);
EXTERN PetscErrorCode MatILUDTFactor_SeqBAIJ(Mat,IS,IS,const MatFactorInfo*,Mat*);

EXTERN PetscErrorCode MatLUFactorSymbolic_SeqBAIJ(Mat,Mat,IS,IS,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactor_SeqBAIJ(Mat,IS,IS,const MatFactorInfo*);
EXTERN PetscErrorCode MatIncreaseOverlap_SeqBAIJ(Mat,PetscInt,IS*,PetscInt);
EXTERN PetscErrorCode MatGetSubMatrix_SeqBAIJ(Mat,IS,IS,MatReuse,Mat*);
EXTERN PetscErrorCode MatGetSubMatrices_SeqBAIJ(Mat,PetscInt,const IS[],const IS[],MatReuse,Mat*[]);
EXTERN PetscErrorCode MatMultTranspose_SeqBAIJ(Mat,Vec,Vec);
EXTERN PetscErrorCode MatMultTransposeAdd_SeqBAIJ(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatScale_SeqBAIJ(Mat,PetscScalar);
EXTERN PetscErrorCode MatNorm_SeqBAIJ(Mat,NormType,PetscReal *);
EXTERN PetscErrorCode MatEqual_SeqBAIJ(Mat,Mat,PetscTruth*);
EXTERN PetscErrorCode MatGetDiagonal_SeqBAIJ(Mat,Vec);
EXTERN PetscErrorCode MatDiagonalScale_SeqBAIJ(Mat,Vec,Vec);
EXTERN PetscErrorCode MatGetInfo_SeqBAIJ(Mat,MatInfoType,MatInfo *);
EXTERN PetscErrorCode MatZeroEntries_SeqBAIJ(Mat);

EXTERN PetscErrorCode MatSeqBAIJ_UpdateFactorNumeric_NaturalOrdering(Mat);

EXTERN PetscErrorCode MatSolve_SeqBAIJ_1(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_1_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_2(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_2_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_3(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_3_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_4(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_4_NaturalOrdering(Mat,Vec,Vec);
#if defined(PETSC_HAVE_SSE)
EXTERN PetscErrorCode MatSolve_SeqBAIJ_4_SSE_Demotion(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_4_NaturalOrdering_SSE_Demotion(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_4_NaturalOrdering_SSE_Demotion_usj(Mat,Vec,Vec);
#endif
EXTERN PetscErrorCode MatSolve_SeqBAIJ_5(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_5_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_6(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_6_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_7(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_7_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolve_SeqBAIJ_N(Mat,Vec,Vec);

EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_Update(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_1(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_1_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_2(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_2_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_3(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_3_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_4(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_4_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_5(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_5_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_6(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_6_NaturalOrdering(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_7(Mat,Vec,Vec);
EXTERN PetscErrorCode MatSolveTranspose_SeqBAIJ_7_NaturalOrdering(Mat,Vec,Vec);

EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_1(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_2(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_2_NaturalOrdering(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_3(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_3_NaturalOrdering(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_4(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_4_NaturalOrdering(Mat,Mat,const MatFactorInfo*);
#if defined(PETSC_HAVE_SSE)
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_4_NaturalOrdering_SSE(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_4_NaturalOrdering_SSE_usj(Mat,Mat,const MatFactorInfo*);
#else
#endif
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_5(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_5_NaturalOrdering(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_6(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_6_NaturalOrdering(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_7(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_7_NaturalOrdering(Mat,Mat,const MatFactorInfo*);
EXTERN PetscErrorCode MatLUFactorNumeric_SeqBAIJ_N(Mat,Mat,const MatFactorInfo*);

EXTERN PetscErrorCode MatMult_SeqBAIJ_1(Mat,Vec,Vec);
EXTERN PetscErrorCode MatMult_SeqBAIJ_2(Mat,Vec,Vec);
EXTERN PetscErrorCode MatMult_SeqBAIJ_3(Mat,Vec,Vec);
EXTERN PetscErrorCode MatMult_SeqBAIJ_4(Mat,Vec,Vec);
EXTERN PetscErrorCode MatMult_SeqBAIJ_5(Mat,Vec,Vec);
EXTERN PetscErrorCode MatMult_SeqBAIJ_6(Mat,Vec,Vec);
EXTERN PetscErrorCode MatMult_SeqBAIJ_7(Mat,Vec,Vec);
EXTERN PetscErrorCode MatMult_SeqBAIJ_N(Mat,Vec,Vec);

EXTERN PetscErrorCode MatMultAdd_SeqBAIJ_1(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatMultAdd_SeqBAIJ_2(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatMultAdd_SeqBAIJ_3(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatMultAdd_SeqBAIJ_4(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatMultAdd_SeqBAIJ_5(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatMultAdd_SeqBAIJ_6(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatMultAdd_SeqBAIJ_7(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatMultAdd_SeqBAIJ_N(Mat,Vec,Vec,Vec);
EXTERN PetscErrorCode MatLoad_SeqBAIJ(PetscViewer, const MatType,Mat*);

#endif
