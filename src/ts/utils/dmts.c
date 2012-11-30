#include <petsc-private/tsimpl.h>     /*I "petscts.h" I*/
#include <petsc-private/dmimpl.h>     /*I "petscdm.h" I*/

#undef __FUNCT__
#define __FUNCT__ "DMTSDestroy"
static PetscErrorCode DMTSDestroy(DMTS *kdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
 if (!*kdm) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*kdm),DMTS_CLASSID,1);
  if (--((PetscObject)(*kdm))->refct > 0) {*kdm = 0; PetscFunctionReturn(0);}
  if ((*kdm)->ops->destroy) {ierr = ((*kdm)->ops->destroy)(*kdm);CHKERRQ(ierr);}
  ierr = PetscHeaderDestroy(kdm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSCreate"
static PetscErrorCode DMTSCreate(MPI_Comm comm,DMTS *kdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
#ifndef PETSC_USE_DYNAMIC_LIBRARIES
  ierr = TSInitializePackage(PETSC_NULL);CHKERRQ(ierr);
#endif
  ierr = PetscHeaderCreate(*kdm, _p_DMTS, struct _DMTSOps, DMTS_CLASSID, -1, "DMKSP", "DMTS", "DMTS", comm, DMTSDestroy, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscMemzero((*kdm)->ops, sizeof(struct _DMTSOps));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCoarsenHook_DMTS"
/* Attaches the DMSNES to the coarse level.
 * Under what conditions should we copy versus duplicate?
 */
static PetscErrorCode DMCoarsenHook_DMTS(DM dm,DM dmc,void *ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMCopyDMTS(dm,dmc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRestrictHook_DMTS"
/* This could restrict auxiliary information to the coarse level.
 */
static PetscErrorCode DMRestrictHook_DMTS(DM dm,Mat Restrict,Vec rscale,Mat Inject,DM dmc,void *ctx)
{

  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSCopy"
/*@C
   DMTSCopy - copies the information in a DMTS to another DMTS

   Not Collective

   Input Argument:
+  kdm - Original DMTS
-  nkdm - DMTS to receive the data, should have been created with DMTSCreate()

   Level: developer

.seealso: DMTSCreate(), DMTSDestroy()
@*/
PetscErrorCode DMTSCopy(DMTS kdm,DMTS nkdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(kdm,DMTS_CLASSID,1);
  PetscValidHeaderSpecific(nkdm,DMTS_CLASSID,2);
  nkdm->ops->rhsfunction    = kdm->ops->rhsfunction;
  nkdm->ops->rhsjacobian    = kdm->ops->rhsjacobian;
  nkdm->ops->ifunction      = kdm->ops->ifunction;
  nkdm->ops->ijacobian      = kdm->ops->ijacobian;
  nkdm->ops->solution       = kdm->ops->solution;
  nkdm->ops->destroy        = kdm->ops->destroy;
  nkdm->ops->duplicate      = kdm->ops->duplicate;

  nkdm->rhsfunctionctx      = kdm->rhsfunctionctx;
  nkdm->rhsjacobianctx      = kdm->rhsjacobianctx;
  nkdm->ifunctionctx        = kdm->ifunctionctx;
  nkdm->ijacobianctx        = kdm->ijacobianctx;
  nkdm->solutionctx         = kdm->solutionctx;

  nkdm->data             = kdm->data;

  /*
  nkdm->fortran_func_pointers[0] = kdm->fortran_func_pointers[0];
  nkdm->fortran_func_pointers[1] = kdm->fortran_func_pointers[1];
  nkdm->fortran_func_pointers[2] = kdm->fortran_func_pointers[2];
  */

  /* implementation specific copy hooks */
  if (kdm->ops->duplicate) {ierr = (*kdm->ops->duplicate)(kdm,nkdm);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetDMTS"
/*@C
   DMGetDMTS - get read-only private DMTS context from a DM

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Argument:
.  tsdm - private DMTS context

   Level: developer

   Notes:
   Use DMGetDMTSWrite() if write access is needed. The DMTSSetXXX API should be used wherever possible.

.seealso: DMGetDMTSWrite()
@*/
PetscErrorCode DMGetDMTS(DM dm,DMTS *tsdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = PetscObjectQuery((PetscObject)dm,"DMTS",(PetscObject*)tsdm);CHKERRQ(ierr);
  if (!*tsdm) {
    DMTS tmptsdm;
    ierr = PetscInfo(dm,"Creating new DMTS\n");CHKERRQ(ierr);
    ierr = DMTSCreate(((PetscObject)dm)->comm,tsdm);CHKERRQ(ierr);
    ierr = PetscObjectCompose((PetscObject)dm,"DMTS",(PetscObject)*tsdm);CHKERRQ(ierr);
    ierr = DMCoarsenHookAdd(dm,DMCoarsenHook_DMTS,DMRestrictHook_DMTS,PETSC_NULL);CHKERRQ(ierr);
    tmptsdm = *tsdm;
    ierr = DMTSDestroy(&tmptsdm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetDMTSWrite"
/*@C
   DMGetDMTSWrite - get write access to private DMTS context from a DM

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Argument:
.  tsdm - private DMTS context

   Level: developer

.seealso: DMGetDMTS()
@*/
PetscErrorCode DMGetDMTSWrite(DM dm,DMTS *tsdm)
{
  PetscErrorCode ierr;
  DMTS           sdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&sdm);CHKERRQ(ierr);
  if (!sdm->originaldm) sdm->originaldm = dm;
  if (sdm->originaldm != dm) {  /* Copy on write */
    DMTS          oldsdm = sdm,tsdm;
    ierr = PetscInfo(dm,"Copying DMTS due to write\n");CHKERRQ(ierr);
    ierr = DMTSCreate(((PetscObject)dm)->comm,&sdm);CHKERRQ(ierr);
    ierr = DMTSCopy(oldsdm,sdm);CHKERRQ(ierr);

    ierr = PetscObjectCompose((PetscObject)dm,"DMTS",(PetscObject)sdm);CHKERRQ(ierr);
    tsdm = sdm;
    ierr = DMTSDestroy(&tsdm );CHKERRQ(ierr);
  }
  *tsdm = sdm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCopyDMTS"
/*@C
   DMCopyDMTS - copies a DM context to a new DM

   Logically Collective

   Input Arguments:
+  dmsrc - DM to obtain context from
-  dmdest - DM to add context to

   Level: developer

   Note:
   The context is copied by reference. This function does not ensure that a context exists.

.seealso: DMGetDMTS(), TSSetDM()
@*/
PetscErrorCode DMCopyDMTS(DM dmsrc,DM dmdest)
{
  PetscErrorCode ierr;
  DMTS           sdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dmsrc,DM_CLASSID,1);
  PetscValidHeaderSpecific(dmdest,DM_CLASSID,2);
  ierr = PetscObjectQuery((PetscObject)dmsrc,"DMTS",(PetscObject*)&sdm);CHKERRQ(ierr);
  if (sdm) {
    ierr = PetscObjectCompose((PetscObject)dmdest,"DMTS",(PetscObject)sdm);CHKERRQ(ierr);
    ierr = DMCoarsenHookAdd(dmdest,DMCoarsenHook_DMTS,DMRestrictHook_DMTS,PETSC_NULL);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetIFunction"
/*@C
   DMTSSetIFunction - set TS implicit function evaluation function

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  func - function evaluation function, see TSSetIFunction() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSSetFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the residual.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSSetIFunction(DM dm,TSIFunction func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (func) tsdm->ops->ifunction = func;
  if (ctx)  tsdm->ifunctionctx = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetIFunction"
/*@C
   DMTSGetIFunction - get TS implicit residual evaluation function

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Arguments:
+  func - function evaluation function, see TSSetIFunction() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSGetFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.

.seealso: DMTSSetContext(), DMTSSetFunction(), TSSetFunction()
@*/
PetscErrorCode DMTSGetIFunction(DM dm,TSIFunction *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->ifunction;
  if (ctx)  *ctx = tsdm->ifunctionctx;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMTSSetRHSFunction"
/*@C
   DMTSSetRHSFunction - set TS explicit residual evaluation function

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  func - RHS function evaluation function, see TSSetRHSFunction() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSSetRSHFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the residual.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSSetRHSFunction(DM dm,TSRHSFunction func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (func) tsdm->ops->rhsfunction = func;
  if (ctx)  tsdm->rhsfunctionctx = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetSolutionFunction"
/*@C
   DMTSGetSolutionFunction - gets the TS solution evaluation function

   Not Collective

   Input Arguments:
.  dm - DM to be used with TS

   Output Parameters:
+  func - solution function evaluation function, see TSSetSolution() for calling sequence
-  ctx - context for solution evaluation

   Level: advanced

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSGetSolutionFunction(DM dm,TSSolutionFunction *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->solution;
  if (ctx)  *ctx  = tsdm->solutionctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetSolutionFunction"
/*@C
   DMTSSetSolutionFunction - set TS solution evaluation function

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  func - solution function evaluation function, see TSSetSolution() for calling sequence
-  ctx - context for solution evaluation

   Level: advanced

   Note:
   TSSetSolutionFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the residual.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSSetSolutionFunction(DM dm,TSSolutionFunction func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (func) tsdm->ops->solution = func;
  if (ctx)  tsdm->solutionctx   = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetRHSFunction"
/*@C
   DMTSGetRHSFunction - get TS explicit residual evaluation function

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Arguments:
+  func - residual evaluation function, see TSSetRHSFunction() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSGetFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.

.seealso: DMTSSetContext(), DMTSSetFunction(), TSSetFunction()
@*/
PetscErrorCode DMTSGetRHSFunction(DM dm,TSRHSFunction *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->rhsfunction;
  if (ctx)  *ctx = tsdm->rhsfunctionctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetIJacobian"
/*@C
   DMTSSetIJacobian - set TS Jacobian evaluation function

   Not Collective

   Input Argument:
+  dm - DM to be used with TS
.  func - Jacobian evaluation function, see TSSetIJacobian() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSSetJacobian() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the Jacobian.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSGetJacobian(), TSSetJacobian()
@*/
PetscErrorCode DMTSSetIJacobian(DM dm,TSIJacobian func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           sdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&sdm);CHKERRQ(ierr);
  if (func) sdm->ops->ijacobian = func;
  if (ctx)  sdm->ijacobianctx   = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetIJacobian"
/*@C
   DMTSGetIJacobian - get TS Jacobian evaluation function

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Arguments:
+  func - Jacobian evaluation function, see TSSetIJacobian() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSGetJacobian() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the Jacobian.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSGetIJacobian(DM dm,TSIJacobian *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->ijacobian;
  if (ctx)  *ctx = tsdm->ijacobianctx;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMTSSetRHSJacobian"
/*@C
   DMTSSetRHSJacobian - set TS Jacobian evaluation function

   Not Collective

   Input Argument:
+  dm - DM to be used with TS
.  func - Jacobian evaluation function, see TSSetRHSJacobian() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSSetJacobian() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the Jacobian.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSGetJacobian(), TSSetJacobian()
@*/
PetscErrorCode DMTSSetRHSJacobian(DM dm,TSRHSJacobian func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (func) tsdm->ops->rhsjacobian = func;
  if (ctx)  tsdm->rhsjacobianctx = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetRHSJacobian"
/*@C
   DMTSGetRHSJacobian - get TS Jacobian evaluation function

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Arguments:
+  func - Jacobian evaluation function, see TSSetRHSJacobian() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSGetJacobian() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the Jacobian.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSGetRHSJacobian(DM dm,TSRHSJacobian *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->rhsjacobian;
  if (ctx)  *ctx = tsdm->rhsjacobianctx;
  PetscFunctionReturn(0);
}
