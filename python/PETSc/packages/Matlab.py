#!/usr/bin/env python
from __future__ import generators
import user
import config.base
import os
import PETSc.package

class Configure(PETSc.package.Package):
  def __init__(self, framework):
    PETSc.package.Package.__init__(self, framework)
    return

  def __str__(self):
    if self.found: return 'Matlab: Using '+self.matlab+'\n'
    return ''
    
  def setupHelp(self, help):
    import nargs
    help.addArgument('Matlab', '-with-matlab=<bool>',         nargs.ArgBool(None, 0, 'Activate Matlab'))
    help.addArgument('Matlab', '-with-matlab-dir=<root dir>', nargs.ArgDir(None, None, 'Specify the root directory of the Matlab installation'))
    return

  def generateGuesses(self):
    '''Generate list of possible locations of Matlab'''
    if 'with-matlab-dir' in self.framework.argDB:
      yield self.framework.argDB['with-matlab-dir']
      raise RuntimeError('You set a value for --with-matlab-dir, but '+self.framework.argDB['with-matlab-dir']+' cannot be used\n')
    if self.getExecutable('matlab', getFullPath = 1):
      # follow any symbolic link of this path
      self.matlab = os.path.realpath(self.matlab)
      yield os.path.dirname(os.path.dirname(self.matlab))
    return

  def configureLibrary(self):
    '''Find a Matlab installation and check if it can work with PETSc'''
    import re

    versionPattern = re.compile('Version ([0-9]*.[0-9]*)')
    for matlab in self.generateGuesses():
      interpreter = os.path.join(matlab,'bin','matlab')
      output      = ''
      try:
        output = config.base.Configure.executeShellCommand(interpreter+' -nojvm -nodisplay -r "[\'Version \' version]; exit"', log = self.framework.log)[0]
        match  = versionPattern.search(output)
        if not match:
          matlab = None
          continue
        r = float(match.group(1))
        if r < 6.0:
          self.framework.log.write('WARNING:Matlab version must be at least 6; yours is '+str(r)+'\n')
          self.framework.log.write('        Run with --with-matlab-dir=Matlabrootdir if you know where it is\n')
          matlab = None
        else:
          # make sure this is true root of Matlab
          if not os.path.isdir(os.path.join(matlab,'extern','lib')):
            self.framework.log.write('WARNING:'+matlab+' is not the root directory for Matlab\n')
            self.framework.log.write('        Run with --with-matlab-dir=Matlabrootdir if you know where it is\n')
            matlab = None
          else:
            # hope there is always only one arch installation in the location
            self.matlab      = matlab
            ls = os.listdir(os.path.join(matlab,'extern','lib'))
            if ls:
              matlab_arch = ls[0]

              self.framework.log.write('Configuring PETSc to use the Matlab at '+matlab+' Matlab arch '+matlab_arch+'\n')
              self.mex = os.path.join(matlab,'bin','mex')
              self.cc = '${CC}'
              self.command = os.path.join(matlab,'bin','matlab')
              self.include = [os.path.join(matlab,'extern','include')]
              if matlab_arch == 'mac':
                matlab_dl = [' -L'+os.path.join(matlab,'sys','os','mac'),' -ldl']
              else:
                matlab_dl = ['']
              # Matlab libraries require libstdc++-libc6.1-2.so.3 which they provide in the sys/os directory
              if matlab_arch == 'glnx86':
                matlab_sys = ':'+os.path.join(matlab,'sys','os',matlab_arch)
              else:
                matlab_sys = ''
              self.lib = [self.setCompilers.CSharedLinkerFlag+os.path.join(matlab,'extern','lib',matlab_arch)+matlab_sys,'-L'+os.path.join(matlab,'extern','lib',matlab_arch),'-L'+os.path.join(matlab,'bin',matlab_arch),'-leng','-lmx','-lmat','-lut'] + matlab_dl
              self.framework.packages.append(self)
              self.addMakeMacro('MATLAB_MEX',self.mex)
              self.addMakeMacro('MATLAB_CC',self.cc)
              self.addMakeMacro('MATLAB_COMMAND',self.command)        
              self.found = 1
              return
            self.framework.log.write('WARNING:Unable to use Matlab because cannot locate Matlab engine at '+os.path.join(matlab,'extern','lib')+'\n')
      except RuntimeError:
        self.framework.log.write('WARNING: Found Matlab at '+matlab+' but unable to run\n')
        self.framework.log.write(output)
        self.framework.log.write('        Run with --with-matlab-dir=Matlabrootdir if you know where it is\n')
        matlab = None
      raise RuntimeError('Could not find a functional '+self.name+'\n')
    return

if __name__ == '__main__':
  import config.framework
  import sys
  framework = config.framework.Framework(sys.argv[1:])
  framework.setupLogging()
  framework.children.append(Configure(framework))
  framework.configure()
  framework.dumpSubstitutions()
