/*
  Defines projective product routines where A is a AIJ matrix
          C = P^T * A * P
*/

#include "src/mat/impls/aij/seq/aij.h"   /*I "petscmat.h" I*/
#include "src/mat/utils/freespace.h"
#include "src/mat/impls/aij/mpi/mpiaij.h"

#undef __FUNCT__
#define __FUNCT__ "MatPtAP"
/*@
   MatPtAP - Creates the matrix projection C = P^T * A * P

   Collective on Mat

   Input Parameters:
+  A - the matrix
.  P - the projection matrix
.  scall - either MAT_INITIAL_MATRIX or MAT_REUSE_MATRIX
-  fill - expected fill as ratio of nnz(C)/(nnz(A) + nnz(P))

   Output Parameters:
.  C - the product matrix

   Notes:
   C will be created and must be destroyed by the user with MatDestroy().

   This routine is currently only implemented for pairs of SeqAIJ matrices and classes
   which inherit from SeqAIJ.  C will be of type MATSEQAIJ.

   Level: intermediate

.seealso: MatPtAPSymbolic(),MatPtAPNumeric(),MatMatMult()
@*/
PetscErrorCode MatPtAP(Mat A,Mat P,MatReuse scall,PetscReal fill,Mat *C) {
  PetscErrorCode ierr;
  PetscErrorCode (*fA)(Mat,Mat,MatReuse,PetscReal,Mat *);
  PetscErrorCode (*fP)(Mat,Mat,MatReuse,PetscReal,Mat *);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_COOKIE,1);
  PetscValidType(A,1);
  MatPreallocated(A);
  if (!A->assembled) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (A->factor) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix"); 
  PetscValidHeaderSpecific(P,MAT_COOKIE,2);
  PetscValidType(P,2);
  MatPreallocated(P);
  if (!P->assembled) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (P->factor) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix"); 
  PetscValidPointer(C,3);

  if (P->M!=A->N) SETERRQ2(PETSC_ERR_ARG_SIZ,"Matrix dimensions are incompatible, %d != %d",P->M,A->N);

  if (fill <=0.0) SETERRQ1(PETSC_ERR_ARG_SIZ,"fill=%g must be > 0.0",fill);

  /* For now, we do not dispatch based on the type of A and P */
  /* When implementations like _SeqAIJ_MAIJ exist, attack the multiple dispatch problem. */  
  fA = A->ops->ptap;
  if (!fA) SETERRQ1(PETSC_ERR_SUP,"MatPtAP not supported for A of type %s",A->type_name);
  fP = P->ops->ptap;
  if (!fP) SETERRQ1(PETSC_ERR_SUP,"MatPtAP not supported for P of type %s",P->type_name);
  if (fP!=fA) SETERRQ2(PETSC_ERR_ARG_INCOMP,"MatPtAP requires A, %s, to be compatible with P, %s",A->type_name,P->type_name);

  ierr = PetscLogEventBegin(MAT_PtAP,A,P,0,0);CHKERRQ(ierr); 
  ierr = (*fA)(A,P,scall,fill,C);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_PtAP,A,P,0,0);CHKERRQ(ierr); 
  PetscFunctionReturn(0);
}

EXTERN PetscErrorCode MatPtAP_SeqAIJ_SeqAIJ_ReducedPt(Mat,Mat,MatReuse,PetscReal,int,int,Mat*); 
EXTERN PetscErrorCode MatPtAPSymbolic_SeqAIJ_SeqAIJ_ReducedPt(Mat,Mat,PetscReal,int,int,Mat*);
EXTERN PetscErrorCode MatPtAPNumeric_SeqAIJ_SeqAIJ_ReducedPt(Mat,Mat,int,int,Mat);
#undef __FUNCT__
#define __FUNCT__ "MatPtAP_MPIAIJ_MPIAIJ"
PetscErrorCode MatPtAP_MPIAIJ_MPIAIJ(Mat A,Mat P,MatReuse scall,PetscReal fill,Mat *C) 
{
  PetscErrorCode    ierr;
  Mat               AP,AP_seq,P_seq,A_loc,C_seq;
  Mat_MPIAIJ        *p = (Mat_MPIAIJ*)P->data;
  Mat_MatMatMultMPI *mult;

  int               prstart,prend,m=P->m;
  int               rank,prid=10;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(A->comm,&rank);CHKERRQ(ierr);

  /* compute symbolic and numeric AP = A*P */
  ierr = MatMatMult_MPIAIJ_MPIAIJ(A,P,scall,fill,&AP);CHKERRQ(ierr);
  mult = (Mat_MatMatMultMPI*)AP->spptr;
  P_seq = mult->bseq[0]; /* = submatrix of P by taking rows of P that equal to nonzero col of A */
  A_loc = mult->aseq[0]; /* = submatrix of A by taking all local rows of A */
  AP_seq  = mult->C_seq;
  
  if (rank == prid){
    ierr = PetscPrintf(PETSC_COMM_SELF," [%d] A_loc: %d, %d\n",rank,A_loc->m,A_loc->n);CHKERRQ(ierr);
    ierr = MatView(A_loc,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_SELF," [%d] P_seq: %d, %d\n",rank,P_seq->m,P_seq->n);CHKERRQ(ierr);
    ierr = MatView(P_seq,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_SELF," [%d]  AP_seq: %d, %d\n",rank,AP_seq->m,AP_seq->n);
    ierr = MatView(AP_seq,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
  }

  prstart = mult->brstart;
  prend   = prstart + m;
  /*
  ierr = PetscPrintf(PETSC_COMM_SELF," [%d] prstart: %d, prend: %d, dim of P_seq: %d, %d\n",rank,prstart,prend,P_seq->m,P_seq->n);
  */

    /* get seq matrix P_subseq by taking local rows of P */
  IS  isrow,iscol;
  Mat P_subseq,*psubseq;
  ierr = ISCreateStride(PETSC_COMM_SELF,m,prstart,1,&isrow);CHKERRQ(ierr);
  ierr = ISCreateStride(PETSC_COMM_SELF,P_seq->n,0,1,&iscol);CHKERRQ(ierr);
  ierr = MatGetSubMatrices(P_seq,1,&isrow,&iscol,MAT_INITIAL_MATRIX,&psubseq);CHKERRQ(ierr); 
  P_subseq = psubseq[0];
  ierr = ISDestroy(isrow);CHKERRQ(ierr);
  ierr = ISDestroy(iscol);CHKERRQ(ierr);

  /* compute P_subseq^T*AP_seq */
  ierr = MatMatMultTranspose_SeqAIJ_SeqAIJ(P_subseq,AP_seq,scall,fill,&C_seq);CHKERRQ(ierr); 
  if (rank == prid){
    ierr = PetscPrintf(PETSC_COMM_SELF," [%d] dim of P_subseq: %d, %d; AP_seq: %d, %d\n",rank,P_subseq->m,P_subseq->n,AP_seq->m,AP_seq->n);
    ierr = PetscPrintf(PETSC_COMM_SELF," [%d] C_seq: %d, %d\n",rank,C_seq->m,C_seq->n);CHKERRQ(ierr);
    ierr = MatView(C_seq,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
  }
  ierr = MatDestroyMatrices(1,&psubseq);CHKERRQ(ierr);

  /* ierr = MatPtAP_SeqAIJ_SeqAIJ_ReducedPt(A_loc,P_seq,scall,fill,prstart,prend,&C_seq);CHKERRQ(ierr); */
  /* ierr = PetscPrintf(PETSC_COMM_SELF," [%d] C_seq dim: %d, %d\n",rank,C_seq->m,C_seq->n); */
  
  /* add C_seq into C */
  ierr = MatMerge_SeqsToMPI(A->comm,C_seq,P->n,P->n,scall,C);CHKERRQ(ierr); 

  /* clean up */
  ierr = MatDestroy(AP);CHKERRQ(ierr);
  ierr = MatDestroy(C_seq);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatMatMultSymbolic_MPIAIJ_MPIAIJ"
PetscErrorCode MatPtAPSymbolic_MPIAIJ_MPIAIJ(Mat A,Mat B,PetscReal fill,Mat *C)
{
  
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatMatMultSymbolic_MPIAIJ_MPIAIJ"
PetscErrorCode MatPtAPNumeric_MPIAIJ_MPIAIJ(Mat A,Mat B,Mat C)
{
 
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatPtAP_SeqAIJ_SeqAIJ"
PetscErrorCode MatPtAP_SeqAIJ_SeqAIJ(Mat A,Mat P,MatReuse scall,PetscReal fill,Mat *C) 
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX){
    ierr = PetscLogEventBegin(MAT_PtAPSymbolic,A,P,0,0);CHKERRQ(ierr);
    ierr = MatPtAPSymbolic_SeqAIJ_SeqAIJ(A,P,fill,C);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(MAT_PtAPSymbolic,A,P,0,0);CHKERRQ(ierr); 
  }
  ierr = PetscLogEventBegin(MAT_PtAPNumeric,A,P,0,0);CHKERRQ(ierr);
  ierr = MatPtAPNumeric_SeqAIJ_SeqAIJ(A,P,*C);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_PtAPNumeric,A,P,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatPtAPSymbolic"
/*
   MatPtAPSymbolic - Creates the (i,j) structure of the matrix projection C = P^T * A * P

   Collective on Mat

   Input Parameters:
+  A - the matrix
-  P - the projection matrix

   Output Parameters:
.  C - the (i,j) structure of the product matrix

   Notes:
   C will be created and must be destroyed by the user with MatDestroy().

   This routine is currently only implemented for pairs of SeqAIJ matrices and classes
   which inherit from SeqAIJ.  C will be of type MATSEQAIJ.  The product is computed using
   this (i,j) structure by calling MatPtAPNumeric().

   Level: intermediate

.seealso: MatPtAP(),MatPtAPNumeric(),MatMatMultSymbolic()
*/
PetscErrorCode MatPtAPSymbolic(Mat A,Mat P,PetscReal fill,Mat *C) {
  PetscErrorCode ierr;
  PetscErrorCode (*fA)(Mat,Mat,PetscReal,Mat*);
  PetscErrorCode (*fP)(Mat,Mat,PetscReal,Mat*);

  PetscFunctionBegin;

  PetscValidHeaderSpecific(A,MAT_COOKIE,1);
  PetscValidType(A,1);
  MatPreallocated(A);
  if (!A->assembled) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (A->factor) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix"); 

  PetscValidHeaderSpecific(P,MAT_COOKIE,2);
  PetscValidType(P,2);
  MatPreallocated(P);
  if (!P->assembled) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (P->factor) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix"); 

  PetscValidPointer(C,3);

  if (P->M!=A->N) SETERRQ2(PETSC_ERR_ARG_SIZ,"Matrix dimensions are incompatible, %d != %d",P->M,A->N);
  if (A->M!=A->N) SETERRQ2(PETSC_ERR_ARG_SIZ,"Matrix 'A' must be square, %d != %d",A->M,A->N);

  /* For now, we do not dispatch based on the type of A and P */
  /* When implementations like _SeqAIJ_MAIJ exist, attack the multiple dispatch problem. */  
  fA = A->ops->ptapsymbolic;
  if (!fA) SETERRQ1(PETSC_ERR_SUP,"MatPtAPSymbolic not supported for A of type %s",A->type_name);
  fP = P->ops->ptapsymbolic;
  if (!fP) SETERRQ1(PETSC_ERR_SUP,"MatPtAPSymbolic not supported for P of type %s",P->type_name);
  if (fP!=fA) SETERRQ2(PETSC_ERR_ARG_INCOMP,"MatPtAPSymbolic requires A, %s, to be compatible with P, %s",A->type_name,P->type_name);

  ierr = PetscLogEventBegin(MAT_PtAPSymbolic,A,P,0,0);CHKERRQ(ierr); 
  ierr = (*fA)(A,P,fill,C);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_PtAPSymbolic,A,P,0,0);CHKERRQ(ierr); 
 
  PetscFunctionReturn(0);
}

typedef struct { 
  Mat    symAP;
} Mat_PtAPstruct;

EXTERN PetscErrorCode MatDestroy_SeqAIJ(Mat);

#undef __FUNCT__  
#define __FUNCT__ "MatDestroy_SeqAIJ_PtAP"
PetscErrorCode MatDestroy_SeqAIJ_PtAP(Mat A)
{
  PetscErrorCode    ierr;
  Mat_PtAPstruct    *ptap=(Mat_PtAPstruct*)A->spptr; 

  PetscFunctionBegin;
  ierr = MatDestroy(ptap->symAP);CHKERRQ(ierr);
  ierr = PetscFree(ptap);CHKERRQ(ierr);
  ierr = MatDestroy_SeqAIJ(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatPtAPSymbolic_SeqAIJ_SeqAIJ"
PetscErrorCode MatPtAPSymbolic_SeqAIJ_SeqAIJ(Mat A,Mat P,PetscReal fill,Mat *C) {
  PetscErrorCode ierr;
  FreeSpaceList  free_space=PETSC_NULL,current_space=PETSC_NULL;
  Mat_SeqAIJ     *a=(Mat_SeqAIJ*)A->data,*p=(Mat_SeqAIJ*)P->data,*c;
  int            *pti,*ptj,*ptJ,*ai=a->i,*aj=a->j,*ajj,*pi=p->i,*pj=p->j,*pjj;
  int            *ci,*cj,*denserow,*sparserow,*ptadenserow,*ptasparserow,*ptaj;
  int            an=A->N,am=A->M,pn=P->N,pm=P->M;
  int            i,j,k,ptnzi,arow,anzj,ptanzi,prow,pnzj,cnzi;
  MatScalar      *ca;

  PetscFunctionBegin;
  /* Get ij structure of P^T */
  ierr = MatGetSymbolicTranspose_SeqAIJ(P,&pti,&ptj);CHKERRQ(ierr);
  ptJ=ptj;

  /* Allocate ci array, arrays for fill computation and */
  /* free space for accumulating nonzero column info */
  ierr = PetscMalloc((pn+1)*sizeof(int),&ci);CHKERRQ(ierr);
  ci[0] = 0;

  ierr = PetscMalloc((2*pn+2*an+1)*sizeof(int),&ptadenserow);CHKERRQ(ierr);
  ierr = PetscMemzero(ptadenserow,(2*pn+2*an+1)*sizeof(int));CHKERRQ(ierr);
  ptasparserow = ptadenserow  + an;
  denserow     = ptasparserow + an;
  sparserow    = denserow     + pn;

  /* Set initial free space to be nnz(A) scaled by aspect ratio of P. */
  /* This should be reasonable if sparsity of PtAP is similar to that of A. */
  ierr          = GetMoreSpace((ai[am]/pm)*pn,&free_space);
  current_space = free_space;

  /* Determine symbolic info for each row of C: */
  for (i=0;i<pn;i++) {
    ptnzi  = pti[i+1] - pti[i];
    ptanzi = 0;
    /* Determine symbolic row of PtA: */
    for (j=0;j<ptnzi;j++) {
      arow = *ptJ++;
      anzj = ai[arow+1] - ai[arow];
      ajj  = aj + ai[arow];
      for (k=0;k<anzj;k++) {
        if (!ptadenserow[ajj[k]]) {
          ptadenserow[ajj[k]]    = -1;
          ptasparserow[ptanzi++] = ajj[k];
        }
      }
    }
      /* Using symbolic info for row of PtA, determine symbolic info for row of C: */
    ptaj = ptasparserow;
    cnzi   = 0;
    for (j=0;j<ptanzi;j++) {
      prow = *ptaj++;
      pnzj = pi[prow+1] - pi[prow];
      pjj  = pj + pi[prow];
      for (k=0;k<pnzj;k++) {
        if (!denserow[pjj[k]]) {
            denserow[pjj[k]]  = -1;
            sparserow[cnzi++] = pjj[k];
        }
      }
    }

    /* sort sparserow */
    ierr = PetscSortInt(cnzi,sparserow);CHKERRQ(ierr);
    
    /* If free space is not available, make more free space */
    /* Double the amount of total space in the list */
    if (current_space->local_remaining<cnzi) {
      ierr = GetMoreSpace(current_space->total_array_size,&current_space);CHKERRQ(ierr);
    }

    /* Copy data into free space, and zero out denserows */
    ierr = PetscMemcpy(current_space->array,sparserow,cnzi*sizeof(int));CHKERRQ(ierr);
    current_space->array           += cnzi;
    current_space->local_used      += cnzi;
    current_space->local_remaining -= cnzi;
    
    for (j=0;j<ptanzi;j++) {
      ptadenserow[ptasparserow[j]] = 0;
    }
    for (j=0;j<cnzi;j++) {
      denserow[sparserow[j]] = 0;
    }
      /* Aside: Perhaps we should save the pta info for the numerical factorization. */
      /*        For now, we will recompute what is needed. */ 
    ci[i+1] = ci[i] + cnzi;
  }
  /* nnz is now stored in ci[ptm], column indices are in the list of free space */
  /* Allocate space for cj, initialize cj, and */
  /* destroy list of free space and other temporary array(s) */
  ierr = PetscMalloc((ci[pn]+1)*sizeof(int),&cj);CHKERRQ(ierr);
  ierr = MakeSpaceContiguous(&free_space,cj);CHKERRQ(ierr);
  ierr = PetscFree(ptadenserow);CHKERRQ(ierr);
  
  /* Allocate space for ca */
  ierr = PetscMalloc((ci[pn]+1)*sizeof(MatScalar),&ca);CHKERRQ(ierr);
  ierr = PetscMemzero(ca,(ci[pn]+1)*sizeof(MatScalar));CHKERRQ(ierr);
  
  /* put together the new matrix */
  ierr = MatCreateSeqAIJWithArrays(A->comm,pn,pn,ci,cj,ca,C);CHKERRQ(ierr);

  /* MatCreateSeqAIJWithArrays flags matrix so PETSc doesn't free the user's arrays. */
  /* Since these are PETSc arrays, change flags to free them as necessary. */
  c = (Mat_SeqAIJ *)((*C)->data);
  c->freedata = PETSC_TRUE;
  c->nonew    = 0;

  /* Clean up. */
  ierr = MatRestoreSymbolicTranspose_SeqAIJ(P,&pti,&ptj);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#include "src/mat/impls/maij/maij.h"
EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "MatPtAPSymbolic_SeqAIJ_SeqMAIJ"
PetscErrorCode MatPtAPSymbolic_SeqAIJ_SeqMAIJ(Mat A,Mat PP,Mat *C) {
  /* This routine requires testing -- I don't think it works. */
  PetscErrorCode ierr;
  FreeSpaceList  free_space=PETSC_NULL,current_space=PETSC_NULL;
  Mat_SeqMAIJ    *pp=(Mat_SeqMAIJ*)PP->data;
  Mat            P=pp->AIJ;
  Mat_SeqAIJ     *a=(Mat_SeqAIJ*)A->data,*p=(Mat_SeqAIJ*)P->data,*c;
  int            *pti,*ptj,*ptJ,*ai=a->i,*aj=a->j,*ajj,*pi=p->i,*pj=p->j,*pjj;
  int            *ci,*cj,*denserow,*sparserow,*ptadenserow,*ptasparserow,*ptaj;
  int            an=A->N,am=A->M,pn=P->N,pm=P->M,ppdof=pp->dof;
  int            i,j,k,dof,pdof,ptnzi,arow,anzj,ptanzi,prow,pnzj,cnzi;
  MatScalar      *ca;

  PetscFunctionBegin;  
  /* Start timer */
  ierr = PetscLogEventBegin(MAT_PtAPSymbolic,A,PP,0,0);CHKERRQ(ierr);

  /* Get ij structure of P^T */
  ierr = MatGetSymbolicTranspose_SeqAIJ(P,&pti,&ptj);CHKERRQ(ierr);

  /* Allocate ci array, arrays for fill computation and */
  /* free space for accumulating nonzero column info */
  ierr = PetscMalloc((pn+1)*sizeof(int),&ci);CHKERRQ(ierr);
  ci[0] = 0;

  ierr = PetscMalloc((2*pn+2*an+1)*sizeof(int),&ptadenserow);CHKERRQ(ierr);
  ierr = PetscMemzero(ptadenserow,(2*pn+2*an+1)*sizeof(int));CHKERRQ(ierr);
  ptasparserow = ptadenserow  + an;
  denserow     = ptasparserow + an;
  sparserow    = denserow     + pn;

  /* Set initial free space to be nnz(A) scaled by aspect ratio of P. */
  /* This should be reasonable if sparsity of PtAP is similar to that of A. */
  ierr          = GetMoreSpace((ai[am]/pm)*pn,&free_space);
  current_space = free_space;

  /* Determine symbolic info for each row of C: */
  for (i=0;i<pn/ppdof;i++) {
    ptnzi  = pti[i+1] - pti[i];
    ptanzi = 0;
    ptJ    = ptj + pti[i];
    for (dof=0;dof<ppdof;dof++) {
    /* Determine symbolic row of PtA: */
      for (j=0;j<ptnzi;j++) {
        arow = ptJ[j] + dof;
        anzj = ai[arow+1] - ai[arow];
        ajj  = aj + ai[arow];
        for (k=0;k<anzj;k++) {
          if (!ptadenserow[ajj[k]]) {
            ptadenserow[ajj[k]]    = -1;
            ptasparserow[ptanzi++] = ajj[k];
          }
        }
      }
      /* Using symbolic info for row of PtA, determine symbolic info for row of C: */
      ptaj = ptasparserow;
      cnzi   = 0;
      for (j=0;j<ptanzi;j++) {
        pdof = *ptaj%dof;
        prow = (*ptaj++)/dof;
        pnzj = pi[prow+1] - pi[prow];
        pjj  = pj + pi[prow];
        for (k=0;k<pnzj;k++) {
          if (!denserow[pjj[k]+pdof]) {
            denserow[pjj[k]+pdof] = -1;
            sparserow[cnzi++]     = pjj[k]+pdof;
          }
        }
      }

      /* sort sparserow */
      ierr = PetscSortInt(cnzi,sparserow);CHKERRQ(ierr);
      
      /* If free space is not available, make more free space */
      /* Double the amount of total space in the list */
      if (current_space->local_remaining<cnzi) {
        ierr = GetMoreSpace(current_space->total_array_size,&current_space);CHKERRQ(ierr);
      }

      /* Copy data into free space, and zero out denserows */
      ierr = PetscMemcpy(current_space->array,sparserow,cnzi*sizeof(int));CHKERRQ(ierr);
      current_space->array           += cnzi;
      current_space->local_used      += cnzi;
      current_space->local_remaining -= cnzi;

      for (j=0;j<ptanzi;j++) {
        ptadenserow[ptasparserow[j]] = 0;
      }
      for (j=0;j<cnzi;j++) {
        denserow[sparserow[j]] = 0;
      }
      /* Aside: Perhaps we should save the pta info for the numerical factorization. */
      /*        For now, we will recompute what is needed. */ 
      ci[i+1+dof] = ci[i+dof] + cnzi;
    }
  }
  /* nnz is now stored in ci[ptm], column indices are in the list of free space */
  /* Allocate space for cj, initialize cj, and */
  /* destroy list of free space and other temporary array(s) */
  ierr = PetscMalloc((ci[pn]+1)*sizeof(int),&cj);CHKERRQ(ierr);
  ierr = MakeSpaceContiguous(&free_space,cj);CHKERRQ(ierr);
  ierr = PetscFree(ptadenserow);CHKERRQ(ierr);
    
  /* Allocate space for ca */
  ierr = PetscMalloc((ci[pn]+1)*sizeof(MatScalar),&ca);CHKERRQ(ierr);
  ierr = PetscMemzero(ca,(ci[pn]+1)*sizeof(MatScalar));CHKERRQ(ierr);
  
  /* put together the new matrix */
  ierr = MatCreateSeqAIJWithArrays(A->comm,pn,pn,ci,cj,ca,C);CHKERRQ(ierr);

  /* MatCreateSeqAIJWithArrays flags matrix so PETSc doesn't free the user's arrays. */
  /* Since these are PETSc arrays, change flags to free them as necessary. */
  c = (Mat_SeqAIJ *)((*C)->data);
  c->freedata = PETSC_TRUE;
  c->nonew    = 0;

  /* Clean up. */
  ierr = MatRestoreSymbolicTranspose_SeqAIJ(P,&pti,&ptj);CHKERRQ(ierr);

  ierr = PetscLogEventEnd(MAT_PtAPSymbolic,A,PP,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__
#define __FUNCT__ "MatPtAPNumeric"
/*
   MatPtAPNumeric - Computes the matrix projection C = P^T * A * P

   Collective on Mat

   Input Parameters:
+  A - the matrix
-  P - the projection matrix

   Output Parameters:
.  C - the product matrix

   Notes:
   C must have been created by calling MatPtAPSymbolic and must be destroyed by
   the user using MatDeatroy().

   This routine is currently only implemented for pairs of AIJ matrices and classes
   which inherit from AIJ.  C will be of type MATAIJ.

   Level: intermediate

.seealso: MatPtAP(),MatPtAPSymbolic(),MatMatMultNumeric()
*/
PetscErrorCode MatPtAPNumeric(Mat A,Mat P,Mat C) {
  PetscErrorCode ierr;
  PetscErrorCode (*fA)(Mat,Mat,Mat);
  PetscErrorCode (*fP)(Mat,Mat,Mat);

  PetscFunctionBegin;

  PetscValidHeaderSpecific(A,MAT_COOKIE,1);
  PetscValidType(A,1);
  MatPreallocated(A);
  if (!A->assembled) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (A->factor) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix"); 

  PetscValidHeaderSpecific(P,MAT_COOKIE,2);
  PetscValidType(P,2);
  MatPreallocated(P);
  if (!P->assembled) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (P->factor) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix"); 

  PetscValidHeaderSpecific(C,MAT_COOKIE,3);
  PetscValidType(C,3);
  MatPreallocated(C);
  if (!C->assembled) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (C->factor) SETERRQ(PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix"); 

  if (P->N!=C->M) SETERRQ2(PETSC_ERR_ARG_SIZ,"Matrix dimensions are incompatible, %d != %d",P->N,C->M);
  if (P->M!=A->N) SETERRQ2(PETSC_ERR_ARG_SIZ,"Matrix dimensions are incompatible, %d != %d",P->M,A->N);
  if (A->M!=A->N) SETERRQ2(PETSC_ERR_ARG_SIZ,"Matrix 'A' must be square, %d != %d",A->M,A->N);
  if (P->N!=C->N) SETERRQ2(PETSC_ERR_ARG_SIZ,"Matrix dimensions are incompatible, %d != %d",P->N,C->N);

  /* For now, we do not dispatch based on the type of A and P */
  /* When implementations like _SeqAIJ_MAIJ exist, attack the multiple dispatch problem. */  
  fA = A->ops->ptapnumeric;
  if (!fA) SETERRQ1(PETSC_ERR_SUP,"MatPtAPNumeric not supported for A of type %s",A->type_name);
  fP = P->ops->ptapnumeric;
  if (!fP) SETERRQ1(PETSC_ERR_SUP,"MatPtAPNumeric not supported for P of type %s",P->type_name);
  if (fP!=fA) SETERRQ2(PETSC_ERR_ARG_INCOMP,"MatPtAPNumeric requires A, %s, to be compatible with P, %s",A->type_name,P->type_name);

  ierr = PetscLogEventBegin(MAT_PtAPNumeric,A,P,0,0);CHKERRQ(ierr); 
  ierr = (*fA)(A,P,C);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_PtAPNumeric,A,P,0,0);CHKERRQ(ierr); 

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatPtAPNumeric_SeqAIJ_SeqAIJ"
PetscErrorCode MatPtAPNumeric_SeqAIJ_SeqAIJ(Mat A,Mat P,Mat C) 
{
  PetscErrorCode ierr;
  int            flops=0;
  Mat_SeqAIJ     *a  = (Mat_SeqAIJ *) A->data;
  Mat_SeqAIJ     *p  = (Mat_SeqAIJ *) P->data;
  Mat_SeqAIJ     *c  = (Mat_SeqAIJ *) C->data;
  int            *ai=a->i,*aj=a->j,*apj,*apjdense,*pi=p->i,*pj=p->j,*pJ=p->j,*pjj;
  int            *ci=c->i,*cj=c->j,*cjj;
  int            am=A->M,cn=C->N,cm=C->M;
  int            i,j,k,anzi,pnzi,apnzj,nextap,pnzj,prow,crow;
  MatScalar      *aa=a->a,*apa,*pa=p->a,*pA=p->a,*paj,*ca=c->a,*caj;

  PetscFunctionBegin;
  /* Allocate temporary array for storage of one row of A*P */
  ierr = PetscMalloc(cn*(sizeof(MatScalar)+2*sizeof(int)),&apa);CHKERRQ(ierr);
  ierr = PetscMemzero(apa,cn*(sizeof(MatScalar)+2*sizeof(int)));CHKERRQ(ierr);

  apj      = (int *)(apa + cn);
  apjdense = apj + cn;

  /* Clear old values in C */
  ierr = PetscMemzero(ca,ci[cm]*sizeof(MatScalar));CHKERRQ(ierr);

  for (i=0;i<am;i++) {
    /* Form sparse row of A*P */
    anzi  = ai[i+1] - ai[i];
    apnzj = 0;
    for (j=0;j<anzi;j++) {
      prow = *aj++;
      pnzj = pi[prow+1] - pi[prow];
      pjj  = pj + pi[prow];
      paj  = pa + pi[prow];
      for (k=0;k<pnzj;k++) {
        if (!apjdense[pjj[k]]) {
          apjdense[pjj[k]] = -1; 
          apj[apnzj++]     = pjj[k];
        }
        apa[pjj[k]] += (*aa)*paj[k];
      }
      flops += 2*pnzj;
      aa++;
    }

    /* Sort the j index array for quick sparse axpy. */
    ierr = PetscSortInt(apnzj,apj);CHKERRQ(ierr);

    /* Compute P^T*A*P using outer product (P^T)[:,j]*(A*P)[j,:]. */
    pnzi = pi[i+1] - pi[i];
    for (j=0;j<pnzi;j++) {
      nextap = 0;
      crow   = *pJ++;
      cjj    = cj + ci[crow];
      caj    = ca + ci[crow];
      /* Perform sparse axpy operation.  Note cjj includes apj. */
      for (k=0;nextap<apnzj;k++) {
        if (cjj[k]==apj[nextap]) {
          caj[k] += (*pA)*apa[apj[nextap++]];
        }
      }
      flops += 2*apnzj;
      pA++;
    }

    /* Zero the current row info for A*P */
    for (j=0;j<apnzj;j++) {
      apa[apj[j]]      = 0.;
      apjdense[apj[j]] = 0;
    }
  }

  /* Assemble the final matrix and clean up */
  ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = PetscFree(apa);CHKERRQ(ierr);
  ierr = PetscLogFlops(flops);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

/* Compute C = P[rstart:rend,:]^T * A * P of seqaij matrices - used by MatPtAP_MPIAIJ_MPIAIJ() */

#undef __FUNCT__
#define __FUNCT__ "MatPtAPNumeric_SeqAIJ_SeqAIJ_ReducedPt"
PetscErrorCode MatPtAPNumeric_SeqAIJ_SeqAIJ_ReducedPt(Mat A,Mat P,int prstart,int prend,Mat C) 
{
  PetscErrorCode ierr;
  int            flops=0;
  Mat_SeqAIJ     *a  = (Mat_SeqAIJ *) A->data;
  Mat_SeqAIJ     *p  = (Mat_SeqAIJ *) P->data;
  Mat_SeqAIJ     *c  = (Mat_SeqAIJ *) C->data;
  int            *ai=a->i,*aj=a->j,*apj,*apjdense,*pi=p->i,*pj=p->j,*pJ=p->j,*pjj;
  int            *ci=c->i,*cj=c->j,*cjj;
  int            am=A->M,cn=C->N,cm=C->M;
  int            i,j,k,anzi,pnzi,apnzj,nextap,pnzj,prow,crow;
  MatScalar      *aa=a->a,*apa,*pa=p->a,*pA=p->a,*paj,*ca=c->a,*caj;

  PetscFunctionBegin;
  pA=p->a+pi[prstart];
  /* Allocate temporary array for storage of one row of A*P */
  ierr = PetscMalloc(cn*(sizeof(MatScalar)+2*sizeof(int)),&apa);CHKERRQ(ierr);
  ierr = PetscMemzero(apa,cn*(sizeof(MatScalar)+2*sizeof(int)));CHKERRQ(ierr);

  apj      = (int *)(apa + cn);
  apjdense = apj + cn;

  /* Clear old values in C */
  ierr = PetscMemzero(ca,ci[cm]*sizeof(MatScalar));CHKERRQ(ierr);

  for (i=0;i<am;i++) {
    /* Form sparse row of A*P */
    anzi  = ai[i+1] - ai[i];
    apnzj = 0;
    for (j=0;j<anzi;j++) {
      prow = *aj++;
      pnzj = pi[prow+1] - pi[prow];
      pjj  = pj + pi[prow];
      paj  = pa + pi[prow];
      for (k=0;k<pnzj;k++) {
        if (!apjdense[pjj[k]]) {
          apjdense[pjj[k]] = -1; 
          apj[apnzj++]     = pjj[k];
        }
        apa[pjj[k]] += (*aa)*paj[k];
      }
      flops += 2*pnzj;
      aa++;
    }

    /* Sort the j index array for quick sparse axpy. */
    ierr = PetscSortInt(apnzj,apj);CHKERRQ(ierr);

    /* Compute P[[prstart:prend,:]^T*A*P using outer product (P^T)[:,j+prstart]*(A*P)[j,:]. */
    pnzi = pi[i+1+prstart] - pi[i+prstart];
    for (j=0;j<pnzi;j++) {
      nextap = 0;
      crow   = *pJ++;
      cjj    = cj + ci[crow];
      caj    = ca + ci[crow];
      /* Perform sparse axpy operation.  Note cjj includes apj. */
      for (k=0;nextap<apnzj;k++) {
        if (cjj[k]==apj[nextap]) {
          caj[k] += (*pA)*apa[apj[nextap++]];
        }
      }
      flops += 2*apnzj;
      pA++;
    }

    /* Zero the current row info for A*P */
    for (j=0;j<apnzj;j++) {
      apa[apj[j]]      = 0.;
      apjdense[apj[j]] = 0;
    }
  }

  /* Assemble the final matrix and clean up */
  ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = PetscFree(apa);CHKERRQ(ierr);
  ierr = PetscLogFlops(flops);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatPtAPSymbolic_SeqAIJ_SeqAIJ_ReducedPt"
PetscErrorCode MatPtAPSymbolic_SeqAIJ_SeqAIJ_ReducedPt(Mat A,Mat P,PetscReal fill,int prstart,int prend,Mat *C) {
  PetscErrorCode ierr;
  FreeSpaceList  free_space=PETSC_NULL,current_space=PETSC_NULL;
  Mat_SeqAIJ     *a=(Mat_SeqAIJ*)A->data,*p=(Mat_SeqAIJ*)P->data,*c;
  int            *pti,*ptj,*ptJ,*ai=a->i,*aj=a->j,*ajj,*pi=p->i,*pj=p->j,*pjj;
  int            *ci,*cj,*denserow,*sparserow,*ptadenserow,*ptasparserow,*ptaj;
  int            an=A->N,am=A->M,pn=P->N,pm=P->M;
  int            i,j,k,ptnzi,arow,anzj,ptanzi,prow,pnzj,cnzi;
  MatScalar      *ca;

  PetscFunctionBegin;
  /* Get ij structure of P[rstart:rend,:]^T */
  Mat *psub,P_sub;
  IS  isrow,iscol;
  int m = prend - prstart;
  ierr = ISCreateStride(PETSC_COMM_SELF,m,prstart,1,&isrow);CHKERRQ(ierr);
  ierr = ISCreateStride(PETSC_COMM_SELF,P->n,0,1,&iscol);CHKERRQ(ierr);
  ierr = MatGetSubMatrices(P,1,&isrow,&iscol,MAT_INITIAL_MATRIX,&psub);CHKERRQ(ierr); 
  ierr = ISDestroy(isrow);CHKERRQ(ierr);
  ierr = ISDestroy(iscol);CHKERRQ(ierr);
  P_sub = psub[0];
  ierr = MatGetSymbolicTranspose_SeqAIJ(P_sub,&pti,&ptj);CHKERRQ(ierr);
  ierr = MatDestroyMatrices(1,&psub);CHKERRQ(ierr);
  ptJ=ptj;

  /* Allocate ci array, arrays for fill computation and */
  /* free space for accumulating nonzero column info */
  ierr = PetscMalloc((pn+1)*sizeof(int),&ci);CHKERRQ(ierr);
  ci[0] = 0;

  ierr = PetscMalloc((2*pn+2*an+1)*sizeof(int),&ptadenserow);CHKERRQ(ierr);
  ierr = PetscMemzero(ptadenserow,(2*pn+2*an+1)*sizeof(int));CHKERRQ(ierr);
  ptasparserow = ptadenserow  + an;
  denserow     = ptasparserow + an;
  sparserow    = denserow     + pn;

  /* Set initial free space to be nnz(A) scaled by aspect ratio of P. */
  /* This should be reasonable if sparsity of PtAP is similar to that of A. */
  ierr          = GetMoreSpace((ai[am]/pm)*pn,&free_space);
  current_space = free_space;

  /* Determine symbolic info for each row of C: */
  for (i=0;i<pn;i++) {
    ptnzi  = pti[i+1] - pti[i];
    ptanzi = 0;
    /* Determine symbolic row of PtA_reduced: */
    for (j=0;j<ptnzi;j++) {
      arow = *ptJ++;
      anzj = ai[arow+1] - ai[arow];
      ajj  = aj + ai[arow];
      for (k=0;k<anzj;k++) {
        if (!ptadenserow[ajj[k]]) {
          ptadenserow[ajj[k]]    = -1;
          ptasparserow[ptanzi++] = ajj[k];
        }
      }
    }
      /* Using symbolic info for row of PtA, determine symbolic info for row of C: */
    ptaj = ptasparserow;
    cnzi   = 0;
    for (j=0;j<ptanzi;j++) {
      prow = *ptaj++;
      pnzj = pi[prow+1] - pi[prow];
      pjj  = pj + pi[prow];
      for (k=0;k<pnzj;k++) {
        if (!denserow[pjj[k]]) {
            denserow[pjj[k]]  = -1;
            sparserow[cnzi++] = pjj[k];
        }
      }
    }

    /* sort sparserow */
    ierr = PetscSortInt(cnzi,sparserow);CHKERRQ(ierr);
    
    /* If free space is not available, make more free space */
    /* Double the amount of total space in the list */
    if (current_space->local_remaining<cnzi) {
      ierr = GetMoreSpace(current_space->total_array_size,&current_space);CHKERRQ(ierr);
    }

    /* Copy data into free space, and zero out denserows */
    ierr = PetscMemcpy(current_space->array,sparserow,cnzi*sizeof(int));CHKERRQ(ierr);
    current_space->array           += cnzi;
    current_space->local_used      += cnzi;
    current_space->local_remaining -= cnzi;
    
    for (j=0;j<ptanzi;j++) {
      ptadenserow[ptasparserow[j]] = 0;
    }
    for (j=0;j<cnzi;j++) {
      denserow[sparserow[j]] = 0;
    }
      /* Aside: Perhaps we should save the pta info for the numerical factorization. */
      /*        For now, we will recompute what is needed. */ 
    ci[i+1] = ci[i] + cnzi;
  }
  /* nnz is now stored in ci[ptm], column indices are in the list of free space */
  /* Allocate space for cj, initialize cj, and */
  /* destroy list of free space and other temporary array(s) */
  ierr = PetscMalloc((ci[pn]+1)*sizeof(int),&cj);CHKERRQ(ierr);
  ierr = MakeSpaceContiguous(&free_space,cj);CHKERRQ(ierr);
  ierr = PetscFree(ptadenserow);CHKERRQ(ierr);
  
  /* Allocate space for ca */
  ierr = PetscMalloc((ci[pn]+1)*sizeof(MatScalar),&ca);CHKERRQ(ierr);
  ierr = PetscMemzero(ca,(ci[pn]+1)*sizeof(MatScalar));CHKERRQ(ierr);
  
  /* put together the new matrix */
  ierr = MatCreateSeqAIJWithArrays(A->comm,pn,pn,ci,cj,ca,C);CHKERRQ(ierr);

  /* MatCreateSeqAIJWithArrays flags matrix so PETSc doesn't free the user's arrays. */
  /* Since these are PETSc arrays, change flags to free them as necessary. */
  c = (Mat_SeqAIJ *)((*C)->data);
  c->freedata = PETSC_TRUE;
  c->nonew    = 0;

  /* Clean up. */
  ierr = MatRestoreSymbolicTranspose_SeqAIJ(P,&pti,&ptj);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatPtAP_SeqAIJ_SeqAIJ_ReducedPt"
PetscErrorCode MatPtAP_SeqAIJ_SeqAIJ_ReducedPt(Mat A,Mat P,MatReuse scall,PetscReal fill,int prstart,int prend,Mat *C) 
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (A->m != prend-prstart) SETERRQ2(PETSC_ERR_ARG_SIZ,"Matrix dimensions are incompatible, %d != %d",A->m,prend-prstart);
  if (prend-prstart > P->m) SETERRQ2(PETSC_ERR_ARG_SIZ," prend-prstart %d cannot be larger than P->m %d",prend-prstart,P->m);
  if (scall == MAT_INITIAL_MATRIX){
    ierr = MatPtAPSymbolic_SeqAIJ_SeqAIJ_ReducedPt(A,P,fill,prstart,prend,C);CHKERRQ(ierr);
  }
  
  ierr = MatPtAPNumeric_SeqAIJ_SeqAIJ_ReducedPt(A,P,prstart,prend,*C);CHKERRQ(ierr);
  
  PetscFunctionReturn(0);
}
 
