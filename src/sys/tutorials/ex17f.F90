! Demonstrates PetscGetVersionNumber(): Fortran Example

program main
#include <petsc/finclude/petscsys.h>
      use petscsys

      implicit none
      PetscErrorCode                    :: ierr
      PetscInt                          :: major,minor,subminor,release
      character(len=PETSC_MAX_PATH_LEN) :: outputString

      ! Every PETSc routine should begin with the PetscInitialize() routine.

      PetscCallA(PetscInitialize(ierr))
      PetscCallA(PetscGetVersionNumber(major,minor,subminor,release,ierr))

      if (major /= PETSC_VERSION_MAJOR) then
        write(outputString,*)'Library major',major,'does not equal include',PETSC_VERSION_MAJOR
        SETERRA(PETSC_COMM_SELF,PETSC_ERR_PLIB,trim(outputString))
      endif

      if (minor /= PETSC_VERSION_MINOR) then
        write(outputString,*)'Library minor',minor,'does not equal include',PETSC_VERSION_MINOR
        SETERRA(PETSC_COMM_SELF,PETSC_ERR_PLIB,trim(outputString))
      endif

      if (subminor /= PETSC_VERSION_SUBMINOR) then
        write(outputString,*)'Library subminor',subminor,'does not equal include',PETSC_VERSION_SUBMINOR
        SETERRA(PETSC_COMM_SELF,PETSC_ERR_PLIB,trim(outputString))
      endif

      PetscCallA(PetscFinalize(ierr))
end program main

!/*TEST
!
!   test:
!     output_file: output/empty.out
!
!TEST*/
