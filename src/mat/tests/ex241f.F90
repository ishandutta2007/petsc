!     Test code contributed by Thibaut Appel <t.appel17@imperial.ac.uk>

  program test_assembly

#include <petsc/finclude/petscmat.h>

  use PetscMat
  use ISO_Fortran_Env, only : real64

  implicit none
  PetscInt,    parameter :: wp = real64, n = 10
  PetscScalar, parameter :: zero = 0.0, one = 1.0
  Mat      :: L
  PetscInt :: istart, iend, row, i1, i0
  PetscErrorCode :: ierr

  PetscInt    cols(1),rows(1)
  PetscScalar  vals(1)

  PetscCallA(PetscInitialize(ierr))

  i0 = 0
  i1 = 1

  PetscCallA(MatCreate(PETSC_COMM_WORLD,L,ierr))
  PetscCallA(MatSetType(L,MATAIJ,ierr))
  PetscCallA(MatSetSizes(L,PETSC_DECIDE,PETSC_DECIDE,n,n,ierr))

  PetscCallA(MatSeqAIJSetPreallocation(L,i1,PETSC_NULL_INTEGER_ARRAY,ierr))
  PetscCallA(MatMPIAIJSetPreallocation(L,i1,PETSC_NULL_INTEGER_ARRAY,i0,PETSC_NULL_INTEGER_ARRAY,ierr)) ! No allocated non-zero in off-diagonal part
  PetscCallA(MatSetOption(L,MAT_IGNORE_ZERO_ENTRIES,PETSC_TRUE,ierr))
  PetscCallA(MatSetOption(L,MAT_NEW_NONZERO_ALLOCATION_ERR,PETSC_TRUE,ierr))
  PetscCallA(MatSetOption(L,MAT_NO_OFF_PROC_ENTRIES,PETSC_TRUE,ierr))

  PetscCallA(MatGetOwnershipRange(L,istart,iend,ierr))

  ! assembling a diagonal matrix
  do row = istart,iend-1

    cols = [row]; vals = [one]; rows = [row]
    PetscCallA(MatSetValues(L,i1,rows,i1,cols,vals,ADD_VALUES,ierr))

  end do

  PetscCallA(MatAssemblyBegin(L,MAT_FINAL_ASSEMBLY,ierr))
  PetscCallA(MatAssemblyEnd(L,MAT_FINAL_ASSEMBLY,ierr))

  PetscCallA(MatSetOption(L,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE,ierr))

  !PetscCallA(MatZeroEntries(L,ierr))

  ! assembling a diagonal matrix, adding a zero value to non-diagonal part
  do row = istart,iend-1

    if (row == 0) then
      cols = [n-1]
      vals = [zero]
      rows = [row]
      PetscCallA(MatSetValues(L,i1,rows,i1,cols,vals,ADD_VALUES,ierr))
    end if
    cols = [row]; vals = [one] ; rows = [ row]
    PetscCallA(MatSetValues(L,i1,rows,i1,cols,vals,ADD_VALUES,ierr))

  end do

  PetscCallA(MatAssemblyBegin(L,MAT_FINAL_ASSEMBLY,ierr))
  PetscCallA(MatAssemblyEnd(L,MAT_FINAL_ASSEMBLY,ierr))
  PetscCallA(MatDestroy(L,ierr))

  PetscCallA(PetscFinalize(ierr))

end program test_assembly

!/*TEST
!
!   build:
!      requires: complex
!
!   test:
!      nsize: 2
!      output_file: output/empty.out
!
!TEST*/
