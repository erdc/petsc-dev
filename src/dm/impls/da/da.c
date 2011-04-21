#include <private/daimpl.h>    /*I   "petscdmda.h"   I*/


#undef __FUNCT__  
#define __FUNCT__ "DMDASetDim"
/*@
  DMDASetDim - Sets the dimension

  Collective on DMDA

  Input Parameters:
+ da - the DMDA
- dim - the dimension (or PETSC_DECIDE)

  Level: intermediate

.seealso: DaGetDim(), DMDASetSizes()
@*/
PetscErrorCode  DMDASetDim(DM da, PetscInt dim)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da, DM_CLASSID, 1);
  if (dd->dim > 0 && dim != dd->dim) SETERRQ2(((PetscObject)da)->comm,PETSC_ERR_ARG_WRONGSTATE,"Cannot change DMDA dim from %D after it was set to %D",dd->dim,dim);
  dd->dim = dim;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetSizes"
/*@
  DMDASetSizes - Sets the global sizes

  Logically Collective on DMDA

  Input Parameters:
+ da - the DMDA
. M - the global X size (or PETSC_DECIDE)
. N - the global Y size (or PETSC_DECIDE)
- P - the global Z size (or PETSC_DECIDE)

  Level: intermediate

.seealso: DMDAGetSize(), PetscSplitOwnership()
@*/
PetscErrorCode  DMDASetSizes(DM da, PetscInt M, PetscInt N, PetscInt P)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da, DM_CLASSID, 1);
  PetscValidLogicalCollectiveInt(da,M,2);
  PetscValidLogicalCollectiveInt(da,N,3);
  PetscValidLogicalCollectiveInt(da,P,4);

  dd->M = M;
  dd->N = N;
  dd->P = P;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetNumProcs"
/*@
  DMDASetNumProcs - Sets the number of processes in each dimension

  Logically Collective on DMDA

  Input Parameters:
+ da - the DMDA
. m - the number of X procs (or PETSC_DECIDE)
. n - the number of Y procs (or PETSC_DECIDE)
- p - the number of Z procs (or PETSC_DECIDE)

  Level: intermediate

.seealso: DMDASetSizes(), DMDAGetSize(), PetscSplitOwnership()
@*/
PetscErrorCode  DMDASetNumProcs(DM da, PetscInt m, PetscInt n, PetscInt p)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da, DM_CLASSID, 1);
  PetscValidLogicalCollectiveInt(da,m,2);
  PetscValidLogicalCollectiveInt(da,n,3);
  PetscValidLogicalCollectiveInt(da,p,4);
  dd->m = m;
  dd->n = n;
  dd->p = p;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetBoundaryType"
/*@
  DMDASetBoundaryType - Sets the type of ghost nodes on domain boundaries.

  Not collective

  Input Parameter:
+ da    - The DMDA
- bx,by,bz - One of DMDA_BOUNDARY_NONE, DMDA_BOUNDARY_GHOSTED, DMDA_BOUNDARY_PERIODIC

  Level: intermediate

.keywords:  distributed array, periodicity
.seealso: DMDACreate(), DMDestroy(), DMDA, DMDABoundaryType
@*/
PetscErrorCode  DMDASetBoundaryType(DM da,DMDABoundaryType bx,DMDABoundaryType by,DMDABoundaryType bz)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidLogicalCollectiveInt(da,bx,2);
  PetscValidLogicalCollectiveInt(da,by,3);
  PetscValidLogicalCollectiveInt(da,bz,4);
  dd->bx = bx;
  dd->by = by;
  dd->bz = bz;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetDof"
/*@
  DMDASetDof - Sets the number of degrees of freedom per vertex

  Not collective

  Input Parameter:
+ da  - The DMDA
- dof - Number of degrees of freedom

  Level: intermediate

.keywords:  distributed array, degrees of freedom
.seealso: DMDACreate(), DMDestroy(), DMDA
@*/
PetscErrorCode  DMDASetDof(DM da, int dof)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  dd->w = dof;
  da->bs = dof;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetStencilType"
/*@
  DMDASetStencilType - Sets the type of the communication stencil

  Logically Collective on DMDA

  Input Parameter:
+ da    - The DMDA
- stype - The stencil type, use either DMDA_STENCIL_BOX or DMDA_STENCIL_STAR.

  Level: intermediate

.keywords:  distributed array, stencil
.seealso: DMDACreate(), DMDestroy(), DMDA
@*/
PetscErrorCode  DMDASetStencilType(DM da, DMDAStencilType stype)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidLogicalCollectiveEnum(da,stype,2);
  dd->stencil_type = stype;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetStencilWidth"
/*@
  DMDASetStencilWidth - Sets the width of the communication stencil

  Logically Collective on DMDA

  Input Parameter:
+ da    - The DMDA
- width - The stencil width

  Level: intermediate

.keywords:  distributed array, stencil
.seealso: DMDACreate(), DMDestroy(), DMDA
@*/
PetscErrorCode  DMDASetStencilWidth(DM da, PetscInt width)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidLogicalCollectiveInt(da,width,2);
  dd->s = width;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDACheckOwnershipRanges_Private"
static PetscErrorCode DMDACheckOwnershipRanges_Private(DM da,PetscInt M,PetscInt m,const PetscInt lx[])
{
  PetscInt i,sum;

  PetscFunctionBegin;
  if (M < 0) SETERRQ(((PetscObject)da)->comm,PETSC_ERR_ARG_WRONGSTATE,"Global dimension not set");
  for (i=sum=0; i<m; i++) sum += lx[i];
  if (sum != M) SETERRQ2(((PetscObject)da)->comm,PETSC_ERR_ARG_INCOMP,"Ownership ranges sum to %D but global dimension is %D",sum,M);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetOwnershipRanges"
/*@
  DMDASetOwnershipRanges - Sets the number of nodes in each direction on each process

  Logically Collective on DMDA

  Input Parameter:
+ da - The DMDA
. lx - array containing number of nodes in the X direction on each process, or PETSC_NULL. If non-null, must be of length da->m
. ly - array containing number of nodes in the Y direction on each process, or PETSC_NULL. If non-null, must be of length da->n
- lz - array containing number of nodes in the Z direction on each process, or PETSC_NULL. If non-null, must be of length da->p.

  Level: intermediate

.keywords:  distributed array
.seealso: DMDACreate(), DMDestroy(), DMDA
@*/
PetscErrorCode  DMDASetOwnershipRanges(DM da, const PetscInt lx[], const PetscInt ly[], const PetscInt lz[])
{
  PetscErrorCode ierr;
  DM_DA          *dd = (DM_DA*)da->data;
  
  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  if (lx) {
    if (dd->m < 0) SETERRQ(((PetscObject)da)->comm,PETSC_ERR_ARG_WRONGSTATE,"Cannot set ownership ranges before setting number of procs");
    ierr = DMDACheckOwnershipRanges_Private(da,dd->M,dd->m,lx);CHKERRQ(ierr);
    if (!dd->lx) {
      ierr = PetscMalloc(dd->m*sizeof(PetscInt), &dd->lx);CHKERRQ(ierr);
    }
    ierr = PetscMemcpy(dd->lx, lx, dd->m*sizeof(PetscInt));CHKERRQ(ierr);
  }
  if (ly) {
    if (dd->n < 0) SETERRQ(((PetscObject)da)->comm,PETSC_ERR_ARG_WRONGSTATE,"Cannot set ownership ranges before setting number of procs");
    ierr = DMDACheckOwnershipRanges_Private(da,dd->N,dd->n,ly);CHKERRQ(ierr);
    if (!dd->ly) {
      ierr = PetscMalloc(dd->n*sizeof(PetscInt), &dd->ly);CHKERRQ(ierr);
    }
    ierr = PetscMemcpy(dd->ly, ly, dd->n*sizeof(PetscInt));CHKERRQ(ierr);
  }
  if (lz) {
    if (dd->p < 0) SETERRQ(((PetscObject)da)->comm,PETSC_ERR_ARG_WRONGSTATE,"Cannot set ownership ranges before setting number of procs");
    ierr = DMDACheckOwnershipRanges_Private(da,dd->P,dd->p,lz);CHKERRQ(ierr);
    if (!dd->lz) {
      ierr = PetscMalloc(dd->p*sizeof(PetscInt), &dd->lz);CHKERRQ(ierr);
    }
    ierr = PetscMemcpy(dd->lz, lz, dd->p*sizeof(PetscInt));CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDACreateOwnershipRanges"
/*
 Ensure that da->lx, ly, and lz exist.  Collective on DMDA.
*/
PetscErrorCode  DMDACreateOwnershipRanges(DM da)
{
  DM_DA          *dd = (DM_DA*)da->data;
  PetscErrorCode ierr;
  PetscInt       n;
  MPI_Comm       comm;
  PetscMPIInt    size;

  PetscFunctionBegin;
  if (!dd->lx) {
    ierr = PetscMalloc(dd->m*sizeof(PetscInt),&dd->lx);CHKERRQ(ierr);
    ierr = DMDAGetProcessorSubset(da,DMDA_X,dd->xs,&comm);CHKERRQ(ierr);
    ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
    n = (dd->xe-dd->xs)/dd->w;
    ierr = MPI_Allgather(&n,1,MPIU_INT,dd->lx,1,MPIU_INT,comm);CHKERRQ(ierr);
  }
  if (dd->dim > 1 && !dd->ly) {
    ierr = PetscMalloc(dd->n*sizeof(PetscInt),&dd->ly);CHKERRQ(ierr);
    ierr = DMDAGetProcessorSubset(da,DMDA_Y,dd->ys,&comm);CHKERRQ(ierr);
    n = dd->ye-dd->ys;
    ierr = MPI_Allgather(&n,1,MPIU_INT,dd->ly,1,MPIU_INT,comm);CHKERRQ(ierr);
  }
  if (dd->dim > 2 && !dd->lz) {
    ierr = PetscMalloc(dd->p*sizeof(PetscInt),&dd->lz);CHKERRQ(ierr);
    ierr = DMDAGetProcessorSubset(da,DMDA_Z,dd->zs,&comm);CHKERRQ(ierr);
    n = dd->ze-dd->zs;
    ierr = MPI_Allgather(&n,1,MPIU_INT,dd->lz,1,MPIU_INT,comm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetInterpolationType"
/*@
       DMDASetInterpolationType - Sets the type of interpolation that will be 
          returned by DMGetInterpolation()

   Logically Collective on DMDA

   Input Parameter:
+  da - initial distributed array
.  ctype - DMDA_Q1 and DMDA_Q0 are currently the only supported forms

   Level: intermediate

   Notes: you should call this on the coarser of the two DMDAs you pass to DMGetInterpolation()

.keywords:  distributed array, interpolation

.seealso: DMDACreate1d(), DMDACreate2d(), DMDACreate3d(), DMDestroy(), DMDA, DMDAInterpolationType
@*/
PetscErrorCode  DMDASetInterpolationType(DM da,DMDAInterpolationType ctype)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidLogicalCollectiveEnum(da,ctype,2);
  dd->interptype = ctype;
  PetscFunctionReturn(0);
}


#undef __FUNCT__  
#define __FUNCT__ "DMDAGetNeighbors"
/*@C
      DMDAGetNeighbors - Gets an array containing the MPI rank of all the current
        processes neighbors.

    Not Collective

   Input Parameter:
.     da - the DMDA object

   Output Parameters:
.     ranks - the neighbors ranks, stored with the x index increasing most rapidly.
              this process itself is in the list

   Notes: In 2d the array is of length 9, in 3d of length 27
          Not supported in 1d
          Do not free the array, it is freed when the DMDA is destroyed.

   Fortran Notes: In fortran you must pass in an array of the appropriate length.

   Level: intermediate

@*/
PetscErrorCode  DMDAGetNeighbors(DM da,const PetscMPIInt *ranks[])
{
  DM_DA *dd = (DM_DA*)da->data;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  *ranks = dd->neighbors;
  PetscFunctionReturn(0);
}

/*@C
      DMDASetElementType - Sets the element type to be returned by DMGetElements()

    Not Collective

   Input Parameter:
.     da - the DMDA object

   Output Parameters:
.     etype - the element type, currently either DMDA_ELEMENT_P1 or ELEMENT_Q1

   Level: intermediate

.seealso: DMDAElementType, DMDAGetElementType(), DMGetElements(), DMRestoreElements()
@*/
#undef __FUNCT__
#define __FUNCT__ "DMDASetElementType"
PetscErrorCode  DMDASetElementType(DM da, DMDAElementType etype)
{
  DM_DA          *dd = (DM_DA*)da->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidLogicalCollectiveEnum(da,etype,2);
  if (dd->elementtype != etype) {
    ierr = PetscFree(dd->e);CHKERRQ(ierr);
    dd->elementtype = etype;
    dd->ne          = 0; 
    dd->e           = PETSC_NULL;
  }
  PetscFunctionReturn(0);
}

/*@C
      DMDAGetElementType - Gets the element type to be returned by DMGetElements()

    Not Collective

   Input Parameter:
.     da - the DMDA object

   Output Parameters:
.     etype - the element type, currently either DMDA_ELEMENT_P1 or ELEMENT_Q1

   Level: intermediate

.seealso: DMDAElementType, DMDASetElementType(), DMGetElements(), DMRestoreElements()
@*/
#undef __FUNCT__
#define __FUNCT__ "DMDAGetElementType"
PetscErrorCode  DMDAGetElementType(DM da, DMDAElementType *etype)
{
  DM_DA *dd = (DM_DA*)da->data;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidPointer(etype,2);
  *etype = dd->elementtype;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMGetElements"
/*@C
      DMGetElements - Gets an array containing the indices (in local coordinates) 
                 of all the local elements

    Not Collective

   Input Parameter:
.     dm - the DM object

   Output Parameters:
+     nel - number of local elements
.     nen - number of element nodes
-     e - the indices of the elements vertices

   Level: intermediate

.seealso: DMElementType, DMSetElementType(), DMRestoreElements()
@*/
PetscErrorCode  DMGetElements(DM dm,PetscInt *nel,PetscInt *nen,const PetscInt *e[])
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidIntPointer(nel,2);
  PetscValidIntPointer(nen,3);
  PetscValidPointer(e,4);
  ierr = (dm->ops->getelements)(dm,nel,nen,e);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMRestoreElements"
/*@C
      DMRestoreElements - Returns an array containing the indices (in local coordinates) 
                 of all the local elements obtained with DMGetElements()

    Not Collective

   Input Parameter:
+     dm - the DM object
.     nel - number of local elements
.     nen - number of element nodes
-     e - the indices of the elements vertices

   Level: intermediate

.seealso: DMElementType, DMSetElementType(), DMGetElements()
@*/
PetscErrorCode  DMRestoreElements(DM dm,PetscInt *nel,PetscInt *nen,const PetscInt *e[])
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidIntPointer(nel,2);
  PetscValidIntPointer(nen,3);
  PetscValidPointer(e,4);
  if (dm->ops->restoreelements) {
    ierr = (dm->ops->restoreelements)(dm,nel,nen,e);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDAGetOwnershipRanges"
/*@C
      DMDAGetOwnershipRanges - Gets the ranges of indices in the x, y and z direction that are owned by each process

    Not Collective

   Input Parameter:
.     da - the DMDA object

   Output Parameter:
+     lx - ownership along x direction (optional)
.     ly - ownership along y direction (optional)
-     lz - ownership along z direction (optional)

   Level: intermediate

    Note: these correspond to the optional final arguments passed to DMDACreate(), DMDACreate2d(), DMDACreate3d()

    In Fortran one must pass in arrays lx, ly, and lz that are long enough to hold the values; the sixth, seventh and
    eighth arguments from DMDAGetInfo()

     In C you should not free these arrays, nor change the values in them. They will only have valid values while the
    DMDA they came from still exists (has not been destroyed).

.seealso: DMDAGetCorners(), DMDAGetGhostCorners(), DMDACreate(), DMDACreate1d(), DMDACreate2d(), DMDACreate3d(), VecGetOwnershipRanges()
@*/
PetscErrorCode  DMDAGetOwnershipRanges(DM da,const PetscInt *lx[],const PetscInt *ly[],const PetscInt *lz[])
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  if (lx) *lx = dd->lx;
  if (ly) *ly = dd->ly;
  if (lz) *lz = dd->lz;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetRefinementFactor"
/*@
     DMDASetRefinementFactor - Set the ratios that the DMDA grid is refined

    Logically Collective on DMDA

  Input Parameters:
+    da - the DMDA object
.    refine_x - ratio of fine grid to coarse in x direction (2 by default)
.    refine_y - ratio of fine grid to coarse in y direction (2 by default)
-    refine_z - ratio of fine grid to coarse in z direction (2 by default)

  Options Database:
+  -da_refine_x - refinement ratio in x direction
.  -da_refine_y - refinement ratio in y direction
-  -da_refine_z - refinement ratio in z direction

  Level: intermediate

    Notes: Pass PETSC_IGNORE to leave a value unchanged

.seealso: DMRefine(), DMDAGetRefinementFactor()
@*/
PetscErrorCode  DMDASetRefinementFactor(DM da, PetscInt refine_x, PetscInt refine_y,PetscInt refine_z)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidLogicalCollectiveInt(da,refine_x,2);
  PetscValidLogicalCollectiveInt(da,refine_y,3);
  PetscValidLogicalCollectiveInt(da,refine_z,4);

  if (refine_x > 0) dd->refine_x = refine_x;
  if (refine_y > 0) dd->refine_y = refine_y;
  if (refine_z > 0) dd->refine_z = refine_z;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDAGetRefinementFactor"
/*@C
     DMDAGetRefinementFactor - Gets the ratios that the DMDA grid is refined

    Not Collective

  Input Parameter:
.    da - the DMDA object

  Output Parameters:
+    refine_x - ratio of fine grid to coarse in x direction (2 by default)
.    refine_y - ratio of fine grid to coarse in y direction (2 by default)
-    refine_z - ratio of fine grid to coarse in z direction (2 by default)

  Level: intermediate

    Notes: Pass PETSC_NULL for values you do not need

.seealso: DMRefine(), DMDASetRefinementFactor()
@*/
PetscErrorCode  DMDAGetRefinementFactor(DM da, PetscInt *refine_x, PetscInt *refine_y,PetscInt *refine_z)
{
  DM_DA *dd = (DM_DA*)da->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  if (refine_x) *refine_x = dd->refine_x;
  if (refine_y) *refine_y = dd->refine_y;
  if (refine_z) *refine_z = dd->refine_z;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMDASetGetMatrix"
/*@C
     DMDASetGetMatrix - Sets the routine used by the DMDA to allocate a matrix.

    Logically Collective on DMDA

  Input Parameters:
+    da - the DMDA object
-    f - the function that allocates the matrix for that specific DMDA

  Level: developer

   Notes: See DMDASetBlockFills() that provides a simple way to provide the nonzero structure for 
       the diagonal and off-diagonal blocks of the matrix

.seealso: DMGetMatrix(), DMDASetBlockFills()
@*/
PetscErrorCode  DMDASetGetMatrix(DM da,PetscErrorCode (*f)(DM, const MatType,Mat*))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  da->ops->getmatrix = f;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDARefineOwnershipRanges"
/*
  Creates "balanced" ownership ranges after refinement, constrained by the need for the
  fine grid boundaries to fall within one stencil width of the coarse partition.

  Uses a greedy algorithm to handle non-ideal layouts, could probably do something better.
*/
static PetscErrorCode DMDARefineOwnershipRanges(DM da,PetscBool periodic,PetscInt stencil_width,PetscInt ratio,PetscInt m,const PetscInt lc[],PetscInt lf[])
{
  PetscInt       i,totalc = 0,remaining,startc = 0,startf = 0;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (ratio < 1) SETERRQ1(((PetscObject)da)->comm,PETSC_ERR_USER,"Requested refinement ratio %D must be at least 1",ratio);
  if (ratio == 1) {
    ierr = PetscMemcpy(lf,lc,m*sizeof(lc[0]));CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  for (i=0; i<m; i++) totalc += lc[i];
  remaining = (!periodic) + ratio * (totalc - (!periodic));
  for (i=0; i<m; i++) {
    PetscInt want = remaining/(m-i) + !!(remaining%(m-i));
    if (i == m-1) lf[i] = want;
    else {
      const PetscInt nextc = startc + lc[i];
      /* Move the first fine node of the next subdomain to the right until the coarse node on its left is within one
       * coarse stencil width of the first coarse node in the next subdomain. */
      while ((startf+want)/ratio < nextc - stencil_width) want++;
      /* Move the last fine node in the current subdomain to the left until the coarse node on its right is within one
       * coarse stencil width of the last coarse node in the current subdomain. */
      while ((startf+want-1+ratio-1)/ratio > nextc-1+stencil_width) want--;
      /* Make sure all constraints are satisfied */
      if (want < 0 || want > remaining
          || ((startf+want)/ratio < nextc - stencil_width)
          || ((startf+want-1+ratio-1)/ratio > nextc-1+stencil_width))
        SETERRQ(((PetscObject)da)->comm,PETSC_ERR_ARG_SIZ,"Could not find a compatible refined ownership range");
    }
    lf[i] = want;
    startc += lc[i];
    startf += lf[i];
    remaining -= lf[i];
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDACoarsenOwnershipRanges"
/*
  Creates "balanced" ownership ranges after coarsening, constrained by the need for the
  fine grid boundaries to fall within one stencil width of the coarse partition.

  Uses a greedy algorithm to handle non-ideal layouts, could probably do something better.
*/
static PetscErrorCode DMDACoarsenOwnershipRanges(DM da,PetscBool periodic,PetscInt stencil_width,PetscInt ratio,PetscInt m,const PetscInt lf[],PetscInt lc[])
{
  PetscInt       i,totalf,remaining,startc,startf;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (ratio < 1) SETERRQ1(((PetscObject)da)->comm,PETSC_ERR_USER,"Requested refinement ratio %D must be at least 1",ratio);
  if (ratio == 1) {
    ierr = PetscMemcpy(lc,lf,m*sizeof(lf[0]));CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  for (i=0,totalf=0; i<m; i++) totalf += lf[i];
  remaining = (!periodic) + (totalf - (!periodic)) / ratio;
  for (i=0,startc=0,startf=0; i<m; i++) {
    PetscInt want = remaining/(m-i) + !!(remaining%(m-i));
    if (i == m-1) lc[i] = want;
    else {
      const PetscInt nextf = startf+lf[i];
      /* Slide first coarse node of next subdomain to the left until the coarse node to the left of the first fine
       * node is within one stencil width. */
      while (nextf/ratio < startc+want-stencil_width) want--;
      /* Slide the last coarse node of the current subdomain to the right until the coarse node to the right of the last
       * fine node is within one stencil width. */
      while ((nextf-1+ratio-1)/ratio > startc+want-1+stencil_width) want++;
      if (want < 0 || want > remaining
          || (nextf/ratio < startc+want-stencil_width)
          || ((nextf-1+ratio-1)/ratio > startc+want-1+stencil_width))
        SETERRQ(((PetscObject)da)->comm,PETSC_ERR_ARG_SIZ,"Could not find a compatible coarsened ownership range");
    }
    lc[i] = want;
    startc += lc[i];
    startf += lf[i];
    remaining -= lc[i];
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMRefine_DA"
PetscErrorCode  DMRefine_DA(DM da,MPI_Comm comm,DM *daref)
{
  PetscErrorCode ierr;
  PetscInt       M,N,P;
  DM             da2;
  DM_DA          *dd = (DM_DA*)da->data,*dd2;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidPointer(daref,3);

  if (dd->bx == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0){
    M = dd->refine_x*dd->M;
  } else {
    M = 1 + dd->refine_x*(dd->M - 1);
  }
  if (dd->by == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0){
    N = dd->refine_y*dd->N;
  } else {
    N = 1 + dd->refine_y*(dd->N - 1);
  }
  if (dd->bz == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0){
    P = dd->refine_z*dd->P;
  } else {
    P = 1 + dd->refine_z*(dd->P - 1);
  }
  ierr = DMDACreateOwnershipRanges(da);CHKERRQ(ierr);
  if (dd->dim == 3) {
    PetscInt *lx,*ly,*lz;
    ierr = PetscMalloc3(dd->m,PetscInt,&lx,dd->n,PetscInt,&ly,dd->p,PetscInt,&lz);CHKERRQ(ierr);
    ierr = DMDARefineOwnershipRanges(da,(PetscBool)(dd->bx == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_x,dd->m,dd->lx,lx);CHKERRQ(ierr);
    ierr = DMDARefineOwnershipRanges(da,(PetscBool)(dd->by == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_y,dd->n,dd->ly,ly);CHKERRQ(ierr);
    ierr = DMDARefineOwnershipRanges(da,(PetscBool)(dd->bz == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_z,dd->p,dd->lz,lz);CHKERRQ(ierr);
    ierr = DMDACreate3d(((PetscObject)da)->comm,dd->bx,dd->by,dd->bz,dd->stencil_type,M,N,P,dd->m,dd->n,dd->p,dd->w,dd->s,lx,ly,lz,&da2);CHKERRQ(ierr);
    ierr = PetscFree3(lx,ly,lz);CHKERRQ(ierr);
  } else if (dd->dim == 2) {
    PetscInt *lx,*ly;
    ierr = PetscMalloc2(dd->m,PetscInt,&lx,dd->n,PetscInt,&ly);CHKERRQ(ierr);
    ierr = DMDARefineOwnershipRanges(da,(PetscBool)(dd->bx == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_x,dd->m,dd->lx,lx);CHKERRQ(ierr);
    ierr = DMDARefineOwnershipRanges(da,(PetscBool)(dd->by == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_y,dd->n,dd->ly,ly);CHKERRQ(ierr);
    ierr = DMDACreate2d(((PetscObject)da)->comm,dd->bx,dd->by,dd->stencil_type,M,N,dd->m,dd->n,dd->w,dd->s,lx,ly,&da2);CHKERRQ(ierr);
    ierr = PetscFree2(lx,ly);CHKERRQ(ierr);
  } else if (dd->dim == 1) {
    PetscInt *lx;
    ierr = PetscMalloc(dd->m*sizeof(PetscInt),&lx);CHKERRQ(ierr);
    ierr = DMDARefineOwnershipRanges(da,(PetscBool)(dd->bx == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_x,dd->m,dd->lx,lx);CHKERRQ(ierr);
    ierr = DMDACreate1d(((PetscObject)da)->comm,dd->bx,M,dd->w,dd->s,lx,&da2);CHKERRQ(ierr);
    ierr = PetscFree(lx);CHKERRQ(ierr);
  }
  dd2 = (DM_DA*)da2->data;

  /* allow overloaded (user replaced) operations to be inherited by refinement clones */
  da2->ops->getmatrix        = da->ops->getmatrix;
  da2->ops->getinterpolation = da->ops->getinterpolation;
  da2->ops->getcoloring      = da->ops->getcoloring;
  dd2->interptype            = dd->interptype;
  
  /* copy fill information if given */
  if (dd->dfill) {
    ierr = PetscMalloc((dd->dfill[dd->w]+dd->w+1)*sizeof(PetscInt),&dd2->dfill);CHKERRQ(ierr);
    ierr = PetscMemcpy(dd2->dfill,dd->dfill,(dd->dfill[dd->w]+dd->w+1)*sizeof(PetscInt));CHKERRQ(ierr);
  }
  if (dd->ofill) {
    ierr = PetscMalloc((dd->ofill[dd->w]+dd->w+1)*sizeof(PetscInt),&dd2->ofill);CHKERRQ(ierr);
    ierr = PetscMemcpy(dd2->ofill,dd->ofill,(dd->ofill[dd->w]+dd->w+1)*sizeof(PetscInt));CHKERRQ(ierr);
  }
  /* copy the refine information */
  dd2->refine_x = dd->refine_x;
  dd2->refine_y = dd->refine_y;
  dd2->refine_z = dd->refine_z;

  /* copy vector type information */
  ierr = PetscFree(da2->vectype);CHKERRQ(ierr);
  ierr = PetscStrallocpy(da->vectype,&da2->vectype);CHKERRQ(ierr);

  /* interpolate coordinates if they are set on the coarse grid */
  if (dd->coordinates) {
    DM  cdaf,cdac;
    Vec coordsc,coordsf;
    Mat II;
    
    ierr = DMDAGetCoordinateDA(da,&cdac);CHKERRQ(ierr);
    ierr = DMDAGetCoordinates(da,&coordsc);CHKERRQ(ierr);
    ierr = DMDAGetCoordinateDA(da2,&cdaf);CHKERRQ(ierr);
    /* force creation of the coordinate vector */
    ierr = DMDASetUniformCoordinates(da2,0.0,1.0,0.0,1.0,0.0,1.0);CHKERRQ(ierr);
    ierr = DMDAGetCoordinates(da2,&coordsf);CHKERRQ(ierr);
    ierr = DMGetInterpolation(cdac,cdaf,&II,PETSC_NULL);CHKERRQ(ierr);
    ierr = MatInterpolate(II,coordsc,coordsf);CHKERRQ(ierr);
    ierr = MatDestroy(&II);CHKERRQ(ierr);
  }
  *daref = da2;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMCoarsen_DA"
PetscErrorCode  DMCoarsen_DA(DM da, MPI_Comm comm,DM *daref)
{
  PetscErrorCode ierr;
  PetscInt       M,N,P;
  DM             da2;
  DM_DA          *dd = (DM_DA*)da->data,*dd2;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  PetscValidPointer(daref,3);

  if (dd->bx == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0){
    M = dd->M / dd->refine_x;
  } else {
    M = 1 + (dd->M - 1) / dd->refine_x;
  }
  if (dd->by == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0){
    N = dd->N / dd->refine_y;
  } else {
    N = 1 + (dd->N - 1) / dd->refine_y;
  }
  if (dd->bz == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0){
    P = dd->P / dd->refine_z;
  } else {
    P = 1 + (dd->P - 1) / dd->refine_z;
  }
  ierr = DMDACreateOwnershipRanges(da);CHKERRQ(ierr);
  if (dd->dim == 3) {
    PetscInt *lx,*ly,*lz;
    ierr = PetscMalloc3(dd->m,PetscInt,&lx,dd->n,PetscInt,&ly,dd->p,PetscInt,&lz);CHKERRQ(ierr);
    ierr = DMDACoarsenOwnershipRanges(da,(PetscBool)(dd->bx == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_x,dd->m,dd->lx,lx);CHKERRQ(ierr);
    ierr = DMDACoarsenOwnershipRanges(da,(PetscBool)(dd->by == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_y,dd->n,dd->ly,ly);CHKERRQ(ierr);
    ierr = DMDACoarsenOwnershipRanges(da,(PetscBool)(dd->bz == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_z,dd->p,dd->lz,lz);CHKERRQ(ierr);
    ierr = DMDACreate3d(((PetscObject)da)->comm,dd->bx,dd->by,dd->bz,dd->stencil_type,M,N,P,dd->m,dd->n,dd->p,dd->w,dd->s,lx,ly,lz,&da2);CHKERRQ(ierr);
    ierr = PetscFree3(lx,ly,lz);CHKERRQ(ierr);
  } else if (dd->dim == 2) {
    PetscInt *lx,*ly;
    ierr = PetscMalloc2(dd->m,PetscInt,&lx,dd->n,PetscInt,&ly);CHKERRQ(ierr);
    ierr = DMDACoarsenOwnershipRanges(da,(PetscBool)(dd->bx == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_x,dd->m,dd->lx,lx);CHKERRQ(ierr);
    ierr = DMDACoarsenOwnershipRanges(da,(PetscBool)(dd->by == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_y,dd->n,dd->ly,ly);CHKERRQ(ierr);
    ierr = DMDACreate2d(((PetscObject)da)->comm,dd->bx,dd->by,dd->stencil_type,M,N,dd->m,dd->n,dd->w,dd->s,lx,ly,&da2);CHKERRQ(ierr);
    ierr = PetscFree2(lx,ly);CHKERRQ(ierr);
  } else if (dd->dim == 1) {
    PetscInt *lx;
    ierr = PetscMalloc(dd->m*sizeof(PetscInt),&lx);CHKERRQ(ierr);
    ierr = DMDACoarsenOwnershipRanges(da,(PetscBool)(dd->bx == DMDA_BOUNDARY_PERIODIC || dd->interptype == DMDA_Q0),dd->s,dd->refine_x,dd->m,dd->lx,lx);CHKERRQ(ierr);
    ierr = DMDACreate1d(((PetscObject)da)->comm,dd->bx,M,dd->w,dd->s,lx,&da2);CHKERRQ(ierr);
    ierr = PetscFree(lx);CHKERRQ(ierr);
  }
  dd2 = (DM_DA*)da2->data;

  /* allow overloaded (user replaced) operations to be inherited by refinement clones */
  da2->ops->getmatrix        = da->ops->getmatrix;
  da2->ops->getinterpolation = da->ops->getinterpolation;
  da2->ops->getcoloring      = da->ops->getcoloring;
  dd2->interptype            = dd->interptype;
  
  /* copy fill information if given */
  if (dd->dfill) {
    ierr = PetscMalloc((dd->dfill[dd->w]+dd->w+1)*sizeof(PetscInt),&dd2->dfill);CHKERRQ(ierr);
    ierr = PetscMemcpy(dd2->dfill,dd->dfill,(dd->dfill[dd->w]+dd->w+1)*sizeof(PetscInt));CHKERRQ(ierr);
  }
  if (dd->ofill) {
    ierr = PetscMalloc((dd->ofill[dd->w]+dd->w+1)*sizeof(PetscInt),&dd2->ofill);CHKERRQ(ierr);
    ierr = PetscMemcpy(dd2->ofill,dd->ofill,(dd->ofill[dd->w]+dd->w+1)*sizeof(PetscInt));CHKERRQ(ierr);
  }
  /* copy the refine information */
  dd2->refine_x = dd->refine_x;
  dd2->refine_y = dd->refine_y;
  dd2->refine_z = dd->refine_z;

  /* copy vector type information */
  ierr = PetscFree(da2->vectype);CHKERRQ(ierr);
  ierr = PetscStrallocpy(da->vectype,&da2->vectype);CHKERRQ(ierr);

  /* inject coordinates if they are set on the fine grid */
  if (dd->coordinates) {
    DM  cdaf,cdac;
    Vec coordsc,coordsf;
    VecScatter inject;
    
    ierr = DMDAGetCoordinateDA(da,&cdaf);CHKERRQ(ierr);
    ierr = DMDAGetCoordinates(da,&coordsf);CHKERRQ(ierr);
    ierr = DMDAGetCoordinateDA(da2,&cdac);CHKERRQ(ierr);
    /* force creation of the coordinate vector */
    ierr = DMDASetUniformCoordinates(da2,0.0,1.0,0.0,1.0,0.0,1.0);CHKERRQ(ierr);
    ierr = DMDAGetCoordinates(da2,&coordsc);CHKERRQ(ierr);
    
    ierr = DMGetInjection(cdac,cdaf,&inject);CHKERRQ(ierr);
    ierr = VecScatterBegin(inject,coordsf,coordsc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(inject  ,coordsf,coordsc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&inject);CHKERRQ(ierr);
  }
  *daref = da2;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMRefineHierarchy_DA"
PetscErrorCode  DMRefineHierarchy_DA(DM da,PetscInt nlevels,DM daf[])
{
  PetscErrorCode ierr;
  PetscInt       i,n,*refx,*refy,*refz;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  if (nlevels < 0) SETERRQ(((PetscObject)da)->comm,PETSC_ERR_ARG_OUTOFRANGE,"nlevels cannot be negative");
  if (nlevels == 0) PetscFunctionReturn(0);
  PetscValidPointer(daf,3);

  /* Get refinement factors, defaults taken from the coarse DMDA */
  ierr = PetscMalloc3(nlevels,PetscInt,&refx,nlevels,PetscInt,&refy,nlevels,PetscInt,&refz);CHKERRQ(ierr);
  for (i=0; i<nlevels; i++) {
    ierr = DMDAGetRefinementFactor(da,&refx[i],&refy[i],&refz[i]);CHKERRQ(ierr);
  }
  n = nlevels;
  ierr = PetscOptionsGetIntArray(((PetscObject)da)->prefix,"-da_refine_hierarchy_x",refx,&n,PETSC_NULL);CHKERRQ(ierr);
  n = nlevels;
  ierr = PetscOptionsGetIntArray(((PetscObject)da)->prefix,"-da_refine_hierarchy_y",refy,&n,PETSC_NULL);CHKERRQ(ierr);
  n = nlevels;
  ierr = PetscOptionsGetIntArray(((PetscObject)da)->prefix,"-da_refine_hierarchy_z",refz,&n,PETSC_NULL);CHKERRQ(ierr);

  ierr = DMDASetRefinementFactor(da,refx[0],refy[0],refz[0]);CHKERRQ(ierr);
  ierr = DMRefine(da,((PetscObject)da)->comm,&daf[0]);CHKERRQ(ierr);
  for (i=1; i<nlevels; i++) {
    ierr = DMDASetRefinementFactor(daf[i-1],refx[i],refy[i],refz[i]);CHKERRQ(ierr);
    ierr = DMRefine(daf[i-1],((PetscObject)da)->comm,&daf[i]);CHKERRQ(ierr);
  }
  ierr = PetscFree3(refx,refy,refz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMCoarsenHierarchy_DA"
PetscErrorCode  DMCoarsenHierarchy_DA(DM da,PetscInt nlevels,DM dac[])
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  if (nlevels < 0) SETERRQ(((PetscObject)da)->comm,PETSC_ERR_ARG_OUTOFRANGE,"nlevels cannot be negative");
  if (nlevels == 0) PetscFunctionReturn(0);
  PetscValidPointer(dac,3);
  ierr = DMCoarsen(da,((PetscObject)da)->comm,&dac[0]);CHKERRQ(ierr);
  for (i=1; i<nlevels; i++) {
    ierr = DMCoarsen(dac[i-1],((PetscObject)da)->comm,&dac[i]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
