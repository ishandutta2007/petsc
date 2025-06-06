import config.package
import os

class Configure(config.package.CMakePackage):
  def __init__(self, framework):
    config.package.CMakePackage.__init__(self, framework)
    self.version          = '7.0.1'
    self.minversion       = '5.2.1' # bugs in 5.2.0 prevent it from functioning
    self.versionname      = 'SUPERLU_MAJOR_VERSION.SUPERLU_MINOR_VERSION.SUPERLU_PATCH_VERSION'
    #self.gitcommit        = 'v'+self.version
    self.gitcommit        = 'b814567e6cdeba61edffe45839a4043a46215c7a' # master feb-26-2024
    self.download         = ['git://https://github.com/xiaoyeli/superlu','https://github.com/xiaoyeli/superlu/archive/'+self.gitcommit+'.tar.gz']
    self.functions        = ['set_default_options']
    self.includes         = ['slu_ddefs.h']
    self.liblist          = [['libsuperlu.a']]
    # SuperLU has NO support for 64-bit integers, use SuperLU_DIST if you need that
    self.requires32bitint = 1;  # 1 means that the package will not work with 64-bit integers
    self.excludedDirs     = ['superlu_dist','superlu_mt']
    # SuperLU does not work with --download-fblaslapack with Compaqf90 compiler on windows.
    # However it should work with intel ifort.
    self.downloadonWindows= 1
    self.hastests         = 1
    self.hastestsdatafiles= 1
    self.precisions       = ['single','double']
    return

  def setupDependencies(self, framework):
    config.package.CMakePackage.setupDependencies(self, framework)
    self.blasLapack = self.framework.require('config.packages.BlasLapack',self)
    self.deps       = [self.blasLapack]
    return

  def formCMakeConfigureArgs(self):
    args = config.package.CMakePackage.formCMakeConfigureArgs(self)
    args.append('-DUSE_XSDK_DEFAULTS=YES')
    args.append('-DCMAKE_DISABLE_FIND_PACKAGE_Doxygen=TRUE')
    args.append('-DTPL_BLAS_LIBRARIES="'+self.libraries.toString(self.blasLapack.dlib)+'"')

    #  Tests are broken on Apple since they depend on a shared library that is not resolved against BLAS
    args.append('-Denable_tests=0')
    args.append('-Denable_examples=0')

    if not hasattr(self.compilers, 'FC'):
      args.append('-DXSDK_ENABLE_Fortran=OFF')

    # Add in fortran mangling flag
    if self.blasLapack.mangling == 'underscore':
      mangledef = '-DAdd_'
    elif self.blasLapack.mangling == 'caps':
      mangledef = '-DUpCase'
    else:
      mangledef = '-DNoChange'
    for place,item in enumerate(args):
      if item.find('CMAKE_C_FLAGS') >= 0 or item.find('CMAKE_CXX_FLAGS') >= 0:
        args[place]=item[:-1]+' '+mangledef+'"'

    return args

