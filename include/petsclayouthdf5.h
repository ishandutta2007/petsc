#pragma once

#include <petscviewerhdf5.h>
#include <petscis.h>

/* MANSEC = Vec */

#if defined(PETSC_HAVE_HDF5)
  #include <H5Ipublic.h>
PETSC_EXTERN PetscErrorCode PetscViewerHDF5ReadSizes(PetscViewer, const char[], PetscInt *, PetscInt *);
PETSC_EXTERN PetscErrorCode PetscViewerHDF5Load(PetscViewer, const char *, PetscLayout, hid_t, void **);
#endif
