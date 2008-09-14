#define PETSCMAT_DLL

/* 
        Provides an interface to the PLAPACKR32 dense solver
*/

#include "src/mat/impls/dense/seq/dense.h"
#include "src/mat/impls/dense/mpi/mpidense.h"

#if defined(PETSC_HAVE_PLAPACK) 
EXTERN_C_BEGIN 
#include "PLA.h"
#include "PLA_prototypes.h"
EXTERN_C_END 

typedef struct {
  MPI_Comm       comm_2d;
  PLA_Obj        A,pivots;
  PLA_Template   templ;
  MPI_Datatype   datatype;
  PetscInt       nb,nb_alg,ierror,rstart;
  VecScatter     ctx;
  IS             is_pla,is_petsc;
  PetscTruth     pla_solved;
  MatStructure   mstruct;
  PetscMPIInt    nprows,npcols;

  /* Flag to clean up (non-global) Plapack objects during Destroy */
  PetscTruth CleanUpPlapack;
} Mat_Plapack;

#undef __FUNCT__  
#define __FUNCT__ "MatDestroy_Plapack"
PetscErrorCode MatDestroy_Plapack(Mat A)
{
  PetscErrorCode ierr;
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
  ierr = MatDestroy_MPIDense(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatSolve_Plapack"
PetscErrorCode MatSolve_Plapack(Mat A,Vec b,Vec x)
{
  MPI_Comm       comm = ((PetscObject)A)->comm;
  Mat_Plapack    *lu = (Mat_Plapack*)A->spptr;
  PetscErrorCode ierr;
  PetscInt       M=A->rmap->N,m=A->rmap->n,rstart,i,j,*idx_pla,*idx_petsc,loc_m,loc_stride;
  PetscScalar    *array;
  PetscReal      one = 1.0;
  PetscMPIInt    size,rank,r_rank,r_nproc,c_rank,c_nproc;;
  PLA_Obj        v_pla = NULL;
  PetscScalar    *loc_buf;
  Vec            loc_x;
   
  PetscFunctionBegin;
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  /* Create PLAPACK vector objects, then copy b into PLAPACK b */
  PLA_Mvector_create(lu->datatype,M,1,lu->templ,PLA_ALIGN_FIRST,&v_pla);  
  PLA_Obj_set_to_zero(v_pla);

  /* Copy b into rhs_pla */
  PLA_API_begin();   
  PLA_Obj_API_open(v_pla);
  ierr = VecGetArray(b,&array);CHKERRQ(ierr);
  PLA_API_axpy_vector_to_global(m,&one,(void *)array,1,v_pla,lu->rstart);
  ierr = VecRestoreArray(b,&array);CHKERRQ(ierr);
  PLA_Obj_API_close(v_pla);
  PLA_API_end(); 

  if (A->factor == MAT_FACTOR_LU){
    /* Apply the permutations to the right hand sides */
    PLA_Apply_pivots_to_rows (v_pla,lu->pivots);

    /* Solve L y = b, overwriting b with y */
    PLA_Trsv( PLA_LOWER_TRIANGULAR,PLA_NO_TRANSPOSE,PLA_UNIT_DIAG,lu->A,v_pla );

    /* Solve U x = y (=b), overwriting b with x */
    PLA_Trsv( PLA_UPPER_TRIANGULAR,PLA_NO_TRANSPOSE,PLA_NONUNIT_DIAG,lu->A,v_pla );
  } else { /* MAT_FACTOR_CHOLESKY */
    PLA_Trsv( PLA_LOWER_TRIANGULAR,PLA_NO_TRANSPOSE,PLA_NONUNIT_DIAG,lu->A,v_pla);
    PLA_Trsv( PLA_LOWER_TRIANGULAR,(lu->datatype == MPI_DOUBLE ? PLA_TRANSPOSE : PLA_CONJUGATE_TRANSPOSE),
                                    PLA_NONUNIT_DIAG,lu->A,v_pla);
  }

  /* Copy PLAPACK x into Petsc vector x  */   
  PLA_Obj_local_length(v_pla, &loc_m);
  PLA_Obj_local_buffer(v_pla, (void**)&loc_buf);
  PLA_Obj_local_stride(v_pla, &loc_stride);
  /*
    PetscPrintf(PETSC_COMM_SELF," [%d] b - local_m %d local_stride %d, loc_buf: %g %g, nb: %d\n",rank,loc_m,loc_stride,loc_buf[0],loc_buf[(loc_m-1)*loc_stride],lu->nb); 
  */
  ierr = VecCreateSeqWithArray(PETSC_COMM_SELF,loc_m*loc_stride,loc_buf,&loc_x);CHKERRQ(ierr);
  if (!lu->pla_solved){
    
    PLA_Temp_comm_row_info(lu->templ,&lu->comm_2d,&r_rank,&r_nproc);
    PLA_Temp_comm_col_info(lu->templ,&lu->comm_2d,&c_rank,&c_nproc);
    /* printf(" [%d] rank: %d %d, nproc: %d %d\n",rank,r_rank,c_rank,r_nproc,c_nproc); */

    /* Create IS and cts for VecScatterring */
    PLA_Obj_local_length(v_pla, &loc_m);
    PLA_Obj_local_stride(v_pla, &loc_stride);
    ierr = PetscMalloc((2*loc_m+1)*sizeof(PetscInt),&idx_pla);CHKERRQ(ierr);
    idx_petsc = idx_pla + loc_m;

    rstart = (r_rank*c_nproc+c_rank)*lu->nb;
    for (i=0; i<loc_m; i+=lu->nb){
      j = 0; 
      while (j < lu->nb && i+j < loc_m){
        idx_petsc[i+j] = rstart + j; j++;
      }
      rstart += size*lu->nb;
    }

    for (i=0; i<loc_m; i++) idx_pla[i] = i*loc_stride;

    ierr = ISCreateGeneral(PETSC_COMM_SELF,loc_m,idx_pla,&lu->is_pla);CHKERRQ(ierr);
    ierr = ISCreateGeneral(PETSC_COMM_SELF,loc_m,idx_petsc,&lu->is_petsc);CHKERRQ(ierr);  
    ierr = PetscFree(idx_pla);CHKERRQ(ierr);
    ierr = VecScatterCreate(loc_x,lu->is_pla,x,lu->is_petsc,&lu->ctx);CHKERRQ(ierr);
  }
  ierr = VecScatterBegin(lu->ctx,loc_x,x,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(lu->ctx,loc_x,x,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  
  /* Free data */
  ierr = VecDestroy(loc_x);CHKERRQ(ierr);
  PLA_Obj_free(&v_pla); 

  lu->pla_solved = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__   
#define __FUNCT__ "MatLUFactorNumeric_Plapack"
PetscErrorCode MatLUFactorNumeric_Plapack(Mat A,MatFactorInfo *info,Mat *F)
{
  Mat_Plapack    *lu = (Mat_Plapack*)(*F)->spptr;
  PetscErrorCode ierr;
  PetscInt       M=A->rmap->N,m=A->rmap->n,rstart,rend;
  PetscInt       info_pla=0;
  PetscScalar    *array,one = 1.0;

  PetscFunctionBegin;
  if (lu->mstruct == SAME_NONZERO_PATTERN){
    PLA_Obj_free(&lu->A);
    PLA_Obj_free (&lu->pivots);
  }
  /* Create PLAPACK matrix object */
  lu->A = NULL; lu->pivots = NULL;
  PLA_Matrix_create(lu->datatype,M,M,lu->templ,PLA_ALIGN_FIRST,PLA_ALIGN_FIRST,&lu->A);  
  PLA_Obj_set_to_zero(lu->A);
  PLA_Mvector_create(MPI_INT,M,1,lu->templ,PLA_ALIGN_FIRST,&lu->pivots);

  /* Copy A into lu->A */
  PLA_API_begin();
  PLA_Obj_API_open(lu->A);  
  ierr = MatGetOwnershipRange(A,&rstart,&rend);CHKERRQ(ierr);
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
  lu->mstruct        = SAME_NONZERO_PATTERN;
  
  (*F)->assembled    = PETSC_TRUE;  /* required by -ksp_view */
  PetscFunctionReturn(0);
}

#undef __FUNCT__   
#define __FUNCT__ "MatCholeskyFactorNumeric_Plapack"
PetscErrorCode MatCholeskyFactorNumeric_Plapack(Mat A,MatFactorInfo *info,Mat *F)
{
  Mat_Plapack    *lu = (Mat_Plapack*)(*F)->spptr;
  PetscErrorCode ierr;
  PetscInt       M=A->rmap->N,m=A->rmap->n,rstart,rend;
  PetscInt       info_pla=0;
  PetscScalar    *array,one = 1.0;

  PetscFunctionBegin;
  if (lu->mstruct == SAME_NONZERO_PATTERN){
    PLA_Obj_free(&lu->A);
  }
  /* Create PLAPACK matrix object */
  lu->A      = NULL; 
  lu->pivots = NULL; 
  PLA_Matrix_create(lu->datatype,M,M,lu->templ,PLA_ALIGN_FIRST,PLA_ALIGN_FIRST,&lu->A);  

  /* Copy A into lu->A */
  PLA_API_begin();
  PLA_Obj_API_open(lu->A);  
  ierr = MatGetOwnershipRange(A,&rstart,&rend);CHKERRQ(ierr);
  ierr = MatGetArray(A,&array);CHKERRQ(ierr);
  PLA_API_axpy_matrix_to_global(m,M, &one,(void *)array,m,lu->A,rstart,0); 
  ierr = MatRestoreArray(A,&array);CHKERRQ(ierr);
  PLA_Obj_API_close(lu->A); 
  PLA_API_end(); 

  /* Factor P A -> Chol */
  info_pla = PLA_Chol(PLA_LOWER_TRIANGULAR,lu->A);
  if (info_pla != 0) 
    SETERRQ1( PETSC_ERR_MAT_CH_ZRPVT,"Nonpositive definite matrix detected at row %d from PLA_Chol()",info_pla);

  lu->CleanUpPlapack = PETSC_TRUE;
  lu->rstart         = rstart;
  lu->mstruct        = SAME_NONZERO_PATTERN;
  
  (*F)->assembled    = PETSC_TRUE;  /* required by -ksp_view */
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatFactorSymbolic_Plapack_Private"
PetscErrorCode MatFactorSymbolic_Plapack_Private(Mat A,MatFactorInfo *info,Mat *F)
{
  Mat            B = *F;
  Mat_Plapack    *lu;   
  PetscErrorCode ierr;
  PetscInt       M=A->rmap->N,N=A->cmap->N;
  MPI_Comm       comm=((PetscObject)A)->comm,comm_2d;
  PetscMPIInt    size;
  PetscInt       ierror;

  PetscFunctionBegin;
  lu = (Mat_Plapack*)(B->spptr);

  /* Set default Plapack parameters */
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  lu->nprows = 1; lu->npcols = size; 
  ierror = 0;
  lu->nb     = M/size;
  if (M - lu->nb*size) lu->nb++; /* without cyclic distribution */
 
  /* Set runtime options */
  ierr = PetscOptionsBegin(((PetscObject)A)->comm,((PetscObject)A)->prefix,"PLAPACK Options","Mat");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_plapack_nprows","row dimension of 2D processor mesh","None",lu->nprows,&lu->nprows,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_plapack_npcols","column dimension of 2D processor mesh","None",lu->npcols,&lu->npcols,PETSC_NULL);CHKERRQ(ierr);
  
  ierr = PetscOptionsInt("-mat_plapack_nb","block size of template vector","None",lu->nb,&lu->nb,PETSC_NULL);CHKERRQ(ierr); 
  ierr = PetscOptionsInt("-mat_plapack_ckerror","error checking flag","None",ierror,&ierror,PETSC_NULL);CHKERRQ(ierr);  
  if (ierror){
    PLA_Set_error_checking(ierror,PETSC_TRUE,PETSC_TRUE,PETSC_FALSE );
  } else {
    PLA_Set_error_checking(ierror,PETSC_FALSE,PETSC_FALSE,PETSC_FALSE );
  }
  lu->ierror = ierror;
  
  lu->nb_alg = 0;
  ierr = PetscOptionsInt("-mat_plapack_nb_alg","algorithmic block size","None",lu->nb_alg,&lu->nb_alg,PETSC_NULL);CHKERRQ(ierr);
  if (lu->nb_alg){
    pla_Environ_set_nb_alg (PLA_OP_ALL_ALG,lu->nb_alg);
  }
  PetscOptionsEnd(); 


  /* Create a 2D communicator */
  PLA_Comm_1D_to_2D(comm,lu->nprows,lu->npcols,&comm_2d); 
  lu->comm_2d = comm_2d;

  /* Initialize PLAPACK */
  PLA_Init(comm_2d);

  /* Create object distribution template */
  lu->templ = NULL;
  PLA_Temp_create(lu->nb, 0, &lu->templ);

  /* Use suggested nb_alg if it is not provided by user */
  if (lu->nb_alg == 0){
    PLA_Environ_nb_alg(PLA_OP_PAN_PAN,lu->templ,&lu->nb_alg);
    pla_Environ_set_nb_alg(PLA_OP_ALL_ALG,lu->nb_alg);
  }

  /* Set the datatype */
#if defined(PETSC_USE_COMPLEX)
  lu->datatype = MPI_DOUBLE_COMPLEX;
#else
  lu->datatype = MPI_DOUBLE;
#endif

  lu->pla_solved     = PETSC_FALSE; /* MatSolve_Plapack() is called yet */
  lu->mstruct        = DIFFERENT_NONZERO_PATTERN;
  lu->CleanUpPlapack = PETSC_TRUE;
  *F                 = B;
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
  ierr = PetscViewerASCIIPrintf(viewer,"  Algorithmic block size: %d\n",lu->nb_alg);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_Plapack"
PetscErrorCode MatView_Plapack(Mat A,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscTruth        iascii;
  PetscViewerFormat format;

  PetscFunctionBegin;
  /* ierr = (*lu->MatView)(A,viewer);CHKERRQ(ierr); MatView_MPIDense() crash! */ 
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_INFO) {
      ierr = MatFactorInfo_Plapack(A,viewer);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}


/*MC
  MATPLAPACK - MATPLAPACK = "plapack" - A matrix type providing direct solvers (LU, Cholesky, and QR) 
  for parallel dense matrices via the external package PLAPACK.

  If PLAPACK is installed (run config/configure.py with the option --download-plapack)
  a matrix type can be constructed which invokes PLAPACK solvers.
  After calling MatCreate(...,A), simply call MatSetType(A,MATPLAPACK).

  This matrix inherits from MATSEQDENSE when constructed with a single process communicator,
  and from MATMPIDENSE otherwise. One can also call MatConvert for an inplace
  conversion to or from the MATSEQDENSE or MATMPIDENSE type (depending on the communicator size)
  without data copy.

  Options Database Keys:
+ -mat_type plapack - sets the matrix type to "plapack" during a call to MatSetFromOptions()
. -mat_plapack_nprows <n> - number of rows in processor partition
. -mat_plapack_npcols <n> - number of columns in processor partition
. -mat_plapack_nb <n> - block size of template vector
. -mat_plapack_nb_alg <n> - algorithmic block size
- -mat_plapack_ckerror <n> - error checking flag

   Level: beginner

.seealso: MATDENSE, PCLU, PCCHOLESKY
M*/

#endif
