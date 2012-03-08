#include <private/linesearchimpl.h> /*I "petsclinesearch.h" I*/

PetscBool  LineSearchRegisterAllCalled = PETSC_FALSE;
PetscFList LineSearchList              = PETSC_NULL;

PetscClassId   LineSearch_CLASSID;
PetscLogEvent  LineSearch_Apply;

#undef __FUNCT__
#define __FUNCT__ "LineSearchCreate"
PetscErrorCode LineSearchCreate(MPI_Comm comm, LineSearch * outlinesearch) {
  PetscErrorCode ierr;
  LineSearch     linesearch;
  PetscFunctionBegin;
  ierr = PetscHeaderCreate(linesearch, _p_LineSearch,struct _LineSearchOps,LineSearch_CLASSID, 0,
                           "LineSearch","Line-search method","LineSearch",comm,LineSearchDestroy,LineSearchView);CHKERRQ(ierr);

  linesearch->ops->precheckstep = PETSC_NULL;
  linesearch->ops->postcheckstep = PETSC_NULL;

  linesearch->lambda        = 1.0;
  linesearch->fnorm         = 1.0;
  linesearch->ynorm         = 1.0;
  linesearch->xnorm         = 1.0;
  linesearch->success       = PETSC_TRUE;
  linesearch->norms         = PETSC_TRUE;
  linesearch->keeplambda    = PETSC_FALSE;
  linesearch->damping       = 1.0;
  linesearch->maxstep       = 1e8;
  linesearch->steptol       = 1e-12;
  linesearch->precheckctx   = PETSC_NULL;
  linesearch->postcheckctx  = PETSC_NULL;
  linesearch->max_its       = 1;
  linesearch->setupcalled   = PETSC_FALSE;
  *outlinesearch            = linesearch;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetUp"
PetscErrorCode LineSearchSetUp(LineSearch linesearch) {
  PetscErrorCode ierr;
  PetscFunctionBegin;

  if (!((PetscObject)linesearch)->type_name) {
    ierr = LineSearchSetType(linesearch,LINESEARCHBASIC);CHKERRQ(ierr);
  }

  if (!linesearch->setupcalled) {
    ierr = VecDuplicate(linesearch->vec_sol, &linesearch->vec_sol_new);CHKERRQ(ierr);
    ierr = VecDuplicate(linesearch->vec_func, &linesearch->vec_func_new);CHKERRQ(ierr);
    if (linesearch->ops->setup) {
      ierr = (*linesearch->ops->setup)(linesearch);CHKERRQ(ierr);
    }
    linesearch->lambda = linesearch->damping;
    linesearch->setupcalled = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchReset"
PetscErrorCode LineSearchReset(LineSearch linesearch) {
  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (linesearch->ops->reset) {
    (*linesearch->ops->reset)(linesearch);
  }
  ierr = VecDestroy(&linesearch->vec_sol_new);CHKERRQ(ierr);
  ierr = VecDestroy(&linesearch->vec_func_new);CHKERRQ(ierr);

  ierr = VecDestroyVecs(linesearch->nwork, &linesearch->work);CHKERRQ(ierr);
  linesearch->nwork = 0;
  linesearch->setupcalled = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchPreCheck"
PetscErrorCode LineSearchPreCheck(LineSearch linesearch, PetscBool * changed)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  *changed = PETSC_FALSE;
  if (linesearch->ops->precheckstep) {
    ierr = (*linesearch->ops->precheckstep)(linesearch, linesearch->vec_sol, linesearch->vec_update, changed);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchPostCheck"
PetscErrorCode LineSearchPostCheck(LineSearch linesearch, PetscBool * changed_W, PetscBool * changed_Y)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  *changed_Y = PETSC_FALSE;
  *changed_W = PETSC_FALSE;
  if (linesearch->ops->postcheckstep) {
    ierr = (*linesearch->ops->postcheckstep)(linesearch, linesearch->vec_sol, linesearch->vec_sol_new, linesearch->vec_update, changed_W, changed_Y);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchApply"
PetscErrorCode LineSearchApply(LineSearch linesearch, Vec X, Vec F, PetscReal * fnorm, Vec Y) {
  PetscErrorCode ierr;
  PetscFunctionBegin;

  /* check the pointers */
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidHeaderSpecific(X,VEC_CLASSID,2);
  PetscValidHeaderSpecific(F,VEC_CLASSID,3);
  PetscValidHeaderSpecific(Y,VEC_CLASSID,4);

  linesearch->success = PETSC_TRUE;

  linesearch->vec_sol = X;
  linesearch->vec_update = Y;
  linesearch->vec_func = F;

  ierr = LineSearchSetUp(linesearch);CHKERRQ(ierr);

  if (!linesearch->keeplambda)
    linesearch->lambda = linesearch->damping; /* set the initial guess to lambda */

  if (fnorm) {
    linesearch->fnorm = *fnorm;
  } else {
    ierr = VecNorm(F, NORM_2, &linesearch->fnorm);CHKERRQ(ierr);
  }

  ierr = PetscLogEventBegin(LineSearch_Apply,linesearch,X,F,Y);CHKERRQ(ierr);

  ierr = (*linesearch->ops->apply)(linesearch);CHKERRQ(ierr);

  ierr = PetscLogEventEnd(LineSearch_Apply,linesearch,X,F,Y);CHKERRQ(ierr);

  if (fnorm)
    *fnorm = linesearch->fnorm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchDestroy"
PetscErrorCode LineSearchDestroy(LineSearch * linesearch) {
  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (!*linesearch) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*linesearch),LineSearch_CLASSID,1);
  if (--((PetscObject)(*linesearch))->refct > 0) {*linesearch = 0; PetscFunctionReturn(0);}
  ierr = PetscObjectDepublish((*linesearch));CHKERRQ(ierr);
  ierr = LineSearchReset(*linesearch);
  if ((*linesearch)->ops->destroy) {
    (*linesearch)->ops->destroy(*linesearch);
  }
  ierr = PetscViewerDestroy(&(*linesearch)->monitor);CHKERRQ(ierr);
  ierr = PetscHeaderDestroy(linesearch);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetMonitor"
/*@C
   SNESLineSearchSetMonitor - Prints information about the progress or lack of progress of the line search

   Input Parameters:
+  snes - nonlinear context obtained from SNESCreate()
-  flg - PETSC_TRUE to monitor the line search

   Logically Collective on SNES

   Options Database:
.   -snes_ls_monitor

   Level: intermediate


.seealso: SNESLineSearchSet(), SNESLineSearchSetPostCheck(), SNESSetUpdate()
@*/
PetscErrorCode  LineSearchSetMonitor(LineSearch linesearch,PetscBool flg)
{

  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (flg && !linesearch->monitor) {
    ierr = PetscViewerASCIIOpen(((PetscObject)linesearch)->comm,"stdout",&linesearch->monitor);CHKERRQ(ierr);
  } else if (!flg && linesearch->monitor) {
    ierr = PetscViewerDestroy(&linesearch->monitor);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetMonitor"

PetscErrorCode  LineSearchGetMonitor(LineSearch linesearch, PetscViewer *monitor)
{

  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  if (monitor) {
    PetscValidPointer(monitor, 2);
    *monitor = linesearch->monitor;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetFromOptions"
PetscErrorCode LineSearchSetFromOptions(LineSearch linesearch) {
  PetscErrorCode ierr;
  const char     *deft = LINESEARCHBASIC;
  char           type[256];
  PetscBool      flg, set;
  PetscFunctionBegin;
  if (!LineSearchRegisterAllCalled) {ierr = LineSearchRegisterAll(PETSC_NULL);CHKERRQ(ierr);}

  ierr = PetscObjectOptionsBegin((PetscObject)linesearch);CHKERRQ(ierr);
  if (((PetscObject)linesearch)->type_name) {
    deft = ((PetscObject)linesearch)->type_name;
  }
  ierr = PetscOptionsList("-linesearch_type","Line-search method","LineSearchSetType",LineSearchList,deft,type,256,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = LineSearchSetType(linesearch,type);CHKERRQ(ierr);
  } else if (!((PetscObject)linesearch)->type_name) {
    ierr = LineSearchSetType(linesearch,deft);CHKERRQ(ierr);
  }
  if (linesearch->ops->setfromoptions) {
    (*linesearch->ops->setfromoptions)(linesearch);CHKERRQ(ierr);
  }

  ierr = PetscOptionsBool("-linesearch_monitor","Print progress of line searches","SNESLineSearchSetMonitor",
                          linesearch->monitor ? PETSC_TRUE : PETSC_FALSE,&flg,&set);CHKERRQ(ierr);
  if (set) {ierr = LineSearchSetMonitor(linesearch,flg);CHKERRQ(ierr);}

  ierr = PetscOptionsReal("-linesearch_damping","Line search damping and initial step guess","LineSearchSetDamping",linesearch->damping,&linesearch->damping,0);CHKERRQ(ierr);
  ierr = PetscOptionsBool("-linesearch_norms","Compute final norms in line search","LineSearchSetComputeNorms",linesearch->norms,&linesearch->norms,0);CHKERRQ(ierr);
  ierr = PetscOptionsBool("-linesearch_keeplambda","Use previous lambda as damping","LineSearchSetKeepLambda",linesearch->keeplambda,&linesearch->keeplambda,0);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-linesearch_max_it","Maximum iterations for iterative line searches","",linesearch->max_its,&linesearch->max_its,0);CHKERRQ(ierr);
  ierr = PetscObjectProcessOptionsHandlers((PetscObject)linesearch);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchView"
PetscErrorCode LineSearchView(LineSearch linesearch) {
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetType"
PetscErrorCode LineSearchSetType(LineSearch linesearch, const LineSearchType type)
{

  PetscErrorCode ierr,(*r)(LineSearch);
  PetscBool      match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidCharPointer(type,2);

  ierr = PetscTypeCompare((PetscObject)linesearch,type,&match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  ierr =  PetscFListFind(LineSearchList,((PetscObject)linesearch)->comm,type,PETSC_TRUE,(void (**)(void)) &r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_UNKNOWN_TYPE,"Unable to find requested Line Search type %s",type);
  /* Destroy the previous private linesearch context */
  if (linesearch->ops->destroy) {
    ierr = (*(linesearch)->ops->destroy)(linesearch);CHKERRQ(ierr);
    linesearch->ops->destroy = PETSC_NULL;
  }
  /* Reinitialize function pointers in LineSearchOps structure */
  linesearch->ops->apply          = 0;
  linesearch->ops->view           = 0;
  linesearch->ops->setfromoptions = 0;
  linesearch->ops->destroy        = 0;

  ierr = PetscObjectChangeTypeName((PetscObject)linesearch,type);CHKERRQ(ierr);
  ierr = (*r)(linesearch);CHKERRQ(ierr);
#if defined(PETSC_HAVE_AMS)
  if (PetscAMSPublishAll) {
    ierr = PetscObjectAMSPublish((PetscObject)linesearch);CHKERRQ(ierr);
  }
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetSNES"
PetscErrorCode  LineSearchSetSNES(LineSearch linesearch, SNES snes){
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidHeaderSpecific(snes,SNES_CLASSID,2);
  linesearch->snes = snes;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetSNES"
PetscErrorCode  LineSearchGetSNES(LineSearch linesearch, SNES *snes){
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidPointer(snes, 2);
  *snes = linesearch->snes;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetLambda"
PetscErrorCode  LineSearchGetLambda(LineSearch linesearch,PetscReal *lambda)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidPointer(lambda, 2);
  *lambda = linesearch->lambda;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetLambda"
PetscErrorCode  LineSearchSetLambda(LineSearch linesearch, PetscReal lambda)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  linesearch->lambda = lambda;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetStepTolerance"
PetscErrorCode  LineSearchGetStepTolerance(LineSearch linesearch ,PetscReal *steptol)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidPointer(steptol, 2);
  *steptol = linesearch->steptol;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetStepTolerance"
PetscErrorCode  LineSearchSetStepTolerance(LineSearch linesearch,PetscReal steptol)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  linesearch->steptol = steptol;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetDamping"
extern PetscErrorCode  LineSearchGetDamping(LineSearch linesearch,PetscReal *damping)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidPointer(damping, 2);
  *damping = linesearch->damping;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetDamping"
extern PetscErrorCode  LineSearchSetDamping(LineSearch linesearch,PetscReal damping)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  linesearch->damping = damping;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetMaxStep"
extern PetscErrorCode  LineSearchGetMaxStep(LineSearch linesearch,PetscReal* maxstep)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidPointer(maxstep, 2);
  *maxstep = linesearch->maxstep;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetMaxStep"
extern PetscErrorCode  LineSearchSetMaxStep(LineSearch linesearch, PetscReal maxstep)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  linesearch->maxstep = maxstep;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetMaxIts"
PetscErrorCode LineSearchGetMaxIts(LineSearch linesearch, PetscInt * max_its)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidPointer(max_its, 2);
  *max_its = linesearch->max_its;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetMaxIts"
PetscErrorCode LineSearchSetMaxIts(LineSearch linesearch, PetscInt max_its)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  linesearch->max_its = max_its;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetNorms"
PetscErrorCode  LineSearchGetNorms(LineSearch linesearch, PetscReal * xnorm, PetscReal * fnorm, PetscReal * ynorm)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  if (xnorm) {
    *xnorm = linesearch->xnorm;
  }
  if (fnorm) {
    *fnorm = linesearch->fnorm;
  }
  if (ynorm) {
    *ynorm = linesearch->ynorm;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetNorms"
PetscErrorCode  LineSearchSetNorms(LineSearch linesearch, PetscReal xnorm, PetscReal fnorm, PetscReal ynorm)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  if (xnorm) {
    linesearch->xnorm = xnorm;
  }
  if (fnorm) {
    linesearch->fnorm = fnorm;
  }
  if (ynorm) {
    linesearch->ynorm = ynorm;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchComputeNorms"
PetscErrorCode LineSearchComputeNorms(LineSearch linesearch)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (linesearch->norms) {
    ierr = VecNormBegin(linesearch->vec_func,   NORM_2, &linesearch->fnorm);CHKERRQ(ierr);
    ierr = VecNormBegin(linesearch->vec_sol,    NORM_2, &linesearch->xnorm);CHKERRQ(ierr);
    ierr = VecNormBegin(linesearch->vec_update, NORM_2, &linesearch->ynorm);CHKERRQ(ierr);
    ierr = VecNormEnd(linesearch->vec_func,     NORM_2, &linesearch->fnorm);CHKERRQ(ierr);
    ierr = VecNormEnd(linesearch->vec_sol,      NORM_2, &linesearch->xnorm);CHKERRQ(ierr);
    ierr = VecNormEnd(linesearch->vec_update,   NORM_2, &linesearch->ynorm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetVecs"
PetscErrorCode LineSearchGetVecs(LineSearch linesearch,Vec *X,Vec *F, Vec *Y,Vec *W,Vec *G) {
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  if (X) {
    PetscValidPointer(X, 2);
    *X = linesearch->vec_sol;
  }
  if (F) {
    PetscValidPointer(F, 3);
    *F = linesearch->vec_func;
  }
  if (Y) {
    PetscValidPointer(Y, 4);
    *Y = linesearch->vec_update;
  }
  if (W) {
    PetscValidPointer(W, 5);
    *W = linesearch->vec_sol_new;
  }
  if (G) {
    PetscValidPointer(G, 6);
    *G = linesearch->vec_func_new;
  }

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetVecs"
PetscErrorCode LineSearchSetVecs(LineSearch linesearch,Vec X,Vec F,Vec Y,Vec W, Vec G) {
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  if (X) {
    PetscValidHeaderSpecific(X,VEC_CLASSID,2);
    linesearch->vec_sol = X;
  }
  if (F) {
    PetscValidHeaderSpecific(F,VEC_CLASSID,3);
    linesearch->vec_func = F;
  }
  if (Y) {
    PetscValidHeaderSpecific(Y,VEC_CLASSID,4);
    linesearch->vec_update = Y;
  }
  if (W) {
    PetscValidHeaderSpecific(W,VEC_CLASSID,5);
    linesearch->vec_sol_new = W;
  }
  if (G) {
    PetscValidHeaderSpecific(G,VEC_CLASSID,6);
    linesearch->vec_func_new = G;
  }

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchAppendOptionsPrefix"
/*@C
   LineSearchAppendOptionsPrefix - Appends to the prefix used for searching for all
   SNES options in the database.

   Logically Collective on SNES

   Input Parameters:
+  snes - the SNES context
-  prefix - the prefix to prepend to all option names

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.

   Level: advanced

.keywords: SNES, append, options, prefix, database

.seealso: SNESGetOptionsPrefix()
@*/
PetscErrorCode  LineSearchAppendOptionsPrefix(LineSearch linesearch,const char prefix[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  ierr = PetscObjectAppendOptionsPrefix((PetscObject)linesearch,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetOptionsPrefix"
/*@C
   LineSearchGetOptionsPrefix - Sets the prefix used for searching for all
   SNES options in the database.

   Not Collective

   Input Parameter:
.  snes - the SNES context

   Output Parameter:
.  prefix - pointer to the prefix string used

   Notes: On the fortran side, the user should pass in a string 'prefix' of
   sufficient length to hold the prefix.

   Level: advanced

.keywords: SNES, get, options, prefix, database

.seealso: SNESAppendOptionsPrefix()
@*/
PetscErrorCode  LineSearchGetOptionsPrefix(LineSearch linesearch,const char *prefix[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  ierr = PetscObjectGetOptionsPrefix((PetscObject)linesearch,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchGetWork"
PetscErrorCode  LineSearchGetWork(LineSearch linesearch, PetscInt nwork)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  if (linesearch->vec_sol) {
    ierr = VecDuplicateVecs(linesearch->vec_sol, nwork, &linesearch->work);CHKERRQ(ierr);
  } else {
    SETERRQ(((PetscObject)linesearch)->comm, PETSC_ERR_USER, "Cannot get linesearch work-vectors without setting a solution vec!");
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "LineSearchGetSuccess"
PetscErrorCode  LineSearchGetSuccess(LineSearch linesearch, PetscBool *success)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscValidPointer(success, 2);
  if (success) {
    *success = linesearch->success;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchSetSuccess"
PetscErrorCode  LineSearchSetSuccess(LineSearch linesearch, PetscBool success)
{
  PetscValidHeaderSpecific(linesearch,LineSearch_CLASSID,1);
  PetscFunctionBegin;
  linesearch->success = success;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "LineSearchRegister"
/*@C
  LineSearchRegister - See LineSearchRegisterDynamic()

  Level: advanced
@*/
PetscErrorCode  LineSearchRegister(const char sname[],const char path[],const char name[],PetscErrorCode (*function)(LineSearch))
{
  char           fullname[PETSC_MAX_PATH_LEN];
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFListConcat(path,name,fullname);CHKERRQ(ierr);
  ierr = PetscFListAdd(&LineSearchList,sname,fullname,(void (*)(void))function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
