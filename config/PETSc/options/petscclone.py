import config.base
import os
import re

def noCheck(command, status, output, error):
  ''' Do no check result'''
  return

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    return

  def setupDependencies(self, framework):
    self.sourceControl = framework.require('config.sourceControl',self)
    self.petscdir = framework.require('PETSc.options.petscdir', self)
    return

  def configureInstallationMethod(self):
    '''Determine if PETSc was obtained via git or a tarball'''
    if os.path.exists(os.path.join(self.petscdir.dir,'lib','petsc','bin','maint')):
      self.logPrint('lib/petsc/bin/maint exists. This appears to be a repository clone')
      if os.path.exists(os.path.join(self.petscdir.dir, '.git')):
        self.logPrint('.git directory exists')
        if hasattr(self.sourceControl,'git'):
          (o1, e1, s1) = self.executeShellCommand([self.sourceControl.git, 'describe', '--match=v*'],checkCommand = noCheck, log = self.log, cwd=self.petscdir.dir)
          (o2, e2, s2) = self.executeShellCommand([self.sourceControl.git, 'log', '-1', '--pretty=format:%H'],checkCommand = noCheck, log = self.log, cwd=self.petscdir.dir)
          (o3, e3, s3) = self.executeShellCommand([self.sourceControl.git, 'log', '-1', '--pretty=format:%ci'],checkCommand = noCheck, log = self.log, cwd=self.petscdir.dir)
          (o4, e4, s4) = self.executeShellCommand([self.sourceControl.git, 'rev-parse', '--abbrev-ref', 'HEAD'],checkCommand = noCheck, log = self.log, cwd=self.petscdir.dir)
          (o5, e5, s5) = self.executeShellCommand([self.sourceControl.git, 'status', '--short', '-uno'],checkCommand = noCheck, log = self.log, cwd=self.petscdir.dir)
          if s2 or s3 or s4:
            self.logPrintWarning('Git branch check is giving errors! Check configure.log for output from "git status"')
            self.logPrint(e5)
          else:
            if not o1: o1 = o2
            self.addDefine('VERSION_GIT','"'+o1+'"')
            self.addDefine('VERSION_DATE_GIT','"'+o3+'"')
            self.addDefine('VERSION_BRANCH_GIT','"'+o4+'"')
        else:
          self.logPrintWarning('PETSC_DIR appears to be a Git clone - but git is not found in PATH')
      else:
        self.logPrint('This repository clone is obtained as a tarball as no .git dirs exist')
    else:
      if os.path.exists(os.path.join(self.petscdir.dir, '.git')):
        raise RuntimeError('Your PETSc source tree is broken. Use "git status" to check, or remove the entire directory and start all over again')
      else:
        self.logPrint('This is a tarball installation')
    return

  def configure(self):
    self.executeTest(self.configureInstallationMethod)
    return
