static const char help[] = "Test of CAD functionality";

#include <petscdmplex.h>

/* TODO
  - Fix IGES
  - Test tessellation using -dm_plex_egads_with_tess
*/

typedef struct {
  char      filename[PETSC_MAX_PATH_LEN];
  PetscBool volumeMesh;
} AppCtx;

static PetscErrorCode ProcessOptions(MPI_Comm comm, AppCtx *options)
{
  PetscFunctionBeginUser;
  options->filename[0] = '\0';
  options->volumeMesh  = PETSC_TRUE;

  PetscOptionsBegin(comm, "", "EGADSPlex Problem Options", "EGADSlite");
  PetscCall(PetscOptionsString("-filename", "The CAD file", "ex37.c", options->filename, options->filename, sizeof(options->filename), NULL));
  PetscCall(PetscOptionsBool("-volume_mesh", "Create a volume mesh", "ex37.c", options->volumeMesh, &options->volumeMesh, NULL));
  PetscOptionsEnd();
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode ComputeVolume(DM dm)
{
  PetscObject obj = (PetscObject)dm;
  DMLabel     bodyLabel, faceLabel, edgeLabel;
  double      surface = 0., volume = 0., vol;
  PetscInt    dim, pStart, pEnd, p, pid;
  const char *name;

  PetscFunctionBeginUser;
  PetscCall(DMGetDimension(dm, &dim));
  if (dim < 2) PetscFunctionReturn(PETSC_SUCCESS);
  PetscCall(DMGetLabel(dm, "EGADS Body ID", &bodyLabel));
  PetscCall(DMGetLabel(dm, "EGADS Face ID", &faceLabel));
  PetscCall(DMGetLabel(dm, "EGADS Edge ID", &edgeLabel));

  PetscCall(DMPlexGetHeightStratum(dm, 0, &pStart, &pEnd));
  for (p = pStart; p < pEnd; ++p) {
    PetscCall(DMLabelGetValue(dim == 2 ? faceLabel : bodyLabel, p, &pid));
    if (pid >= 0) {
      PetscCall(DMPlexComputeCellGeometryFVM(dm, p, &vol, NULL, NULL));
      volume += vol;
    }
  }
  PetscCall(DMPlexGetHeightStratum(dm, 1, &pStart, &pEnd));
  for (p = pStart; p < pEnd; ++p) {
    PetscCall(DMLabelGetValue(dim == 2 ? edgeLabel : faceLabel, p, &pid));
    if (pid >= 0) {
      PetscCall(DMPlexComputeCellGeometryFVM(dm, p, &vol, NULL, NULL));
      surface += vol;
    }
  }

  PetscCall(PetscObjectGetName(obj, &name));
  PetscCall(PetscPrintf(PetscObjectComm(obj), "DM %s: Surface Area = %.6e Volume = %.6e\n", name ? name : "", surface, volume));
  PetscFunctionReturn(PETSC_SUCCESS);
}

int main(int argc, char *argv[])
{
  DM     surface, dm;
  AppCtx ctx;

  PetscFunctionBeginUser;
  PetscCall(PetscInitialize(&argc, &argv, NULL, help));
  PetscCall(ProcessOptions(PETSC_COMM_WORLD, &ctx));
  PetscCall(DMPlexCreateFromFile(PETSC_COMM_WORLD, ctx.filename, "ex37_plex", PETSC_TRUE, &surface));
  PetscCall(PetscObjectSetName((PetscObject)surface, "CAD Surface"));
  PetscCall(PetscObjectSetOptionsPrefix((PetscObject)surface, "sur_"));
  PetscCall(DMSetFromOptions(surface));
  PetscCall(DMViewFromOptions(surface, NULL, "-dm_view"));
  PetscCall(ComputeVolume(surface));

  if (ctx.volumeMesh) {
    PetscCall(DMPlexGenerate(surface, "tetgen", PETSC_TRUE, &dm));
    PetscCall(PetscObjectSetName((PetscObject)dm, "CAD Mesh"));
    PetscCall(DMPlexSetRefinementUniform(dm, PETSC_TRUE));
    PetscCall(DMViewFromOptions(dm, NULL, "-pre_dm_view"));

    PetscCall(DMPlexInflateToGeomModel(dm, PETSC_TRUE));
    PetscCall(DMViewFromOptions(dm, NULL, "-inf_dm_view"));

    PetscCall(DMSetFromOptions(dm));
    PetscCall(DMViewFromOptions(dm, NULL, "-dm_view"));
    PetscCall(ComputeVolume(dm));
    PetscCall(DMDestroy(&dm));
  }
  PetscCall(DMDestroy(&surface));
  PetscCall(PetscFinalize());
  return 0;
}

/*TEST

  build:
    requires: egads tetgen datafilespath

  test:
    suffix: sphere_0
    args: -filename ${DATAFILESPATH}/meshes/cad/sphere_example.egadslite -dm_refine 1 -sur_dm_view -dm_plex_check_all -dm_plex_geom_print_model -sur_dm_plex_view_labels "EGADS Body ID","EGADS Face ID","EGADS Edge ID"

  test:
    suffix: sphere_egads
    args: -filename ${DATAFILESPATH}/meshes/cad/sphere_example.egads -dm_refine 1 -sur_dm_view -dm_plex_check_all -dm_plex_geom_print_model -sur_dm_plex_view_labels "EGADS Body ID","EGADS Face ID","EGADS Edge ID"

  test:
    suffix: sphere_iges
    TODO: broken
    args: -filename ${DATAFILESPATH}/meshes/cad/sphere_example.igs -dm_refine 1 -sur_dm_view -dm_plex_check_all -dm_plex_geom_print_model -sur_dm_plex_view_labels "EGADS Body ID","EGADS Face ID","EGADS Edge ID"

  test:
    suffix: sphere_step
    args: -filename ${DATAFILESPATH}/meshes/cad/sphere_example.stp -dm_refine 1 -sur_dm_view -dm_plex_check_all -dm_plex_geom_print_model -sur_dm_plex_view_labels "EGADS Body ID","EGADS Face ID","EGADS Edge ID"

  test:
    suffix: nozzle_0
    args: -filename ${DATAFILESPATH}/meshes/cad/nozzle.egadslite -sur_dm_refine 1 -sur_dm_view -dm_plex_check_all

  test:
    suffix: nozzle_egads
    args: -filename ${DATAFILESPATH}/meshes/cad/nozzle.egads -sur_dm_refine 1 -sur_dm_view -dm_plex_check_all

  test:
    suffix: nozzle_iges
    args: -filename ${DATAFILESPATH}/meshes/cad/nozzle.igs -sur_dm_refine 1 -sur_dm_view -dm_plex_check_all

  test:
    suffix: nozzle_step
    args: -filename ${DATAFILESPATH}/meshes/cad/nozzle.stp -sur_dm_refine 1 -sur_dm_view -dm_plex_check_all

TEST*/
