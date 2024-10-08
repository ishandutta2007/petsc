#!/usr/bin/env python3

import os
petsc_hash_pkgs=os.path.join(os.getenv('HOME'),'petsc-hash-pkgs')

configure_options = [
  '--package-prefix-hash='+petsc_hash_pkgs,
  '--with-clanguage=cxx',
  '--with-debugging=0',

  #'--useThreads=0', # for some reason cmake hangs when invoked from configure on bsd?
  #'-download-fblaslapack=1',
  '--download-mpich=1',
  '--download-mpich-device=ch3:sock',
  '--download-cmake=1',
  '--download-metis=1',
  '--download-parmetis=1',
  '--download-triangle=1',
  '--download-triangle-build-exec=1',
  '--download-superlu=1',
  '--download-superlu_dist=1',
  '--download-scalapack=1',
  '--download-mumps=1',
  '--download-elemental=1',
  '--download-hdf5',
  '--with-zlib=1',
  '--download-sundials2=1',
  '--download-hypre=1',
  '--download-suitesparse=1',
  '--download-make=1', # required by suitesparse
  '--download-chaco=1',
  '--download-spai=1',
  '--download-netcdf=1',
  #'--download-moab=1', # moab-5.5: OS freebsd12.1 is not supported # if needed, use https://web.cels.anl.gov/projects/petsc/download/externalpackages/moab-5.0.0.tar.gz
  '--download-saws',
  '--download-ks',
  '--download-codipack=1',
  '--download-adblaslapack=1',
  '--download-hpddm=1',
  '--with-strict-petscerrorcode',
  ]

if __name__ == '__main__':
  import sys,os
  sys.path.insert(0,os.path.abspath('config'))
  import configure
  configure.petsc_configure(configure_options)
