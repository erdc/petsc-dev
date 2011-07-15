/* Program usage:  mpiexec ex1 [-help] [all PETSc options] */

static char help[] = "Basic vector routines.\n\n";

/*T
   Concepts: vectors^basic routines;
   Processors: n
T*/

/* 
  Include "petscvec.h" so that we can use vectors.  Note that this file
  automatically includes:
     petscsys.h       - base PETSc routines   petscis.h     - index sets
     petscviewer.h - viewers
*/

#define _GNU_SOURCE
#include <sched.h>
#include <petscvec.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Vec            x,y,w;               /* vectors */
  Vec            *z;                    /* array of vectors */
  PetscReal      norm,v,v1,v2,maxval;
  PetscInt       n = 20,maxind;
  PetscErrorCode ierr;
  PetscScalar    one = 1.0,two = 2.0,three = 3.0,dots[3],dot;
  int icorr = 0; cpu_set_t mset;

  icorr = get_nprocs();
  printf("Number of On-line Cores = %d\n",icorr);

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr); 
  ierr = PetscOptionsGetInt(PETSC_NULL,"-main",&icorr,PETSC_NULL);CHKERRQ(ierr);
  CPU_ZERO(&mset);
  CPU_SET(icorr,&mset);
  sched_setaffinity(0,sizeof(cpu_set_t),&mset);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-n",&n,PETSC_NULL);CHKERRQ(ierr);

  /* 
     Create a vector, specifying only its global dimension.
     When using VecCreate(), VecSetSizes() and VecSetFromOptions(), the vector format 
     (currently parallel, shared, or sequential) is determined at runtime.  Also, the 
     parallel partitioning of the vector is determined by PETSc at runtime.

     Routines for creating particular vector types directly are:
        VecCreateSeq() - uniprocessor vector
        VecCreateMPI() - distributed vector, where the user can
                         determine the parallel partitioning
        VecCreateShared() - parallel vector that uses shared memory
                            (available only on the SGI); otherwise,
                            is the same as VecCreateMPI()

     With VecCreate(), VecSetSizes() and VecSetFromOptions() the option -vec_type mpi or 
     -vec_type shared causes the particular type of vector to be formed.
y

  */

  ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
  ierr = VecSetSizes(x,PETSC_DECIDE,n);CHKERRQ(ierr);
  ierr = VecSetFromOptions(x);CHKERRQ(ierr);
  /*
     Duplicate some work vectors (of the same format and
     partitioning as the initial vector).
  */
  ierr = VecDuplicate(x,&y);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&w);CHKERRQ(ierr);
  ierr = VecNorm(w,NORM_2,&norm);CHKERRQ(ierr);
  /*
     Duplicate more work vectors (of the same format and
     partitioning as the initial vector).  Here we duplicate
     an array of vectors, which is often more convenient than
     duplicating individual ones.
  */
  ierr = VecDuplicateVecs(x,3,&z);CHKERRQ(ierr); 
  /*
     Set the vectors to entries to a constant value.
  */
  ierr = VecSet(x,one);CHKERRQ(ierr);
  ierr = VecSet(y,two);CHKERRQ(ierr);
  ierr = VecSet(z[0],one);CHKERRQ(ierr);
  ierr = VecSet(z[1],two);CHKERRQ(ierr);
  ierr = VecSet(z[2],three);CHKERRQ(ierr);
  /*
     Demonstrate various basic vector routines.
  */
  ierr = VecDot(x,x,&dot);CHKERRQ(ierr);
  ierr = VecMDot(x,3,z,dots);CHKERRQ(ierr);

  /* 
     Note: If using a complex numbers version of PETSc, then
     PETSC_USE_COMPLEX is defined in the makefiles; otherwise,
     (when using real numbers) it is undefined.
  */
  //printf("Size Of Void* =  %ld, Size Of PetscErrorCode =  %ld\n",sizeof(void*),sizeof(PetscErrorCode));
  //printf("Size of PetscBool = %ld\n",sizeof(PetscBool));

#if defined(PETSC_USE_COMPLEX)
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Vector length %D\n",(PetscInt) (PetscRealPart(dot)));CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Vector length %D %D %D\n",(PetscInt)PetscRealPart(dots[0]),
                             (PetscInt)PetscRealPart(dots[1]),(PetscInt)PetscRealPart(dots[2]));CHKERRQ(ierr);
#else
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Vector length %D\n",(PetscInt)dot);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Vector length %D %D %D\n",(PetscInt)dots[0],
                             (PetscInt)dots[1],(PetscInt)dots[2]);CHKERRQ(ierr);
#endif
  //struct timeval t1, t2, result;
  //gettimeofday(&t1,NULL);
  ierr = VecMax(x,&maxind,&maxval);CHKERRQ(ierr);
  //gettimeofday(&t2,NULL);
  //timersub(&t2,&t1,&result);
  //printf("Time for VecMax = %lf\n",result.tv_sec+result.tv_usec/1000000.0);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecMax %G, VecInd %D\n",maxval,maxind);CHKERRQ(ierr);

  ierr = VecMin(x,&maxind,&maxval);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecMin %G, VecInd %D\n",maxval,maxind);CHKERRQ(ierr);
  /*
  const PetscInt ix_kds[] = {3,7,14};
  const PetscScalar y_kds[] = {13.2,69.3,-8.7};
  ierr = VecSetValues(x,3,ix_kds,y_kds,INSERT_VALUES);CHKERRQ(ierr);
  ierr = VecMax(x,&maxind,&maxval);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecMax %G, VecInd %D\n",maxval,maxind);CHKERRQ(ierr);
  ierr = VecMin(x,&maxind,&maxval);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecMax %G, VecInd %D\n",maxval,maxind);CHKERRQ(ierr);
   */
  ierr = PetscPrintf(PETSC_COMM_WORLD,"All other values should be near zero\n");CHKERRQ(ierr);


  ierr = VecScale(x,two);CHKERRQ(ierr);
  ierr = VecNorm(x,NORM_2,&norm);CHKERRQ(ierr);
  v = norm-2.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecScale %G\n",v);CHKERRQ(ierr);


  ierr = VecCopy(x,w);CHKERRQ(ierr);
  ierr = VecNorm(w,NORM_2,&norm);CHKERRQ(ierr);
  v = norm-2.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecCopy  %G\n",v);CHKERRQ(ierr);

  ierr = VecAXPY(y,three,x);CHKERRQ(ierr);
  ierr = VecNorm(y,NORM_2,&norm);CHKERRQ(ierr);
  v = norm-8.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecAXPY %G\n",v);CHKERRQ(ierr);

  ierr = VecAYPX(y,two,x);CHKERRQ(ierr);
  ierr = VecNorm(y,NORM_2,&norm);CHKERRQ(ierr);
  v = norm-18.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecAYPX %G\n",v);CHKERRQ(ierr);

  ierr = VecSwap(x,y);CHKERRQ(ierr);
  ierr = VecNorm(y,NORM_2,&norm);CHKERRQ(ierr);
  v = norm-2.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecSwap  %G\n",v);CHKERRQ(ierr);
  ierr = VecNorm(x,NORM_2,&norm);CHKERRQ(ierr);
  v = norm-18.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecSwap  %G\n",v);CHKERRQ(ierr);

  ierr = VecWAXPY(w,two,x,y);CHKERRQ(ierr);
  ierr = VecNorm(w,NORM_2,&norm);CHKERRQ(ierr);
  v = norm-38.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecWAXPY %G\n",v);CHKERRQ(ierr);

  ierr = VecPointwiseMult(w,y,x);CHKERRQ(ierr);
  ierr = VecNorm(w,NORM_2,&norm);CHKERRQ(ierr); 
  v = norm-36.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecPointwiseMult %G\n",v);CHKERRQ(ierr);

  ierr = VecPointwiseDivide(w,x,y);CHKERRQ(ierr);
  ierr = VecNorm(w,NORM_2,&norm);CHKERRQ(ierr);
  v = norm-9.0*sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecPointwiseDivide %G\n",v);CHKERRQ(ierr);

  dots[0] = one;
  dots[1] = three;
  dots[2] = two;
  ierr = VecSet(x,one);CHKERRQ(ierr);
  ierr = VecMAXPY(x,3,dots,z);CHKERRQ(ierr);
  ierr = VecNorm(z[0],NORM_2,&norm);CHKERRQ(ierr);
  v = norm-sqrt((double)n); if (v > -PETSC_SMALL && v < PETSC_SMALL) v = 0.0; 
  ierr = VecNorm(z[1],NORM_2,&norm);CHKERRQ(ierr);
  v1 = norm-2.0*sqrt((double)n); if (v1 > -PETSC_SMALL && v1 < PETSC_SMALL) v1 = 0.0; 
  ierr = VecNorm(z[2],NORM_2,&norm);CHKERRQ(ierr);
  v2 = norm-3.0*sqrt((double)n); if (v2 > -PETSC_SMALL && v2 < PETSC_SMALL) v2 = 0.0; 
  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecMAXPY %G %G %G \n",v,v1,v2);CHKERRQ(ierr);
  

  /* 
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
  */
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&y);CHKERRQ(ierr);
  ierr = VecDestroy(&w);CHKERRQ(ierr);
  ierr = VecDestroyVecs(3,&z);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
 
