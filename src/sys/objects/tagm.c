#define PETSC_DLL
/*
      Some PETSc utilites
*/
#include "petscsys.h"             /*I    "petscsys.h"   I*/
#if defined(PETSC_HAVE_STDLIB_H)
#include <stdlib.h>
#endif

/* ---------------------------------------------------------------- */
/*
   A simple way to manage tags inside a communicator.  

   It uses the attributes to determine if a new communicator
      is needed and to store the available tags.

   Notes on the implementation

   The tagvalues to use are stored in a two element array.  The first element
   is the first free tag value.  The second is used to indicate how
   many references of the communicator there, when it equals zero the communicator may be freed.

  
*/

static PetscMPIInt Petsc_Tag_keyval       = MPI_KEYVAL_INVALID;
static PetscMPIInt Petsc_InnerComm_keyval = MPI_KEYVAL_INVALID;
static PetscMPIInt Petsc_OuterComm_keyval = MPI_KEYVAL_INVALID;
EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "Petsc_DelTag" 
/*
   Private routine to delete internal tag storage when a communicator is freed.

   This is called by MPI, not by users.

   Note: this is declared extern "C" because it is passed to MPI_Keyval_create

*/
PetscMPIInt PETSC_DLLEXPORT Petsc_DelTag(MPI_Comm comm,PetscMPIInt keyval,void* attr_val,void* extra_state)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscInfo1(0,"Deleting tag data in an MPI_Comm %ld\n",(long)comm);if (ierr) PetscFunctionReturn((PetscMPIInt)ierr);
  ierr = PetscFree(attr_val);if (ierr) PetscFunctionReturn((PetscMPIInt)ierr);
  PetscFunctionReturn(MPI_SUCCESS);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "Petsc_DelComm" 
/*
  This does not actually free anything, it simply marks when a reference count to an internal MPI_Comm reaches zero and the
  the external MPI_Comm drops its reference to the internal MPI_Comm

  This is called by MPI, not by users.

  Note: this is declared extern "C" because it is passed to MPI_Keyval_create

*/
PetscMPIInt PETSC_DLLEXPORT Petsc_DelComm(MPI_Comm comm,PetscMPIInt keyval,void* attr_val,void* extra_state)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscInfo1(0,"Deleting PETSc communicator imbedded in a user MPI_Comm %ld\n",(long)comm);if (ierr) PetscFunctionReturn((PetscMPIInt)ierr);
  /* actually don't delete anything because we cannot increase the reference count of the communicator anyways */
  PetscFunctionReturn(MPI_SUCCESS);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "PetscObjectGetNewTag" 
/*@C
    PetscObjectGetNewTag - Gets a unique new tag from a PETSc object. All 
    processors that share the object MUST call this routine EXACTLY the same
    number of times.  This tag should only be used with the current objects
    communicator; do NOT use it with any other MPI communicator.

    Collective on PetscObject

    Input Parameter:
.   obj - the PETSc object; this must be cast with a (PetscObject), for example, 
         PetscObjectGetNewTag((PetscObject)mat,&tag);

    Output Parameter:
.   tag - the new tag

    Level: developer

    Concepts: tag^getting
    Concepts: message tag^getting
    Concepts: MPI message tag^getting

.seealso: PetscCommGetNewTag()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscObjectGetNewTag(PetscObject obj,PetscMPIInt *tag)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscCommGetNewTag(obj->comm,tag);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscCommGetNewTag" 
/*@
    PetscCommGetNewTag - Gets a unique new tag from a PETSc communicator. All 
    processors that share the communicator MUST call this routine EXACTLY the same
    number of times.  This tag should only be used with the current objects
    communicator; do NOT use it with any other MPI communicator.

    Collective on comm

    Input Parameter:
.   comm - the MPI communicator

    Output Parameter:
.   tag - the new tag

    Level: developer

    Concepts: tag^getting
    Concepts: message tag^getting
    Concepts: MPI message tag^getting

.seealso: PetscObjectGetNewTag(), PetscCommDuplicate()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscCommGetNewTag(MPI_Comm comm,PetscMPIInt *tag)
{
  PetscErrorCode ierr;
  PetscMPIInt    *tagvalp=0,*maxval;
  PetscTruth     flg;

  PetscFunctionBegin;
  PetscValidIntPointer(tag,2);

  if (Petsc_Tag_keyval == MPI_KEYVAL_INVALID) {
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelTag,&Petsc_Tag_keyval,(void*)0);CHKERRQ(ierr);
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelComm,&Petsc_InnerComm_keyval,(void*)0);CHKERRQ(ierr);
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelComm,&Petsc_OuterComm_keyval,(void*)0);CHKERRQ(ierr);
  }

  ierr = MPI_Attr_get(comm,Petsc_Tag_keyval,(void**)&tagvalp,(PetscMPIInt*)&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ(PETSC_ERR_ARG_CORRUPT,"Bad MPI communicator supplied; must be a PETSc communicator");

  if (tagvalp[0] < 1) {
    ierr = PetscInfo1(0,"Out of tags for object, starting to recycle. Comm reference count %d\n",tagvalp[1]);CHKERRQ(ierr);
    ierr = MPI_Attr_get(MPI_COMM_WORLD,MPI_TAG_UB,(void**)&maxval,(PetscMPIInt*)&flg);CHKERRQ(ierr);
    if (!flg) {
      SETERRQ(PETSC_ERR_LIB,"MPI error: MPI_Attr_get() is not returning a MPI_TAG_UB");
    }
    tagvalp[0] = *maxval - 128; /* hope that any still active tags were issued right at the beginning of the run */
  }

  *tag = tagvalp[0]--;
#if defined(PETSC_USE_DEBUG)
  /* 
     Hanging here means that some processes have called PetscCommGetNewTag() and others have not.
  */
  ierr = MPI_Barrier(comm);CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscCommDuplicate" 
/*@C
  PetscCommDuplicate - Duplicates the communicator only if it is not already a PETSc communicator.

  Collective on MPI_Comm

  Input Parameters:
. comm_in - Input communicator

  Output Parameters:
+ comm_out - Output communicator.  May be comm_in.
- first_tag - Tag available that has not already been used with this communicator (you may
              pass in PETSC_NULL if you do not need a tag)

  PETSc communicators are just regular MPI communicators that keep track of which 
  tags have been used to prevent tag conflict. If you pass a non-PETSc communicator into
  a PETSc creation routine it will attach a private communicator for use in the objects communications.
  The internal MPI_Comm is used to perform all the MPI calls for PETSc, the outter MPI_Comm is a user
  level MPI_Comm that may be performing communication for the user or other library and so IS NOT used by PETSc.

  Level: developer

  Concepts: communicator^duplicate

.seealso: PetscObjectGetNewTag(), PetscCommGetNewTag()
@*/
PetscErrorCode PETSC_DLLEXPORT PetscCommDuplicate(MPI_Comm comm_in,MPI_Comm *comm_out,PetscMPIInt* first_tag)
{
  PetscErrorCode ierr;
  PetscMPIInt    *tagvalp,*maxval;
  PetscTruth     flg;

  PetscFunctionBegin;
  if (Petsc_Tag_keyval == MPI_KEYVAL_INVALID) {
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelTag,&Petsc_Tag_keyval,(void*)0);CHKERRQ(ierr);
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelComm,&Petsc_InnerComm_keyval,(void*)0);CHKERRQ(ierr);
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelComm,&Petsc_OuterComm_keyval,(void*)0);CHKERRQ(ierr);
  }
  ierr = MPI_Attr_get(comm_in,Petsc_Tag_keyval,(void**)&tagvalp,(PetscMPIInt*)&flg);CHKERRQ(ierr);

  if (!flg) {  /* this is NOT a PETSc comm */
    void *ptr;
    /* check if this communicator has a PETSc communicator imbedded in it */
    ierr = MPI_Attr_get(comm_in,Petsc_InnerComm_keyval,&ptr,(PetscMPIInt*)&flg);CHKERRQ(ierr);
    if (!flg) {
      /* This communicator is not yet known to this system, so we duplicate it and make an internal communicator */
      ierr       = MPI_Comm_dup(comm_in,comm_out);CHKERRQ(ierr);
      ierr       = MPI_Attr_get(MPI_COMM_WORLD,MPI_TAG_UB,(void**)&maxval,(PetscMPIInt*)&flg);CHKERRQ(ierr);
      if (!flg) {
        SETERRQ(PETSC_ERR_LIB,"MPI error: MPI_Attr_get() is not returning a MPI_TAG_UB");
      }
      ierr = PetscMalloc(2*sizeof(PetscMPIInt),&tagvalp);CHKERRQ(ierr);
      tagvalp[0] = *maxval;
      tagvalp[1] = 0;
      ierr       = MPI_Attr_put(*comm_out,Petsc_Tag_keyval,tagvalp);CHKERRQ(ierr);
      ierr = PetscInfo3(0,"Duplicating a communicator %ld %ld max tags = %d\n",(long)comm_in,(long)*comm_out,*maxval);CHKERRQ(ierr);

      /* save PETSc communicator inside user communicator, so we can get it next time */
      /*  Use PetscMemcpy() because casting from pointer to integer of different size is not allowed with some compilers  */
      ierr = PetscMemcpy(&ptr,comm_out,sizeof(MPI_Comm));CHKERRQ(ierr);
      ierr = MPI_Attr_put(comm_in,Petsc_InnerComm_keyval,ptr);CHKERRQ(ierr);
      /*  Use PetscMemcpy() because casting from pointer to integer of different size is not allowed with some compilers  */
      ierr = PetscMemcpy(&ptr,&comm_in,sizeof(MPI_Comm));CHKERRQ(ierr);
      ierr = MPI_Attr_put(*comm_out,Petsc_OuterComm_keyval,ptr);CHKERRQ(ierr);
    } else {
      /* pull out the inner MPI_Comm and hand it back to the caller */
      /*  Use PetscMemcpy() because casting from pointer to integer of different size is not allowed with some compilers  */
      ierr = PetscMemcpy(comm_out,&ptr,sizeof(MPI_Comm));CHKERRQ(ierr);
      ierr = MPI_Attr_get(*comm_out,Petsc_Tag_keyval,(void**)&tagvalp,(PetscMPIInt*)&flg);CHKERRQ(ierr);
      if (!flg) {
        SETERRQ(PETSC_ERR_PLIB,"Inner PETSc communicator does not have its tagvalp attribute set");
      }
      ierr = PetscInfo2(0,"Using internal PETSc communicator %ld %ld\n",(long)comm_in,(long)*comm_out);CHKERRQ(ierr);
    }
  } else {
    *comm_out = comm_in;
  }

#if defined(PETSC_USE_DEBUG)
  /* 
     Hanging here means that some processes have called PetscCommDuplicate() and others have not.
     This likley means that a subset of processes in a MPI_Comm have attempted to create a PetscObject!
     ALL processes that share a communicator MUST shared objects created from that communicator.
  */
  ierr = MPI_Barrier(comm_in);CHKERRQ(ierr);
#endif

  if (tagvalp[0] < 1) {
    ierr = PetscInfo1(0,"Out of tags for object, starting to recycle. Comm reference count %d\n",tagvalp[1]);CHKERRQ(ierr);
    ierr = MPI_Attr_get(MPI_COMM_WORLD,MPI_TAG_UB,(void**)&maxval,(PetscMPIInt*)&flg);CHKERRQ(ierr);
    if (!flg) {
      SETERRQ(PETSC_ERR_LIB,"MPI error: MPI_Attr_get() is not returning a MPI_TAG_UB");
    }
    tagvalp[0] = *maxval - 128; /* hope that any still active tags were issued right at the beginning of the run */
  }

  if (first_tag) {
    *first_tag = tagvalp[0]--;
    ierr = PetscInfo1(0,"  returning tag %ld\n",(long)*first_tag);CHKERRQ(ierr);
  }
  tagvalp[1]++; /* number of references to this comm */
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscCommDestroy" 
/*@C
   PetscCommDestroy - Frees communicator.  Use in conjunction with PetscCommDuplicate().

   Collective on MPI_Comm

   Input Parameter:
.  comm - the communicator to free

   Level: developer

   Concepts: communicator^destroy

@*/
PetscErrorCode PETSC_DLLEXPORT PetscCommDestroy(MPI_Comm *comm)
{
  PetscErrorCode ierr;
  PetscMPIInt    *tagvalp;
  PetscTruth     flg;
  MPI_Comm       icomm = *comm,ocomm;
  void           *ptr;

  PetscFunctionBegin;
  if (Petsc_Tag_keyval == MPI_KEYVAL_INVALID) {
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelTag,&Petsc_Tag_keyval,(void*)0);CHKERRQ(ierr);
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelComm,&Petsc_InnerComm_keyval,(void*)0);CHKERRQ(ierr);
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,Petsc_DelComm,&Petsc_OuterComm_keyval,(void*)0);CHKERRQ(ierr);
  }
  ierr = MPI_Attr_get(icomm,Petsc_Tag_keyval,(void**)&tagvalp,(PetscMPIInt*)&flg);CHKERRQ(ierr);
  if (!flg) { /* not a PETSc comm, check if it has an inner comm */
    ierr  = MPI_Attr_get(icomm,Petsc_InnerComm_keyval,&ptr,(PetscMPIInt*)&flg);CHKERRQ(ierr);
    if (!flg) {
      SETERRQ(PETSC_ERR_ARG_CORRUPT,"MPI_Comm does not have tagvalues nor does it have inner MPI_Comm");
    }
    /*  Use PetscMemcpy() because casting from pointer to integer of different size is not allowed with some compilers  */
    ierr = PetscMemcpy(&icomm,&ptr,sizeof(MPI_Comm));CHKERRQ(ierr);
    ierr = MPI_Attr_get(icomm,Petsc_Tag_keyval,(void**)&tagvalp,(PetscMPIInt*)&flg);CHKERRQ(ierr);
    if (!flg) {
      SETERRQ(PETSC_ERR_ARG_CORRUPT,"Inner MPI_Comm does not have expected tagvalues, problem with corrupted memory");
    }
  }
  tagvalp[1]--;
  if (!tagvalp[1]) {

    /* if MPI_Comm has outter comm then remove reference to inner MPI_Comm from outter MPI_Comm */
    ierr  = MPI_Attr_get(icomm,Petsc_OuterComm_keyval,&ptr,(PetscMPIInt*)&flg);CHKERRQ(ierr);
    /*  Use PetscMemcpy() because casting from pointer to integer of different size is not allowed with some compilers  */
    ierr = PetscMemcpy(&ocomm,&ptr,sizeof(MPI_Comm));CHKERRQ(ierr);
    if (flg) {
      ierr = MPI_Attr_delete(ocomm,Petsc_InnerComm_keyval);CHKERRQ(ierr);
    }

    ierr = PetscInfo1(0,"Deleting PETSc MPI_Comm %ld\n",(long)icomm);CHKERRQ(ierr);
    ierr = MPI_Comm_free(&icomm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

