import config.package
import os

class Configure(config.package.GNUPackage):
  def __init__(self, framework):
    config.package.GNUPackage.__init__(self, framework)
    self.version           = '2.8.7'
    self.gitcommit         = 'v'+self.version
    self.download          = ['git://https://github.com/cburstedde/p4est','https://github.com/cburstedde/p4est/archive/'+self.gitcommit+'.tar.gz']
    self.versionname       = 'P4EST_VERSION_MAJOR.P4EST_VERSION_MINOR.P4EST_VERSION_POINT'
    self.versioninclude    = 'p4est_config.h'
    self.functions         = ['p4est_init']
    self.includes          = ['p4est_bits.h']
    self.liblist           = [['libp4est.a', 'libsc.a']]
    self.downloadonWindows = 1
    return

  def setupHelp(self,help):
    '''Default GNU setupHelp, but p4est debugging option'''
    config.package.GNUPackage.setupHelp(self,help)
    import nargs
    help.addArgument(self.PACKAGE,'-with-p4est-debugging=<bool>',nargs.ArgBool(None,0,"Use p4est's (sometimes computationally intensive) debugging"))
    return

  def setupDependencies(self, framework):
    config.package.GNUPackage.setupDependencies(self, framework)
    self.mpi        = framework.require('config.packages.MPI',self)
    self.blasLapack = framework.require('config.packages.BlasLapack',self)
    self.zlib       = framework.require('config.packages.zlib',self)
    self.memalign   = framework.argDB['with-memalign']
    self.deps       = [self.blasLapack,self.zlib]
    self.odeps      = [self.mpi]
    return

  def formGNUConfigureArgs(self):
    from shlex import quote
    args = config.package.GNUPackage.formGNUConfigureArgs(self)
    if self.argDB['with-p4est-debugging']:
      args.append('--enable-debug')
    if not self.mpi.usingMPIUni:
      args.append('--enable-mpi')
      if self.mpi.mpiexecExecutable:
        args.append('PATH='+quote(os.environ['PATH']+':'+os.path.dirname(self.mpi.mpiexecExecutable)))
    else:
      args.append('--disable-mpi')
    args.append('CPPFLAGS='+quote(self.headers.toStringNoDupes(self.dinclude)))
    args.append('LIBS='+quote(self.libraries.toString(self.dlib)))
    args.append('--enable-memalign='+self.memalign)
    return args

  def updateGitDir(self):
    config.package.GNUPackage.updateGitDir(self)
    if not hasattr(self.sourceControl, 'git') or (self.packageDir != os.path.join(self.externalPackagesDir,'git.'+self.package)):
      return
    Dir = self.getDir()
    try:
      libsc = self.libsc
    except AttributeError:
      try:
        self.executeShellCommand([self.sourceControl.git, 'submodule', 'update', '--init'], cwd=Dir, log=self.log)
        if os.path.isfile(os.path.join(Dir,'sc','README')):
          self.libsc = os.path.join(Dir,'sc')
        else:
          raise RuntimeError
      except RuntimeError:
        raise RuntimeError('Could not initialize sc submodule needed by p4est')
    return

  def preInstall(self):
    self.Bootstrap('./bootstrap')
