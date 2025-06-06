const char help[] = "Test memory allocation in DMPlex refinement.\n\n";

#include <petsc.h>

int main(int argc, char **argv)
{
  DM dm;

  PetscFunctionBeginUser;
  PetscCall(PetscInitialize(&argc, &argv, NULL, help));
  PetscCall(DMCreate(PETSC_COMM_WORLD, &dm));
  PetscCall(PetscObjectSetName((PetscObject)dm, "BaryDM"));
  PetscCall(DMSetType(dm, DMPLEX));
  PetscCall(DMSetFromOptions(dm));
  PetscCall(DMViewFromOptions(dm, NULL, "-dm_view"));
  //PetscCall(DMPlexSetRefinementUniform(dm, PETSC_TRUE));
  //PetscCall(DMRefine(dm, comm, &rdm));
  PetscCall(PetscObjectSetName((PetscObject)dm, "RefinedDM"));
  PetscCall(PetscObjectSetOptionsPrefix((PetscObject)dm, "ref_"));
  PetscCall(DMSetFromOptions(dm));
  PetscCall(DMViewFromOptions(dm, NULL, "-dm_view"));
  PetscCall(DMDestroy(&dm));
  PetscCall(PetscFinalize());
  return 0;
}

/*TEST

  test:
    requires: datafilespath hdf5 double !complex !defined(PETSC_USE_64BIT_INDICES)
    args: -dm_plex_filename ${DATAFILESPATH}/meshes/hdf5-petsc/petsc-v3.16.0/v1.0.0/barycentricallyrefinedcube.h5 -dm_view ascii::ASCII_INFO_DETAIL -ref_dm_refine 1 -ref_dm_view ascii::ASCII_INFO_DETAIL

TEST*/
