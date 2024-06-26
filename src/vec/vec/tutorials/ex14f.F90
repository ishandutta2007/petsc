!
!
! Description: Illustrates the use of VecCreateGhost()
!
!
!      Ghost padding is one way to handle local calculations that
!      involve values from other processors. VecCreateGhostBlock() provides
!      a way to create vectors with extra room at the end of the vector
!      array to contain the needed ghost values from other processors,
!      vector computations are otherwise unaffected.
!

      program main
#include <petsc/finclude/petscvec.h>
      use petscvec
      implicit none

      PetscMPIInt size,rank
      PetscInt nlocal,nghost,ifrom(2)
      PetscInt i,rstart,rend,bs,ione
      PetscBool       flag
      PetscErrorCode ierr
      PetscScalar  value,tarray(20)
      Vec          lx,gx,gxs
      PetscViewer singleton

      nlocal = 6
      nghost = 2
      bs     = 2
      nlocal = bs*nlocal

      PetscCallA(PetscInitialize(ierr))
      PetscCallMPIA(MPI_Comm_rank(PETSC_COMM_WORLD,rank,ierr))
      PetscCallMPIA(MPI_Comm_size(PETSC_COMM_WORLD,size,ierr))

      PetscCheckA(size .eq. 2,PETSC_COMM_WORLD,PETSC_ERR_WRONG_MPI_SIZE,'Requires 2 processors')

!
!     Construct a two dimensional graph connecting nlocal degrees of
!     freedom per processor. From this we will generate the global
!     indices of needed ghost values
!
!     For simplicity we generate the entire graph on each processor:
!     in real application the graph would stored in parallel, but this
!     example is only to demonstrate the management of ghost padding
!     with VecCreateGhost().
!
!     In this example we consider the vector as representing
!     degrees of freedom in a one dimensional grid with periodic
!     boundary conditions.
!
!        ----Processor  1-----------  ----Processor 2 --------
!         0 1 2 3 4 5 6 7 8 9 10 11    12 13 14 15 16 17 18 19 20 21 22 23
!                              |--|----|---|
!         |-|--------------------------------------------------------|--|
!

      if (rank .eq. 0) then
        ifrom(1) = 11
        ifrom(2) = 6
      else
        ifrom(1) = 0
        ifrom(2) = 5
      endif

!     Create the vector with two slots for ghost points. Note that both
!     the local vector (lx) and the global vector (gx) share the same
!     array for storing vector values.

      PetscCallA(PetscOptionsHasName(PETSC_NULL_OPTIONS,PETSC_NULL_CHARACTER,'-allocate',flag,ierr))
      if (flag) then
        PetscCallA(VecCreateGhostBlockWithArray(PETSC_COMM_WORLD,bs,nlocal,PETSC_DECIDE,nghost,ifrom,tarray,gxs,ierr))
      else
        PetscCallA(VecCreateGhostBlock(PETSC_COMM_WORLD,bs,nlocal,PETSC_DECIDE,nghost,ifrom,gxs,ierr))
      endif

!      Test VecDuplicate

      PetscCallA(VecDuplicate(gxs,gx,ierr))
      PetscCallA(VecDestroy(gxs,ierr))

!      Access the local Form

      PetscCallA(VecGhostGetLocalForm(gx,lx,ierr))

!     Set the values from 0 to 12 into the "global" vector

      PetscCallA(VecGetOwnershipRange(gx,rstart,rend,ierr))

      ione = 1
      do 10, i=rstart,rend-1
        value = i
        PetscCallA(VecSetValues(gx,ione,[i],[value],INSERT_VALUES,ierr))
 10   continue

      PetscCallA(VecAssemblyBegin(gx,ierr))
      PetscCallA(VecAssemblyEnd(gx,ierr))

      PetscCallA(VecGhostUpdateBegin(gx,INSERT_VALUES,SCATTER_FORWARD,ierr))
      PetscCallA(VecGhostUpdateEnd(gx,INSERT_VALUES,SCATTER_FORWARD,ierr))

!     Print out each vector, including the ghost padding region.

      PetscCallA(PetscViewerGetSubViewer(PETSC_VIEWER_STDOUT_WORLD,PETSC_COMM_SELF,singleton,ierr))
      PetscCallA(VecView(lx,singleton,ierr))
      PetscCallA(PetscViewerRestoreSubViewer(PETSC_VIEWER_STDOUT_WORLD,PETSC_COMM_SELF,singleton,ierr))

      PetscCallA(VecGhostRestoreLocalForm(gx,lx,ierr))
      PetscCallA(VecDestroy(gx,ierr))
      PetscCallA(PetscFinalize(ierr))
      end

!/*TEST
!
!     test:
!       nsize: 2
!
!TEST*/
