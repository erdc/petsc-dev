#!/usr/bin/env python
from __future__ import generators
import user
import config.base
import os

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    return

  def __str__(self):
    return ''
    
  def setupHelp(self, help):
    import nargs
    help.addArgument('PETSc', '-with-gnu-copyright-code=<bool>',nargs.ArgBool(None, 1, 'Compile in GNU copyright code (see http://www.mcs.anl.gov/petsc/petsc-as/documentation/copyright.html'))
    help.addArgument('PETSc', '-with-log=<bool>',              nargs.ArgBool(None, 1, 'Activate logging code in PETSc'))    
    help.addArgument('PETSc', '-with-info=<bool>',             nargs.ArgBool(None, 1, 'Activate PetscInfo() (i.e. -info)  code in PETSc'))
    help.addArgument('PETSc', '-with-ctable=<bool>',           nargs.ArgBool(None, 1, 'Activate CTABLE hashing for certain search functions - to conserve memory'))
    help.addArgument('PETSc', '-with-fortran-kernels=<none,generic,bgl>',  nargs.ArgString(None, None, 'Use Fortran for linear algebra kernels'))
    help.addArgument('PETSc', '-with-64-bit-indices=<bool>',   nargs.ArgBool(None, 0, 'Use 64 bit integers (long long) for indexing in vectors and matrices'))
    help.addArgument('PETSc', '-with-is-color-value-type=<char,short>',nargs.ArgString(None, 'char', 'char, short can store 256, 65536 colors'))
    return

  def setupDependencies(self, framework):
    config.base.Configure.setupDependencies(self, framework)
    self.debugging = framework.require('PETSc.utilities.debugging', self)
    self.compilers = framework.require('config.compilers', self)
    self.libraries = framework.require('config.libraries', self)
    self.types     = framework.require('config.types', self)
    self.languages = framework.require('PETSc.utilities.languages', self)
    return

  def isBGL(self):
    '''Returns true if compiler is IBM cross compiler for BGL'''
    if self.libraries.check('', 'bgl_perfctr_void') and self.libraries.check('', '_xlqadd'):
      self.logPrint('BGL/IBM detected')
      return 1
    else:
      self.logPrint('BGL/IBM test failure')
      return 0

  def configureLibraryOptions(self):
    '''Sets PETSC_USE_DEBUG, PETSC_USE_INFO, PETSC_USE_LOG, PETSC_USE_CTABLE and PETSC_USE_FORTRAN_KERNELS'''
    self.useLog   = self.framework.argDB['with-log']
    self.addDefine('USE_LOG',   self.useLog)

    if self.debugging.debugging:
      self.addDefine('USE_DEBUG', '1')

    if not self.framework.argDB['with-gnu-copyright-code']:
      self.addDefine('AVOID_GNUCOPYRIGHT_CODE', '1')
      
    self.useInfo   = self.framework.argDB['with-info']
    self.addDefine('USE_INFO',   self.useInfo)

    self.useCtable = self.framework.argDB['with-ctable']
    if self.useCtable:
      self.addDefine('USE_CTABLE', '1')

    # If user doesn't specify this option - automatically enable bgl-kernels for IBM-bgl-crosscompilers
    if 'with-fortran-kernels' not in self.framework.argDB and self.isBGL():
      self.useFortranKernels = 'bgl'
    elif 'with-fortran-kernels' in self.framework.argDB:
      self.useFortranKernels = self.framework.argDB['with-fortran-kernels'].lower()
    elif self.languages.precision == 'matsingle':
      self.logPrint('Enabling fortran kernels automatically due to matsingle option')
      self.useFortranKernels = 'generic'
    else:
      self.useFortranKernels = 'none'

    if self.useFortranKernels == '1' or self.useFortranKernels == 'yes' : self.useFortranKernels = 'generic'
    elif self.useFortranKernels == '0' or self.useFortranKernels == 'no' : self.useFortranKernels = 'none'

    if not hasattr(self.compilers, 'FC') and self.useFortranKernels != 'none':
      raise RuntimeError('Cannot use fortran kernels without a Fortran compiler')
    if self.useFortranKernels == 'bgl':
      self.addDefine('USE_FORTRAN_KERNELS', 1)
      self.addDefine('USE_FORTRAN_KERNELS_BGL', 1)
    elif self.useFortranKernels == 'generic':
      self.addDefine('USE_FORTRAN_KERNELS', 1)
    elif self.useFortranKernels != 'none':
      raise RuntimeError('Unknown Fortran kernel type specified :'+self.framework.argDB['with-fortran-kernels'])

    if self.framework.argDB['with-64-bit-indices']:
      self.integerSize = 64
      self.addDefine('USE_64BIT_INDICES', 1)
      if self.libraries.check('-lgcc_s.1', '__floatdidf'):
        self.compilers.LIBS += ' '+self.libraries.getLibArgument('-lgcc_s.1')
    else:
      self.integerSize = 32
      self.addDefine('USE_32BIT_INT', 1)
    return

  def configureISColorValueType(self):
    '''Sets PETSC_IS_COLOR_VALUE_TYPE, MPIU_COLORING_VALUE, IS_COLORING_MAX required by ISColor'''
    self.isColorValueType  = self.framework.argDB['with-is-color-value-type']
    if self.isColorValueType != 'char' and self.isColorValueType != 'short':
      raise RuntimeError('Incorrect --with-is-color-value-type value specified. Can be either char or short. Specified value is :'+self.isColorValueType)
    if self.isColorValueType == 'char':
      max = pow(2,self.types.sizes['sizeof_char']*self.types.bits_per_byte)-1
      mpi_type = 'MPI_UNSIGNED_CHAR'
    else:
      max = pow(2,self.types.sizes['sizeof_short']*self.types.bits_per_byte)-1
      mpi_type = 'MPI_UNSIGNED_SHORT'

    self.framework.addDefine('MPIU_COLORING_VALUE',mpi_type)
    self.framework.addDefine('IS_COLORING_MAX',max)
    self.addDefine('IS_COLOR_VALUE_TYPE', self.isColorValueType)
    return
  
  def configure(self):
    self.executeTest(self.configureLibraryOptions)
    self.executeTest(self.configureISColorValueType)
    return
