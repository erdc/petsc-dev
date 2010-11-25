%%
% Solves a nonlinear variational inequality where the user manages the mesh--solver interactions
%
% This is a translation of snes/examples/tests/ex8.c
%
%   Set the Matlab path and initialize PETSc
figure(1),clf;figure(2),clf;
path(path,'../../')
PetscInitialize({'-snes_monitor','-ksp_monitor'});
%%
%   Open a viewer to display PETSc objects
viewer = PetscViewer();
viewer.SetType('ascii');
%%
%  Create DM to manage the grid and get work vectors
user.mx = 10;user.my = 10;
user.dm = PetscDMDACreate2d(PetscDM.NONPERIODIC,PetscDM.STENCIL_BOX,user.mx,user.my,PetscObject.DECIDE,PetscObject.DECIDE,1,1);
x  = user.dm.CreateGlobalVector();
r  = x.Duplicate();
J  = user.dm.GetMatrix('aij');

%%
%  Set Boundary conditions
[user] = MSA_BoundaryConditions(user);
%%
%  Set initial guess
[x] = MSA_InitialGuess(user,x);
%%
%  Create the nonlinear solver 
snes = PetscSNES();
snes.SetType('vi');

%%
%  Set minimum surface area problem function routine
snes.SetFunction(r,'snesdvi_function',user);


%%
%  Set minimum surface area problem jacobian routine
snes.SetJacobian(J,J,'snesdvi_jacobian',user);

%%
%  Set solution monitoring routine
snes.MonitorSet('snesdvi_monitor',user);
%%
%   Set VI bounds
xl = x.Duplicate();
xu = x.Duplicate();
xl.Set(-10000000);
xu.Set(100000000);

snes.VISetVariableBounds(xl,xu);    

%%
%  Solve the nonlinear system
snes.SetFromOptions();
snes.Solve(x);
x.View(viewer);
snes.View(viewer);

%%
%   Free PETSc objects and Shutdown PETSc
%
user.bottom.Destroy();
user.top.Destroy();
user.right.Destroy();
user.left.Destroy();
r.Destroy();
x.Destroy();
xl.Destroy();
xu.Destroy();
user.dm.Destroy();
J.Destroy();
snes.Destroy();
viewer.Destroy();
PetscFinalize();
