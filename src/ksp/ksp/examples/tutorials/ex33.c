/*T
   Concepts: KSP^solving a system of linear equations
   Concepts: KSP^Laplacian, 2d
   Processors: n
T*/

/*
Added at the request of Marc Garbey.

Inhomogeneous Laplacian in 2D. Modeled by the partial differential equation

   div \rho grad u = f,  0 < x,y < 1,

with forcing function

   f = e^{-(1 - x)^2/\nu} e^{-(1 - y)^2/\nu}

with Dirichlet boundary conditions

   u = f(x,y) for x = 0, x = 1, y = 0, y = 1

or pure Neumman boundary conditions

This uses multigrid to solve the linear system

The 2D test mesh

         13
  14--29----31---12
    |\    |\    |
    2 2 5 2 3 7 3
    7  6  8  0  2
    | 4 \ | 6 \ |
    |    \|    \|
  15--20-16-24---11
    |\    |\    |
    1 1 1 2 2 3 2
    8  7  1  2  5
    | 0 \ | 2 \ |
    |    \|    \|
   8--19----23---10
          9

*/

static char help[] = "Solves 2D inhomogeneous Laplacian using multigrid.\n\n";

#include "petscmesh.h"
#include "petscksp.h"
#include "petscmg.h"
#include "petscdmmg.h"

PetscErrorCode MeshView_Sieve_Newer(ALE::Obj<ALE::Two::Mesh>, PetscViewer);
PetscErrorCode CreateMeshBoundary(ALE::Obj<ALE::Two::Mesh>);
PetscErrorCode updateOperator(Mat, ALE::Obj<ALE::Two::Mesh::field_type>, const ALE::Two::Mesh::point_type&, PetscScalar [], InsertMode);

extern PetscErrorCode ComputeRHS(DMMG,Vec);
extern PetscErrorCode ComputeJacobian(DMMG,Mat,Mat);

typedef enum {DIRICHLET, NEUMANN} BCType;

typedef struct {
  PetscScalar   nu;
  BCType        bcType;
} UserContext;

PetscInt debug;

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  MPI_Comm       comm;
  DMMG          *dmmg;
  UserContext    user;
  PetscViewer    viewer;
  const char    *bcTypes[2] = {"dirichlet", "neumann"};
  PetscReal      refinementLimit, norm;
  PetscInt       dim, bc, l;
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc,&argv,(char *)0,help);CHKERRQ(ierr);
  ierr = PetscOptionsBegin(comm, "", "Options for the inhomogeneous Poisson equation", "DMMG");CHKERRQ(ierr);
    debug = 0;
    ierr = PetscOptionsInt("-debug", "The debugging flag", "ex33.c", 0, &debug, PETSC_NULL);CHKERRQ(ierr);
    dim  = 2;
    ierr = PetscOptionsInt("-dim", "The mesh dimension", "ex33.c", 2, &dim, PETSC_NULL);CHKERRQ(ierr);
    refinementLimit = 0.0;
    ierr = PetscOptionsReal("-refinement_limit", "The area of the largest triangle in the mesh", "ex33.c", 1.0, &refinementLimit, PETSC_NULL);CHKERRQ(ierr);
    user.nu = 0.1;
    ierr = PetscOptionsScalar("-nu", "The width of the Gaussian source", "ex33.c", 0.1, &user.nu, PETSC_NULL);CHKERRQ(ierr);
    bc = (PetscInt)DIRICHLET;
    ierr = PetscOptionsEList("-bc_type","Type of boundary condition","ex33.c",bcTypes,2,bcTypes[0],&bc,PETSC_NULL);CHKERRQ(ierr);
    user.bcType = (BCType) bc;
  ierr = PetscOptionsEnd();
  comm = PETSC_COMM_WORLD;

  ALE::Obj<ALE::Two::Mesh> meshBoundary = ALE::Two::Mesh(comm, dim-1, debug);
  ALE::Obj<ALE::Two::Mesh> mesh;

  try {
    ALE::LogStage stage = ALE::LogStageRegister("MeshCreation");
    ALE::LogStagePush(stage);
    ierr = PetscPrintf(comm, "Generating mesh\n");CHKERRQ(ierr);
    ierr = CreateMeshBoundary(meshBoundary);CHKERRQ(ierr);
    mesh = ALE::Two::Generator::generate(meshBoundary);
    ALE::Obj<ALE::Two::Mesh::sieve_type> topology = mesh->getTopology();
    ierr = PetscPrintf(comm, "  Read %d elements\n", topology->heightStratum(0)->size());CHKERRQ(ierr);
    ierr = PetscPrintf(comm, "  Read %d vertices\n", topology->depthStratum(0)->size());CHKERRQ(ierr);
#if 0
    ierr = PetscPrintf(comm, "Distributing mesh\n");CHKERRQ(ierr);
    mesh->distribute();
#endif

    if (refinementLimit > 0.0) {
      stage = ALE::LogStageRegister("MeshRefine");
      ALE::LogStagePush(stage);
      ierr = PetscPrintf(comm, "Refining mesh\n");CHKERRQ(ierr);
      mesh = ALE::Two::Generator::refine(mesh, refinementLimit);
      ALE::LogStagePop(stage);
      ierr = PetscPrintf(comm, "  Read %d elements\n", topology->heightStratum(0)->size());CHKERRQ(ierr);
      ierr = PetscPrintf(comm, "  Read %d vertices\n", topology->depthStratum(0)->size());CHKERRQ(ierr);
    }

    ierr = PetscPrintf(comm, "Creating boundary\n");CHKERRQ(ierr);
    ALE::Obj<ALE::Two::Mesh::field_type> boundary = mesh->getBoundary();
    ALE::Two::Mesh::field_type::patch_type patch;

    boundary->setTopology(topology);
    boundary->setPatch(topology->leaves(), patch);
    boundary->setFiberDimensionByDepth(patch, 0, 1);
    boundary->orderPatches();

    ALE::Obj<ALE::Two::Mesh::sieve_type::depthMarkerSequence> bdVertices = boundary->getTopology()->depthStratum(0, 1);

    for(ALE::Two::Mesh::sieve_type::depthMarkerSequence::iterator v_iter = bdVertices->begin(); v_iter != bdVertices->end(); ++v_iter) {
      //double *coords = mesh->getCoordinates()->restrict(patch, *v_iter);
      double values[1] = {0.0};

      boundary->update(patch, *v_iter, values);
    }

    ALE::Obj<ALE::Two::Mesh::field_type> u = mesh->getField("u");
    u->setPatch(topology->leaves(), ALE::Two::Mesh::field_type::patch_type());
    u->setFiberDimensionByDepth(patch, 0, 1);
    u->orderPatches();
    ALE::Obj<ALE::Two::Mesh::sieve_type::heightSequence> elements = topology->heightStratum(0);
    std::string orderName("element");

    for(ALE::Two::Mesh::sieve_type::heightSequence::iterator e_iter = elements->begin(); e_iter != elements->end(); e_iter++) {
      // setFiberDimensionByDepth() does not work here since we only want it to apply to the patch cone
      //   What we really need is the depthStratum relative to the patch
      ALE::Obj<ALE::def::PointSet> cone = topology->nCone(*e_iter, topology->depth());

      u->setPatch(orderName, *e_iter, *e_iter);
      for(ALE::def::PointSet::iterator c_iter = cone->begin(); c_iter != cone->end(); ++c_iter) {
        u->setFiberDimension(orderName, *e_iter, *c_iter, 1);
      }
    }
    u->orderPatches(orderName);

    Mesh petscMesh;
    ierr = MeshCreate(comm, &petscMesh);CHKERRQ(ierr);
    ierr = MeshSetMesh(petscMesh, mesh);CHKERRQ(ierr);
    ierr = DMMGCreate(comm,3,PETSC_NULL,&dmmg);CHKERRQ(ierr);
    ierr = DMMGSetDM(dmmg, (DM) petscMesh);CHKERRQ(ierr);
    ierr = MeshDestroy(petscMesh);CHKERRQ(ierr);
    for (l = 0; l < DMMGGetLevels(dmmg); l++) {
      ierr = DMMGSetUser(dmmg,l,&user);CHKERRQ(ierr);
    }

    ierr = DMMGSetKSP(dmmg,ComputeRHS,ComputeJacobian);CHKERRQ(ierr);
    if (user.bcType == NEUMANN) {
      ierr = DMMGSetNullSpace(dmmg,PETSC_TRUE,0,PETSC_NULL);CHKERRQ(ierr);
    }

    ierr = DMMGSolve(dmmg);CHKERRQ(ierr);

    ierr = MatMult(DMMGGetJ(dmmg),DMMGGetx(dmmg),DMMGGetr(dmmg));CHKERRQ(ierr);
    ierr = VecAXPY(DMMGGetr(dmmg),-1.0,DMMGGetRHS(dmmg));CHKERRQ(ierr);
    ierr = VecNorm(DMMGGetr(dmmg),NORM_2,&norm);CHKERRQ(ierr);
    ierr = PetscPrintf(comm,"Residual norm %g\n",norm);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(DMMGGetx(dmmg));CHKERRQ(ierr);
    ierr = VecAssemblyEnd(DMMGGetx(dmmg));CHKERRQ(ierr);

    stage = ALE::LogStageRegister("MeshOutput");
    ALE::LogStagePush(stage);
    ierr = PetscPrintf(comm, "Creating VTK mesh file\n");CHKERRQ(ierr);
    ierr = PetscViewerCreate(comm, &viewer);CHKERRQ(ierr);
    ierr = PetscViewerSetType(viewer, PETSC_VIEWER_ASCII);CHKERRQ(ierr);
    ierr = PetscViewerSetFormat(viewer, PETSC_VIEWER_ASCII_VTK);CHKERRQ(ierr);
    ierr = PetscViewerFileSetName(viewer, "poisson.vtk");CHKERRQ(ierr);
    ierr = MeshView_Sieve_Newer(mesh, viewer);CHKERRQ(ierr);
    //ierr = VecView(DMMGGetRHS(dmmg), viewer);CHKERRQ(ierr);
    ierr = VecView(DMMGGetx(dmmg), viewer);CHKERRQ(ierr);
    ierr = PetscViewerDestroy(viewer);CHKERRQ(ierr);
    ALE::LogStagePop(stage);

    ierr = DMMGDestroy(dmmg);CHKERRQ(ierr);
  } catch (ALE::Exception e) {
    std::cout << e << std::endl;
  }
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "CreateSquareBoundary"
/*
  Simple square boundary:

  6--14-5--13-4
  |     |     |
  15   19    12
  |     |     |
  7--20-8--18-3
  |     |     |
  16   17    11
  |     |     |
  0--9--1--10-2
*/
PetscErrorCode CreateSquareBoundary(ALE::Obj<ALE::Two::Mesh> mesh)
{
  MPI_Comm          comm = mesh->getComm();
  ALE::Obj<ALE::Two::Mesh::sieve_type> topology = mesh->getTopology();
  PetscScalar       coords[18] = {0.0, 0.0,
                                  1.0, 0.0,
                                  2.0, 0.0,
                                  2.0, 1.0,
                                  2.0, 2.0,
                                  1.0, 2.0,
                                  0.0, 2.0,
                                  0.0, 1.0,
                                  1.0, 1.0};
  ALE::Two::Mesh::point_type vertices[9];
  PetscInt          order = 0;
  PetscMPIInt       rank;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  if (rank == 0) {
    ALE::Two::Mesh::point_type edge;

    /* Create topology and ordering */
    for(int v = 0; v < 9; v++) {
      vertices[v] = ALE::Two::Mesh::point_type(0, v);
    }
    for(int e = 9; e < 17; e++) {
      edge = ALE::Two::Mesh::point_type(0, e);
      topology->addArrow(vertices[e-9],     edge, order++);
      topology->addArrow(vertices[(e-8)%8], edge, order++);
    }
    edge = ALE::Two::Mesh::point_type(0, 17);
    topology->addArrow(vertices[1], edge, order++);
    topology->addArrow(vertices[8], edge, order++);
    edge = ALE::Two::Mesh::point_type(0, 18);
    topology->addArrow(vertices[3], edge, order++);
    topology->addArrow(vertices[8], edge, order++);
    edge = ALE::Two::Mesh::point_type(0, 19);
    topology->addArrow(vertices[5], edge, order++);
    topology->addArrow(vertices[8], edge, order++);
    edge = ALE::Two::Mesh::point_type(0, 20);
    topology->addArrow(vertices[7], edge, order++);
    topology->addArrow(vertices[8], edge, order++);
  }
  topology->stratify();
  mesh->createSerialCoordinates(2, 0, coords);
  /* Create boundary conditions */
  if (rank == 0) {
    ALE::Obj<ALE::def::PointSet> cone = ALE::def::PointSet();

    for(int v = 0; v < 8; v++) {
      cone->insert(ALE::Two::Mesh::point_type(0, v));
    }
    for(int e = 9; e < 17; e++) {
      cone->insert(ALE::Two::Mesh::point_type(0, e));
    }
    topology->setMarker(cone, 1);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CreateCubeBoundary"
/*
  Simple cube boundary:

      7-----6
     /|    /|
    3-----2 |
    | |   | |
    | 4---|-5
    |/    |/
    0-----1
*/
PetscErrorCode CreateCubeBoundary(ALE::Obj<ALE::Two::Mesh> mesh)
{
  MPI_Comm          comm = mesh->getComm();
  ALE::Obj<ALE::Two::Mesh::sieve_type> topology = mesh->getTopology();
  PetscScalar       coords[24] = {0.0, 0.0, 0.0,
                                  1.0, 0.0, 0.0,
                                  1.0, 1.0, 0.0,
                                  0.0, 1.0, 0.0,
                                  0.0, 0.0, 1.0,
                                  1.0, 0.0, 1.0,
                                  1.0, 1.0, 1.0,
                                  0.0, 1.0, 1.0};

  ALE::Obj<std::set<ALE::Two::Mesh::point_type> > cone = std::set<ALE::Two::Mesh::point_type>();
  ALE::Two::Mesh::point_type            vertices[8];
  ALE::Two::Mesh::point_type            edges[12];
  ALE::Two::Mesh::point_type            edge;
  PetscInt                              embedDim = 3;
  PetscInt                              order = 0;
  PetscMPIInt                           rank;
  PetscErrorCode                        ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  if (rank == 0) {
    ALE::Two::Mesh::point_type face;

    /* Create topology and ordering */
    /* Vertices: 0 .. 3 on the bottom of the cube, 4 .. 7 on the top */
    for(int v = 0; v < 8; v++) {
      vertices[v] = ALE::Two::Mesh::point_type(0, v);
    }

    /* Edges on the bottom: Sieve element numbers e = 8 .. 11, edge numbers e - 8 = 0 .. 3 */
    for(int e = 8; e < 12; e++) {
      edge = ALE::Two::Mesh::point_type(0, e);
      edges[e-8] = edge;
      topology->addArrow(vertices[e-8],     edge, order++);
      topology->addArrow(vertices[(e-7)%4], edge, order++);
    }
    /* Edges on the top: Sieve element numbers e = 12 .. 15, edge numbers e - 8 = 4 .. 7 */
    for(int e = 12; e < 16; e++) {
      edge = ALE::Two::Mesh::point_type(0, e); 
      edges[e-8] = edge;
      topology->addArrow(vertices[e-8],        edge, order++);
      topology->addArrow(vertices[(e-11)%4+4], edge, order++);
    }
    /* Edges from bottom to top: Sieve element numbers e = 16 .. 19, edge numbers e - 8 = 8 .. 11 */
    for(int e = 16; e < 20; e++) {
      edge = ALE::Two::Mesh::point_type(0, e); 
      edges[e-8] = edge;
      topology->addArrow(vertices[e-16],   edge, order++);
      topology->addArrow(vertices[e-16+4], edge, order++);
    }

    /* Bottom face */
    face = ALE::Two::Mesh::point_type(0, 20); 
    topology->addArrow(edges[0], face, order++);
    topology->addArrow(edges[1], face, order++);
    topology->addArrow(edges[2], face, order++);
    topology->addArrow(edges[3], face, order++);
    /* Top face */
    face = ALE::Two::Mesh::point_type(0, 21); 
    topology->addArrow(edges[4], face, order++);
    topology->addArrow(edges[5], face, order++);
    topology->addArrow(edges[6], face, order++);
    topology->addArrow(edges[7], face, order++);
    /* Side faces: f = 22 .. 25 */
    for(int f = 22; f < 26; f++) {
      face = ALE::Two::Mesh::point_type(0, f);
      int v = f - 22;
      /* Covered by edges f - 22, f - 22 + 4, f - 22 + 8, (f - 21)%4 + 8 */
      topology->addArrow(edges[v],         face, order++);
      topology->addArrow(edges[(v+1)%4+8], face, order++);
      topology->addArrow(edges[v+4],       face, order++);
      topology->addArrow(edges[v+8],       face, order++);
    }
  }/* if(rank == 0) */
  topology->stratify();
  if (rank == 0) {
    ALE::Obj<ALE::Two::Mesh::bundle_type> vertexBundle = mesh->getBundle(0);
    ALE::Obj<ALE::Two::Mesh::bundle_type::PointArray> points = ALE::Two::Mesh::bundle_type::PointArray();
    const std::string orderName("element");
    /* Bottom face */
    ALE::Two::Mesh::point_type face = ALE::Two::Mesh::point_type(0, 20); 
    points->clear();
    points->push_back(vertices[0]);
    points->push_back(vertices[1]);
    points->push_back(vertices[2]);
    points->push_back(vertices[3]);
    vertexBundle->setPatch(orderName, points, face);
    /* Top face */
    face = ALE::Two::Mesh::point_type(0, 21); 
    points->clear();
    points->push_back(vertices[4]);
    points->push_back(vertices[5]);
    points->push_back(vertices[6]);
    points->push_back(vertices[7]);
    vertexBundle->setPatch(orderName, points, face);
    /* Side faces: f = 22 .. 25 */
    for(int f = 22; f < 26; f++) {
      face = ALE::Two::Mesh::point_type(0, f);
      int v = f - 22;
      /* Covered by edges f - 22, f - 22 + 4, f - 22 + 8, (f - 21)%4 + 8 */
      points->clear();
      points->push_back(vertices[v]);
      points->push_back(vertices[(v+1)%4]);
      points->push_back(vertices[(v+1)%4+4]);
      points->push_back(vertices[v+4]);
      vertexBundle->setPatch(orderName, points, face);
    }
  }
  mesh->createSerialCoordinates(embedDim, 0, coords);

  /* Create boundary conditions: set marker 1 to all of the sieve elements, 
     since everything is on the boundary (no internal faces, edges or vertices)  */
  if (rank == 0) {
    /* set marker to the base of the topology sieve -- the faces and the edges */
    topology->setMarker(topology->base(), 1);
    /* set marker to the vertices -- the 0-depth stratum */
    topology->setMarker(topology->depthStratum(0), 1);
  }

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CreateMeshBoundary"
/*
  Simple square boundary:

  6--14-5--13-4
  |     |     |
  15   19    12
  |     |     |
  7--20-8--18-3
  |     |     |
  16   17    11
  |     |     |
  0--9--1--10-2
*/
PetscErrorCode CreateMeshBoundary(ALE::Obj<ALE::Two::Mesh> mesh)
{
  int            dim = mesh->getDimension();
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (dim == 1) {
    ierr = CreateSquareBoundary(mesh);
  } else if (dim == 2) {
    ierr = CreateCubeBoundary(mesh);
  } else {
    SETERRQ1(PETSC_ERR_SUP, "Cannot construct a boundary of dimension %d", dim);
  }
  PetscFunctionReturn(0);
}

#ifndef MESH_3D

#define NUM_QUADRATURE_POINTS 9

/* Quadrature points */
static double points[18] = {
  -0.794564690381,
  -0.822824080975,
  -0.866891864322,
  -0.181066271119,
  -0.952137735426,
  0.575318923522,
  -0.0885879595127,
  -0.822824080975,
  -0.409466864441,
  -0.181066271119,
  -0.787659461761,
  0.575318923522,
  0.617388771355,
  -0.822824080975,
  0.0479581354402,
  -0.181066271119,
  -0.623181188096,
  0.575318923522};

/* Quadrature weights */
static double weights[9] = {
  0.223257681932,
  0.2547123404,
  0.0775855332238,
  0.357212291091,
  0.407539744639,
  0.124136853158,
  0.223257681932,
  0.2547123404,
  0.0775855332238};

#define NUM_BASIS_FUNCTIONS 3

/* Nodal basis function evaluations */
static double Basis[27] = {
  0.808694385678,
  0.10271765481,
  0.0885879595127,
  0.52397906772,
  0.0665540678392,
  0.409466864441,
  0.188409405952,
  0.0239311322871,
  0.787659461761,
  0.455706020244,
  0.455706020244,
  0.0885879595127,
  0.29526656778,
  0.29526656778,
  0.409466864441,
  0.10617026912,
  0.10617026912,
  0.787659461761,
  0.10271765481,
  0.808694385678,
  0.0885879595127,
  0.0665540678392,
  0.52397906772,
  0.409466864441,
  0.0239311322871,
  0.188409405952,
  0.787659461761};

/* Nodal basis function derivative evaluations */
static double BasisDerivatives[54] = {
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5,
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5,
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5,
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5,
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5,
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5,
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5,
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5,
  -0.5,
  -0.5,
  0.5,
  4.74937635818e-17,
  0.0,
  0.5};

#else

#define NUM_QUADRATURE_POINTS 27

/* Quadrature points */
static double points[81] = {
  -0.809560240317,
  -0.835756864273,
  -0.854011951854,
  -0.865851516496,
  -0.884304792128,
  -0.305992467923,
  -0.939397037651,
  -0.947733495427,
  0.410004419777,
  -0.876607962782,
  -0.240843539439,
  -0.854011951854,
  -0.913080888692,
  -0.465239359176,
  -0.305992467923,
  -0.960733394129,
  -0.758416359732,
  0.410004419777,
  -0.955631394718,
  0.460330056095,
  -0.854011951854,
  -0.968746121484,
  0.0286773243482,
  -0.305992467923,
  -0.985880737721,
  -0.53528439884,
  0.410004419777,
  -0.155115591937,
  -0.835756864273,
  -0.854011951854,
  -0.404851369974,
  -0.884304792128,
  -0.305992467923,
  -0.731135462175,
  -0.947733495427,
  0.410004419777,
  -0.452572254354,
  -0.240843539439,
  -0.854011951854,
  -0.61438408645,
  -0.465239359176,
  -0.305992467923,
  -0.825794030022,
  -0.758416359732,
  0.410004419777,
  -0.803159052121,
  0.460330056095,
  -0.854011951854,
  -0.861342428212,
  0.0286773243482,
  -0.305992467923,
  -0.937360010468,
  -0.53528439884,
  0.410004419777,
  0.499329056443,
  -0.835756864273,
  -0.854011951854,
  0.0561487765469,
  -0.884304792128,
  -0.305992467923,
  -0.522873886699,
  -0.947733495427,
  0.410004419777,
  -0.0285365459258,
  -0.240843539439,
  -0.854011951854,
  -0.315687284208,
  -0.465239359176,
  -0.305992467923,
  -0.690854665916,
  -0.758416359732,
  0.410004419777,
  -0.650686709523,
  0.460330056095,
  -0.854011951854,
  -0.753938734941,
  0.0286773243482,
  -0.305992467923,
  -0.888839283216,
  -0.53528439884,
  0.410004419777};

/* Quadrature weights */
static double weights[27] = {
  0.0701637994372,
  0.0653012061324,
  0.0133734490519,
  0.0800491405774,
  0.0745014590358,
  0.0152576273199,
  0.0243830167241,
  0.022693189565,
  0.0046474825267,
  0.1122620791,
  0.104481929812,
  0.021397518483,
  0.128078624924,
  0.119202334457,
  0.0244122037118,
  0.0390128267586,
  0.0363091033041,
  0.00743597204272,
  0.0701637994372,
  0.0653012061324,
  0.0133734490519,
  0.0800491405774,
  0.0745014590358,
  0.0152576273199,
  0.0243830167241,
  0.022693189565,
  0.0046474825267};

#define NUM_BASIS_FUNCTIONS 4

/* Nodal basis function evaluations */
static double Basis[108] = {
  0.749664528222,
  0.0952198798417,
  0.0821215678634,
  0.0729940240731,
  0.528074388273,
  0.0670742417521,
  0.0578476039361,
  0.347003766038,
  0.23856305665,
  0.0303014811743,
  0.0261332522867,
  0.705002209888,
  0.485731727037,
  0.0616960186091,
  0.379578230281,
  0.0729940240731,
  0.342156357896,
  0.0434595556538,
  0.267380320412,
  0.347003766038,
  0.154572667042,
  0.0196333029355,
  0.120791820134,
  0.705002209888,
  0.174656645238,
  0.0221843026408,
  0.730165028048,
  0.0729940240731,
  0.12303063253,
  0.0156269392579,
  0.514338662174,
  0.347003766038,
  0.0555803583921,
  0.00705963113955,
  0.23235780058,
  0.705002209888,
  0.422442204032,
  0.422442204032,
  0.0821215678634,
  0.0729940240731,
  0.297574315013,
  0.297574315013,
  0.0578476039361,
  0.347003766038,
  0.134432268912,
  0.134432268912,
  0.0261332522867,
  0.705002209888,
  0.273713872823,
  0.273713872823,
  0.379578230281,
  0.0729940240731,
  0.192807956775,
  0.192807956775,
  0.267380320412,
  0.347003766038,
  0.0871029849888,
  0.0871029849888,
  0.120791820134,
  0.705002209888,
  0.0984204739396,
  0.0984204739396,
  0.730165028048,
  0.0729940240731,
  0.0693287858938,
  0.0693287858938,
  0.514338662174,
  0.347003766038,
  0.0313199947658,
  0.0313199947658,
  0.23235780058,
  0.705002209888,
  0.0952198798417,
  0.749664528222,
  0.0821215678634,
  0.0729940240731,
  0.0670742417521,
  0.528074388273,
  0.0578476039361,
  0.347003766038,
  0.0303014811743,
  0.23856305665,
  0.0261332522867,
  0.705002209888,
  0.0616960186091,
  0.485731727037,
  0.379578230281,
  0.0729940240731,
  0.0434595556538,
  0.342156357896,
  0.267380320412,
  0.347003766038,
  0.0196333029355,
  0.154572667042,
  0.120791820134,
  0.705002209888,
  0.0221843026408,
  0.174656645238,
  0.730165028048,
  0.0729940240731,
  0.0156269392579,
  0.12303063253,
  0.514338662174,
  0.347003766038,
  0.00705963113955,
  0.0555803583921,
  0.23235780058,
  0.705002209888};

/* Nodal basis function derivative evaluations */
static double BasisDerivatives[324] = {
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  8.15881875835e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  1.08228622783e-16,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  1.43034809879e-16,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  2.8079494593e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  7.0536336094e-17,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  1.26006930238e-16,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  -3.49866380524e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  2.61116525673e-17,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  1.05937620823e-16,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  1.52807111565e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  6.1520690456e-17,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  1.21934019246e-16,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  -1.48832491782e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  4.0272766482e-17,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  1.1233505023e-16,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  -5.04349365259e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  1.52296507396e-17,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  1.01021564153e-16,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  -5.10267652705e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  1.4812758129e-17,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  1.00833228612e-16,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  -5.78459929494e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  1.00091968699e-17,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  9.86631702223e-17,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6964953901e-17,
  -6.58832349994e-17,
  0.0,
  0.5,
  1.52600313755e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.6938349868e-17,
  4.34764891191e-18,
  0.0,
  0.5,
  -1.68114298577e-17,
  0.0,
  0.0,
  0.5,
  -0.5,
  -0.5,
  -0.5,
  0.5,
  2.69035912409e-17,
  9.61055074835e-17,
  0.0,
  0.5,
  -5.87133459397e-17,
  0.0,
  0.0,
  0.5};

#endif

#undef __FUNCT__
#define __FUNCT__ "ExpandSetIntervals"
PetscErrorCode ExpandSetIntervals(ALE::Point_set intervals, PetscInt *indices)
{
  int k = 0;

  PetscFunctionBegin;
  for(ALE::Point_set::iterator i_itor = intervals.begin(); i_itor != intervals.end(); i_itor++) {
    for(int i = 0; i < (*i_itor).index; i++) {
      indices[k++] = (*i_itor).prefix + i;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ElementGeometry"
PetscErrorCode ElementGeometry(ALE::Obj<ALE::Two::Mesh> mesh, const ALE::Two::Mesh::point_type& e, PetscReal v0[], PetscReal J[], PetscReal invJ[], PetscReal *detJ)
{
  const double  *coords = mesh->getCoordinates()->restrict(std::string("element"), e);
  int            dim = mesh->getDimension();
  PetscReal      det, invDet;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (v0) {
    for(int d = 0; d < dim; d++) {
      v0[d] = coords[d];
    }
  }
  if (J) {
    for(int d = 0; d < dim; d++) {
      for(int f = 0; f < dim; f++) {
        J[d*dim+f] = 0.5*(coords[(f+1)*dim+d] - coords[0*dim+d]);
      }
    }
    if (debug) {
      MPI_Comm    comm = mesh->getComm();
      PetscMPIInt rank;

      ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
      for(int d = 0; d < dim; d++) {
        if (d == 0) {
          PetscSynchronizedPrintf(comm, "[%d]J = /", rank);
        } else if (d == dim-1) {
          PetscSynchronizedPrintf(comm, "[%d]    \\", rank);
        } else {
          PetscSynchronizedPrintf(comm, "[%d]    |", rank);
        }
        for(int e = 0; e < dim; e++) {
          PetscSynchronizedPrintf(comm, " %g", J[d*dim+e]);
        }
        if (d == 0) {
          PetscSynchronizedPrintf(comm, " \\\n");
        } else if (d == dim-1) {
          PetscSynchronizedPrintf(comm, " /\n");
        } else {
          PetscSynchronizedPrintf(comm, " |\n");
        }
      }
    }
    if (dim == 2) {
      det = fabs(J[0]*J[3] - J[1]*J[2]);
    } else if (dim == 3) {
      det = fabs(J[0*3+0]*(J[1*3+1]*J[2*3+2] - J[1*3+2]*J[2*3+1]) +
                 J[0*3+1]*(J[1*3+2]*J[2*3+0] - J[1*3+0]*J[2*3+2]) +
                 J[0*3+2]*(J[1*3+0]*J[2*3+1] - J[1*3+1]*J[2*3+0]));
    }
    invDet = 1.0/det;
    if (detJ) {
      *detJ = det;
    }
    if (invJ) {
      if (dim == 2) {
        invJ[0] =  invDet*J[3];
        invJ[1] = -invDet*J[1];
        invJ[2] = -invDet*J[2];
        invJ[3] =  invDet*J[0];
      } else if (dim == 3) {
        // FIX: This may be wrong
        invJ[0*3+0] = invDet*(J[1*3+1]*J[2*3+2] - J[1*3+2]*J[2*3+1]);
        invJ[0*3+1] = invDet*(J[1*3+2]*J[2*3+0] - J[1*3+0]*J[2*3+2]);
        invJ[0*3+2] = invDet*(J[1*3+0]*J[2*3+1] - J[1*3+1]*J[2*3+0]);
        invJ[1*3+0] = invDet*(J[0*3+1]*J[2*3+2] - J[0*3+2]*J[2*3+1]);
        invJ[1*3+1] = invDet*(J[0*3+2]*J[2*3+0] - J[0*3+0]*J[2*3+2]);
        invJ[1*3+2] = invDet*(J[0*3+0]*J[2*3+1] - J[0*3+1]*J[2*3+0]);
        invJ[2*3+0] = invDet*(J[0*3+1]*J[1*3+2] - J[0*3+2]*J[1*3+1]);
        invJ[2*3+1] = invDet*(J[0*3+2]*J[1*3+0] - J[0*3+0]*J[1*3+2]);
        invJ[2*3+2] = invDet*(J[0*3+0]*J[1*3+1] - J[0*3+1]*J[1*3+0]);
      }
      if (debug) {
        MPI_Comm    comm = mesh->getComm();
        PetscMPIInt rank;

        ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
        for(int d = 0; d < dim; d++) {
          if (d == 0) {
            PetscSynchronizedPrintf(comm, "[%d]Jinv = /", rank);
          } else if (d == dim-1) {
            PetscSynchronizedPrintf(comm, "[%d]       \\", rank);
          } else {
            PetscSynchronizedPrintf(comm, "[%d]       |", rank);
          }
          for(int e = 0; e < dim; e++) {
            PetscSynchronizedPrintf(comm, " %g", invJ[d*dim+e]);
          }
          if (d == 0) {
            PetscSynchronizedPrintf(comm, " \\\n");
          } else if (d == dim-1) {
            PetscSynchronizedPrintf(comm, " /\n");
          } else {
            PetscSynchronizedPrintf(comm, " |\n");
          }
        }
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ComputeRho"
PetscErrorCode ComputeRho(PetscReal x, PetscReal y, PetscScalar *rho)
{
  PetscFunctionBegin;
  if ((x > 1.0/3.0) && (x < 2.0/3.0) && (y > 1.0/3.0) && (y < 2.0/3.0)) {
    //*rho = 100.0;
    *rho = 1.0;
  } else {
    *rho = 1.0;
  }
  PetscFunctionReturn(0);
}
#if 0
#undef __FUNCT__
#define __FUNCT__ "ComputeBlock"
PetscErrorCode ComputeBlock(DMMG dmmg, Vec u, Vec r, ALE::Point_set block)
{
  Mesh              mesh = (Mesh) dmmg->dm;
  UserContext      *user = (UserContext *) dmmg->user;
  ALE::Sieve       *topology;
  ALE::PreSieve    *orientation;
  ALE::IndexBundle *bundle;
  ALE::IndexBundle *coordBundle;
  ALE::Point_set    elements;
  PetscInt          dim;
  Vec               coordinates;
  PetscScalar      *coords;
  PetscScalar      *array;
  PetscScalar      *field;
  PetscReal         elementVec[NUM_BASIS_FUNCTIONS];
  PetscReal         linearVec[NUM_BASIS_FUNCTIONS];
  PetscReal        *v0, *Jac, *Jinv, *t_der, *b_der;
  PetscReal         xi, eta, x_q, y_q, detJ, rho, funcValue;
  PetscInt          f, g, q;
  PetscErrorCode    ierr;

  ierr = MeshGetTopology(mesh, (void **) &topology);CHKERRQ(ierr);
  ierr = MeshGetOrientation(mesh, (void **) &orientation);CHKERRQ(ierr);
  ierr = MeshGetBundle(mesh, (void **) &bundle);CHKERRQ(ierr);
  ierr = MeshGetCoordinateBundle(mesh, (void **) &coordBundle);CHKERRQ(ierr);
  ierr = MeshGetCoordinates(mesh, &coordinates);CHKERRQ(ierr);
  ierr = VecGetArray(coordinates, &coords);CHKERRQ(ierr);
  ierr = VecGetArray(u, &array); CHKERRQ(ierr);
  dim = coordBundle->getFiberDimension(*coordBundle->getTopology()->depthStratum(0).begin());
  ierr = PetscMalloc(dim * sizeof(PetscReal), &v0);CHKERRQ(ierr);
  ierr = PetscMalloc(dim * sizeof(PetscReal), &t_der);CHKERRQ(ierr);
  ierr = PetscMalloc(dim * sizeof(PetscReal), &b_der);CHKERRQ(ierr);
  ierr = PetscMalloc(dim*dim * sizeof(PetscReal), &Jac);CHKERRQ(ierr);
  ierr = PetscMalloc(dim*dim * sizeof(PetscReal), &Jinv);CHKERRQ(ierr);
  elements = topology->star(block);
  for(ALE::Point_set::iterator element_itor = elements.begin(); element_itor != elements.end(); element_itor++) {
    ALE::Point e = *element_itor;

    ierr = ElementGeometry(coordBundle, orientation, coords, e, v0, Jac, Jinv, &detJ); CHKERRQ(ierr);
    ierr = restrictField(bundle, orientation, array, e, &field); CHKERRQ(ierr);
    /* Element integral */
    ierr = PetscMemzero(elementVec, NUM_BASIS_FUNCTIONS*sizeof(PetscScalar));CHKERRQ(ierr);
    for(q = 0; q < NUM_QUADRATURE_POINTS; q++) {
      xi = points[q*2+0] + 1.0;
      eta = points[q*2+1] + 1.0;
      x_q = Jac[0]*xi + Jac[1]*eta + v0[0];
      y_q = Jac[2]*xi + Jac[3]*eta + v0[1];
      ierr = ComputeRho(x_q, y_q, &rho);CHKERRQ(ierr);
      funcValue = PetscExpScalar(-(x_q*x_q)/user->nu)*PetscExpScalar(-(y_q*y_q)/user->nu);
      for(f = 0; f < NUM_BASIS_FUNCTIONS; f++) {
        t_der[0] = Jinv[0]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+f)*2+0] + Jinv[2]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+f)*2+1];
        t_der[1] = Jinv[1]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+f)*2+0] + Jinv[3]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+f)*2+1];
        for(g = 0; g < NUM_BASIS_FUNCTIONS; g++) {
          b_der[0] = Jinv[0]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+g)*2+0] + Jinv[2]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+g)*2+1];
          b_der[1] = Jinv[1]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+g)*2+0] + Jinv[3]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+g)*2+1];
          linearVec[f] += rho*(t_der[0]*b_der[0] + t_der[1]*b_der[1])*field[g];
        }
        elementVec[f] += (Basis[q*NUM_BASIS_FUNCTIONS+f]*funcValue - linearVec[f])*weights[q]*detJ;
      }
    }
    if (debug) {printf("elementVec = [%g %g %g]\n", elementVec[0], elementVec[1], elementVec[2]);}
    /* Assembly */
    ierr = assembleField(bundle, orientation, r, e, elementVec, ADD_VALUES); CHKERRQ(ierr);
  }
  ierr = PetscFree(v0);CHKERRQ(ierr);
  ierr = PetscFree(t_der);CHKERRQ(ierr);
  ierr = PetscFree(b_der);CHKERRQ(ierr);
  ierr = PetscFree(Jac);CHKERRQ(ierr);
  ierr = PetscFree(Jinv);CHKERRQ(ierr);
  ierr = VecRestoreArray(coordinates, &coords);CHKERRQ(ierr);
  ierr = VecRestoreArray(u, &array); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif
#undef __FUNCT__
#define __FUNCT__ "ComputeRHS"
PetscErrorCode ComputeRHS(DMMG dmmg, Vec b)
{
  ALE::Obj<ALE::Two::Mesh> m;
  Mesh                mesh = (Mesh) dmmg->dm;
  UserContext        *user = (UserContext *) dmmg->user;
  MPI_Comm            comm;
  PetscReal           elementVec[NUM_BASIS_FUNCTIONS];
  PetscReal           *v0, *Jac;
  PetscReal           xi, eta, x_q, y_q, detJ, funcValue;
  PetscInt            dim;
  PetscInt            f, q;
  PetscErrorCode      ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject) mesh, &comm);CHKERRQ(ierr);
  ierr = MeshGetMesh(mesh, &m);CHKERRQ(ierr);
  dim  = m->getDimension();
  ierr = PetscMalloc(dim * sizeof(PetscReal), &v0);CHKERRQ(ierr);
  ierr = PetscMalloc(dim*dim * sizeof(PetscReal), &Jac);CHKERRQ(ierr);
  ALE::Obj<ALE::Two::Mesh::sieve_type::heightSequence> elements = m->getTopology()->heightStratum(0);
  for(ALE::Two::Mesh::sieve_type::heightSequence::iterator e_itor = elements->begin(); e_itor != elements->end(); e_itor++) {
    ierr = ElementGeometry(m, *e_itor, v0, Jac, PETSC_NULL, &detJ);CHKERRQ(ierr);
    /* Element integral */
    ierr = PetscMemzero(elementVec, NUM_BASIS_FUNCTIONS*sizeof(PetscScalar));CHKERRQ(ierr);
    for(q = 0; q < NUM_QUADRATURE_POINTS; q++) {
      xi = points[q*2+0] + 1.0;
      eta = points[q*2+1] + 1.0;
      x_q = Jac[0]*xi + Jac[1]*eta + v0[0];
      y_q = Jac[2]*xi + Jac[3]*eta + v0[1];
      funcValue = PetscExpScalar(-(x_q*x_q)/user->nu)*PetscExpScalar(-(y_q*y_q)/user->nu);
      for(f = 0; f < NUM_BASIS_FUNCTIONS; f++) {
        elementVec[f] += Basis[q*NUM_BASIS_FUNCTIONS+f]*funcValue*weights[q]*detJ;
      }
    }
    if (debug) {PetscSynchronizedPrintf(comm, "elementVec = [%g %g %g]\n", elementVec[0], elementVec[1], elementVec[2]);}
    /* Assembly */
    m->getField(std::string("u"))->updateAdd(std::string("element"), *e_itor, elementVec);
    if (debug) {ierr = PetscSynchronizedFlush(comm);CHKERRQ(ierr);}
  }
  ierr = PetscFree(v0);CHKERRQ(ierr);
  ierr = PetscFree(Jac);CHKERRQ(ierr);
  ierr = VecAssemblyBegin(b);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(b);CHKERRQ(ierr);

  /* force right hand side to be consistent for singular matrix */
  /* note this is really a hack, normally the model would provide you with a consistent right handside */
  if (user->bcType == NEUMANN) {
    MatNullSpace nullspace;

    ierr = KSPGetNullSpace(dmmg->ksp,&nullspace);CHKERRQ(ierr);
    ierr = MatNullSpaceRemove(nullspace,b,PETSC_NULL);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ComputeJacobian"
PetscErrorCode ComputeJacobian(DMMG dmmg, Mat J, Mat jac)
{
  ALE::Obj<ALE::Two::Mesh> m;
  Mesh              mesh = (Mesh) dmmg->dm;
  UserContext      *user = (UserContext *) dmmg->user;
  MPI_Comm          comm;
  PetscReal         elementMat[NUM_BASIS_FUNCTIONS*NUM_BASIS_FUNCTIONS];
  PetscReal        *v0, *Jac, *Jinv, *t_der, *b_der;
  PetscReal         xi, eta, x_q, y_q, detJ, rho;
  PetscInt          dim;
  PetscInt          f, g, q;
  PetscMPIInt       rank;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject) mesh, &comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = MeshGetMesh(mesh, &m);CHKERRQ(ierr);
  dim  = m->getDimension();
  ierr = PetscMalloc(dim * sizeof(PetscReal), &v0);CHKERRQ(ierr);
  ierr = PetscMalloc(dim * sizeof(PetscReal), &t_der);CHKERRQ(ierr);
  ierr = PetscMalloc(dim * sizeof(PetscReal), &b_der);CHKERRQ(ierr);
  ierr = PetscMalloc(dim*dim * sizeof(PetscReal), &Jac);CHKERRQ(ierr);
  ierr = PetscMalloc(dim*dim * sizeof(PetscReal), &Jinv);CHKERRQ(ierr);
  ALE::Obj<ALE::Two::Mesh::field_type> field = m->getField("u");
  ALE::Obj<ALE::Two::Mesh::sieve_type::heightSequence> elements = m->getTopology()->heightStratum(0);
  for(ALE::Two::Mesh::sieve_type::heightSequence::iterator e_itor = elements->begin(); e_itor != elements->end(); e_itor++) {
    CHKMEMQ;
    ierr = ElementGeometry(m, *e_itor, v0, Jac, Jinv, &detJ);CHKERRQ(ierr);
    /* Element integral */
    ierr = PetscMemzero(elementMat, NUM_BASIS_FUNCTIONS*NUM_BASIS_FUNCTIONS*sizeof(PetscScalar));CHKERRQ(ierr);
    for(q = 0; q < NUM_QUADRATURE_POINTS; q++) {
      xi = points[q*2+0] + 1.0;
      eta = points[q*2+1] + 1.0;
      x_q = Jac[0]*xi + Jac[1]*eta + v0[0];
      y_q = Jac[2]*xi + Jac[3]*eta + v0[1];
      ierr = ComputeRho(x_q, y_q, &rho);CHKERRQ(ierr);
      for(f = 0; f < NUM_BASIS_FUNCTIONS; f++) {
        t_der[0] = Jinv[0]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+f)*2+0] + Jinv[2]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+f)*2+1];
        t_der[1] = Jinv[1]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+f)*2+0] + Jinv[3]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+f)*2+1];
        for(g = 0; g < NUM_BASIS_FUNCTIONS; g++) {
          b_der[0] = Jinv[0]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+g)*2+0] + Jinv[2]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+g)*2+1];
          b_der[1] = Jinv[1]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+g)*2+0] + Jinv[3]*BasisDerivatives[(q*NUM_BASIS_FUNCTIONS+g)*2+1];
          elementMat[f*NUM_BASIS_FUNCTIONS+g] += rho*(t_der[0]*b_der[0] + t_der[1]*b_der[1])*weights[q]*detJ;
        }
      }
    }
    if (debug) {
      ierr = PetscSynchronizedPrintf(comm, "[%d]elementMat = [%g %g %g]\n                [%g %g %g]\n                [%g %g %g]\n",
                                     rank, elementMat[0], elementMat[1], elementMat[2], elementMat[3], elementMat[4],
                                     elementMat[5], elementMat[6], elementMat[7], elementMat[8]);CHKERRQ(ierr);
    }
    /* Assembly */
    ierr = updateOperator(jac, field, *e_itor, elementMat, ADD_VALUES);CHKERRQ(ierr);
    if (debug) {ierr = PetscSynchronizedFlush(comm);CHKERRQ(ierr);}
  }
  ierr = PetscFree(v0);CHKERRQ(ierr);
  ierr = PetscFree(t_der);CHKERRQ(ierr);
  ierr = PetscFree(b_der);CHKERRQ(ierr);
  ierr = PetscFree(Jac);CHKERRQ(ierr);
  ierr = PetscFree(Jinv);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(jac, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(jac, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (user->bcType == DIRICHLET) {
    /* Zero out BC rows */
    ALE::Obj<ALE::Two::Mesh::sieve_type::depthMarkerSequence> bdVertices = m->getBoundary()->getTopology()->depthStratum(0, 1);
    ALE::Two::Mesh::field_type::patch_type patch;
    PetscInt *boundaryIndices;
    PetscInt k = 0;

    ALE::Obj<ALE::Two::Mesh::bundle_type> bdBundle = ALE::Two::Mesh::bundle_type(m->debug);
    bdBundle->setTopology(m->getTopology());
    bdBundle->setPatch(bdVertices, patch);
    bdBundle->setFiberDimensionByDepth(patch, 0, 1);
    bdBundle->orderPatches();

    int numBoundaryIndices = bdBundle->getSize(patch);
    ierr = PetscMalloc(numBoundaryIndices * sizeof(PetscInt), &boundaryIndices); CHKERRQ(ierr);
    for(ALE::Two::Mesh::sieve_type::depthMarkerSequence::iterator p = bdVertices->begin(); p != bdVertices->end(); ++p) {
      const ALE::Two::Mesh::field_type::index_type& idx = field->getIndex(patch, *p);

      for(int i = 0; i < idx.index; i++) {
        boundaryIndices[k++] = idx.prefix + i;
      }
    }
    if (debug) {
      for(int i = 0; i < numBoundaryIndices; i++) {
        ierr = PetscSynchronizedPrintf(comm, "[%d]boundaryIndices[%d] = %d\n", rank, i, boundaryIndices[i]);CHKERRQ(ierr);
      }
    }
    ierr = PetscSynchronizedFlush(comm);
    ierr = MatZeroRows(jac, numBoundaryIndices, boundaryIndices, 1.0);CHKERRQ(ierr);
    ierr = PetscFree(boundaryIndices);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(jac, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(jac, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
