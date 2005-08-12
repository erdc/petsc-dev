#define PETSCMAT_DLL

/* 
        Provides an interface to the PLAPACKR32 dense solver
*/

#include "src/mat/impls/dense/seq/dense.h"
#include "src/mat/impls/dense/mpi/mpidense.h"

EXTERN_C_BEGIN 
#include "PLA.h"
EXTERN_C_END 

typedef struct {
  PLA_Obj        A,v,pivots;
  PLA_Template   templ;
  MPI_Datatype   datatype;
  PetscInt       nb,ierror,rstart;
  VecScatter     ctx;
  IS             is_pla,is_petsc;
  PetscTruth     pla_solved;
  MatStructure   flg;
  PetscMPIInt    nprows,npcols;

  /* A few function pointers for inheritance */
  PetscErrorCode (*MatDuplicate)(Mat,MatDuplicateOption,Mat*);
  PetscErrorCode (*MatView)(Mat,PetscViewer);
  PetscErrorCode (*MatAssemblyEnd)(Mat,MatAssemblyType);
  PetscErrorCode (*MatLUFactorSymbolic)(Mat,IS,IS,MatFactorInfo*,Mat*);
  PetscErrorCode (*MatDestroy)(Mat);

  /* Flag to clean up (non-global) Plapack objects during Destroy */
  PetscTruth CleanUpPlapack;
} Mat_Plapack;

EXTERN PetscErrorCode MatDuplicate_Plapack(Mat,MatDuplicateOption,Mat*);

EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "MatConvert_Plapack_Base"
PetscErrorCode PETSCMAT_DLLEXPORT MatConvert_Plapack_Base(Mat A,MatType type,MatReuse reuse,Mat *newmat) 
{

  PetscErrorCode   ierr;
  Mat              B=*newmat;
  Mat_Plapack      *lu=(Mat_Plapack *)A->spptr;

  PetscFunctionBegin;
  if (reuse == MAT_INITIAL_MATRIX) {
    ierr = MatDuplicate(A,MAT_COPY_VALUES,&B);CHKERRQ(ierr);
  }
  /* Reset the original function pointers */
  B->ops->duplicate        = lu->MatDuplicate;
  B->ops->view             = lu->MatView;
  B->ops->assemblyend      = lu->MatAssemblyEnd;
  B->ops->lufactorsymbolic = lu->MatLUFactorSymbolic;
  B->ops->destroy          = lu->MatDestroy;

  ierr = PetscFree(lu);CHKERRQ(ierr);

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqdense_plapack_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_plapack_seqdense_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_mpidense_plapack_C","",PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_plapack_mpidense_C","",PETSC_NULL);CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)B,type);CHKERRQ(ierr);
  *newmat = B;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "MatDestroy_Plapack"
PetscErrorCode MatDestroy_Plapack(Mat A)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;
  Mat_Plapack    *lu=(Mat_Plapack*)A->spptr; 
    
  PetscFunctionBegin;
  if (lu->CleanUpPlapack) {
    /* Deallocate Plapack storage */
    PLA_Obj_free(&lu->A);
    PLA_Obj_free (&lu->pivots);
    PLA_Temp_free(&lu->templ);
    PLA_Finalize();

    ierr = ISDestroy(lu->is_pla);CHKERRQ(ierr);
    ierr = ISDestroy(lu->is_petsc);CHKERRQ(ierr);
    ierr = VecScatterDestroy(lu->ctx);CHKERRQ(ierr);
  }
  ierr = MPI_Comm_size(A->comm,&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr = MatConvert_Plapack_Base(A,MATSEQDENSE,MAT_REUSE_MATRIX,&A);CHKERRQ(ierr);
  } else {
    ierr = MatConvert_Plapack_Base(A,MATMPIDENSE,MAT_REUSE_MATRIX,&A);CHKERRQ(ierr);
  }
  ierr = (*A->ops->destroy)(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_Plapack"
PetscErrorCode MatSolve_Plapack(Mat A,Vec b,Vec x)
{
  MPI_Comm       comm = A->comm;
  Mat_Plapack    *lu = (Mat_Plapack*)A->spptr;
  PetscErrorCode ierr;
  PetscInt       M=A->M,m=A->m,rstart;
  PetscScalar    *array;
  PetscReal      d_one = 1.0;
  PetscMPIInt    size,rank;
  PLA_Obj        x_pla = NULL;
  PetscInt       loc_m,loc_stride;
  PetscScalar    *loc_buf;
  Vec            loc_x;
  PetscInt       i,j,*idx_pla,*idx_petsc;
   
  PetscFunctionBegin;
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  /* Create PLAPACK vector objects, then copy b into PLAPACK b */
  PLA_Mvector_create(lu->datatype,M,1,lu->templ,PLA_ALIGN_FIRST,&lu->v);  

  /* Copy b into rhs_pla */
  PLA_API_begin();   
  PLA_Obj_API_open(lu->v);
  ierr = VecGetArray(b,&array);CHKERRQ(ierr);
  PLA_API_axpy_vector_to_global(m,&d_one,(void *)array,1,lu->v,lu->rstart);
  ierr = VecRestoreArray(b,&array);CHKERRQ(ierr);
  PLA_Obj_API_close(lu->v);
  PLA_API_end(); 

  /* Apply the permutations to the right hand sides */
  PLA_Apply_pivots_to_rows (lu->v,lu->pivots);

  /* Solve L y = b, overwriting b with y */
  PLA_Trsv( PLA_LOWER_TRIANGULAR, PLA_NO_TRANSPOSE, PLA_UNIT_DIAG, lu->A, lu->v );

  /* Solve U x = y (=b), overwriting b with x */
  PLA_Trsv( PLA_UPPER_TRIANGULAR, PLA_NO_TRANSPOSE,  PLA_NONUNIT_DIAG, lu->A, lu->v );

  /* Copy PLAPACK x into Petsc vector x  */   
  PLA_Obj_local_length(lu->v, &loc_m);
  PLA_Obj_local_buffer(lu->v, (void**)&loc_buf);
  PLA_Obj_local_stride(lu->v, &loc_stride);
  /*
    PetscPrintf(PETSC_COMM_SELF," [%d] b - local_m %d local_stride %d, loc_buf: %g %g, nb: %d\n",rank,loc_m,loc_stride,loc_buf[0],loc_buf[(loc_m-1)*loc_stride],lu->nb); */
  

  ierr = VecCreateSeqWithArray(PETSC_COMM_SELF,loc_m*loc_stride,loc_buf,&loc_x);CHKERRQ(ierr);
  if (!lu->pla_solved){
    /* Create IS and cts for VecScatterring */
    PLA_Obj_local_length(lu->v, &loc_m);
    PLA_Obj_local_stride(lu->v, &loc_stride);
    ierr = PetscMalloc((2*loc_m+1)*sizeof(PetscInt),&idx_pla);CHKERRQ(ierr);
    idx_petsc = idx_pla + loc_m;
    rstart = rank*lu->nb;
    for (i=0; i<loc_m; i+=lu->nb){
      j = 0; 
      while (j < lu->nb && i+j < loc_m){
        idx_petsc[i+j] = rstart + j; 
        j++;
      }
      rstart += size*lu->nb;
    }
    for (i=0; i<loc_m; i++) idx_pla[i] = i*loc_stride;

    ierr = ISCreateGeneral(PETSC_COMM_SELF,loc_m,idx_pla,&lu->is_pla);CHKERRQ(ierr);
    ierr = ISCreateGeneral(PETSC_COMM_SELF,loc_m,idx_petsc,&lu->is_petsc);CHKERRQ(ierr);  
    ierr = PetscFree(idx_pla);CHKERRQ(ierr);
    ierr = VecScatterCreate(loc_x,lu->is_pla,x,lu->is_petsc,&lu->ctx);CHKERRQ(ierr);
  }
  ierr = VecScatterBegin(loc_x,x,INSERT_VALUES,SCATTER_FORWARD,lu->ctx);CHKERRQ(ierr);
  ierr = VecScatterEnd(loc_x,x,INSERT_VALUES,SCATTER_FORWARD,lu->ctx);CHKERRQ(ierr);
  
  /* Free data */
  ierr = VecDestroy(loc_x);CHKERRQ(ierr);
  PLA_Obj_free(&lu->v);

  lu->pla_solved = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__   
#define __FUNCT__ "MatLUFactorNumeric_Plapack"
PetscErrorCode MatLUFactorNumeric_Plapack(Mat A,MatFactorInfo *info,Mat *F)
{
  Mat_Plapack    *lu = (Mat_Plapack*)(*F)->spptr;
  PetscErrorCode ierr;
  PetscInt       M=A->M,m=A->m,rstart;
  MPI_Comm       comm=A->comm,comm_2d;
  MPI_Datatype   datatype;
  PLA_Template   templ = PETSC_NULL;
  PetscMPIInt    size,rank;
  PetscInt       i,j,info_pla=0;
  PetscScalar    *array,one = 1.0;
  PLA_Obj        A_pla=NULL,pivots=NULL;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  /* Create PLAPACK matrix object */
  PLA_Matrix_create(lu->datatype,M,M,lu->templ,PLA_ALIGN_FIRST,PLA_ALIGN_FIRST,&lu->A);  
  PLA_Mvector_create(MPI_INT,M,1,lu->templ,PLA_ALIGN_FIRST,&lu->pivots);

  /* Copy A into lu->A */
  PLA_API_begin();
  PLA_Obj_API_open(lu->A);  
  ierr = MatGetOwnershipRange(A,&rstart,PETSC_NULL);CHKERRQ(ierr);
  ierr = MatGetArray(A,&array);CHKERRQ(ierr);
  PLA_API_axpy_matrix_to_global(m,M, &one,(void *)array,m,lu->A,rstart,0); 
  ierr = MatRestoreArray(A,&array);CHKERRQ(ierr);
  PLA_Obj_API_close(lu->A); 
  PLA_API_end(); 

  /* Factor P A -> L U overwriting lower triangular portion of A with L, upper, U */
  info_pla = PLA_LU(lu->A,lu->pivots);
  if (info_pla != 0) 
    SETERRQ1(PETSC_ERR_MAT_LU_ZRPVT,"Zero pivot encountered at row %d from PLA_LU()",info_pla);

  lu->CleanUpPlapack = PETSC_TRUE;
  lu->rstart         = rstart;
  
  (*F)->ops->solve   = MatSolve_Plapack; 
  (*F)->factor       = FACTOR_LU;
  (*F)->assembled    = PETSC_TRUE;  /* required by -ksp_view */

  lu->flg            = SAME_NONZERO_PATTERN;
  PetscFunctionReturn(0);
}

/* Note the Petsc r and c permutations are ignored */
#undef __FUNCT__  
#define __FUNCT__ "MatLUFactorSymbolic_Plapack"
PetscErrorCode MatLUFactorSymbolic_Plapack(Mat A,IS r,IS c,MatFactorInfo *info,Mat *F)
{
  Mat            B;
  Mat_Plapack    *lu;   
  PetscErrorCode ierr;
  PetscInt       M=A->M,N=A->N;
  MPI_Comm       comm=A->comm,comm_2d;
  PetscMPIInt    size,rank;
  PetscInt       ierror,nb;

  PetscFunctionBegin;
  /* Create the factorization matrix */
  ierr = MatCreate(A->comm,&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,A->m,A->n,M,N);CHKERRQ(ierr);
  ierr = MatSetType(B,A->type_name);CHKERRQ(ierr);

  B->ops->lufactornumeric  = MatLUFactorNumeric_Plapack;
  B->ops->solve            = MatSolve_Plapack;
  B->factor                = FACTOR_LU;  

  lu = (Mat_Plapack*)(B->spptr);

  /* Set default Plapack parameters */
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  lu->nprows = size; lu->npcols = 1; 
  ierror = 0;
  lu->nb     = M/size;
  if (M - lu->nb*size) lu->nb++; /* without cyclic distribution */
 
  /* Set runtime options */
  ierr = PetscOptionsBegin(A->comm,A->prefix,"PLAPACK Options","Mat");CHKERRQ(ierr);
  /* Give wrong solution when both nprows and npcols are larger than 1! Enable these options when the problem is fixed */
  /*
  ierr = PetscOptionsInt("-mat_plapack_nprows","row dimension of 2D processor mesh","None",lu->nprows,&lu->nprows,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_plapack_npcols","column dimension of 2D processor mesh","None",lu->npcols,&lu->npcols,PETSC_NULL);CHKERRQ(ierr);
  */
  ierr = PetscOptionsInt("-mat_plapack_nb","block size of template vector","None",lu->nb,&lu->nb,PETSC_NULL);CHKERRQ(ierr); 
  ierr = PetscOptionsInt("-mat_plapack_ckerror","error checking flags","None",ierror,&ierror,PETSC_NULL);CHKERRQ(ierr);  
  PetscOptionsEnd(); 

  if ( ierror ){
    PLA_Set_error_checking(ierror,PETSC_TRUE,PETSC_TRUE,PETSC_FALSE );
  } else {
    PLA_Set_error_checking(ierror,PETSC_FALSE,PETSC_FALSE,PETSC_FALSE );
  }
  lu->ierror = ierror;

  /* Create a 2D communicator */
  PLA_Comm_1D_to_2D(comm,lu->nprows,lu->npcols,&comm_2d); 

  /* Initialize PLAPACK */
  PLA_Init(comm_2d);

  /* Create object distribution template */
  PLA_Temp_create(lu->nb, 0, &lu->templ);


  /* Set the datatype */
#if defined(PETSC_USE_COMPLEX)
  lu->datatype = MPI_DOUBLE_COMPLEX;
#else
  lu->datatype = MPI_DOUBLE;
#endif

  lu->pla_solved     = PETSC_FALSE; /* MatSolve_Plapack() is called yet */
  lu->flg            = DIFFERENT_NONZERO_PATTERN;
  lu->CleanUpPlapack = PETSC_TRUE;
  *F                 = B;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_Plapack"
PetscErrorCode MatAssemblyEnd_Plapack(Mat A,MatAssemblyType mode) 
{
  PetscErrorCode   ierr;
  Mat_Plapack      *lu=(Mat_Plapack*)(A->spptr);

  PetscFunctionBegin;
  ierr = (*lu->MatAssemblyEnd)(A,mode);CHKERRQ(ierr);
  lu->MatLUFactorSymbolic  = A->ops->lufactorsymbolic;
  A->ops->lufactorsymbolic = MatLUFactorSymbolic_Plapack;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFactorInfo_Plapack"
PetscErrorCode MatFactorInfo_Plapack(Mat A,PetscViewer viewer)
{
  Mat_Plapack       *lu=(Mat_Plapack*)A->spptr;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  /* check if matrix is plapack type */
  if (A->ops->solve != MatSolve_Plapack) PetscFunctionReturn(0);

  ierr = PetscViewerASCIIPrintf(viewer,"PLAPACK run parameters:\n");CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  Processor mesh: nprows %d, npcols %d\n",lu->nprows, lu->npcols);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  Distr. block size nb: %d \n",lu->nb);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  Error checking: %d\n",lu->ierror);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_Plapack"
PetscErrorCode MatView_Plapack(Mat A,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscTruth        iascii;
  PetscViewerFormat format;
  Mat_Plapack       *lu=(Mat_Plapack*)(A->spptr);

  PetscFunctionBegin;
  /* ierr = (*lu->MatView)(A,viewer);CHKERRQ(ierr); MatView_MPIDense() crash! */ 
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_FACTOR_INFO) {
      ierr = MatFactorInfo_Plapack(A,viewer);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "MatConvert_Base_Plapack"
PetscErrorCode PETSCMAT_DLLEXPORT MatConvert_Base_Plapack(Mat A,MatType type,MatReuse reuse,Mat *newmat) 
{
  /* This routine is only called to convert to MATPLAPACK from MATDENSE, so we ignore 'MatType type'. */
  PetscErrorCode ierr;
  PetscMPIInt    size;
  Mat            B=*newmat;
  Mat_Plapack    *lu;

  PetscFunctionBegin;
  if (reuse == MAT_INITIAL_MATRIX) {
    ierr = MatDuplicate(A,MAT_COPY_VALUES,&B);CHKERRQ(ierr);
  }

  ierr = PetscNew(Mat_Plapack,&lu);CHKERRQ(ierr);
  lu->MatDuplicate         = A->ops->duplicate;
  lu->MatView              = A->ops->view;
  lu->MatAssemblyEnd       = A->ops->assemblyend;
  lu->MatLUFactorSymbolic  = A->ops->lufactorsymbolic;
  lu->MatDestroy           = A->ops->destroy;
  lu->CleanUpPlapack       = PETSC_FALSE;

  B->spptr                 = (void*)lu;
  B->ops->duplicate        = MatDuplicate_Plapack;
  B->ops->view             = MatView_Plapack;
  B->ops->assemblyend      = MatAssemblyEnd_Plapack;
  B->ops->lufactorsymbolic = MatLUFactorSymbolic_Plapack;
  B->ops->destroy          = MatDestroy_Plapack;
  ierr = MPI_Comm_size(A->comm,&size);CHKERRQ(ierr);CHKERRQ(ierr);
  if (size == 1) { 
    ierr = PetscObjectComposeFunctionDynamic((PetscObject)B,"MatConvert_seqdense_plapack_C",
                                             "MatConvert_Base_Plapack",MatConvert_Base_Plapack);CHKERRQ(ierr);
    ierr = PetscObjectComposeFunctionDynamic((PetscObject)B,"MatConvert_plapack_seqdense_C",
                                             "MatConvert_Plapack_Base",MatConvert_Plapack_Base);CHKERRQ(ierr);
  } else {
    ierr = PetscObjectComposeFunctionDynamic((PetscObject)B,"MatConvert_mpidense_plapack_C",
                                             "MatConvert_Base_Plapack",MatConvert_Base_Plapack);CHKERRQ(ierr);
    ierr = PetscObjectComposeFunctionDynamic((PetscObject)B,"MatConvert_plapack_mpidense_C",
                                             "MatConvert_Plapack_Base",MatConvert_Plapack_Base);CHKERRQ(ierr);
  }   
  ierr = PetscLogInfo((0,"MatConvert_Base_Plapack:Using Plapack for dense LU factorization and solves.\n"));CHKERRQ(ierr); 
  ierr = PetscObjectChangeTypeName((PetscObject)B,MATPLAPACK);CHKERRQ(ierr);
  *newmat = B;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__
#define __FUNCT__ "MatDuplicate_Plapack"
PetscErrorCode MatDuplicate_Plapack(Mat A, MatDuplicateOption op, Mat *M) 
{
  PetscErrorCode ierr;
  Mat_Plapack    *lu=(Mat_Plapack *)A->spptr,*lu_new;

  PetscFunctionBegin;
  ierr = (*lu->MatDuplicate)(A,op,M);CHKERRQ(ierr);
  ierr = PetscMemcpy((*M)->spptr,lu,sizeof(Mat_Plapack));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
  MATPLAPACK - MATPLAPACK = "plapack" - A matrix type providing direct solvers (LU, Cholesky, and QR) 
  for parallel dense matrices via the external package PLAPACK.

  If PLAPACK is installed (see the manual for
  instructions on how to declare the existence of external packages),
  a matrix type can be constructed which invokes PLAPACK solvers.
  After calling MatCreate(...,A), simply call MatSetType(A,MATPLAPACK).

  This matrix inherits from MATSEQDENSE when constructed with a single process communicator,
  and from MATMPIDENSE otherwise. One can also call MatConvert for an inplace
  conversion to or from the MATSEQDENSE or MATMPIDENSE type (depending on the communicator size)
  without data copy.

  Options Database Keys:
+ -mat_type plapack - sets the matrix type to "plapack" during a call to MatSetFromOptions()
. -mat_plapack_r <n> - number of rows in processor partition
. -mat_plapack_c <n> - number of columns in processor partition
- -mat_plapack_nb <n> - distribution block size

   Level: beginner

.seealso: MATDENSE, PCLU 
M*/

EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "MatCreate_Plapack"
PetscErrorCode PETSCMAT_DLLEXPORT MatCreate_Plapack(Mat A) 
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  /* Change type name before calling MatSetType to force proper construction */
  /*   of SEQDENSE or MPIDENSE  and Plapack types */
  ierr = PetscObjectChangeTypeName((PetscObject)A,MATPLAPACK);CHKERRQ(ierr);
  ierr = MPI_Comm_size(A->comm,&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr = MatSetType(A,MATSEQDENSE);CHKERRQ(ierr);
  } else {
    ierr = MatSetType(A,MATMPIDENSE);CHKERRQ(ierr);
  }
  ierr = MatConvert_Base_Plapack(A,MATPLAPACK,MAT_REUSE_MATRIX,&A);CHKERRQ(ierr); 
  PetscFunctionReturn(0);
}
EXTERN_C_END

