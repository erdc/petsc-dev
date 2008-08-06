#define ALE_MEM_LOGGING

#include <petsc.h>
#include <petscmesh_formats.hh>
#include <Mesh.hh>
#include <Generator.hh>
#include "unitTests.hh"

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

class FunctionTestIMesh : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(FunctionTestIMesh);

  CPPUNIT_TEST(testStratify);

  CPPUNIT_TEST_SUITE_END();
public:
  typedef ALE::IMesh            mesh_type;
  typedef mesh_type::point_type point_type;
protected:
  ALE::Obj<mesh_type> _mesh;
  int                 _debug; // The debugging level
  PetscInt            _iters; // The number of test repetitions
  PetscInt            _size;  // The interval size
  ALE::Obj<ALE::Mesh>             _m;
  std::map<point_type,point_type> _renumbering;
public:
  PetscErrorCode processOptions() {
    PetscErrorCode ierr;

    this->_debug = 0;
    this->_iters = 1;
    this->_size  = 10;

    PetscFunctionBegin;
    ierr = PetscOptionsBegin(PETSC_COMM_WORLD, "", "Options for interval section stress test", "ISection");CHKERRQ(ierr);
      ierr = PetscOptionsInt("-debug", "The debugging level", "isection.c", this->_debug, &this->_debug, PETSC_NULL);CHKERRQ(ierr);
      ierr = PetscOptionsInt("-iterations", "The number of test repetitions", "isection.c", this->_iters, &this->_iters, PETSC_NULL);CHKERRQ(ierr);
      ierr = PetscOptionsInt("-size", "The interval size", "isection.c", this->_size, &this->_size, PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsEnd();CHKERRQ(ierr);
    PetscFunctionReturn(0);
  };

  /// Setup data.
  void setUp(void) {
    this->processOptions();
    double                    lower[3] = {0.0, 0.0, 0.0};
    double                    upper[3] = {1.0, 1.0, 1.0};
    int                       faces[3] = {3, 3, 3};
    const ALE::Obj<ALE::Mesh> mB       = ALE::MeshBuilder<ALE::Mesh>::createCubeBoundary(PETSC_COMM_WORLD, lower, upper, faces, this->_debug);
    this->_m    = ALE::Generator<ALE::Mesh>::generateMesh(mB, true);
    this->_mesh = new mesh_type(PETSC_COMM_WORLD, 1, this->_debug);
    ALE::Obj<mesh_type::sieve_type> sieve = new mesh_type::sieve_type(PETSC_COMM_WORLD, 0, 119, this->_debug);

    this->_mesh->setSieve(sieve);
    ALE::ISieveConverter::convertSieve(*this->_m->getSieve(), *this->_mesh->getSieve(), this->_renumbering);
  };

  /// Tear down data.
  void tearDown(void) {};

  void testStratify() {
    this->_mesh->stratify();
    const ALE::Obj<ALE::Mesh::label_type>& h1 = this->_m->getLabel("height");
    const ALE::Obj<mesh_type::label_type>& h2 = this->_mesh->getLabel("height");

    for(int h = 0; h < 4; ++h) {
      CPPUNIT_ASSERT_EQUAL(h1->support(h)->size(), h2->support(h)->size());
      const ALE::Obj<ALE::Mesh::label_sequence>& points1 = h1->support(h);
      const ALE::Obj<mesh_type::label_sequence>& points2 = h2->support(h);
      ALE::Mesh::label_sequence::iterator        p_iter1 = points1->begin();
      mesh_type::label_sequence::iterator        p_iter2 = points2->begin();
      ALE::Mesh::label_sequence::iterator        end1    = points1->end();

      while(p_iter1 != end1) {
        CPPUNIT_ASSERT_EQUAL(this->_renumbering[*p_iter1], *p_iter2);
        ++p_iter1;
        ++p_iter2;
      }
    }
    const ALE::Obj<ALE::Mesh::label_type>& d1 = this->_m->getLabel("depth");
    const ALE::Obj<mesh_type::label_type>& d2 = this->_mesh->getLabel("depth");

    for(int d = 0; d < 4; ++d) {
      CPPUNIT_ASSERT_EQUAL(d1->support(d)->size(), d2->support(d)->size());
      const ALE::Obj<ALE::Mesh::label_sequence>& points1 = d1->support(d);
      const ALE::Obj<mesh_type::label_sequence>& points2 = d2->support(d);
      ALE::Mesh::label_sequence::iterator        p_iter1 = points1->begin();
      mesh_type::label_sequence::iterator        p_iter2 = points2->begin();
      ALE::Mesh::label_sequence::iterator        end1    = points1->end();

      while(p_iter1 != end1) {
        CPPUNIT_ASSERT_EQUAL(this->_renumbering[*p_iter1], *p_iter2);
        ++p_iter1;
        ++p_iter2;
      }
    }
  };
};

#undef __FUNCT__
#define __FUNCT__ "RegisterIMeshFunctionSuite"
PetscErrorCode RegisterIMeshFunctionSuite() {
  CPPUNIT_TEST_SUITE_REGISTRATION(FunctionTestIMesh);
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

class MemoryTestIMesh : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(MemoryTestIMesh);

  CPPUNIT_TEST(testPCICE);

  CPPUNIT_TEST_SUITE_END();
public:
  typedef ALE::IMesh            mesh_type;
  typedef mesh_type::sieve_type sieve_type;
  typedef mesh_type::point_type point_type;
protected:
  ALE::Obj<mesh_type> _mesh;
  int                 _debug; // The debugging level
  PetscInt            _iters; // The number of test repetitions
public:
  PetscErrorCode processOptions() {
    PetscErrorCode ierr;

    this->_debug = 0;
    this->_iters = 1;

    PetscFunctionBegin;
    ierr = PetscOptionsBegin(PETSC_COMM_WORLD, "", "Options for interval section stress test", "ISection");CHKERRQ(ierr);
      ierr = PetscOptionsInt("-debug", "The debugging level", "isection.c", this->_debug, &this->_debug, PETSC_NULL);CHKERRQ(ierr);
      ierr = PetscOptionsInt("-iterations", "The number of test repetitions", "isection.c", this->_iters, &this->_iters, PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscOptionsEnd();CHKERRQ(ierr);
    PetscFunctionReturn(0);
  };

  /// Setup data.
  void setUp(void) {
    this->processOptions();
  };

  /// Tear down data.
  void tearDown(void) {};

  void testPCICE(void) {
    ALE::MemoryLogger& logger      = ALE::MemoryLogger::singleton();
    const char        *nameOld     = "PCICE Old";
    const char        *name        = "PCICE";
    const bool         interpolate = false;
    PetscLogDouble     osMemStart, osMemSemiEnd, osMemEnd, osMemOld, osMem;

    PetscMemoryGetCurrentUsage(&osMemStart);
    logger.setDebug(this->_debug);
    logger.stagePush(nameOld);
    {
      const int                             dim   = 2;
      ALE::Obj<mesh_type>                   mesh  = new mesh_type(PETSC_COMM_WORLD, dim, this->_debug);
      ALE::Obj<sieve_type>                  sieve = new sieve_type(mesh->comm(), this->_debug);
      const ALE::Obj<ALE::Mesh>             m     = new ALE::Mesh(mesh->comm(), dim, this->_debug);
      const ALE::Obj<ALE::Mesh::sieve_type> s     = new ALE::Mesh::sieve_type(mesh->comm(), this->_debug);
      int                                  *cells         = NULL;
      double                               *coordinates   = NULL;
      //const std::string&                    coordFilename = "../tutorials/data/ex1_2d.nodes";
      //const std::string&                    adjFilename   = "../tutorials/data/ex1_2d.lcon";
      const std::string&                    coordFilename = "data/3d.nodes";
      const std::string&                    adjFilename   = "data/3d.lcon";
      const bool                            useZeroBase   = true;
      int                                   numCells = 0, numVertices = 0, numCorners = dim+1;
      PetscErrorCode                        ierr;

      ALE::PCICE::Builder::readConnectivity(mesh->comm(), adjFilename, numCorners, useZeroBase, numCells, &cells);
      ALE::PCICE::Builder::readCoordinates(mesh->comm(), coordFilename, dim, numVertices, &coordinates);
      ALE::SieveBuilder<ALE::Mesh>::buildTopology(s, dim, numCells, cells, numVertices, interpolate, numCorners, -1, m->getArrowSection("orientation"));
      m->setSieve(s);
      m->stratify();
      PetscMemoryGetCurrentUsage(&osMemOld);
      logger.stagePush(name);
      mesh->setSieve(sieve);
      mesh_type::renumbering_type renumbering;
      ALE::ISieveConverter::convertSieve(*s, *sieve, renumbering, false);
      mesh->stratify();
      ALE::ISieveConverter::convertOrientation(*s, *sieve, renumbering, m->getArrowSection("orientation").ptr());
      ALE::SieveBuilder<PETSC_MESH_TYPE>::buildCoordinates(mesh, dim, coordinates);
      if (cells)       {ierr = PetscFree(cells);}
      if (coordinates) {ierr = PetscFree(coordinates);}
      logger.stagePop();
      PetscMemoryGetCurrentUsage(&osMem);
    }
    mesh_type::MeshNumberingFactory::singleton(PETSC_COMM_WORLD, this->_debug, true);
    ALE::Mesh::MeshNumberingFactory::singleton(PETSC_COMM_WORLD, this->_debug, true);
    logger.stagePop();
    PetscMemoryGetCurrentUsage(&osMemSemiEnd);
    // Reallocate difference
    const int diffSize = (int)((osMemOld-osMemStart)-(osMem-osMemOld));
    char     *tmp      = new char[diffSize];
    std::cout << "Allocated " << diffSize << " bytes to check for empty space" << std::endl;
    PetscMemoryGetCurrentUsage(&osMemEnd);
    std::cout << std::endl << nameOld << " " << logger.getNumAllocations(nameOld) << " allocations " << logger.getAllocationTotal(nameOld) << " bytes" << std::endl;
    std::cout << std::endl << nameOld << " " << logger.getNumDeallocations(nameOld) << " deallocations " << logger.getDeallocationTotal(nameOld) << " bytes" << std::endl;
    std::cout << std::endl << name << " " << logger.getNumAllocations(name) << " allocations " << logger.getAllocationTotal(name) << " bytes" << std::endl;
    std::cout << std::endl << name << " " << logger.getNumDeallocations(name) << " deallocations " << logger.getDeallocationTotal(name) << " bytes" << std::endl;
    std::cout << std::endl << "osMemOld: " << osMemOld-osMemStart << " osMem: " << osMem-osMemOld << " osMemSemiEnd: " << osMemSemiEnd-osMemStart << " osMemEnd: " << osMemEnd-osMemStart << std::endl;
    std::cout << std::endl << osMemStart<<" "<<osMemOld <<" "<<osMem<<" "<<osMemSemiEnd<<" "<<osMemEnd << std::endl;
    CPPUNIT_ASSERT_EQUAL(logger.getNumAllocations(nameOld)+logger.getNumAllocations(name), logger.getNumDeallocations(nameOld)+logger.getNumDeallocations(name));
    CPPUNIT_ASSERT_EQUAL(logger.getAllocationTotal(nameOld)+logger.getAllocationTotal(name), logger.getDeallocationTotal(nameOld)+logger.getDeallocationTotal(name));
  };
};

#undef __FUNCT__
#define __FUNCT__ "RegisterIMeshMemorySuite"
PetscErrorCode RegisterIMeshMemorySuite() {
  CPPUNIT_TEST_SUITE_REGISTRATION(MemoryTestIMesh);
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}
