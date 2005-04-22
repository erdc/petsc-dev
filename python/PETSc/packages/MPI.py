#!/usr/bin/env python
from __future__ import generators
import user
import config.base
import os
import PETSc.package
from stat import *

class Configure(PETSc.package.Package):
  def __init__(self, framework):
    PETSc.package.Package.__init__(self, framework)
    self.download_lam   = ['http://www.lam-mpi.org/download/files/lam-7.1.1.tar.gz']
    self.download_mpich = ['ftp://ftp.mcs.anl.gov/pub/mpi/mpich2.tar.gz']
    self.functions      = ['MPI_Init','MPI_Comm_create']
    self.includes       = ['mpi.h']
    self.liblist_mpich  = [['libmpich.a'],
                           ['mpich.lib'],
                           ['libmpich.a', 'libpmpich.a'],
                           ['libfmpich.a','libmpich.a', 'libpmpich.a'],
                           ['libfmpich.a','libmpich.a', 'libpmpich.a', 'libmpich.a', 'libpmpich.a', 'libpmpich.a'],
                           ['libmpich.a', 'libpmpich.a', 'libmpich.a', 'libpmpich.a', 'libpmpich.a']]
    self.liblist_lam    = [['liblammpi++.a','libmpi.a','liblam.a'],
                           ['libmpi.a','libmpi++.a'],['libmpi.a'],
                           ['liblammpio.a','libpmpi.a','liblamf77mpi.a','libmpi.a','liblam.a'],
                           ['liblammpio.a','libpmpi.a','liblamf90mpi.a','libmpi.a','liblam.a'],
                           ['liblammpio.a','libpmpi.a','libmpi.a','liblam.a'],
                           ['liblammpi++.a','libmpi.a','liblam.a'],
                           ['libmpi.a','liblam.a']]
    self.liblist        = [[]] + self.liblist_lam + self.liblist_mpich
    # defaults to --with-mpi=yes
    self.required       = 1
    self.complex        = 1
    self.isPOE          = 0
    self.usingMPIUni    = 0
    return

  def setupHelp(self, help):
    PETSc.package.Package.setupHelp(self,help)
    import nargs
    help.addArgument('MPI', '-download-lam=<no,yes,ifneeded>',    nargs.ArgFuzzyBool(None, 0, 'Download and install LAM/MPI'))
    help.addArgument('MPI', '-download-mpich=<no,yes,ifneeded>',  nargs.ArgFuzzyBool(None, 0, 'Download and install MPICH-2'))
    help.addArgument('MPI', '-with-mpirun=<prog>',                nargs.Arg(None, None, 'The utility used to launch MPI jobs'))
    help.addArgument('MPI', '-with-mpi-shared=<bool>',            nargs.ArgBool(None, 0, 'Require that the MPI library be shared'))
    help.addArgument('MPI', '-with-mpi-compilers=<bool>',         nargs.ArgBool(None, 1, 'Try to use the MPI compilers, e.g. mpicc'))
    help.addArgument('MPI', '-download-mpich-machines=[machine1,machine2...]',  nargs.Arg(None, ['localhost','localhost'], 'Machines for MPI to use'))
    help.addArgument('MPI', '-download-mpich-pm=mpd or gforker',  nargs.Arg(None, 'mpd', 'Launcher for MPI processes')) 
    return

  def setupDependencies(self, framework):
    PETSc.package.Package.setupDependencies(self, framework)
    self.types = framework.require('config.types',     self)
    return

  # search many obscure locations for MPI
  def getSearchDirectories(self):
    import re
    yield ''
    # Try configure package directories
    dirExp = re.compile(r'mpi(ch)?(-.*)?')
    for packageDir in self.framework.argDB['package-dirs']:
      packageDir = os.path.abspath(packageDir)
      if not os.path.isdir(packageDir):
        raise RuntimeError('Invalid package directory: '+packageDir)
      for f in os.listdir(packageDir):
        dir = os.path.join(packageDir, f)
        if not os.path.isdir(dir):
          continue
        if not dirExp.match(f):
          continue
        yield (dir)
    # Try SUSE location
    yield (os.path.abspath(os.path.join('/opt', 'mpich')))
    # Try IBM
    self.isPOE = 1
    dir = os.path.abspath(os.path.join('/usr', 'lpp', 'ppe.poe'))
    yield (os.path.abspath(os.path.join('/usr', 'lpp', 'ppe.poe')))
    self.isPOE = 0
    # Try /usr/local
    yield (os.path.abspath(os.path.join('/usr', 'local')))
    # Try /usr/local/*mpich*
    if os.path.isdir(dir):
      ls = os.listdir(dir)
      for dir in ls:
        if dir.find('mpich') >= 0:
          dir = os.path.join('/usr','local',dir)
          if os.path.isdir(dir):
            yield (dir)
    # Try ~/mpich*
    ls = os.listdir(os.getenv('HOME'))
    for dir in ls:
      if dir.find('mpich') >= 0:
        dir = os.path.join(os.getenv('HOME'),dir)
        if os.path.isdir(dir):
          yield (dir)
    # Try MPICH install locations under Windows
    yield(os.path.join('/cygdrive','c','Program\\ Files','MPICH'))
    yield(os.path.join('/cygdrive','c','Program\\ Files','MPICH','SDK.gcc'))
    yield(os.path.join('/cygdrive','c','Program\\ Files','MPICH','SDK'))
    return

  def checkSharedLibrary(self):
    '''Check that the libraries for MPI are shared libraries'''
    if self.framework.argDB['with-mpi-shared']:
      return self.libraries.checkShared('#include <mpi.h>\n','MPI_Init','MPI_Initialized','MPI_Finalize',checkLink = self.checkLink,libraries = self.lib)
    else:
      return 1

  def configureMPIRUN(self):
    '''Checking for mpirun'''
    if 'with-mpirun' in self.framework.argDB:
      self.framework.argDB['with-mpirun'] = os.path.expanduser(self.framework.argDB['with-mpirun'])
      if not self.getExecutable(self.framework.argDB['with-mpirun'], resultName = 'mpirun'):
        raise RuntimeError('Invalid mpirun specified: '+str(self.framework.argDB['with-mpirun']))
      return
    if self.isPOE:
      self.mpirun = os.path.join(self.arch.dir, 'bin', 'mpirun.poe')
      return
    mpiruns = ['mpiexec', 'mpirun']
    path    = []
    if 'with-mpi-dir' in self.framework.argDB:
      path.append(os.path.join(os.path.abspath(self.framework.argDB['with-mpi-dir']), 'bin'))
      # MPICH-NT-1.2.5 installs MPIRun.exe in mpich/mpd/bin
      path.append(os.path.join(os.path.abspath(self.framework.argDB['with-mpi-dir']), 'mpd','bin'))
    for inc in self.include:
      path.append(os.path.join(os.path.dirname(inc), 'bin'))
      # MPICH-NT-1.2.5 installs MPIRun.exe in mpich/SDK/include/../../mpd/bin
      path.append(os.path.join(os.path.dirname(os.path.dirname(inc)),'mpd','bin'))
    for lib in self.lib:
      path.append(os.path.join(os.path.dirname(os.path.dirname(lib)), 'bin'))
    self.pushLanguage('C')
    if os.path.basename(self.getCompiler()) == 'mpicc' and os.path.dirname(self.getCompiler()):
      path.append(os.path.dirname(self.getCompiler()))
    self.popLanguage()
    self.getExecutable(mpiruns, path = path, useDefaultPath = 1, resultName = 'mpirun')
    return
        
  def configureConversion(self):
    '''Check for the functions which convert communicators between C and Fortran
       - Define HAVE_MPI_COMM_F2C and HAVE_MPI_COMM_C2F if they are present
       - Some older MPI 1 implementations are missing these'''
    oldFlags = self.compilers.CPPFLAGS
    oldLibs  = self.framework.argDB['LIBS']
    self.compilers.CPPFLAGS       += ' '.join([self.headers.getIncludeArgument(inc) for inc in self.include])
    self.framework.argDB['LIBS']   = self.libraries.toString(self.lib)+' '+self.framework.argDB['LIBS']

    if self.checkLink('#include <mpi.h>\n', 'if (MPI_Comm_f2c(MPI_COMM_WORLD));\n'):
      self.addDefine('HAVE_MPI_COMM_F2C', 1)
    if self.checkLink('#include <mpi.h>\n', 'if (MPI_Comm_c2f(MPI_COMM_WORLD));\n'):
      self.addDefine('HAVE_MPI_COMM_C2F', 1)
    if self.checkLink('#include <mpi.h>\n', 'MPI_Fint a;\n'):
      self.addDefine('HAVE_MPI_FINT', 1)

    self.compilers.CPPFLAGS      = oldFlags
    self.framework.argDB['LIBS'] = oldLibs
    return

  def configureTypes(self):
    '''Checking for MPI types'''
    oldFlags = self.compilers.CPPFLAGS
    self.compilers.CPPFLAGS += ' '.join([self.headers.getIncludeArgument(inc) for inc in self.include])
    self.framework.batchIncludeDirs.extend([self.headers.getIncludeArgument(inc) for inc in self.include])
    self.types.checkSizeof('MPI_Comm', 'mpi.h')
    if 'HAVE_MPI_FINT' in self.defines:
      self.types.checkSizeof('MPI_Fint', 'mpi.h')
    self.compilers.CPPFLAGS = oldFlags
    return

  def alternateConfigureLibrary(self):
    '''Setup MPIUNI, our uniprocessor version of MPI'''
    self.framework.addDefine('HAVE_MPI', 1)
    self.include = [os.path.join(self.arch.dir,'include','mpiuni')]
    if 'STDCALL' in self.compilers.defines:
      self.include.append(' -DMPIUNI_USE_STDCALL')
    self.lib = [os.path.join(self.arch.dir,'lib',self.arch.arch,'libmpiuni')]
    self.mpirun = '${PETSC_DIR}/bin/mpirun.uni'
    self.addMakeMacro('MPIRUN','${PETSC_DIR}/bin/mpirun.uni')
    self.addDefine('HAVE_MPI_COMM_F2C', 1)
    self.addDefine('HAVE_MPI_COMM_C2F', 1)
    self.addDefine('HAVE_MPI_FINT', 1)
    self.framework.packages.append(self)
    self.usingMPIUni = 1
    return

  def configureMissingPrototypes(self):
    '''Checks for missing prototypes, which it adds to petscfix.h'''
    if not 'HAVE_MPI_FINT' in self.defines:
      self.addPrototype('typedef int MPI_Fint;')
    if not 'HAVE_MPI_COMM_F2C' in self.defines:
      self.addPrototype('#define MPI_Comm_f2c(a) (a)')
    if not 'HAVE_MPI_COMM_C2F' in self.defines:
      self.addPrototype('#define MPI_Comm_c2f(a) (a)')
    return

  def configureMPICHShared(self):
    '''MPICH cannot be used with shared libraries on the Mac, reject if trying'''
    if self.framework.host_cpu == 'powerpc' and self.framework.host_vendor == 'apple' and self.framework.host_os.startswith('darwin'):
      if self.framework.argDB['with-shared']:
        for lib in self.lib:
          if lib.find('mpich') >= 0:
            raise RuntimeError('Sorry, we have not been able to figure out how to use shared libraries on the \n \
              Mac with MPICH. Either run config/configure.py with --with-shared=0 or use LAM instead of MPICH; \n\
              for instance with --download-lam=0')
    return

  def checkDownload(self,preOrPost):
    '''Check if we should download LAM or MPICH'''
    if self.framework.argDB['download-lam'] == preOrPost:
      return os.path.abspath(os.path.join(self.InstallLAM(),self.arch.arch))
    if self.framework.argDB['download-mpich'] == preOrPost:
      return os.path.abspath(os.path.join(self.InstallMPICH(),self.arch.arch))
    return None

  def InstallLAM(self):
    self.liblist      = self.liblist_lam   # only generate LAM MPI guesses
    self.download     = self.download_lam
    self.downloadname = 'lam'
    lamDir = self.getDir()

    # Get the LAM directories
    installDir = os.path.join(lamDir, self.arch.arch)
    # Configure and Build LAM
    self.framework.pushLanguage('C')
    args = ['--prefix='+installDir, '--with-rsh=ssh','--with-CC="'+self.framework.getCompiler()+' '+self.framework.getCompilerFlags()+'"']
    self.framework.popLanguage()
    if 'CXX' in self.framework.argDB:
      self.framework.pushLanguage('Cxx')
      args.append('--with-CXX="'+self.framework.getCompiler()+' '+self.framework.getCompilerFlags()+'"')
      self.framework.popLanguage()
    else:
      args.append('--disable-CXX')
    if 'FC' in self.framework.argDB:
      self.framework.pushLanguage('FC')
      args.append('--with-F77="'+self.framework.getCompiler()+' '+self.framework.getCompilerFlags()+'"')
      self.framework.popLanguage()
    else:
      args.append('--disable-F77')
      args.append('--disable-F90')
    args = ' '.join(args)

    try:
      fd      = file(os.path.join(installDir,'config.args'))
      oldargs = fd.readline()
      fd.close()
    except:
      oldargs = ''
    if not oldargs == args:
      self.framework.log.write('Have to rebuild LAM oldargs = '+oldargs+'\n new args = '+args+'\n')
      try:
        self.logPrintBox('Configuring LAM/MPI; this may take several minutes')
        output  = config.base.Configure.executeShellCommand('cd '+lamDir+';./configure '+args, timeout=900, log = self.framework.log)[0]
      except RuntimeError, e:
        raise RuntimeError('Error running configure on LAM/MPI: '+str(e))
      try:
        self.logPrintBox('Compiling LAM/MPI; this may take several minutes')
        output  = config.base.Configure.executeShellCommand('cd '+lamDir+';LAM_INSTALL_DIR='+installDir+';export LAM_INSTALL_DIR; make install', timeout=2500, log = self.framework.log)[0]
      except RuntimeError, e:
        raise RuntimeError('Error running make on LAM/MPI: '+str(e))
      if not os.path.isdir(os.path.join(installDir,'lib')):
        self.framework.log.write('Error running make on LAM/MPI   ******(libraries not installed)*******\n')
        self.framework.log.write('********Output of running make on LAM follows *******\n')        
        self.framework.log.write(output)
        self.framework.log.write('********End of Output of running make on LAM *******\n')
        raise RuntimeError('Error running make on LAM, libraries not installed')
      
      fd = file(os.path.join(installDir,'config.args'), 'w')
      fd.write(args)
      fd.close()
      #need to run ranlib on the libraries using the full path
      try:
        output  = config.base.Configure.executeShellCommand(self.setCompilers.RANLIB+' '+os.path.join(installDir,'lib')+'/lib*.a', timeout=2500, log = self.framework.log)[0]
      except RuntimeError, e:
        raise RuntimeError('Error running ranlib on LAM/MPI libraries: '+str(e))
      # start up LAM demon; note lamboot does not close stdout, so call will ALWAYS timeout.
      try:
        output  = config.base.Configure.executeShellCommand('PATH=${PATH}:'+os.path.join(installDir,'bin')+' '+os.path.join(installDir,'bin','lamboot'), timeout=10, log = self.framework.log)[0]
      except:
        pass
      self.framework.actions.addArgument(self.PACKAGE, 'Install', 'Installed LAM/MPI into '+installDir)
    return self.getDir()

  def InstallMPICH(self):
    self.liblist      = self.liblist_mpich   # only generate MPICH guesses
    self.download     = self.download_mpich
    self.downloadname = 'mpich'
    mpichDir = self.getDir()
    installDir = os.path.join(mpichDir, self.arch.arch)
    if not os.path.isdir(installDir):
      os.mkdir(installDir)
      
    # Configure and Build MPICH
    self.framework.pushLanguage('C')
    args = ['--prefix='+installDir]
    envs = 'CC="'+self.framework.getCompiler()+' '+self.framework.getCompilerFlags()+'"'
    self.framework.popLanguage()
    if 'CXX' in self.framework.argDB:
      self.framework.pushLanguage('Cxx')
      envs += ' CXX="'+self.framework.getCompiler()+' '+self.framework.getCompilerFlags()+'"'
      self.framework.popLanguage()
    else:
      args.append('--disable-cxx')
    if 'FC' in self.framework.argDB:
      self.framework.pushLanguage('FC')      
      fc = self.framework.getCompiler()
      args.append('--disable-f90')      
      if self.compilers.fortranIsF90:
        try:
          output, error, status = self.executeShellCommand(fc+' -v')
          output += error
        except:
          output = ''
        if output.find('IBM') >= 0:
          fc = os.path.join(os.path.dirname(fc), 'xlf')
          self.framework.log.write('Using IBM f90 compiler for PETSc, switching to xlf for compiling MPICH\n')      
      envs += ' FC="'+fc+' '+self.framework.getCompilerFlags().replace('-Mfree','')+'"'
      self.framework.popLanguage()
    else:
      args.append('--disable-f77')
    args.append('--without-mpe')
    args.append('--with-pm='+self.argDB['download-mpich-pm'])
    args = ' '.join(args)
    configArgsFilename = os.path.join(installDir,'config.args')
    try:
      fd      = file(configArgsFilename)
      oldargs = fd.readline()
      fd.close()
    except:
      self.framework.logPrint('Unable to find old configure arguments in '+configArgsFilename)
      oldargs = ''
    if not oldargs == args:
      self.framework.logPrint('Have to rebuild MPICH oldargs = '+oldargs+'\n new args = '+args)
      try:
        self.logPrintBox('Running configure on MPICH; this may take several minutes')
        output  = config.base.Configure.executeShellCommand('cd '+mpichDir+';'+envs+' ./configure '+args, timeout=900, log = self.framework.log)[0]
      except RuntimeError, e:
        if self.arch.hostOsBase.startswith('cygwin'):
          raise RuntimeError('Error running configure on MPICH. \n \
  On Microsoft Windows systems, please obtain and install the binary distribution from \n \
    http://www.mcs.anl.gov/mpi/mpich/mpich-nt \n \
  then rerun PETSc\'s configure.  \n \
  If you choose to install MPICH to a location other than the default, use \n \
    --with-mpi-dir=<directory> \n \
  to specify the location of the installation when you rerun configure.')
        raise RuntimeError('Error running configure on MPICH: '+str(e))
      try:
        self.logPrintBox('Running make on MPICH; this may take several minutes')
        output  = config.base.Configure.executeShellCommand('cd '+mpichDir+';make; make install', timeout=2500, log = self.framework.log)[0]
      except RuntimeError, e:
        if self.arch.hostOsBase.startswith('cygwin'):
          raise RuntimeError('Error running make; make install on MPICH. \n \
  On Microsoft Windows systems, please obtain and install the binary distribution from \n \
    http://www.mcs.anl.gov/mpi/mpich/mpich-nt \n \
  then rerun PETSc\'s configure.  \n \
  If you choose to install MPICH to a location other than the default, use \n \
    --with-mpi-dir=<directory> \n \
  to specify the location of the installation when you rerun configure.')
        raise RuntimeError('Error running make; make install on MPICH: '+str(e))

      try:
        fd = file(configArgsFilename, 'w')
        fd.write(args)
        fd.close()
      except:
        self.framework.logPrint('Unable to output configure arguments into '+configArgsFilename)
      if self.argDB['download-mpich-pm'] == 'mpd':
        if not os.path.isfile(os.path.join(os.getenv('HOME'),'.mpd.conf')):
          fd = open(os.path.join(os.getenv('HOME'),'.mpd.conf'),'w')
          fd.write('secretword=mr45-j9z\n')
          fd.close()
          os.chmod(os.path.join(os.getenv('HOME'),'.mpd.conf'),S_IRWXU)

        # start up MPICH's demon
        self.framework.logPrint('Starting up MPICH mpd demon needed for mpirun')
        try:
          self.executeShellCommand('cd '+installDir+'; bin/mpdboot',timeout=25)
        except:
          pass
        self.framework.logPrint('Started up MPICH mpd demon needed for mpirun')
      self.framework.actions.addArgument('MPI', 'Install', 'Installed MPICH into '+installDir)
    return self.getDir()

  def addExtraLibraries(self):
    '''Check for various auxiliary libraries we may need'''
    extraLib = []
    if self.executeTest(self.libraries.check, [['rt'], 'timer_create', None, extraLib]):
      extraLib.append('librt.a')
    if self.executeTest(self.libraries.check, [['aio'], 'aio_read', None, extraLib]):
      extraLib.insert(0, 'libaio.a')
    if self.executeTest(self.libraries.check, [['nsl'], 'exit', None, extraLib]):
      extraLib.insert(0, 'libnsl.a')
    self.extraLib.extend(extraLib)
    return

  def configureLibrary(self):
    '''Calls the regular package configureLibrary and then does an additional test needed by MPI'''
    self.addExtraLibraries()
    PETSc.package.Package.configureLibrary(self)
    self.executeTest(self.configureMPICHShared)
    self.executeTest(self.configureMPIRUN)
    self.executeTest(self.configureConversion)
    self.executeTest(self.configureTypes)
    self.executeTest(self.configureMissingPrototypes)      

if __name__ == '__main__':
  import config.framework
  import sys
  framework = config.framework.Framework(sys.argv[1:])
  framework.setupLogging(framework.clArgs)
  framework.children.append(Configure(framework))
  framework.configure()
  framework.dumpSubstitutions()
