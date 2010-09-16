#!/usr/bin/env python

"""
PETSc: Portable, Extensible Toolkit for Scientific Computation
==============================================================

The Portable, Extensible Toolkit for Scientific Computation (PETSc),
is a suite of data structures and routines for the scalable (parallel)
solution of scientific applications modeled by partial differential
equations. It employs the Message Passing Interface (MPI) standard for
all message-passing communication.
"""

import sys, os
from distutils.core import setup
from distutils.util import get_platform
from distutils.spawn import find_executable
from distutils.command.build import build as _build
if 'setuptools' in sys.modules:
    from setuptools.command.install import install as _install
else:
    from distutils.command.install import install as _install
from distutils.command.sdist import sdist as _sdist
from distutils import log

PETSC_DIR  = None
PETSC_ARCH = None

init_py = """\
# Author:  PETSc Team
# Contact: petsc-users@mcs.anl.gov

def get_petsc_dir():
    import os
    return os.path.dirname(__file__)

def get_config():
    conf = {}
    conf['petsc_dir'] = get_petsc_dir()
    return conf
"""

metadata = {
    'provides' : ['petsc'],
    'requires' : [],
}

def bootstrap():
    # Set PETSC_DIR and PETSC_ARCH, 
    global PETSC_DIR, PETSC_ARCH
    PETSC_DIR  = os.path.abspath(os.getcwd())
    PETSC_ARCH = get_platform() + '-python'
    os.environ['PETSC_DIR']  = PETSC_DIR
    os.environ['PETSC_ARCH'] = PETSC_ARCH
    sys.path.insert(0, os.path.join(PETSC_DIR, 'config'))
    # Generate package __init__.py file
    try:
        if not os.path.exists(PETSC_ARCH):
            os.mkdir(PETSC_ARCH)
        pkgfile = os.path.join(PETSC_ARCH, '__init__.py')
        open(pkgfile, 'wt').write(init_py)
    except:
        pass
    # Simple-minded lookup for MPI and mpi4py
    mpi4py = mpicc = None
    try:
        import mpi4py
        conf = mpi4py.get_config()
        mpicc = conf.get('mpicc')
    except ImportError: # mpi4py is not installed
        mpicc = os.environ.get('MPICC') or find_executable('mpicc')
    except AttributeError: # mpi4py is too old
        pass
    if not mpi4py and mpicc:
        if (('distribute' in sys.modules) or
            ('setuptools' in sys.modules)):
            metadata['install_requires']= ['mpi4py>=1.2.2']

def config(dry_run=False):
    log.info('PETSc: configure')
    if dry_run: return
    options = [
        'PETSC_ARCH='+os.environ['PETSC_ARCH'],
        '--with-shared-libraries',
        '--with-cmake=0', # not needed
        ]
    # MPI
    try:
        import mpi4py
        conf = mpi4py.get_config()
        mpicc = conf.get('mpicc')
    except (ImportError, AttributeError):
        mpicc = os.environ.get('MPICC') or find_executable('mpicc')
    if mpicc:
        options.append('--with-cc='+mpicc)
    else:
        options.append('--with-mpi=0')
    options.append('--with-cxx=0') # XXX mpicxx?
    options.append('--with-fc=0')  # XXX mpif90?
    # Run PETSc configure
    import configure
    configure.petsc_configure(options)
    import logger
    logger.Logger.defaultLog = None

def build(dry_run=False):
    log.info('PETSc: build')
    if dry_run: return
    # Run PETSc builder
    import builder
    builder.PETScMaker().run()
    import logger
    logger.Logger.defaultLog = None

def install(dest_dir, prefix=None, dry_run=False):
    log.info('PETSc: install')
    if dry_run: return
    if prefix is None:
        prefix = dest_dir
    options = [
        '--destDir=' + dest_dir,
        '--prefix='  + prefix
        ]
    # Run PETSc installer
    import install
    install.Installer(options).run()
    import logger
    logger.Logger.defaultLog = None

class context:
    def __init__(self):
        self.sys_argv = sys.argv[:]
        self.wdir = os.getcwd()
    def enter(self):
        del sys.argv[1:]
        pdir = os.environ['PETSC_DIR']
        os.chdir(pdir)
        return self
    def exit(self):
        sys.argv[:] = self.sys_argv
        os.chdir(self.wdir)

class cmd_build(_build):

    def run(self):
        _build.run(self)
        ctx = context().enter()
        try:
            config(self.dry_run)
            build(self.dry_run)
        finally:
            ctx.exit()

class cmd_install(_install):

    def run(self):
        root_dir = self.install_platlib
        dest_dir = os.path.join(root_dir, 'petsc')
        bdist_base = self.get_finalized_command('bdist').bdist_base
        if dest_dir.startswith(bdist_base):
            prefix = dest_dir[len(bdist_base)+1:]
            prefix = prefix[prefix.index(os.path.sep):]
        else:
            prefix = dest_dir
        dest_dir = os.path.abspath(dest_dir)
        prefix   = os.path.abspath(prefix)
        #
        _install.run(self)
        ctx = context().enter()
        try:
            install(dest_dir, prefix, self.dry_run)
        finally:
            ctx.exit()

class cmd_sdist(_sdist):

    def initialize_options(self):
        _sdist.initialize_options(self)
        self.force_manifest = 1
        self.template = os.path.join('config', 'manifest.in')

def version():
    import re
    version_re = {
        'major'  : re.compile(r"#define\s+PETSC_VERSION_MAJOR\s+(\d+)"),
        'minor'  : re.compile(r"#define\s+PETSC_VERSION_MINOR\s+(\d+)"),
        'micro'  : re.compile(r"#define\s+PETSC_VERSION_SUBMINOR\s+(\d+)"),
        'patch'  : re.compile(r"#define\s+PETSC_VERSION_PATCH\s+(\d+)"),
        'release': re.compile(r"#define\s+PETSC_VERSION_RELEASE\s+(\d+)"),
        }
    petscversion_h = os.path.join('include','petscversion.h')
    data = open(petscversion_h, 'rt').read()
    major = int(version_re['major'].search(data).groups()[0])
    minor = int(version_re['minor'].search(data).groups()[0])
    micro = int(version_re['micro'].search(data).groups()[0])
    patch = int(version_re['patch'].search(data).groups()[0])
    release = int(version_re['release'].search(data).groups()[0])
    if release:
        v = "%d.%d" % (major, minor)
        if micro > 0:
            v += ".%d" % micro
        if patch > 0:
            v += ".post%d" % patch
    else:
        v = "%d.%d.dev%d" % (major, minor+1, 0)
    return v

def tarball():
    return None
    return ('http://ftp.mcs.anl.gov/pub/petsc/<XXX>/' # XXX fix this line
            'petsc-lite-%s.tar.gz' % version() )

description = __doc__.split('\n')[1:-1]; del description[1:3]
classifiers = """
License :: Public Domain
Operating System :: POSIX
Intended Audience :: Developers
Intended Audience :: Science/Research
Programming Language :: C
Programming Language :: C++
Programming Language :: Fortran
Programming Language :: Python
Topic :: Scientific/Engineering
Topic :: Software Development :: Libraries
"""

bootstrap()
setup(name='petsc',
      version=version(),
      description=description.pop(0),
      long_description='\n'.join(description),
      classifiers= classifiers.split('\n')[1:-1],
      keywords = ['PETSc', 'MPI'],
      platforms=['POSIX'],
      license='PETSc',

      url='http://www.mcs.anl.gov/petsc/',
      download_url=tarball(),

      author='PETSc Team',
      author_email='petsc-maint@mcs.anl.gov',
      maintainer='Lisandro Dalcin',
      maintainer_email='dalcinl@gmail.com',

      packages = ['petsc'],
      package_dir = {'petsc': PETSC_ARCH},
      cmdclass={
        'build': cmd_build,
        'install': cmd_install,
        'sdist': cmd_sdist,
        },
      **metadata)
