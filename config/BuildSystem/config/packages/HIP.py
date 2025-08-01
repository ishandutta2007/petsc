# Portions of this code are under:
# Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.

import config.package
import os

class Configure(config.package.Package):
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)

    self.minversion       = '5.0.0'
    # Check version from rocm-core here, as HIP_VERSION_PATCH (e.g., 31061 from hip_version.h) is not necessarily the AMD advertised patch version, e.g., in 5.6.0
    self.versionname      = 'ROCM_VERSION_MAJOR.ROCM_VERSION_MINOR.ROCM_VERSION_PATCH'
    self.versioninclude   = ['rocm-core/rocm_version.h', 'rocm_version.h']
    self.requiresversion  = 1
    self.functions        = ['hipInit']
    self.includes         = ['hip/hip_runtime.h']
    self.includedir       = ['include']
    # PETSc does not use hipsparse or hipblas, but dependencies can (e.g., magma)
    self.liblist          = [['libhipsparse.a','libhipblas.a','libhipsolver.a','librocsparse.a','librocsolver.a','librocblas.a','librocrand.a','libamdhip64.a','libhsa-runtime64.a'],
                             ['hipsparse.lib','hipblas.lib','hipsolver.lib','rocsparse.lib','rocsolver.lib','rocblas.lib','rocrand.lib','amdhip64.lib','hsa-runtime64.lib'],]
    self.precisions       = ['single','double']
    self.buildLanguages   = ['HIP']
    self.devicePackage    = 1
    self.fullPathHIPC     = ''
    self.hasROCTX         = None
    self.unifiedMemory    = False
    self.skipMPIDependency= 1
    return

  def setupHelp(self, help):
    import nargs
    config.package.Package.setupHelp(self, help)
    help.addArgument('HIP', '-with-hip-arch', nargs.ArgString(None, None, 'AMD GPU architecture for code generation, for example gfx908, (this may be used by external packages)'))
    return

  def setupDependencies(self, framework):
    config.package.Package.setupDependencies(self, framework)
    self.setCompilers = framework.require('config.setCompilers',self)
    self.headers      = framework.require('config.headers',self)
    return

  def setHasROCTX(self):
    if self.hasROCTX is not None:
      return

    if not hasattr(self,'platform'):
      if 'HIP_PLATFORM' in os.environ:
        self.platform = os.environ['HIP_PLATFORM']
      elif hasattr(self,'systemNvcc'):
        self.platform = 'nvidia'
      else:
        self.platform = 'amd'

    if self.platform == 'amd':
      for prefix in self.getSearchDirectories():
        if not self.version_tuple:
          self.checkVersion()
        if self.version_tuple[0] >= 6 and self.version_tuple[1] >= 4:
          if os.path.exists(os.path.join(prefix, 'rocprofiler-sdk-roctx', 'roctx.h')):
            self.includes = ['hip/hip_runtime.h', 'rocprofiler-sdk-roctx/roctx.h']
            self.liblist[0] += ['librocprofiler-sdk-roctx.a']
            self.liblist[1] += ['rocprofiler-sdk-roctx.lib']
            self.hasROCTX = True
        elif self.version_tuple[0] >= 6:
          if os.path.exists(os.path.join(prefix, 'include', 'roctracer')):
            self.includes = ['hip/hip_runtime.h', 'roctracer/roctx.h']
            self.liblist[0] += ['libroctx64.a']
            self.liblist[1] += ['roctx64.lib']
            self.hasROCTX = True
        else:
          if os.path.exists(os.path.join(prefix, 'roctracer')):
            self.includes = ['hip/hip_runtime.h', 'roctx.h']
            self.includedir = ['include', os.path.join('roctracer', 'include')]
            self.liblist[0] += ['libroctx64.a']
            self.liblist[1] += ['roctx64.lib']
            self.hasROCTX = True

    if not self.hasROCTX:
      self.hasROCTX = False


  def __str__(self):
    output  = config.package.Package.__str__(self)
    if hasattr(self,'hipArch'):
      if self.unifiedMemory: uminfo = ' with unified memory'
      else: uminfo = ''
      output += '  HIP arch: '+ self.hipArch + uminfo + '\n'
    return output

  def checkSizeofVoidP(self):
    '''Checks if the HIPC compiler agrees with the C compiler on what size of void * should be'''
    self.log.write('Checking if sizeof(void*) in HIP is the same as with regular compiler\n')
    size = self.types.checkSizeof('void *', (8, 4), lang='HIP', save=False)
    if size != self.types.sizes['void-p']:
      raise RuntimeError('HIP Error: sizeof(void*) with HIP compiler is ' + str(size) + ' which differs from sizeof(void*) with C compiler')
    return

  def configureTypes(self):
    import config.setCompilers
    if not self.getDefaultPrecision() in ['double', 'single']:
      raise RuntimeError('Must use either single or double precision with HIP')
    self.checkSizeofVoidP()
    return

  def checkHIPCDoubleAlign(self):
    if 'known-hip-align-double' in self.argDB:
      if not self.argDB['known-hip-align-double']:
        raise RuntimeError('HIP error: PETSc currently requires that HIP double alignment match the C compiler')
    else:
      typedef = 'typedef struct {double a; int b;} teststruct;\n'
      hip_size = self.types.checkSizeof('teststruct', (16, 12), lang='HIP', codeBegin=typedef, save=False)
      c_size = self.types.checkSizeof('teststruct', (16, 12), lang='C', codeBegin=typedef, save=False)
      if c_size != hip_size:
        raise RuntimeError('HIP compiler error: memory alignment doesn\'t match C compiler (try adding -malign-double to compiler options)')
    return

  def setFullPathHIPC(self):
    self.pushLanguage('HIP')
    HIPC = self.getCompiler()
    self.popLanguage()
    self.getExecutable(HIPC,getFullPath=1,resultName='fullPathHIPC')
    if not hasattr(self,'fullPathHIPC'):
      raise RuntimeError('Unable to locate the HIPC compiler')

  def getSearchDirectories(self):
    # Package.getSearchDirectories() return '' by default, so that HIPC's default include path could
    # be checked. But here we lower priority of '', so that once we validated a header path, it will
    # be added to HIP_INCLUDE.  Other compilers, ex. CC or CXX, might need this path for compilation.
    yield os.path.dirname(os.path.dirname(self.fullPathHIPC)) # yield /opt/rocm from /opt/rocm/bin/hipcc
    yield ''

  def configureLibrary(self):
    self.setFullPathHIPC()
    self.setHasROCTX()
    config.package.Package.configureLibrary(self)
    self.getExecutable('hipconfig',getFullPath=1,resultName='hip_config')
    if hasattr(self,'hip_config'):
      try:
        self.platform = config.package.Package.executeShellCommand([self.hip_config,'--platform'],log=self.log)[0]
      except RuntimeError:
        pass

    # Handle the platform issues
    if not hasattr(self,'platform'):
      if 'HIP_PLATFORM' in os.environ:
        self.platform = os.environ['HIP_PLATFORM']
      elif hasattr(self,'systemNvcc'):
        self.platform = 'nvidia'
      else:
        self.platform = 'amd'

    self.libraries.pushLanguage('HIP')
    self.addDefine('HAVE_HIP','1')
    self.addDefine('HAVE_CUPM','1') # Have either CUDA or HIP
    if self.platform in ['nvcc','nvidia']:
      self.pushLanguage('CUDA')
      petscNvcc = self.getCompiler()
      cudaFlags = self.getCompilerFlags()
      self.popLanguage()
      self.getExecutable(petscNvcc,getFullPath=1,resultName='systemNvcc')
      if hasattr(self,'systemNvcc'):
        nvccDir = os.path.dirname(self.systemNvcc)
        cudaDir = os.path.split(nvccDir)[0]
      else:
        raise RuntimeError('Unable to locate CUDA NVCC compiler')
      self.includedir = ['include',os.path.join(cudaDir,'include')]
      self.delDefine('HAVE_CUDA')
      self.addDefine('HAVE_HIPCUDA',1)
      self.framework.addDefine('__HIP_PLATFORM_NVCC__',1) # deprecated from 4.3.0
      self.framework.addDefine('__HIP_PLATFORM_NVIDIA__',1)
    else:
      self.addDefine('HAVE_HIPROCM',1)
      if self.hasROCTX:
        self.framework.addDefine('HAVE_ROCTX',1)
      self.framework.addDefine('__HIP_PLATFORM_HCC__',1) # deprecated from 4.3.0
      self.framework.addDefine('__HIP_PLATFORM_AMD__',1)
      if 'with-hip-arch' in self.framework.clArgDB:
        if self.argDB['with-hip-arch'].split('_')[-1].lower() == 'apu': # take care of user input gfx942_apu and the like
          self.unifiedMemory = True
        self.hipArch = self.argDB['with-hip-arch'].split('_')[0]
      else:
        self.getExecutable('rocminfo',getFullPath=1)
        if hasattr(self,'rocminfo'):
          try:
            (out, err, ret) = Configure.executeShellCommand(self.rocminfo + ' | grep -e "AMD Instinct MI300A " -e " gfx" ',timeout = 60, log = self.log, threads = 1)
          except Exception as e:
            self.log.write('ROCM utility ' + self.rocminfo + ' failed: '+str(e)+'\n')
          else:
            try:
              s = set([i for i in out.split() if 'gfx' in i])
              self.hipArch = list(s)[0]
              uminfo = ''
              if 'AMD Instinct MI300A' in out:
                self.unifiedMemory = True
                uminfo = ' with unified memory'
              self.log.write('ROCM utility ' + self.rocminfo + ' said the HIP arch is ' + self.hipArch + uminfo + '\n')
            except:
              self.log.write('Unable to parse the ROCM utility ' + self.rocminfo + '\n')
      if hasattr(self,'hipArch'):
        self.hipArch.lower() # to have a uniform format even if user set hip arch in weird cases
        if not self.hipArch.startswith('gfx'):
          raise RuntimeError('HIP arch name ' + self.hipArch + ' is not in the supported gfxnnn or gfxnnn_apu format')
        # See https://rocm.docs.amd.com/en/latest/reference/gpu-arch-specs.html, even for MI300A, the LLVM target name is still gfx942
        self.setCompilers.HIPFLAGS += ' --offload-arch=' + self.hipArch +' '
      else:
        raise RuntimeError('You must set --with-hip-arch=gfx942_apu, gfx942, gfx900, gfx906, gfx908, gfx90a etc or make ROCM utility "rocminfo" available on your PATH')

      # Record rocBlas and rocSparse directories as they are needed by Kokkos-Kernels HIP TPL, so that we can handle
      # a weird (but valid) case --with-hipcc=/opt/rocm-4.5.2/hip/bin/hipcc --with-hip-dir=/opt/rocm-4.5.2 (which
      # should be better written as --with-hipcc=/opt/rocm-4.5.2/bin/hipcc --with-hip-dir=/opt/rocm-4.5.2 or simply
      # --with-hip-dir=/opt/rocm-4.5.2)
      if self.directory:
        self.hipDir = self.directory
      else: # directory is '', indicating we are using the compiler's default, so the last resort is to guess the dir from hipcc
        self.hipDir = os.path.dirname(os.path.dirname(self.fullPathHIPC)) # Ex. peel /opt/rocm-4.5.2/bin/hipcc twice

      self.rocBlasDir   = self.hipDir
      self.rocSparseDir = self.hipDir
    #self.checkHIPDoubleAlign()
    self.configureTypes()
    self.libraries.popLanguage()
    return
