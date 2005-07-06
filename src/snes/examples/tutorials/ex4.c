/* Program usage:  mpirun -np <procs> ex5 [-help] [all PETSc options] */

static char help[] = "Nonlinear PDE in 2d.\n\
We solve the Lane-Emden equation in a 2D rectangular\n\
domain, using distributed arrays (DAs) to partition the parallel grid.\n\n";

/*T
   Concepts: SNES^parallel Lane-Emden example
   Concepts: DA^using distributed arrays;
   Processors: n
T*/

/* ------------------------------------------------------------------------

    The Lane-Emden equation is given by the partial differential equation
  
            -alpha*Laplacian u - lambda*u^3 = 0,  0 < x,y < 1,
  
    with boundary conditions
   
             u = 0  for  x = 0, x = 1, y = 0, y = 1.
  
    A bilinear finite element approximation is used to discretize the boundary
    value problem to obtain a nonlinear system of equations.

  ------------------------------------------------------------------------- */

/* 
   Include "petscda.h" so that we can use distributed arrays (DAs).
   Include "petscsnes.h" so that we can use SNES solvers.  Note that this
   file automatically includes:
     petsc.h       - base PETSc routines   petscvec.h - vectors
     petscsys.h    - system routines       petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners
     petscksp.h   - linear solvers
*/
#include "petscda.h"
#include "petscdmmg.h"
#include "petscsnes.h"

/* 
   User-defined application context - contains data needed by the 
   application-provided call-back routines, FormJacobianLocal() and
   FormFunctionLocal().
*/
typedef struct {
   DA        da;             /* distributed array data structure */
   PetscReal alpha;          /* parameter controlling linearity */
   PetscReal lambda;         /* parameter controlling nonlinearity */
} AppCtx;

static PetscScalar Kref[16] = { 0.666667, -0.166667, -0.333333, -0.166667,
                               -0.166667,  0.666667, -0.166667, -0.333333,
                               -0.333333, -0.166667,  0.666667, -0.166667,
                               -0.166667, -0.333333, -0.166667,  0.666667};
/* 
   User-defined routines
*/
extern PetscErrorCode FormInitialGuess(AppCtx*,Vec);
extern PetscErrorCode FormFunctionLocal(DALocalInfo*,PetscScalar**,PetscScalar**,AppCtx*);
extern PetscErrorCode FormJacobianLocal(DALocalInfo*,PetscScalar**,Mat,AppCtx*);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  SNES                   snes;                 /* nonlinear solver */
  Vec                    x,r;                  /* solution, residual vectors */
  Mat                    J;                    /* Jacobian matrix */
  AppCtx                 user;                 /* user-defined work context */
  PetscInt               its;                  /* iterations for convergence */
  SNESConvergedReason    reason;
  PetscErrorCode         ierr;
  PetscReal              lambda_max = 6.81, lambda_min = 0.0;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize program
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  PetscInitialize(&argc,&argv,(char *)0,help);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize problem parameters
  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  user.alpha = 1.0;
  user.lambda = 6.0;
  ierr = PetscOptionsGetReal(PETSC_NULL,"-alpha",&user.alpha,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(PETSC_NULL,"-lambda",&user.lambda,PETSC_NULL);CHKERRQ(ierr);
  if (user.lambda > lambda_max || user.lambda < lambda_min) {
    SETERRQ3(1,"Lambda %g is out of range [%g, %g]", user.lambda, lambda_min, lambda_max);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create nonlinear solver context
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SNESCreate(PETSC_COMM_WORLD,&snes);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create distributed array (DA) to manage parallel grid and vectors
  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DACreate2d(PETSC_COMM_WORLD,DA_NONPERIODIC,DA_STENCIL_BOX,-4,-4,PETSC_DECIDE,PETSC_DECIDE,
                    1,1,PETSC_NULL,PETSC_NULL,&user.da);CHKERRQ(ierr);

  /*  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Extract global vectors from DA; then duplicate for remaining
     vectors that are the same types
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DACreateGlobalVector(user.da,&x);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create matrix data structure; set Jacobian evaluation routine
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = VecDuplicate(x,&r);CHKERRQ(ierr);
  ierr = SNESSetFunction(snes,r,SNESDAFormFunction,&user);CHKERRQ(ierr);
  ierr = DAGetMatrix(user.da,MATAIJ,&J);CHKERRQ(ierr);
  ierr = SNESSetJacobian(snes,J,J,SNESDAComputeJacobian,&user);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set local function evaluation routine
  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DASetLocalFunction(user.da,(DALocalFunction1)FormFunctionLocal);CHKERRQ(ierr);
  ierr = DASetLocalJacobian(user.da,(DALocalFunction1)FormJacobianLocal);CHKERRQ(ierr); 

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Customize nonlinear solver; set runtime options
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Evaluate initial guess
     Note: The user should initialize the vector, x, with the initial guess
     for the nonlinear solver prior to calling SNESSolve().  In particular,
     to employ an initial guess of zero, the user should explicitly set
     this vector to zero by calling VecSet().
  */
  ierr = FormInitialGuess(&user,x);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Solve nonlinear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SNESSolve(snes,PETSC_NULL,x);CHKERRQ(ierr); 
  ierr = SNESGetIterationNumber(snes,&its);CHKERRQ(ierr);
  ierr = SNESGetConvergedReason(snes, &reason);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Number of Newton iterations = %D, %s\n",its,SNESConvergedReasons[reason]);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatDestroy(J);CHKERRQ(ierr);
  ierr = VecDestroy(x);CHKERRQ(ierr);
  ierr = VecDestroy(r);CHKERRQ(ierr);      
  ierr = SNESDestroy(snes);CHKERRQ(ierr);
  ierr = DADestroy(user.da);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "FormInitialGuess"
/* 
   FormInitialGuess - Forms initial approximation.

   Input Parameters:
   user - user-defined application context
   X - vector

   Output Parameter:
   X - vector
 */
PetscErrorCode FormInitialGuess(AppCtx *user,Vec X)
{
  PetscInt       i,j,Mx,My,xs,ys,xm,ym;
  PetscErrorCode ierr;
  PetscReal      lambda,temp1,temp,hx,hy;
  PetscScalar    **x;

  PetscFunctionBegin;
  ierr = DAGetInfo(user->da,PETSC_IGNORE,&Mx,&My,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,
                   PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE);

  lambda = user->lambda;
  hx     = 1.0/(PetscReal)(Mx-1);
  hy     = 1.0/(PetscReal)(My-1);
  if (lambda == 0.0) {
    temp1  = 1.0;
  } else {
    temp1  = lambda/(lambda + 1.0);
  }

  /*
     Get a pointer to vector data.
       - For default PETSc vectors, VecGetArray() returns a pointer to
         the data array.  Otherwise, the routine is implementation dependent.
       - You MUST call VecRestoreArray() when you no longer need access to
         the array.
  */
  ierr = DAVecGetArray(user->da,X,&x);CHKERRQ(ierr);

  /*
     Get local grid boundaries (for 2-dimensional DA):
       xs, ys   - starting grid indices (no ghost points)
       xm, ym   - widths of local grid (no ghost points)

  */
  ierr = DAGetCorners(user->da,&xs,&ys,PETSC_NULL,&xm,&ym,PETSC_NULL);CHKERRQ(ierr);

  /*
     Compute initial guess over the locally owned part of the grid
  */
  for (j=ys; j<ys+ym; j++) {
    temp = (PetscReal)(PetscMin(j,My-j-1))*hy;
    for (i=xs; i<xs+xm; i++) {

      if (i == 0 || j == 0 || i == Mx-1 || j == My-1) {
        /* boundary conditions are all zero Dirichlet */
        x[j][i] = 0.0; 
      } else {
        x[j][i] = temp1*sqrt(PetscMin((PetscReal)(PetscMin(i,Mx-i-1))*hx,temp)); 
      }
    }
  }

  /*
     Restore vector
  */
  ierr = DAVecRestoreArray(user->da,X,&x);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

PetscErrorCode nonlinearResidual(PetscReal lambda, PetscScalar u[], PetscScalar r[]) {
  PetscFunctionBegin;
  r[0] += lambda*(48.0*u[0]*u[0]*u[0] + 12.0*u[1]*u[1]*u[1] + 9.0*u[0]*u[0]*(4.0*u[1] + u[2] + 4.0*u[3]) + u[1]*u[1]*(9.0*u[2] + 6.0*u[3]) + u[1]*(6.0*u[2]*u[2] + 8.0*u[2]*u[3] + 6.0*u[3]*u[3])
           + 3.0*(u[2]*u[2]*u[2] + 2.0*u[2]*u[2]*u[3] + 3.0*u[2]*u[3]*u[3] + 4.0*u[3]*u[3]*u[3])
           + 2.0*u[0]*(12.0*u[1]*u[1] + u[1]*(6.0*u[2] + 9.0*u[3]) + 2.0*(u[2]*u[2] + 3.0*u[2]*u[3] + 6.0*u[3]*u[3])))/1200.0;
  r[1] += lambda*(12.0*u[0]*u[0]*u[0] + 48.0*u[1]*u[1]*u[1] + 9.0*u[1]*u[1]*(4.0*u[2] + u[3]) + 3.0*u[0]*u[0]*(8.0*u[1] + 2.0*u[2] + 3.0*u[3])
           + 4.0*u[1]*(6.0*u[2]*u[2] + 3.0*u[2]*u[3] + u[3]*u[3]) + 3.0*(4.0*u[2]*u[2]*u[2] + 3.0*u[2]*u[2]*u[3] + 2.0*u[2]*u[3]*u[3] + u[3]*u[3]*u[3])
           + 2.0*u[0]*((18.0*u[1]*u[1] + 3.0*u[2]*u[2] + 4.0*u[2]*u[3] + 3.0*u[3]*u[3]) + u[1]*(9.0*u[2] + 6.0*u[3])))/1200.0;
  r[2] += lambda*(3.0*u[0]*u[0]*u[0] + u[0]*u[0]*(6.0*u[1] + 4.0*u[2] + 6.0*u[3]) + u[0]*(9.0*u[1]*u[1] + 9.0*u[2]*u[2] + 12.0*u[2]*u[3] + 9.0*u[3]*u[3] + 4.0*u[1]*(3.0*u[2] + 2.0*u[3]))
           + 6.0*(2.0*u[1]*u[1]*u[1] + u[1]*u[1]*(4.0*u[2] + u[3]) + u[1]*(6.0*u[2]*u[2] + 3.0*u[2]*u[3] + u[3]*u[3]) + 2.0*(4.0*u[2]*u[2]*u[2] + 3.0*u[2]*u[2]*u[3] + 2.0*u[2]*u[3]*u[3] + u[3]*u[3]*u[3])))/1200.0;
  r[3] += lambda*(12.0*u[0]*u[0]*u[0] + 3.0*u[1]*u[1]*u[1] + u[1]*u[1]*(6.0*u[2] + 4.0*u[3]) + 3.0*u[0]*u[0]*(3.0*u[1] + 2.0*u[2] + 8.0*u[3])
           + 3.0*u[1]*(3.0*u[2]*u[2] + 4.0*u[2]*u[3] + 3.0*u[3]*u[3]) + 12.0*(u[2]*u[2]*u[2] + 2.0*u[2]*u[2]*u[3] + 3.0*u[2]*u[3]*u[3] + 4.0*u[3]*u[3]*u[3])
           + 2.0*u[0]*(3.0*u[1]*u[1] + u[1]*(4.0*u[2] + 6.0*u[3]) + 3.0*(u[2]*u[2] + 3.0*u[2]*u[3] + 6.0*u[3]*u[3])))/1200.0;
  PetscFunctionReturn(0);
}

PetscErrorCode nonlinearJacobian(PetscReal lambda, PetscScalar u[], PetscScalar J[]) {
  PetscFunctionBegin;
  J[0]  = lambda*(72.0*u[0]*u[0] + 12.0*u[1]*u[1] + 9.0*u[0]*(4.0*u[1] + u[2] + 4.0*u[3]) + u[1]*(6.0*u[2] + 9.0*u[3]) + 2.0*(u[2]*u[2] + 3.0*u[2]*u[3] + 6.0*u[3]*u[3]))/600.0;
  J[1]  = lambda*(18.0*u[0]*u[0] + 18.0*u[1]*u[1] + 3.0*u[2]*u[2] + 4.0*u[2]*u[3] + 3.0*u[3]*u[3] + 3.0*u[0]*(8.0*u[1] + 2.0*u[2] + 3.0*u[3]) + u[1]*(9.0*u[2] + 6.0*u[3]))/600.0;
  J[2]  = lambda*( 9.0*u[0]*u[0] +  9.0*u[1]*u[1] + 9.0*u[2]*u[2] + 12.0*u[2]*u[3] + 9.0*u[3]*u[3] + 4.0*u[1]*(3.0*u[2] + 2.0*u[3]) + 4.0*u[0]*(3.0*u[1] + 2.0*u[2] + 3.0*u[3]))/1200.0;
  J[3]  = lambda*(18.0*u[0]*u[0] +  3.0*u[1]*u[1] + u[1]*(4.0*u[2] + 6.0*u[3]) + 3.0*u[0]*(3.0*u[1] + 2.0*u[2] + 8.0*u[3]) + 3.0*(u[2]*u[2] + 3.0*u[2]*u[3] + 6.0*u[3]*u[3]))/600.0;

  J[4]  = lambda*(18.0*u[0]*u[0] + 18.0*u[1]*u[1] + 3.0*u[2]*u[2] + 4.0*u[2]*u[3] + 3.0*u[3]*u[3] + 3.0*u[0]*(8.0*u[1] + 2.0*u[2] + 3.0*u[3]) + u[1]*(9.0*u[2] + 6.0*u[3]))/600.0;
  J[5]  = lambda*(12.0*u[0]*u[0] + 72.0*u[1]*u[1] + 9.0*u[1]*(4.0*u[2] + u[3]) + u[0]*(36.0*u[1] + 9.0*u[2] + 6.0*u[3]) + 2.0*(6.0*u[2]*u[2] + 3.0*u[2]*u[3] + u[3]*u[3]))/600.0;
  J[6]  = lambda*( 3.0*u[0]*u[0] + u[0]*(9.0*u[1] + 6.0*u[2] + 4.0*u[3]) + 3.0*(6.0*u[1]*u[1] + 6.0*u[2]*u[2] + 3.0*u[2]*u[3] + u[3]*u[3] + 2.0*u[1]*(4.0*u[2] + u[3])))/600.0;
  J[7]  = lambda*( 9.0*u[0]*u[0] +  9.0*u[1]*u[1] + 9.0*u[2]*u[2] + 12.0*u[2]*u[3] + 9.0*u[3]*u[3] + 4.0*u[1]*(3.0*u[2] + 2.0*u[3]) + 4.0*u[0]*(3.0*u[1] + 2.0*u[2] + 3.0*u[3]))/1200.0;

  J[8]  = lambda*( 9.0*u[0]*u[0] +  9.0*u[1]*u[1] + 9.0*u[2]*u[2] + 12.0*u[2]*u[3] + 9.0*u[3]*u[3] + 4.0*u[1]*(3.0*u[2] + 2.0*u[3]) + 4.0*u[0]*(3.0*u[1] + 2.0*u[2] + 3.0*u[3]))/1200.0;
  J[9]  = lambda*( 3.0*u[0]*u[0] + u[0]*(9.0*u[1] + 6.0*u[2] + 4.0*u[3]) + 3.0*(6.0*u[1]*u[1] + 6.0*u[2]*u[2] + 3.0*u[2]*u[3] + u[3]*u[3] + 2.0*u[1]*(4.0*u[2] + u[3])))/600.0;
  J[10] = lambda*( 2.0*u[0]*u[0] + u[0]*(6.0*u[1] + 9.0*u[2] + 6.0*u[3]) + 3.0*(4.0*u[1]*u[1] + 3.0*u[1]*(4.0*u[2] + u[3]) + 4.0*(6.0*u[2]*u[2] + 3.0*u[2]*u[3] + u[3]*u[3])))/600.0;
  J[11] = lambda*( 3.0*u[0]*u[0] + u[0]*(4.0*u[1] + 6.0*u[2] + 9.0*u[3]) + 3.0*(u[1]*u[1] + 6.0*u[2]*u[2] + 8.0*u[2]*u[3] + 6.0*u[3]*u[3] + u[1]*(3.0*u[2] + 2.0*u[3])))/600.0;

  J[12] = lambda*(18.0*u[0]*u[0] +  3.0*u[1]*u[1] + u[1]*(4.0*u[2] + 6.0*u[3]) + 3.0*u[0]*(3.0*u[1] + 2.0*u[2] + 8.0*u[3]) + 3.0*(u[2]*u[2] + 3.0*u[2]*u[3] + 6.0*u[3]*u[3]))/600.0;
  J[13] = lambda*( 9.0*u[0]*u[0] +  9.0*u[1]*u[1] + 9.0*u[2]*u[2] + 12.0*u[2]*u[3] + 9.0*u[3]*u[3] + 4.0*u[1]*(3.0*u[2] + 2.0*u[3]) + 4.0*u[0]*(3.0*u[1] + 2.0*u[2] + 3.0*u[3]))/1200.0;
  J[14] = lambda*( 3.0*u[0]*u[0] + u[0]*(4.0*u[1] + 6.0*u[2] + 9.0*u[3]) + 3.0*(u[1]*u[1] + 6.0*u[2]*u[2] + 8.0*u[2]*u[3] + 6.0*u[3]*u[3] + u[1]*(3.0*u[2] + 2.0*u[3])))/600.0;
  J[15] = lambda*(12.0*u[0]*u[0] +  2.0*u[1]*u[1] + u[1]*(6.0*u[2] + 9.0*u[3]) + 12.0*(u[2]*u[2] + 3.0*u[2]*u[3] + 6.0*u[3]*u[3]) + u[0]*(6.0*u[1] + 9.0*(u[2] + 4.0*u[3])))/600.0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "FormFunctionLocal"
/* 
   FormFunctionLocal - Evaluates nonlinear function, F(x).

       Process adiC(36): FormFunctionLocal

 */
PetscErrorCode FormFunctionLocal(DALocalInfo *info,PetscScalar **x,PetscScalar **f,AppCtx *user)
{
  PetscScalar    uLocal[4];
  PetscScalar    rLocal[4];
  PetscReal      alpha,lambda,hx,hy,hxhy,sc;
  PetscInt       i,j,k,l;
  PetscErrorCode ierr;

  PetscFunctionBegin;

  alpha  = user->alpha;
  lambda = user->lambda;
  hx     = 1.0/(PetscReal)(info->mx-1);
  hy     = 1.0/(PetscReal)(info->my-1);
  sc     = hx*hy*lambda;
  hxhy   = hx*hy; 

  /* Compute function over the locally owned part of the grid. For each
     vertex (i,j), we consider the element below:

       3         2
     i,j+1 --- i+1,j
       |         |
       |         |
      i,j  --- i+1,j
       0         1

     and therefore we do not loop over the last vertex in each dimension.
  */
  for(j = info->ys; j < info->ys+info->ym-1; j++) {
    for(i = info->xs; i < info->xs+info->xm-1; i++) {
      uLocal[0] = x[j][i];
      uLocal[1] = x[j][i+1];
      uLocal[2] = x[j+1][i+1];
      uLocal[3] = x[j+1][i];
      for(k = 0; k < 4; k++) {
        rLocal[k] = 0.0;
        for(l = 0; l < 4; l++) {
          rLocal[k] += Kref[k*4 + l]*uLocal[l];
        }
        rLocal[k] *= hxhy*alpha;
      }
      ierr = nonlinearResidual(-1.0*sc, uLocal, rLocal);CHKERRQ(ierr);
      f[j][i]     += rLocal[0];
      f[j][i+1]   += rLocal[1];
      f[j+1][i+1] += rLocal[2];
      f[j+1][i]   += rLocal[3];
      if (i == 0 || j == 0) {
        f[j][i] = x[j][i];
      }
      if ((i == info->mx-2) || (j == 0)) {
        f[j][i+1] = x[j][i+1];
      }
      if ((i == info->mx-2) || (j == info->my-2)) {
        f[j+1][i+1] = x[j+1][i+1];
      }
      if ((i == 0) || (j == info->my-2)) {
        f[j+1][i] = x[j+1][i];
      }
    }
  }

  ierr = PetscLogFlops(68*(info->ym-1)*(info->xm-1));CHKERRQ(ierr);
  PetscFunctionReturn(0); 
} 

#undef __FUNCT__
#define __FUNCT__ "FormJacobianLocal"
/*
   FormJacobianLocal - Evaluates Jacobian matrix.
*/
PetscErrorCode FormJacobianLocal(DALocalInfo *info,PetscScalar **x,Mat jac,AppCtx *user)
{
  PetscScalar    JLocal[16], ELocal[16], uLocal[4];
  MatStencil     rows[4], cols[4], ident;
  PetscInt       localRows[4];
  PetscScalar    alpha,lambda,hx,hy,hxhy,sc;
  PetscInt       i,j,k,l,numRows;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  alpha  = user->alpha;
  lambda = user->lambda;
  hx     = 1.0/(PetscReal)(info->mx-1);
  hy     = 1.0/(PetscReal)(info->my-1);
  sc     = hx*hy*lambda;
  hxhy   = hx*hy; 


  /* 
     Compute entries for the locally owned part of the Jacobian.
      - Currently, all PETSc parallel matrix formats are partitioned by
        contiguous chunks of rows across the processors. 
      - Each processor needs to insert only elements that it owns
        locally (but any non-local elements will be sent to the
        appropriate processor during matrix assembly). 
      - Here, we set all entries for a particular row at once.
      - We can set matrix entries either using either
        MatSetValuesLocal() or MatSetValues(), as discussed above.
  */
  for (j=info->ys; j<info->ys+info->ym-1; j++) {
    for (i=info->xs; i<info->xs+info->xm-1; i++) {
      numRows = 0;
      uLocal[0] = x[j][i];
      uLocal[1] = x[j][i+1];
      uLocal[2] = x[j+1][i+1];
      uLocal[3] = x[j+1][i];
      /* i,j */
      if (i == 0 || j == 0) {
        ident.i = i; ident.j = j;
        JLocal[0] = 1.0;
        ierr = MatAssemblyBegin(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatSetValuesStencil(jac,1,&ident,1,&ident,JLocal,INSERT_VALUES);CHKERRQ(ierr);
        ierr = MatAssemblyBegin(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
      } else {
        localRows[numRows] = 0;
        rows[numRows].i = i; rows[numRows].j = j;
        numRows++;
      }
      cols[0].i = i; cols[0].j = j;
      /* i+1,j */
      if ((i == info->mx-2) || (j == 0)) {
        ident.i = i+1; ident.j = j;
        JLocal[0] = 1.0;
        ierr = MatAssemblyBegin(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatSetValuesStencil(jac,1,&ident,1,&ident,JLocal,INSERT_VALUES);CHKERRQ(ierr);
        ierr = MatAssemblyBegin(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
      } else {
        localRows[numRows] = 1;
        rows[numRows].i = i+1; rows[numRows].j = j;
        numRows++;
      }
      cols[1].i = i+1; cols[1].j = j;
      /* i+1,j+1 */
      if ((i == info->mx-2) || (j == info->my-2)) {
        ident.i = i+1; ident.j = j+1;
        JLocal[0] = 1.0;
        ierr = MatAssemblyBegin(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatSetValuesStencil(jac,1,&ident,1,&ident,JLocal,INSERT_VALUES);CHKERRQ(ierr);
        ierr = MatAssemblyBegin(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
      } else {
        localRows[numRows] = 2;
        rows[numRows].i = i+1; rows[numRows].j = j+1;
        numRows++;
      }
      cols[2].i = i+1; cols[2].j = j+1;
      /* i,j+1 */
      if ((i == 0) || (j == info->my-2)) {
        ident.i = i; ident.j = j+1;
        JLocal[0] = 1.0;
        ierr = MatAssemblyBegin(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatSetValuesStencil(jac,1,&ident,1,&ident,JLocal,INSERT_VALUES);CHKERRQ(ierr);
        ierr = MatAssemblyBegin(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
        ierr = MatAssemblyEnd(jac,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
      } else {
        localRows[numRows] = 3;
        rows[numRows].i = i; rows[numRows].j = j+1;
        numRows++;
      }
      cols[3].i = i; cols[3].j = j+1;
      ierr = nonlinearJacobian(-1.0*sc, uLocal, ELocal);CHKERRQ(ierr);
      for(k = 0; k < numRows; k++) {
        for(l = 0; l < 4; l++) {
          JLocal[k*4 + l] = (hxhy*alpha*Kref[localRows[k]*4 + l] + ELocal[localRows[k]*4 + l]);
        }
      }
      ierr = MatSetValuesStencil(jac,numRows,rows,4,cols,JLocal,ADD_VALUES);CHKERRQ(ierr);
    }
  }

  /* 
     Assemble matrix, using the 2-step process:
       MatAssemblyBegin(), MatAssemblyEnd().
  */
  ierr = MatAssemblyBegin(jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  /*
     Tell the matrix we will never add a new nonzero location to the
     matrix. If we do, it will generate an error.
  */
  ierr = MatSetOption(jac,MAT_NEW_NONZERO_LOCATION_ERR);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
