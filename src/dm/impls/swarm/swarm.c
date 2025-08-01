#include "petscdmswarm.h"
#define PETSCDM_DLL
#include <petsc/private/dmswarmimpl.h> /*I   "petscdmswarm.h"   I*/
#include <petsc/private/hashsetij.h>
#include <petsc/private/petscfeimpl.h>
#include <petscviewer.h>
#include <petscdraw.h>
#include <petscdmplex.h>
#include <petscblaslapack.h>
#include "../src/dm/impls/swarm/data_bucket.h"
#include <petscdmlabel.h>
#include <petscsection.h>

PetscLogEvent DMSWARM_Migrate, DMSWARM_SetSizes, DMSWARM_AddPoints, DMSWARM_RemovePoints, DMSWARM_Sort;
PetscLogEvent DMSWARM_DataExchangerTopologySetup, DMSWARM_DataExchangerBegin, DMSWARM_DataExchangerEnd;
PetscLogEvent DMSWARM_DataExchangerSendCount, DMSWARM_DataExchangerPack;

const char *DMSwarmTypeNames[]          = {"basic", "pic", NULL};
const char *DMSwarmMigrateTypeNames[]   = {"basic", "dmcellnscatter", "dmcellexact", "user", NULL};
const char *DMSwarmCollectTypeNames[]   = {"basic", "boundingbox", "general", "user", NULL};
const char *DMSwarmRemapTypeNames[]     = {"none", "pfak", "colella", "DMSwarmRemapType", "DMSWARM_REMAP_", NULL};
const char *DMSwarmPICLayoutTypeNames[] = {"regular", "gauss", "subdivision", NULL};

const char DMSwarmField_pid[]     = "DMSwarm_pid";
const char DMSwarmField_rank[]    = "DMSwarm_rank";
const char DMSwarmPICField_coor[] = "DMSwarmPIC_coor";

PetscInt SwarmDataFieldId = -1;

#if defined(PETSC_HAVE_HDF5)
  #include <petscviewerhdf5.h>

static PetscErrorCode VecView_Swarm_HDF5_Internal(Vec v, PetscViewer viewer)
{
  DM        dm;
  PetscReal seqval;
  PetscInt  seqnum, bs;
  PetscBool isseq, ists;

  PetscFunctionBegin;
  PetscCall(VecGetDM(v, &dm));
  PetscCall(VecGetBlockSize(v, &bs));
  PetscCall(PetscViewerHDF5PushGroup(viewer, "/particle_fields"));
  PetscCall(PetscObjectTypeCompare((PetscObject)v, VECSEQ, &isseq));
  PetscCall(DMGetOutputSequenceNumber(dm, &seqnum, &seqval));
  PetscCall(PetscViewerHDF5IsTimestepping(viewer, &ists));
  if (ists) PetscCall(PetscViewerHDF5SetTimestep(viewer, seqnum));
  /* PetscCall(DMSequenceView_HDF5(dm, "time", seqnum, (PetscScalar) seqval, viewer)); */
  PetscCall(VecViewNative(v, viewer));
  PetscCall(PetscViewerHDF5WriteObjectAttribute(viewer, (PetscObject)v, "Nc", PETSC_INT, (void *)&bs));
  PetscCall(PetscViewerHDF5PopGroup(viewer));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmView_HDF5(DM dm, PetscViewer viewer)
{
  DMSwarmCellDM celldm;
  Vec           coordinates;
  PetscInt      Np, Nfc;
  PetscBool     isseq;
  const char  **coordFields;

  PetscFunctionBegin;
  PetscCall(DMSwarmGetCellDMActive(dm, &celldm));
  PetscCall(DMSwarmCellDMGetCoordinateFields(celldm, &Nfc, &coordFields));
  PetscCheck(Nfc == 1, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "We only support a single coordinate field right now, not %" PetscInt_FMT, Nfc);
  PetscCall(DMSwarmGetSize(dm, &Np));
  PetscCall(DMSwarmCreateGlobalVectorFromField(dm, coordFields[0], &coordinates));
  PetscCall(PetscObjectSetName((PetscObject)coordinates, "coordinates"));
  PetscCall(PetscViewerHDF5PushGroup(viewer, "/particles"));
  PetscCall(PetscObjectTypeCompare((PetscObject)coordinates, VECSEQ, &isseq));
  PetscCall(VecViewNative(coordinates, viewer));
  PetscCall(PetscViewerHDF5WriteObjectAttribute(viewer, (PetscObject)coordinates, "Np", PETSC_INT, (void *)&Np));
  PetscCall(PetscViewerHDF5PopGroup(viewer));
  PetscCall(DMSwarmDestroyGlobalVectorFromField(dm, coordFields[0], &coordinates));
  PetscFunctionReturn(PETSC_SUCCESS);
}
#endif

static PetscErrorCode VecView_Swarm(Vec v, PetscViewer viewer)
{
  DM dm;
#if defined(PETSC_HAVE_HDF5)
  PetscBool ishdf5;
#endif

  PetscFunctionBegin;
  PetscCall(VecGetDM(v, &dm));
  PetscCheck(dm, PetscObjectComm((PetscObject)v), PETSC_ERR_ARG_WRONG, "Vector not generated from a DM");
#if defined(PETSC_HAVE_HDF5)
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERHDF5, &ishdf5));
  if (ishdf5) {
    PetscCall(VecView_Swarm_HDF5_Internal(v, viewer));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
#endif
  PetscCall(VecViewNative(v, viewer));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmVectorGetField - Gets the fields from which to define a `Vec` object
  when `DMCreateLocalVector()`, or `DMCreateGlobalVector()` is called

  Not collective

  Input Parameter:
. sw - a `DMSWARM`

  Output Parameters:
+ Nf         - the number of fields
- fieldnames - the textual name given to each registered field, or NULL if it has not been set

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmVectorDefineField()`, `DMSwarmRegisterPetscDatatypeField()`, `DMCreateGlobalVector()`, `DMCreateLocalVector()`
@*/
PetscErrorCode DMSwarmVectorGetField(DM sw, PetscInt *Nf, const char **fieldnames[])
{
  DMSwarmCellDM celldm;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(sw, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
  PetscCall(DMSwarmCellDMGetFields(celldm, Nf, fieldnames));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmVectorDefineField - Sets the field from which to define a `Vec` object
  when `DMCreateLocalVector()`, or `DMCreateGlobalVector()` is called

  Collective

  Input Parameters:
+ dm        - a `DMSWARM`
- fieldname - the textual name given to each registered field

  Level: beginner

  Notes:
  The field with name `fieldname` must be defined as having a data type of `PetscScalar`.

  This function must be called prior to calling `DMCreateLocalVector()`, `DMCreateGlobalVector()`.
  Multiple calls to `DMSwarmVectorDefineField()` are permitted.

.seealso: `DM`, `DMSWARM`, `DMSwarmVectorDefineFields()`, `DMSwarmVectorGetField()`, `DMSwarmRegisterPetscDatatypeField()`, `DMCreateGlobalVector()`, `DMCreateLocalVector()`
@*/
PetscErrorCode DMSwarmVectorDefineField(DM dm, const char fieldname[])
{
  PetscFunctionBegin;
  PetscCall(DMSwarmVectorDefineFields(dm, 1, &fieldname));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmVectorDefineFields - Sets the fields from which to define a `Vec` object
  when `DMCreateLocalVector()`, or `DMCreateGlobalVector()` is called

  Collective, No Fortran support

  Input Parameters:
+ sw         - a `DMSWARM`
. Nf         - the number of fields
- fieldnames - the textual name given to each registered field

  Level: beginner

  Notes:
  Each field with name in `fieldnames` must be defined as having a data type of `PetscScalar`.

  This function must be called prior to calling `DMCreateLocalVector()`, `DMCreateGlobalVector()`.
  Multiple calls to `DMSwarmVectorDefineField()` are permitted.

.seealso: `DM`, `DMSWARM`, `DMSwarmVectorDefineField()`, `DMSwarmVectorGetField()`, `DMSwarmRegisterPetscDatatypeField()`, `DMCreateGlobalVector()`, `DMCreateLocalVector()`
@*/
PetscErrorCode DMSwarmVectorDefineFields(DM sw, PetscInt Nf, const char *fieldnames[])
{
  DM_Swarm     *swarm = (DM_Swarm *)sw->data;
  DMSwarmCellDM celldm;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(sw, DM_CLASSID, 1, DMSWARM);
  if (fieldnames) PetscAssertPointer(fieldnames, 3);
  if (!swarm->issetup) PetscCall(DMSetUp(sw));
  PetscCheck(Nf >= 0, PetscObjectComm((PetscObject)sw), PETSC_ERR_ARG_OUTOFRANGE, "Number of fields must be non-negative, not %" PetscInt_FMT, Nf);
  // Create a dummy cell DM if none has been specified (I think we should not support this mode)
  if (!swarm->activeCellDM) {
    DM            dm;
    DMSwarmCellDM celldm;

    PetscCall(DMCreate(PetscObjectComm((PetscObject)sw), &dm));
    PetscCall(DMSetType(dm, DMSHELL));
    PetscCall(PetscObjectSetName((PetscObject)dm, "dummy"));
    PetscCall(DMSwarmCellDMCreate(dm, 0, NULL, 0, NULL, &celldm));
    PetscCall(DMDestroy(&dm));
    PetscCall(DMSwarmAddCellDM(sw, celldm));
    PetscCall(DMSwarmCellDMDestroy(&celldm));
    PetscCall(DMSwarmSetCellDMActive(sw, "dummy"));
  }
  PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
  for (PetscInt f = 0; f < celldm->Nf; ++f) PetscCall(PetscFree(celldm->dmFields[f]));
  PetscCall(PetscFree(celldm->dmFields));

  celldm->Nf = Nf;
  PetscCall(PetscMalloc1(Nf, &celldm->dmFields));
  for (PetscInt f = 0; f < Nf; ++f) {
    PetscDataType type;

    // Check all fields are of type PETSC_REAL or PETSC_SCALAR
    PetscCall(DMSwarmGetFieldInfo(sw, fieldnames[f], NULL, &type));
    PetscCheck(type == PETSC_REAL, PetscObjectComm((PetscObject)sw), PETSC_ERR_SUP, "Only valid for PETSC_REAL");
    PetscCall(PetscStrallocpy(fieldnames[f], (char **)&celldm->dmFields[f]));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* requires DMSwarmDefineFieldVector has been called */
static PetscErrorCode DMCreateGlobalVector_Swarm(DM sw, Vec *vec)
{
  DM_Swarm     *swarm = (DM_Swarm *)sw->data;
  DMSwarmCellDM celldm;
  Vec           x;
  char          name[PETSC_MAX_PATH_LEN];
  PetscInt      bs = 0, n;

  PetscFunctionBegin;
  if (!swarm->issetup) PetscCall(DMSetUp(sw));
  PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
  PetscCheck(celldm->Nf, PetscObjectComm((PetscObject)sw), PETSC_ERR_USER, "Active cell DM does not define any fields");
  PetscCall(DMSwarmDataBucketGetSizes(swarm->db, &n, NULL, NULL));

  PetscCall(PetscStrncpy(name, "DMSwarmField", PETSC_MAX_PATH_LEN));
  for (PetscInt f = 0; f < celldm->Nf; ++f) {
    PetscInt fbs;
    PetscCall(PetscStrlcat(name, "_", PETSC_MAX_PATH_LEN));
    PetscCall(PetscStrlcat(name, celldm->dmFields[f], PETSC_MAX_PATH_LEN));
    PetscCall(DMSwarmGetFieldInfo(sw, celldm->dmFields[f], &fbs, NULL));
    bs += fbs;
  }
  PetscCall(VecCreate(PetscObjectComm((PetscObject)sw), &x));
  PetscCall(PetscObjectSetName((PetscObject)x, name));
  PetscCall(VecSetSizes(x, n * bs, PETSC_DETERMINE));
  PetscCall(VecSetBlockSize(x, bs));
  PetscCall(VecSetDM(x, sw));
  PetscCall(VecSetFromOptions(x));
  PetscCall(VecSetOperation(x, VECOP_VIEW, (void (*)(void))VecView_Swarm));
  *vec = x;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* requires DMSwarmDefineFieldVector has been called */
static PetscErrorCode DMCreateLocalVector_Swarm(DM sw, Vec *vec)
{
  DM_Swarm     *swarm = (DM_Swarm *)sw->data;
  DMSwarmCellDM celldm;
  Vec           x;
  char          name[PETSC_MAX_PATH_LEN];
  PetscInt      bs = 0, n;

  PetscFunctionBegin;
  if (!swarm->issetup) PetscCall(DMSetUp(sw));
  PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
  PetscCheck(celldm->Nf, PetscObjectComm((PetscObject)sw), PETSC_ERR_USER, "Active cell DM does not define any fields");
  PetscCall(DMSwarmDataBucketGetSizes(swarm->db, &n, NULL, NULL));

  PetscCall(PetscStrncpy(name, "DMSwarmField", PETSC_MAX_PATH_LEN));
  for (PetscInt f = 0; f < celldm->Nf; ++f) {
    PetscInt fbs;
    PetscCall(PetscStrlcat(name, "_", PETSC_MAX_PATH_LEN));
    PetscCall(PetscStrlcat(name, celldm->dmFields[f], PETSC_MAX_PATH_LEN));
    PetscCall(DMSwarmGetFieldInfo(sw, celldm->dmFields[f], &fbs, NULL));
    bs += fbs;
  }
  PetscCall(VecCreate(PETSC_COMM_SELF, &x));
  PetscCall(PetscObjectSetName((PetscObject)x, name));
  PetscCall(VecSetSizes(x, n * bs, PETSC_DETERMINE));
  PetscCall(VecSetBlockSize(x, bs));
  PetscCall(VecSetDM(x, sw));
  PetscCall(VecSetFromOptions(x));
  *vec = x;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmDestroyVectorFromField_Private(DM dm, const char fieldname[], Vec *vec)
{
  DM_Swarm        *swarm = (DM_Swarm *)dm->data;
  DMSwarmDataField gfield;
  PetscInt         bs, nlocal, fid = -1, cfid = -2;
  PetscBool        flg;

  PetscFunctionBegin;
  /* check vector is an inplace array */
  PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldIdByName(swarm->db, fieldname, &fid));
  PetscCall(PetscObjectComposedDataGetInt((PetscObject)*vec, SwarmDataFieldId, cfid, flg));
  (void)flg; /* avoid compiler warning */
  PetscCheck(cfid == fid, PetscObjectComm((PetscObject)dm), PETSC_ERR_USER, "Vector being destroyed was not created from DMSwarm field(%s)! %" PetscInt_FMT " != %" PetscInt_FMT, fieldname, cfid, fid);
  PetscCall(VecGetLocalSize(*vec, &nlocal));
  PetscCall(VecGetBlockSize(*vec, &bs));
  PetscCheck(nlocal / bs == swarm->db->L, PetscObjectComm((PetscObject)dm), PETSC_ERR_USER, "DMSwarm sizes have changed since vector was created - cannot ensure pointers are valid");
  PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldByName(swarm->db, fieldname, &gfield));
  PetscCall(DMSwarmDataFieldRestoreAccess(gfield));
  PetscCall(VecResetArray(*vec));
  PetscCall(VecDestroy(vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmCreateVectorFromField_Private(DM dm, const char fieldname[], MPI_Comm comm, Vec *vec)
{
  DM_Swarm     *swarm = (DM_Swarm *)dm->data;
  PetscDataType type;
  PetscScalar  *array;
  PetscInt      bs, n, fid;
  char          name[PETSC_MAX_PATH_LEN];
  PetscMPIInt   size;
  PetscBool     iscuda, iskokkos, iship;

  PetscFunctionBegin;
  if (!swarm->issetup) PetscCall(DMSetUp(dm));
  PetscCall(DMSwarmDataBucketGetSizes(swarm->db, &n, NULL, NULL));
  PetscCall(DMSwarmGetField(dm, fieldname, &bs, &type, (void **)&array));
  PetscCheck(type == PETSC_REAL, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Only valid for PETSC_REAL");

  PetscCallMPI(MPI_Comm_size(comm, &size));
  PetscCall(PetscStrcmp(dm->vectype, VECKOKKOS, &iskokkos));
  PetscCall(PetscStrcmp(dm->vectype, VECCUDA, &iscuda));
  PetscCall(PetscStrcmp(dm->vectype, VECHIP, &iship));
  PetscCall(VecCreate(comm, vec));
  PetscCall(VecSetSizes(*vec, n * bs, PETSC_DETERMINE));
  PetscCall(VecSetBlockSize(*vec, bs));
  if (iskokkos) PetscCall(VecSetType(*vec, VECKOKKOS));
  else if (iscuda) PetscCall(VecSetType(*vec, VECCUDA));
  else if (iship) PetscCall(VecSetType(*vec, VECHIP));
  else PetscCall(VecSetType(*vec, VECSTANDARD));
  PetscCall(VecPlaceArray(*vec, array));

  PetscCall(PetscSNPrintf(name, PETSC_MAX_PATH_LEN - 1, "DMSwarmSharedField_%s", fieldname));
  PetscCall(PetscObjectSetName((PetscObject)*vec, name));

  /* Set guard */
  PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldIdByName(swarm->db, fieldname, &fid));
  PetscCall(PetscObjectComposedDataSetInt((PetscObject)*vec, SwarmDataFieldId, fid));

  PetscCall(VecSetDM(*vec, dm));
  PetscCall(VecSetOperation(*vec, VECOP_VIEW, (void (*)(void))VecView_Swarm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmDestroyVectorFromFields_Private(DM sw, PetscInt Nf, const char *fieldnames[], Vec *vec)
{
  DM_Swarm          *swarm = (DM_Swarm *)sw->data;
  const PetscScalar *array;
  PetscInt           bs, n, id = 0, cid = -2;
  PetscBool          flg;

  PetscFunctionBegin;
  for (PetscInt f = 0; f < Nf; ++f) {
    PetscInt fid;

    PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldIdByName(swarm->db, fieldnames[f], &fid));
    id += fid;
  }
  PetscCall(PetscObjectComposedDataGetInt((PetscObject)*vec, SwarmDataFieldId, cid, flg));
  (void)flg; /* avoid compiler warning */
  PetscCheck(cid == id, PetscObjectComm((PetscObject)sw), PETSC_ERR_USER, "Vector being destroyed was not created from DMSwarm field(%s)! %" PetscInt_FMT " != %" PetscInt_FMT, fieldnames[0], cid, id);
  PetscCall(VecGetLocalSize(*vec, &n));
  PetscCall(VecGetBlockSize(*vec, &bs));
  n /= bs;
  PetscCheck(n == swarm->db->L, PetscObjectComm((PetscObject)sw), PETSC_ERR_USER, "DMSwarm sizes have changed since vector was created - cannot ensure pointers are valid");
  PetscCall(VecGetArrayRead(*vec, &array));
  for (PetscInt f = 0, off = 0; f < Nf; ++f) {
    PetscScalar  *farray;
    PetscDataType ftype;
    PetscInt      fbs;

    PetscCall(DMSwarmGetField(sw, fieldnames[f], &fbs, &ftype, (void **)&farray));
    PetscCheck(off + fbs <= bs, PETSC_COMM_SELF, PETSC_ERR_PLIB, "Invalid blocksize %" PetscInt_FMT " < %" PetscInt_FMT, bs, off + fbs);
    for (PetscInt i = 0; i < n; ++i) {
      for (PetscInt b = 0; b < fbs; ++b) farray[i * fbs + b] = array[i * bs + off + b];
    }
    off += fbs;
    PetscCall(DMSwarmRestoreField(sw, fieldnames[f], &fbs, &ftype, (void **)&farray));
  }
  PetscCall(VecRestoreArrayRead(*vec, &array));
  PetscCall(VecDestroy(vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmCreateVectorFromFields_Private(DM sw, PetscInt Nf, const char *fieldnames[], MPI_Comm comm, Vec *vec)
{
  DM_Swarm    *swarm = (DM_Swarm *)sw->data;
  PetscScalar *array;
  PetscInt     n, bs = 0, id = 0;
  char         name[PETSC_MAX_PATH_LEN];

  PetscFunctionBegin;
  if (!swarm->issetup) PetscCall(DMSetUp(sw));
  PetscCall(DMSwarmDataBucketGetSizes(swarm->db, &n, NULL, NULL));
  for (PetscInt f = 0; f < Nf; ++f) {
    PetscDataType ftype;
    PetscInt      fbs;

    PetscCall(DMSwarmGetFieldInfo(sw, fieldnames[f], &fbs, &ftype));
    PetscCheck(ftype == PETSC_REAL, PetscObjectComm((PetscObject)sw), PETSC_ERR_SUP, "Only valid for PETSC_REAL");
    bs += fbs;
  }

  PetscCall(VecCreate(comm, vec));
  PetscCall(VecSetSizes(*vec, n * bs, PETSC_DETERMINE));
  PetscCall(VecSetBlockSize(*vec, bs));
  PetscCall(VecSetType(*vec, sw->vectype));

  PetscCall(VecGetArrayWrite(*vec, &array));
  for (PetscInt f = 0, off = 0; f < Nf; ++f) {
    PetscScalar  *farray;
    PetscDataType ftype;
    PetscInt      fbs;

    PetscCall(DMSwarmGetField(sw, fieldnames[f], &fbs, &ftype, (void **)&farray));
    for (PetscInt i = 0; i < n; ++i) {
      for (PetscInt b = 0; b < fbs; ++b) array[i * bs + off + b] = farray[i * fbs + b];
    }
    off += fbs;
    PetscCall(DMSwarmRestoreField(sw, fieldnames[f], &fbs, &ftype, (void **)&farray));
  }
  PetscCall(VecRestoreArrayWrite(*vec, &array));

  PetscCall(PetscStrncpy(name, "DMSwarmField", PETSC_MAX_PATH_LEN));
  for (PetscInt f = 0; f < Nf; ++f) {
    PetscCall(PetscStrlcat(name, "_", PETSC_MAX_PATH_LEN));
    PetscCall(PetscStrlcat(name, fieldnames[f], PETSC_MAX_PATH_LEN));
  }
  PetscCall(PetscObjectSetName((PetscObject)*vec, name));

  for (PetscInt f = 0; f < Nf; ++f) {
    PetscInt fid;

    PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldIdByName(swarm->db, fieldnames[f], &fid));
    id += fid;
  }
  PetscCall(PetscObjectComposedDataSetInt((PetscObject)*vec, SwarmDataFieldId, id));

  PetscCall(VecSetDM(*vec, sw));
  PetscCall(VecSetOperation(*vec, VECOP_VIEW, (void (*)(void))VecView_Swarm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* This creates a "mass matrix" between a finite element and particle space. If a finite element interpolant is given by

     \hat f = \sum_i f_i \phi_i

   and a particle function is given by

     f = \sum_i w_i \delta(x - x_i)

   then we want to require that

     M \hat f = M_p f

   where the particle mass matrix is given by

     (M_p)_{ij} = \int \phi_i \delta(x - x_j)

   The way Dave May does particles, they amount to quadratue weights rather than delta functions, so he has |J| is in
   his integral. We allow this with the boolean flag.
*/
static PetscErrorCode DMSwarmComputeMassMatrix_Private(DM dmc, DM dmf, Mat mass, PetscBool useDeltaFunction, void *ctx)
{
  const char   *name = "Mass Matrix";
  MPI_Comm      comm;
  DMSwarmCellDM celldm;
  PetscDS       prob;
  PetscSection  fsection, globalFSection;
  PetscHSetIJ   ht;
  PetscLayout   rLayout, colLayout;
  PetscInt     *dnz, *onz;
  PetscInt      locRows, locCols, rStart, colStart, colEnd, *rowIDXs;
  PetscReal    *xi, *v0, *J, *invJ, detJ = 1.0, v0ref[3] = {-1.0, -1.0, -1.0};
  PetscScalar  *elemMat;
  PetscInt      dim, Nf, Nfc, cStart, cEnd, totDim, maxC = 0, totNc = 0;
  const char  **coordFields;
  PetscReal   **coordVals;
  PetscInt     *bs;

  PetscFunctionBegin;
  PetscCall(PetscObjectGetComm((PetscObject)mass, &comm));
  PetscCall(DMGetCoordinateDim(dmf, &dim));
  PetscCall(DMGetDS(dmf, &prob));
  PetscCall(PetscDSGetNumFields(prob, &Nf));
  PetscCall(PetscDSGetTotalDimension(prob, &totDim));
  PetscCall(PetscMalloc3(dim, &v0, dim * dim, &J, dim * dim, &invJ));
  PetscCall(DMGetLocalSection(dmf, &fsection));
  PetscCall(DMGetGlobalSection(dmf, &globalFSection));
  PetscCall(DMPlexGetHeightStratum(dmf, 0, &cStart, &cEnd));
  PetscCall(MatGetLocalSize(mass, &locRows, &locCols));

  PetscCall(DMSwarmGetCellDMActive(dmc, &celldm));
  PetscCall(DMSwarmCellDMGetCoordinateFields(celldm, &Nfc, &coordFields));
  PetscCall(PetscMalloc2(Nfc, &coordVals, Nfc, &bs));

  PetscCall(PetscLayoutCreate(comm, &colLayout));
  PetscCall(PetscLayoutSetLocalSize(colLayout, locCols));
  PetscCall(PetscLayoutSetBlockSize(colLayout, 1));
  PetscCall(PetscLayoutSetUp(colLayout));
  PetscCall(PetscLayoutGetRange(colLayout, &colStart, &colEnd));
  PetscCall(PetscLayoutDestroy(&colLayout));

  PetscCall(PetscLayoutCreate(comm, &rLayout));
  PetscCall(PetscLayoutSetLocalSize(rLayout, locRows));
  PetscCall(PetscLayoutSetBlockSize(rLayout, 1));
  PetscCall(PetscLayoutSetUp(rLayout));
  PetscCall(PetscLayoutGetRange(rLayout, &rStart, NULL));
  PetscCall(PetscLayoutDestroy(&rLayout));

  PetscCall(PetscCalloc2(locRows, &dnz, locRows, &onz));
  PetscCall(PetscHSetIJCreate(&ht));

  PetscCall(PetscSynchronizedFlush(comm, NULL));
  for (PetscInt field = 0; field < Nf; ++field) {
    PetscObject  obj;
    PetscClassId id;
    PetscInt     Nc;

    PetscCall(PetscDSGetDiscretization(prob, field, &obj));
    PetscCall(PetscObjectGetClassId(obj, &id));
    if (id == PETSCFE_CLASSID) PetscCall(PetscFEGetNumComponents((PetscFE)obj, &Nc));
    else PetscCall(PetscFVGetNumComponents((PetscFV)obj, &Nc));
    totNc += Nc;
  }
  /* count non-zeros */
  PetscCall(DMSwarmSortGetAccess(dmc));
  for (PetscInt field = 0; field < Nf; ++field) {
    PetscObject  obj;
    PetscClassId id;
    PetscInt     Nc;

    PetscCall(PetscDSGetDiscretization(prob, field, &obj));
    PetscCall(PetscObjectGetClassId(obj, &id));
    if (id == PETSCFE_CLASSID) PetscCall(PetscFEGetNumComponents((PetscFE)obj, &Nc));
    else PetscCall(PetscFVGetNumComponents((PetscFV)obj, &Nc));

    for (PetscInt cell = cStart; cell < cEnd; ++cell) {
      PetscInt *findices, *cindices; /* fine is vertices, coarse is particles */
      PetscInt  numFIndices, numCIndices;

      PetscCall(DMPlexGetClosureIndices(dmf, fsection, globalFSection, cell, PETSC_FALSE, &numFIndices, &findices, NULL, NULL));
      PetscCall(DMSwarmSortGetPointsPerCell(dmc, cell, &numCIndices, &cindices));
      maxC = PetscMax(maxC, numCIndices);
      {
        PetscHashIJKey key;
        PetscBool      missing;
        for (PetscInt i = 0; i < numFIndices; ++i) {
          key.j = findices[i]; /* global column (from Plex) */
          if (key.j >= 0) {
            /* Get indices for coarse elements */
            for (PetscInt j = 0; j < numCIndices; ++j) {
              for (PetscInt c = 0; c < Nc; ++c) {
                // TODO Need field offset on particle here
                key.i = cindices[j] * totNc + c + rStart; /* global cols (from Swarm) */
                if (key.i < 0) continue;
                PetscCall(PetscHSetIJQueryAdd(ht, key, &missing));
                if (missing) {
                  if ((key.j >= colStart) && (key.j < colEnd)) ++dnz[key.i - rStart];
                  else ++onz[key.i - rStart];
                } else SETERRQ(PetscObjectComm((PetscObject)dmf), PETSC_ERR_SUP, "Set new value at %" PetscInt_FMT ",%" PetscInt_FMT, key.i, key.j);
              }
            }
          }
        }
        PetscCall(DMSwarmSortRestorePointsPerCell(dmc, cell, &numCIndices, &cindices));
      }
      PetscCall(DMPlexRestoreClosureIndices(dmf, fsection, globalFSection, cell, PETSC_FALSE, &numFIndices, &findices, NULL, NULL));
    }
  }
  PetscCall(PetscHSetIJDestroy(&ht));
  PetscCall(MatXAIJSetPreallocation(mass, 1, dnz, onz, NULL, NULL));
  PetscCall(MatSetOption(mass, MAT_NEW_NONZERO_ALLOCATION_ERR, PETSC_TRUE));
  PetscCall(PetscFree2(dnz, onz));
  PetscCall(PetscMalloc3(maxC * totNc * totDim, &elemMat, maxC * totNc, &rowIDXs, maxC * dim, &xi));
  for (PetscInt field = 0; field < Nf; ++field) {
    PetscTabulation Tcoarse;
    PetscObject     obj;
    PetscClassId    id;
    PetscInt        Nc;

    PetscCall(PetscDSGetDiscretization(prob, field, &obj));
    PetscCall(PetscObjectGetClassId(obj, &id));
    if (id == PETSCFE_CLASSID) PetscCall(PetscFEGetNumComponents((PetscFE)obj, &Nc));
    else PetscCall(PetscFVGetNumComponents((PetscFV)obj, &Nc));

    for (PetscInt i = 0; i < Nfc; ++i) PetscCall(DMSwarmGetField(dmc, coordFields[i], &bs[i], NULL, (void **)&coordVals[i]));
    for (PetscInt cell = cStart; cell < cEnd; ++cell) {
      PetscInt *findices, *cindices;
      PetscInt  numFIndices, numCIndices;

      /* TODO: Use DMField instead of assuming affine */
      PetscCall(DMPlexComputeCellGeometryFEM(dmf, cell, NULL, v0, J, invJ, &detJ));
      PetscCall(DMPlexGetClosureIndices(dmf, fsection, globalFSection, cell, PETSC_FALSE, &numFIndices, &findices, NULL, NULL));
      PetscCall(DMSwarmSortGetPointsPerCell(dmc, cell, &numCIndices, &cindices));
      for (PetscInt j = 0; j < numCIndices; ++j) {
        PetscReal xr[8];
        PetscInt  off = 0;

        for (PetscInt i = 0; i < Nfc; ++i) {
          for (PetscInt b = 0; b < bs[i]; ++b, ++off) xr[off] = coordVals[i][cindices[j] * bs[i] + b];
        }
        PetscCheck(off == dim, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "The total block size of coordinates is %" PetscInt_FMT " != %" PetscInt_FMT " the DM coordinate dimension", off, dim);
        CoordinatesRealToRef(dim, dim, v0ref, v0, invJ, xr, &xi[j * dim]);
      }
      if (id == PETSCFE_CLASSID) PetscCall(PetscFECreateTabulation((PetscFE)obj, 1, numCIndices, xi, 0, &Tcoarse));
      else PetscCall(PetscFVCreateTabulation((PetscFV)obj, 1, numCIndices, xi, 0, &Tcoarse));
      /* Get elemMat entries by multiplying by weight */
      PetscCall(PetscArrayzero(elemMat, numCIndices * Nc * totDim));
      for (PetscInt i = 0; i < numFIndices / Nc; ++i) {
        for (PetscInt j = 0; j < numCIndices; ++j) {
          for (PetscInt c = 0; c < Nc; ++c) {
            // TODO Need field offset on particle and field here
            /* B[(p*pdim + i)*Nc + c] is the value at point p for basis function i and component c */
            elemMat[(j * totNc + c) * numFIndices + i * Nc + c] += Tcoarse->T[0][(j * numFIndices + i * Nc + c) * Nc + c] * (useDeltaFunction ? 1.0 : detJ);
          }
        }
      }
      for (PetscInt j = 0; j < numCIndices; ++j)
        // TODO Need field offset on particle here
        for (PetscInt c = 0; c < Nc; ++c) rowIDXs[j * Nc + c] = cindices[j] * totNc + c + rStart;
      if (0) PetscCall(DMPrintCellMatrix(cell, name, numCIndices * Nc, numFIndices, elemMat));
      PetscCall(MatSetValues(mass, numCIndices * Nc, rowIDXs, numFIndices, findices, elemMat, ADD_VALUES));
      PetscCall(DMSwarmSortRestorePointsPerCell(dmc, cell, &numCIndices, &cindices));
      PetscCall(DMPlexRestoreClosureIndices(dmf, fsection, globalFSection, cell, PETSC_FALSE, &numFIndices, &findices, NULL, NULL));
      PetscCall(PetscTabulationDestroy(&Tcoarse));
    }
    for (PetscInt i = 0; i < Nfc; ++i) PetscCall(DMSwarmRestoreField(dmc, coordFields[i], &bs[i], NULL, (void **)&coordVals[i]));
  }
  PetscCall(PetscFree3(elemMat, rowIDXs, xi));
  PetscCall(DMSwarmSortRestoreAccess(dmc));
  PetscCall(PetscFree3(v0, J, invJ));
  PetscCall(PetscFree2(coordVals, bs));
  PetscCall(MatAssemblyBegin(mass, MAT_FINAL_ASSEMBLY));
  PetscCall(MatAssemblyEnd(mass, MAT_FINAL_ASSEMBLY));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* Returns empty matrix for use with SNES FD */
static PetscErrorCode DMCreateMatrix_Swarm(DM sw, Mat *m)
{
  Vec      field;
  PetscInt size;

  PetscFunctionBegin;
  PetscCall(DMGetGlobalVector(sw, &field));
  PetscCall(VecGetLocalSize(field, &size));
  PetscCall(DMRestoreGlobalVector(sw, &field));
  PetscCall(MatCreate(PETSC_COMM_WORLD, m));
  PetscCall(MatSetFromOptions(*m));
  PetscCall(MatSetSizes(*m, PETSC_DECIDE, PETSC_DECIDE, size, size));
  PetscCall(MatSeqAIJSetPreallocation(*m, 1, NULL));
  PetscCall(MatZeroEntries(*m));
  PetscCall(MatAssemblyBegin(*m, MAT_FINAL_ASSEMBLY));
  PetscCall(MatAssemblyEnd(*m, MAT_FINAL_ASSEMBLY));
  PetscCall(MatShift(*m, 1.0));
  PetscCall(MatSetDM(*m, sw));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* FEM cols, Particle rows */
static PetscErrorCode DMCreateMassMatrix_Swarm(DM dmCoarse, DM dmFine, Mat *mass)
{
  DMSwarmCellDM celldm;
  PetscSection  gsf;
  PetscInt      m, n, Np, bs;
  void         *ctx;

  PetscFunctionBegin;
  PetscCall(DMSwarmGetCellDMActive(dmCoarse, &celldm));
  PetscCheck(celldm->Nf, PetscObjectComm((PetscObject)dmCoarse), PETSC_ERR_USER, "Active cell DM does not define any fields");
  PetscCall(DMGetGlobalSection(dmFine, &gsf));
  PetscCall(PetscSectionGetConstrainedStorageSize(gsf, &m));
  PetscCall(DMSwarmGetLocalSize(dmCoarse, &Np));
  PetscCall(DMSwarmCellDMGetBlockSize(celldm, dmCoarse, &bs));
  n = Np * bs;
  PetscCall(MatCreate(PetscObjectComm((PetscObject)dmCoarse), mass));
  PetscCall(MatSetSizes(*mass, n, m, PETSC_DETERMINE, PETSC_DETERMINE));
  PetscCall(MatSetType(*mass, dmCoarse->mattype));
  PetscCall(DMGetApplicationContext(dmFine, &ctx));

  PetscCall(DMSwarmComputeMassMatrix_Private(dmCoarse, dmFine, *mass, PETSC_TRUE, ctx));
  PetscCall(MatViewFromOptions(*mass, NULL, "-mass_mat_view"));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmComputeMassMatrixSquare_Private(DM dmc, DM dmf, Mat mass, PetscBool useDeltaFunction, void *ctx)
{
  const char   *name = "Mass Matrix Square";
  MPI_Comm      comm;
  DMSwarmCellDM celldm;
  PetscDS       prob;
  PetscSection  fsection, globalFSection;
  PetscHSetIJ   ht;
  PetscLayout   rLayout, colLayout;
  PetscInt     *dnz, *onz, *adj, depth, maxConeSize, maxSupportSize, maxAdjSize;
  PetscInt      locRows, locCols, rStart, colStart, colEnd, *rowIDXs;
  PetscReal    *xi, *v0, *J, *invJ, detJ = 1.0, v0ref[3] = {-1.0, -1.0, -1.0};
  PetscScalar  *elemMat, *elemMatSq;
  PetscInt      cdim, Nf, Nfc, cStart, cEnd, totDim, maxC = 0;
  const char  **coordFields;
  PetscReal   **coordVals;
  PetscInt     *bs;

  PetscFunctionBegin;
  PetscCall(PetscObjectGetComm((PetscObject)mass, &comm));
  PetscCall(DMGetCoordinateDim(dmf, &cdim));
  PetscCall(DMGetDS(dmf, &prob));
  PetscCall(PetscDSGetNumFields(prob, &Nf));
  PetscCall(PetscDSGetTotalDimension(prob, &totDim));
  PetscCall(PetscMalloc3(cdim, &v0, cdim * cdim, &J, cdim * cdim, &invJ));
  PetscCall(DMGetLocalSection(dmf, &fsection));
  PetscCall(DMGetGlobalSection(dmf, &globalFSection));
  PetscCall(DMPlexGetHeightStratum(dmf, 0, &cStart, &cEnd));
  PetscCall(MatGetLocalSize(mass, &locRows, &locCols));

  PetscCall(DMSwarmGetCellDMActive(dmc, &celldm));
  PetscCall(DMSwarmCellDMGetCoordinateFields(celldm, &Nfc, &coordFields));
  PetscCall(PetscMalloc2(Nfc, &coordVals, Nfc, &bs));

  PetscCall(PetscLayoutCreate(comm, &colLayout));
  PetscCall(PetscLayoutSetLocalSize(colLayout, locCols));
  PetscCall(PetscLayoutSetBlockSize(colLayout, 1));
  PetscCall(PetscLayoutSetUp(colLayout));
  PetscCall(PetscLayoutGetRange(colLayout, &colStart, &colEnd));
  PetscCall(PetscLayoutDestroy(&colLayout));

  PetscCall(PetscLayoutCreate(comm, &rLayout));
  PetscCall(PetscLayoutSetLocalSize(rLayout, locRows));
  PetscCall(PetscLayoutSetBlockSize(rLayout, 1));
  PetscCall(PetscLayoutSetUp(rLayout));
  PetscCall(PetscLayoutGetRange(rLayout, &rStart, NULL));
  PetscCall(PetscLayoutDestroy(&rLayout));

  PetscCall(DMPlexGetDepth(dmf, &depth));
  PetscCall(DMPlexGetMaxSizes(dmf, &maxConeSize, &maxSupportSize));
  maxAdjSize = PetscPowInt(maxConeSize * maxSupportSize, depth);
  PetscCall(PetscMalloc1(maxAdjSize, &adj));

  PetscCall(PetscCalloc2(locRows, &dnz, locRows, &onz));
  PetscCall(PetscHSetIJCreate(&ht));
  /* Count nonzeros
       This is just FVM++, but we cannot use the Plex P0 allocation since unknowns in a cell will not be contiguous
  */
  PetscCall(DMSwarmSortGetAccess(dmc));
  for (PetscInt cell = cStart; cell < cEnd; ++cell) {
    PetscInt *cindices;
    PetscInt  numCIndices;
#if 0
    PetscInt  adjSize = maxAdjSize, a, j;
#endif

    PetscCall(DMSwarmSortGetPointsPerCell(dmc, cell, &numCIndices, &cindices));
    maxC = PetscMax(maxC, numCIndices);
    /* Diagonal block */
    for (PetscInt i = 0; i < numCIndices; ++i) dnz[cindices[i]] += numCIndices;
#if 0
    /* Off-diagonal blocks */
    PetscCall(DMPlexGetAdjacency(dmf, cell, &adjSize, &adj));
    for (a = 0; a < adjSize; ++a) {
      if (adj[a] >= cStart && adj[a] < cEnd && adj[a] != cell) {
        const PetscInt ncell = adj[a];
        PetscInt      *ncindices;
        PetscInt       numNCIndices;

        PetscCall(DMSwarmSortGetPointsPerCell(dmc, ncell, &numNCIndices, &ncindices));
        {
          PetscHashIJKey key;
          PetscBool      missing;

          for (i = 0; i < numCIndices; ++i) {
            key.i = cindices[i] + rStart; /* global rows (from Swarm) */
            if (key.i < 0) continue;
            for (j = 0; j < numNCIndices; ++j) {
              key.j = ncindices[j] + rStart; /* global column (from Swarm) */
              if (key.j < 0) continue;
              PetscCall(PetscHSetIJQueryAdd(ht, key, &missing));
              if (missing) {
                if ((key.j >= colStart) && (key.j < colEnd)) ++dnz[key.i - rStart];
                else                                         ++onz[key.i - rStart];
              }
            }
          }
        }
        PetscCall(DMSwarmSortRestorePointsPerCell(dmc, ncell, &numNCIndices, &ncindices));
      }
    }
#endif
    PetscCall(DMSwarmSortRestorePointsPerCell(dmc, cell, &numCIndices, &cindices));
  }
  PetscCall(PetscHSetIJDestroy(&ht));
  PetscCall(MatXAIJSetPreallocation(mass, 1, dnz, onz, NULL, NULL));
  PetscCall(MatSetOption(mass, MAT_NEW_NONZERO_ALLOCATION_ERR, PETSC_TRUE));
  PetscCall(PetscFree2(dnz, onz));
  PetscCall(PetscMalloc4(maxC * totDim, &elemMat, maxC * maxC, &elemMatSq, maxC, &rowIDXs, maxC * cdim, &xi));
  /* Fill in values
       Each entry is a sum of terms \phi_i(x_p) \phi_i(x_q)
       Start just by producing block diagonal
       Could loop over adjacent cells
         Produce neighboring element matrix
         TODO Determine which columns and rows correspond to shared dual vector
         Do MatMatMult with rectangular matrices
         Insert block
  */
  for (PetscInt field = 0; field < Nf; ++field) {
    PetscTabulation Tcoarse;
    PetscObject     obj;
    PetscInt        Nc;

    PetscCall(PetscDSGetDiscretization(prob, field, &obj));
    PetscCall(PetscFEGetNumComponents((PetscFE)obj, &Nc));
    PetscCheck(Nc == 1, PetscObjectComm((PetscObject)dmf), PETSC_ERR_SUP, "Can only interpolate a scalar field from particles, Nc = %" PetscInt_FMT, Nc);
    for (PetscInt i = 0; i < Nfc; ++i) PetscCall(DMSwarmGetField(dmc, coordFields[i], &bs[i], NULL, (void **)&coordVals[i]));
    for (PetscInt cell = cStart; cell < cEnd; ++cell) {
      PetscInt *findices, *cindices;
      PetscInt  numFIndices, numCIndices;

      /* TODO: Use DMField instead of assuming affine */
      PetscCall(DMPlexComputeCellGeometryFEM(dmf, cell, NULL, v0, J, invJ, &detJ));
      PetscCall(DMPlexGetClosureIndices(dmf, fsection, globalFSection, cell, PETSC_FALSE, &numFIndices, &findices, NULL, NULL));
      PetscCall(DMSwarmSortGetPointsPerCell(dmc, cell, &numCIndices, &cindices));
      for (PetscInt p = 0; p < numCIndices; ++p) {
        PetscReal xr[8];
        PetscInt  off = 0;

        for (PetscInt i = 0; i < Nfc; ++i) {
          for (PetscInt b = 0; b < bs[i]; ++b, ++off) xr[off] = coordVals[i][cindices[p] * bs[i] + b];
        }
        PetscCheck(off == cdim, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "The total block size of coordinates is %" PetscInt_FMT " != %" PetscInt_FMT " the DM coordinate dimension", off, cdim);
        CoordinatesRealToRef(cdim, cdim, v0ref, v0, invJ, xr, &xi[p * cdim]);
      }
      PetscCall(PetscFECreateTabulation((PetscFE)obj, 1, numCIndices, xi, 0, &Tcoarse));
      /* Get elemMat entries by multiplying by weight */
      PetscCall(PetscArrayzero(elemMat, numCIndices * totDim));
      for (PetscInt i = 0; i < numFIndices; ++i) {
        for (PetscInt p = 0; p < numCIndices; ++p) {
          for (PetscInt c = 0; c < Nc; ++c) {
            /* B[(p*pdim + i)*Nc + c] is the value at point p for basis function i and component c */
            elemMat[p * numFIndices + i] += Tcoarse->T[0][(p * numFIndices + i) * Nc + c] * (useDeltaFunction ? 1.0 : detJ);
          }
        }
      }
      PetscCall(PetscTabulationDestroy(&Tcoarse));
      for (PetscInt p = 0; p < numCIndices; ++p) rowIDXs[p] = cindices[p] + rStart;
      if (0) PetscCall(DMPrintCellMatrix(cell, name, 1, numCIndices, elemMat));
      /* Block diagonal */
      if (numCIndices) {
        PetscBLASInt blasn, blask;
        PetscScalar  one = 1.0, zero = 0.0;

        PetscCall(PetscBLASIntCast(numCIndices, &blasn));
        PetscCall(PetscBLASIntCast(numFIndices, &blask));
        PetscCallBLAS("BLASgemm", BLASgemm_("T", "N", &blasn, &blasn, &blask, &one, elemMat, &blask, elemMat, &blask, &zero, elemMatSq, &blasn));
      }
      PetscCall(MatSetValues(mass, numCIndices, rowIDXs, numCIndices, rowIDXs, elemMatSq, ADD_VALUES));
      /* TODO off-diagonal */
      PetscCall(DMSwarmSortRestorePointsPerCell(dmc, cell, &numCIndices, &cindices));
      PetscCall(DMPlexRestoreClosureIndices(dmf, fsection, globalFSection, cell, PETSC_FALSE, &numFIndices, &findices, NULL, NULL));
    }
    for (PetscInt i = 0; i < Nfc; ++i) PetscCall(DMSwarmRestoreField(dmc, coordFields[i], &bs[i], NULL, (void **)&coordVals[i]));
  }
  PetscCall(PetscFree4(elemMat, elemMatSq, rowIDXs, xi));
  PetscCall(PetscFree(adj));
  PetscCall(DMSwarmSortRestoreAccess(dmc));
  PetscCall(PetscFree3(v0, J, invJ));
  PetscCall(PetscFree2(coordVals, bs));
  PetscCall(MatAssemblyBegin(mass, MAT_FINAL_ASSEMBLY));
  PetscCall(MatAssemblyEnd(mass, MAT_FINAL_ASSEMBLY));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmCreateMassMatrixSquare - Creates the block-diagonal of the square, M^T_p M_p, of the particle mass matrix M_p

  Collective

  Input Parameters:
+ dmCoarse - a `DMSWARM`
- dmFine   - a `DMPLEX`

  Output Parameter:
. mass - the square of the particle mass matrix

  Level: advanced

  Note:
  We only compute the block diagonal since this provides a good preconditioner and is completely local. It would be possible in the
  future to compute the full normal equations.

.seealso: `DM`, `DMSWARM`, `DMCreateMassMatrix()`
@*/
PetscErrorCode DMSwarmCreateMassMatrixSquare(DM dmCoarse, DM dmFine, Mat *mass)
{
  PetscInt n;
  void    *ctx;

  PetscFunctionBegin;
  PetscCall(DMSwarmGetLocalSize(dmCoarse, &n));
  PetscCall(MatCreate(PetscObjectComm((PetscObject)dmCoarse), mass));
  PetscCall(MatSetSizes(*mass, n, n, PETSC_DETERMINE, PETSC_DETERMINE));
  PetscCall(MatSetType(*mass, dmCoarse->mattype));
  PetscCall(DMGetApplicationContext(dmFine, &ctx));

  PetscCall(DMSwarmComputeMassMatrixSquare_Private(dmCoarse, dmFine, *mass, PETSC_TRUE, ctx));
  PetscCall(MatViewFromOptions(*mass, NULL, "-mass_sq_mat_view"));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* This creates a "gradient matrix" between a finite element and particle space, which is meant to enforce a weak divergence condition on the particle space. We are looking for a finite element field that has the same divergence as our particle field, so that

     \int_X \psi_i \nabla \cdot \hat f = \int_X \psi_i \nabla \cdot f

   and then integrate by parts

     \int_X \nabla \psi_i \cdot \hat f = \int_X \nabla \psi_i \cdot f

   where \psi is from a scalar FE space. If a finite element interpolant is given by

     \hat f^c = \sum_i f_i \phi^c_i

   and a particle function is given by

     f^c = \sum_p f^c_p \delta(x - x_p)

   then we want to require that

     D_f \hat f = D_p f

   where the gradient matrices are given by

     (D_f)_{i(jc)} = \int \partial_c \psi_i \phi_j
     (D_p)_{i(jc)} = \int \partial_c \psi_i \delta(x - x_j)

   Thus we need two finite element spaces, a scalar and a vector. The vector space holds the representer for the
   vector particle field. The scalar space holds the output of D_p or D_f, which is the weak divergence of the field.

   The way Dave May does particles, they amount to quadratue weights rather than delta functions, so he has |J| is in
   his integral. We allow this with the boolean flag.
*/
static PetscErrorCode DMSwarmComputeGradientMatrix_Private(DM sw, DM dm, Mat derv, PetscBool useDeltaFunction, void *ctx)
{
  const char   *name = "Derivative Matrix";
  MPI_Comm      comm;
  DMSwarmCellDM celldm;
  PetscDS       ds;
  PetscSection  fsection, globalFSection;
  PetscLayout   rLayout;
  PetscInt      locRows, rStart, *rowIDXs;
  PetscReal    *xi, *v0, *J, *invJ, detJ = 1.0, v0ref[3] = {-1.0, -1.0, -1.0};
  PetscScalar  *elemMat;
  PetscInt      cdim, Nf, Nfc, cStart, cEnd, totDim, maxNpc = 0, totNc = 0;
  const char  **coordFields;
  PetscReal   **coordVals;
  PetscInt     *bs;

  PetscFunctionBegin;
  PetscCall(PetscObjectGetComm((PetscObject)derv, &comm));
  PetscCall(DMGetCoordinateDim(dm, &cdim));
  PetscCall(DMGetDS(dm, &ds));
  PetscCall(PetscDSGetNumFields(ds, &Nf));
  PetscCheck(Nf == 1, comm, PETSC_ERR_SUP, "Currently, we only support a single field");
  PetscCall(PetscDSGetTotalDimension(ds, &totDim));
  PetscCall(PetscMalloc3(cdim, &v0, cdim * cdim, &J, cdim * cdim, &invJ));
  PetscCall(DMGetLocalSection(dm, &fsection));
  PetscCall(DMGetGlobalSection(dm, &globalFSection));
  PetscCall(DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd));
  PetscCall(MatGetLocalSize(derv, &locRows, NULL));

  PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
  PetscCall(DMSwarmCellDMGetCoordinateFields(celldm, &Nfc, &coordFields));
  PetscCheck(Nfc == 1, comm, PETSC_ERR_SUP, "Currently, we only support a single field");
  PetscCall(PetscMalloc2(Nfc, &coordVals, Nfc, &bs));

  PetscCall(PetscLayoutCreate(comm, &rLayout));
  PetscCall(PetscLayoutSetLocalSize(rLayout, locRows));
  PetscCall(PetscLayoutSetBlockSize(rLayout, cdim));
  PetscCall(PetscLayoutSetUp(rLayout));
  PetscCall(PetscLayoutGetRange(rLayout, &rStart, NULL));
  PetscCall(PetscLayoutDestroy(&rLayout));

  for (PetscInt field = 0; field < Nf; ++field) {
    PetscObject obj;
    PetscInt    Nc;

    PetscCall(PetscDSGetDiscretization(ds, field, &obj));
    PetscCall(PetscFEGetNumComponents((PetscFE)obj, &Nc));
    totNc += Nc;
  }
  PetscCheck(totNc == 1, comm, PETSC_ERR_ARG_WRONG, "The number of field components %" PetscInt_FMT " != 1", totNc);
  /* count non-zeros */
  PetscCall(DMSwarmSortGetAccess(sw));
  for (PetscInt field = 0; field < Nf; ++field) {
    PetscObject obj;
    PetscInt    Nc;

    PetscCall(PetscDSGetDiscretization(ds, field, &obj));
    PetscCall(PetscFEGetNumComponents((PetscFE)obj, &Nc));
    for (PetscInt cell = cStart; cell < cEnd; ++cell) {
      PetscInt *pind;
      PetscInt  Npc;

      PetscCall(DMSwarmSortGetPointsPerCell(sw, cell, &Npc, &pind));
      maxNpc = PetscMax(maxNpc, Npc);
      PetscCall(DMSwarmSortRestorePointsPerCell(sw, cell, &Npc, &pind));
    }
  }
  PetscCall(PetscMalloc3(maxNpc * cdim * totDim, &elemMat, maxNpc * cdim, &rowIDXs, maxNpc * cdim, &xi));
  for (PetscInt field = 0; field < Nf; ++field) {
    PetscTabulation Tcoarse;
    PetscFE         fe;

    PetscCall(PetscDSGetDiscretization(ds, field, (PetscObject *)&fe));
    for (PetscInt i = 0; i < Nfc; ++i) PetscCall(DMSwarmGetField(sw, coordFields[i], &bs[i], NULL, (void **)&coordVals[i]));
    for (PetscInt cell = cStart; cell < cEnd; ++cell) {
      PetscInt *findices, *pind;
      PetscInt  numFIndices, Npc;

      /* TODO: Use DMField instead of assuming affine */
      PetscCall(DMPlexComputeCellGeometryFEM(dm, cell, NULL, v0, J, invJ, &detJ));
      PetscCall(DMPlexGetClosureIndices(dm, fsection, globalFSection, cell, PETSC_FALSE, &numFIndices, &findices, NULL, NULL));
      PetscCall(DMSwarmSortGetPointsPerCell(sw, cell, &Npc, &pind));
      for (PetscInt j = 0; j < Npc; ++j) {
        PetscReal xr[8];
        PetscInt  off = 0;

        for (PetscInt i = 0; i < Nfc; ++i) {
          for (PetscInt b = 0; b < bs[i]; ++b, ++off) xr[off] = coordVals[i][pind[j] * bs[i] + b];
        }
        PetscCheck(off == cdim, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "The total block size of coordinates is %" PetscInt_FMT " != %" PetscInt_FMT " the DM coordinate dimension", off, cdim);
        CoordinatesRealToRef(cdim, cdim, v0ref, v0, invJ, xr, &xi[j * cdim]);
      }
      PetscCall(PetscFECreateTabulation(fe, 1, Npc, xi, 1, &Tcoarse));
      /* Get elemMat entries by multiplying by weight */
      PetscCall(PetscArrayzero(elemMat, Npc * cdim * totDim));
      for (PetscInt i = 0; i < numFIndices; ++i) {
        for (PetscInt j = 0; j < Npc; ++j) {
          /* D[((p*pdim + i)*Nc + c)*cdim + d] is the value at point p for basis function i, component c, derviative d */
          for (PetscInt d = 0; d < cdim; ++d) {
            xi[d] = 0.;
            for (PetscInt e = 0; e < cdim; ++e) xi[d] += invJ[e * cdim + d] * Tcoarse->T[1][(j * numFIndices + i) * cdim + e];
            elemMat[(j * cdim + d) * numFIndices + i] += xi[d] * (useDeltaFunction ? 1.0 : detJ);
          }
        }
      }
      for (PetscInt j = 0; j < Npc; ++j)
        for (PetscInt d = 0; d < cdim; ++d) rowIDXs[j * cdim + d] = pind[j] * cdim + d + rStart;
      if (0) PetscCall(DMPrintCellMatrix(cell, name, Npc * cdim, numFIndices, elemMat));
      PetscCall(MatSetValues(derv, Npc * cdim, rowIDXs, numFIndices, findices, elemMat, ADD_VALUES));
      PetscCall(DMSwarmSortRestorePointsPerCell(sw, cell, &Npc, &pind));
      PetscCall(DMPlexRestoreClosureIndices(dm, fsection, globalFSection, cell, PETSC_FALSE, &numFIndices, &findices, NULL, NULL));
      PetscCall(PetscTabulationDestroy(&Tcoarse));
    }
    for (PetscInt i = 0; i < Nfc; ++i) PetscCall(DMSwarmRestoreField(sw, coordFields[i], &bs[i], NULL, (void **)&coordVals[i]));
  }
  PetscCall(PetscFree3(elemMat, rowIDXs, xi));
  PetscCall(DMSwarmSortRestoreAccess(sw));
  PetscCall(PetscFree3(v0, J, invJ));
  PetscCall(PetscFree2(coordVals, bs));
  PetscCall(MatAssemblyBegin(derv, MAT_FINAL_ASSEMBLY));
  PetscCall(MatAssemblyEnd(derv, MAT_FINAL_ASSEMBLY));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* FEM cols:      this is a scalar space
   Particle rows: this is a vector space that contracts with the derivative
*/
static PetscErrorCode DMCreateGradientMatrix_Swarm(DM sw, DM dm, Mat *derv)
{
  DMSwarmCellDM celldm;
  PetscSection  gs;
  PetscInt      cdim, m, n, Np, bs;
  void         *ctx;
  MPI_Comm      comm;

  PetscFunctionBegin;
  PetscCall(PetscObjectGetComm((PetscObject)sw, &comm));
  PetscCall(DMGetCoordinateDim(dm, &cdim));
  PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
  PetscCheck(celldm->Nf, comm, PETSC_ERR_USER, "Active cell DM does not define any fields");
  PetscCall(DMGetGlobalSection(dm, &gs));
  PetscCall(PetscSectionGetConstrainedStorageSize(gs, &n));
  PetscCall(DMSwarmGetLocalSize(sw, &Np));
  PetscCall(DMSwarmCellDMGetBlockSize(celldm, sw, &bs));
  PetscCheck(cdim == bs, comm, PETSC_ERR_ARG_WRONG, "Coordinate dimension %" PetscInt_FMT " != %" PetscInt_FMT " swarm field block size", cdim, bs);
  m = Np * bs;
  PetscCall(MatCreate(PetscObjectComm((PetscObject)sw), derv));
  PetscCall(PetscObjectSetName((PetscObject)*derv, "Swarm Derivative Matrix"));
  PetscCall(MatSetSizes(*derv, m, n, PETSC_DETERMINE, PETSC_DETERMINE));
  PetscCall(MatSetType(*derv, sw->mattype));
  PetscCall(DMGetApplicationContext(dm, &ctx));

  PetscCall(DMSwarmComputeGradientMatrix_Private(sw, dm, *derv, PETSC_TRUE, ctx));
  PetscCall(MatViewFromOptions(*derv, NULL, "-gradient_mat_view"));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmCreateGlobalVectorFromField - Creates a `Vec` object sharing the array associated with a given field

  Collective

  Input Parameters:
+ dm        - a `DMSWARM`
- fieldname - the textual name given to a registered field

  Output Parameter:
. vec - the vector

  Level: beginner

  Note:
  The vector must be returned using a matching call to `DMSwarmDestroyGlobalVectorFromField()`.

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmDestroyGlobalVectorFromField()`
@*/
PetscErrorCode DMSwarmCreateGlobalVectorFromField(DM dm, const char fieldname[], Vec *vec)
{
  MPI_Comm comm = PetscObjectComm((PetscObject)dm);

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmCreateVectorFromField_Private(dm, fieldname, comm, vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmDestroyGlobalVectorFromField - Destroys the `Vec` object which share the array associated with a given field

  Collective

  Input Parameters:
+ dm        - a `DMSWARM`
- fieldname - the textual name given to a registered field

  Output Parameter:
. vec - the vector

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmCreateGlobalVectorFromField()`
@*/
PetscErrorCode DMSwarmDestroyGlobalVectorFromField(DM dm, const char fieldname[], Vec *vec)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmDestroyVectorFromField_Private(dm, fieldname, vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmCreateLocalVectorFromField - Creates a `Vec` object sharing the array associated with a given field

  Collective

  Input Parameters:
+ dm        - a `DMSWARM`
- fieldname - the textual name given to a registered field

  Output Parameter:
. vec - the vector

  Level: beginner

  Note:
  The vector must be returned using a matching call to DMSwarmDestroyLocalVectorFromField().

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmDestroyLocalVectorFromField()`
@*/
PetscErrorCode DMSwarmCreateLocalVectorFromField(DM dm, const char fieldname[], Vec *vec)
{
  MPI_Comm comm = PETSC_COMM_SELF;

  PetscFunctionBegin;
  PetscCall(DMSwarmCreateVectorFromField_Private(dm, fieldname, comm, vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmDestroyLocalVectorFromField - Destroys the `Vec` object which share the array associated with a given field

  Collective

  Input Parameters:
+ dm        - a `DMSWARM`
- fieldname - the textual name given to a registered field

  Output Parameter:
. vec - the vector

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmCreateLocalVectorFromField()`
@*/
PetscErrorCode DMSwarmDestroyLocalVectorFromField(DM dm, const char fieldname[], Vec *vec)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmDestroyVectorFromField_Private(dm, fieldname, vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmCreateGlobalVectorFromFields - Creates a `Vec` object sharing the array associated with a given field set

  Collective

  Input Parameters:
+ dm         - a `DMSWARM`
. Nf         - the number of fields
- fieldnames - the textual names given to the registered fields

  Output Parameter:
. vec - the vector

  Level: beginner

  Notes:
  The vector must be returned using a matching call to `DMSwarmDestroyGlobalVectorFromFields()`.

  This vector is copyin-copyout, rather than a direct pointer like `DMSwarmCreateGlobalVectorFromField()`

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmDestroyGlobalVectorFromFields()`
@*/
PetscErrorCode DMSwarmCreateGlobalVectorFromFields(DM dm, PetscInt Nf, const char *fieldnames[], Vec *vec)
{
  MPI_Comm comm = PetscObjectComm((PetscObject)dm);

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmCreateVectorFromFields_Private(dm, Nf, fieldnames, comm, vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmDestroyGlobalVectorFromFields - Destroys the `Vec` object which share the array associated with a given field set

  Collective

  Input Parameters:
+ dm         - a `DMSWARM`
. Nf         - the number of fields
- fieldnames - the textual names given to the registered fields

  Output Parameter:
. vec - the vector

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmCreateGlobalVectorFromField()`
@*/
PetscErrorCode DMSwarmDestroyGlobalVectorFromFields(DM dm, PetscInt Nf, const char *fieldnames[], Vec *vec)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmDestroyVectorFromFields_Private(dm, Nf, fieldnames, vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmCreateLocalVectorFromFields - Creates a `Vec` object sharing the array associated with a given field set

  Collective

  Input Parameters:
+ dm         - a `DMSWARM`
. Nf         - the number of fields
- fieldnames - the textual names given to the registered fields

  Output Parameter:
. vec - the vector

  Level: beginner

  Notes:
  The vector must be returned using a matching call to DMSwarmDestroyLocalVectorFromField().

  This vector is copyin-copyout, rather than a direct pointer like `DMSwarmCreateLocalVectorFromField()`

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmDestroyLocalVectorFromField()`
@*/
PetscErrorCode DMSwarmCreateLocalVectorFromFields(DM dm, PetscInt Nf, const char *fieldnames[], Vec *vec)
{
  MPI_Comm comm = PETSC_COMM_SELF;

  PetscFunctionBegin;
  PetscCall(DMSwarmCreateVectorFromFields_Private(dm, Nf, fieldnames, comm, vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmDestroyLocalVectorFromFields - Destroys the `Vec` object which share the array associated with a given field set

  Collective

  Input Parameters:
+ dm         - a `DMSWARM`
. Nf         - the number of fields
- fieldnames - the textual names given to the registered fields

  Output Parameter:
. vec - the vector

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmCreateLocalVectorFromFields()`
@*/
PetscErrorCode DMSwarmDestroyLocalVectorFromFields(DM dm, PetscInt Nf, const char *fieldnames[], Vec *vec)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmDestroyVectorFromFields_Private(dm, Nf, fieldnames, vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmInitializeFieldRegister - Initiates the registration of fields to a `DMSWARM`

  Collective

  Input Parameter:
. dm - a `DMSWARM`

  Level: beginner

  Note:
  After all fields have been registered, you must call `DMSwarmFinalizeFieldRegister()`.

.seealso: `DM`, `DMSWARM`, `DMSwarmFinalizeFieldRegister()`, `DMSwarmRegisterPetscDatatypeField()`,
          `DMSwarmRegisterUserStructField()`, `DMSwarmRegisterUserDatatypeField()`
@*/
PetscErrorCode DMSwarmInitializeFieldRegister(DM dm)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  if (!swarm->field_registration_initialized) {
    swarm->field_registration_initialized = PETSC_TRUE;
    PetscCall(DMSwarmRegisterPetscDatatypeField(dm, DMSwarmField_pid, 1, PETSC_INT64)); /* unique identifier */
    PetscCall(DMSwarmRegisterPetscDatatypeField(dm, DMSwarmField_rank, 1, PETSC_INT));  /* used for communication */
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmFinalizeFieldRegister - Finalizes the registration of fields to a `DMSWARM`

  Collective

  Input Parameter:
. dm - a `DMSWARM`

  Level: beginner

  Note:
  After `DMSwarmFinalizeFieldRegister()` has been called, no new fields can be defined on the `DMSWARM`.

.seealso: `DM`, `DMSWARM`, `DMSwarmInitializeFieldRegister()`, `DMSwarmRegisterPetscDatatypeField()`,
          `DMSwarmRegisterUserStructField()`, `DMSwarmRegisterUserDatatypeField()`
@*/
PetscErrorCode DMSwarmFinalizeFieldRegister(DM dm)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  if (!swarm->field_registration_finalized) PetscCall(DMSwarmDataBucketFinalize(swarm->db));
  swarm->field_registration_finalized = PETSC_TRUE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmSetLocalSizes - Sets the length of all registered fields on the `DMSWARM`

  Not Collective

  Input Parameters:
+ sw     - a `DMSWARM`
. nlocal - the length of each registered field
- buffer - the length of the buffer used to efficient dynamic re-sizing

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmGetLocalSize()`
@*/
PetscErrorCode DMSwarmSetLocalSizes(DM sw, PetscInt nlocal, PetscInt buffer)
{
  DM_Swarm   *swarm = (DM_Swarm *)sw->data;
  PetscMPIInt rank;
  PetscInt   *rankval;

  PetscFunctionBegin;
  PetscCall(PetscLogEventBegin(DMSWARM_SetSizes, 0, 0, 0, 0));
  PetscCall(DMSwarmDataBucketSetSizes(swarm->db, nlocal, buffer));
  PetscCall(PetscLogEventEnd(DMSWARM_SetSizes, 0, 0, 0, 0));

  // Initialize values in pid and rank placeholders
  PetscCallMPI(MPI_Comm_rank(PetscObjectComm((PetscObject)sw), &rank));
  PetscCall(DMSwarmGetField(sw, DMSwarmField_rank, NULL, NULL, (void **)&rankval));
  for (PetscInt p = 0; p < nlocal; p++) rankval[p] = rank;
  PetscCall(DMSwarmRestoreField(sw, DMSwarmField_rank, NULL, NULL, (void **)&rankval));
  /* TODO: [pid - use MPI_Scan] */
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmSetCellDM - Attaches a `DM` to a `DMSWARM`

  Collective

  Input Parameters:
+ sw - a `DMSWARM`
- dm - the `DM` to attach to the `DMSWARM`

  Level: beginner

  Note:
  The attached `DM` (dm) will be queried for point location and
  neighbor MPI-rank information if `DMSwarmMigrate()` is called.

.seealso: `DM`, `DMSWARM`, `DMSwarmSetType()`, `DMSwarmGetCellDM()`, `DMSwarmMigrate()`
@*/
PetscErrorCode DMSwarmSetCellDM(DM sw, DM dm)
{
  DMSwarmCellDM celldm;
  const char   *name;
  char         *coordName;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscValidHeaderSpecific(dm, DM_CLASSID, 2);
  PetscCall(PetscStrallocpy(DMSwarmPICField_coor, &coordName));
  PetscCall(DMSwarmCellDMCreate(dm, 0, NULL, 1, (const char **)&coordName, &celldm));
  PetscCall(PetscFree(coordName));
  PetscCall(PetscObjectGetName((PetscObject)celldm, &name));
  PetscCall(DMSwarmAddCellDM(sw, celldm));
  PetscCall(DMSwarmCellDMDestroy(&celldm));
  PetscCall(DMSwarmSetCellDMActive(sw, name));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmGetCellDM - Fetches the active cell `DM`

  Collective

  Input Parameter:
. sw - a `DMSWARM`

  Output Parameter:
. dm - the active `DM` for the `DMSWARM`

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmSetCellDM()`
@*/
PetscErrorCode DMSwarmGetCellDM(DM sw, DM *dm)
{
  DM_Swarm     *swarm = (DM_Swarm *)sw->data;
  DMSwarmCellDM celldm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscCall(PetscObjectListFind(swarm->cellDMs, swarm->activeCellDM, (PetscObject *)&celldm));
  PetscCheck(celldm, PetscObjectComm((PetscObject)sw), PETSC_ERR_ARG_WRONG, "There is no cell DM named %s in this Swarm", swarm->activeCellDM);
  PetscCall(DMSwarmCellDMGetDM(celldm, dm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmGetCellDMNames - Get the list of cell `DM` names

  Not collective

  Input Parameter:
. sw - a `DMSWARM`

  Output Parameters:
+ Ndm     - the number of `DMSwarmCellDM` in the `DMSWARM`
- celldms - the name of each `DMSwarmCellDM`

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmSetCellDM()`, `DMSwarmGetCellDMByName()`
@*/
PetscErrorCode DMSwarmGetCellDMNames(DM sw, PetscInt *Ndm, const char **celldms[])
{
  DM_Swarm       *swarm = (DM_Swarm *)sw->data;
  PetscObjectList next  = swarm->cellDMs;
  PetscInt        n     = 0;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscAssertPointer(Ndm, 2);
  PetscAssertPointer(celldms, 3);
  while (next) {
    next = next->next;
    ++n;
  }
  PetscCall(PetscMalloc1(n, celldms));
  next = swarm->cellDMs;
  n    = 0;
  while (next) {
    (*celldms)[n] = (const char *)next->obj->name;
    next          = next->next;
    ++n;
  }
  *Ndm = n;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmSetCellDMActive - Activates a cell `DM` for a `DMSWARM`

  Collective

  Input Parameters:
+ sw   - a `DMSWARM`
- name - name of the cell `DM` to active for the `DMSWARM`

  Level: beginner

  Note:
  The attached `DM` (dmcell) will be queried for point location and
  neighbor MPI-rank information if `DMSwarmMigrate()` is called.

.seealso: `DM`, `DMSWARM`, `DMSwarmCellDM`, `DMSwarmSetType()`, `DMSwarmAddCellDM()`, `DMSwarmSetCellDM()`, `DMSwarmMigrate()`
@*/
PetscErrorCode DMSwarmSetCellDMActive(DM sw, const char name[])
{
  DM_Swarm     *swarm = (DM_Swarm *)sw->data;
  DMSwarmCellDM celldm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscCall(PetscInfo(sw, "Setting cell DM to %s\n", name));
  PetscCall(PetscFree(swarm->activeCellDM));
  PetscCall(PetscStrallocpy(name, (char **)&swarm->activeCellDM));
  PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmGetCellDMActive - Returns the active cell `DM` for a `DMSWARM`

  Collective

  Input Parameter:
. sw - a `DMSWARM`

  Output Parameter:
. celldm - the active `DMSwarmCellDM`

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmCellDM`, `DMSwarmSetType()`, `DMSwarmAddCellDM()`, `DMSwarmSetCellDM()`, `DMSwarmMigrate()`
@*/
PetscErrorCode DMSwarmGetCellDMActive(DM sw, DMSwarmCellDM *celldm)
{
  DM_Swarm *swarm = (DM_Swarm *)sw->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscAssertPointer(celldm, 2);
  PetscCheck(swarm->activeCellDM, PetscObjectComm((PetscObject)sw), PETSC_ERR_ARG_WRONGSTATE, "Swarm has no active cell DM");
  PetscCall(PetscObjectListFind(swarm->cellDMs, swarm->activeCellDM, (PetscObject *)celldm));
  PetscCheck(*celldm, PetscObjectComm((PetscObject)sw), PETSC_ERR_ARG_WRONGSTATE, "Swarm has no valid cell DM for %s", swarm->activeCellDM);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmGetCellDMByName - Get a `DMSwarmCellDM` from its name

  Not collective

  Input Parameters:
+ sw   - a `DMSWARM`
- name - the name

  Output Parameter:
. celldm - the `DMSwarmCellDM`

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmSetCellDM()`, `DMSwarmGetCellDMNames()`
@*/
PetscErrorCode DMSwarmGetCellDMByName(DM sw, const char name[], DMSwarmCellDM *celldm)
{
  DM_Swarm *swarm = (DM_Swarm *)sw->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscAssertPointer(name, 2);
  PetscAssertPointer(celldm, 3);
  PetscCall(PetscObjectListFind(swarm->cellDMs, name, (PetscObject *)celldm));
  PetscCheck(*celldm, PetscObjectComm((PetscObject)sw), PETSC_ERR_ARG_WRONGSTATE, "Swarm has no valid cell DM for %s", name);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmAddCellDM - Adds a cell `DM` to the `DMSWARM`

  Collective

  Input Parameters:
+ sw     - a `DMSWARM`
- celldm - the `DMSwarmCellDM`

  Level: beginner

  Note:
  Cell DMs with the same name will share the cellid field

.seealso: `DM`, `DMSWARM`, `DMSwarmSetType()`, `DMSwarmPushCellDM()`, `DMSwarmSetCellDM()`, `DMSwarmMigrate()`
@*/
PetscErrorCode DMSwarmAddCellDM(DM sw, DMSwarmCellDM celldm)
{
  DM_Swarm   *swarm = (DM_Swarm *)sw->data;
  const char *name;
  PetscInt    dim;
  PetscBool   flg;
  MPI_Comm    comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscCall(PetscObjectGetComm((PetscObject)sw, &comm));
  PetscValidHeaderSpecific(celldm, DMSWARMCELLDM_CLASSID, 2);
  PetscCall(PetscObjectGetName((PetscObject)celldm, &name));
  PetscCall(PetscObjectListAdd(&swarm->cellDMs, name, (PetscObject)celldm));
  PetscCall(DMGetDimension(sw, &dim));
  for (PetscInt f = 0; f < celldm->Nfc; ++f) {
    PetscCall(DMSwarmDataFieldStringInList(celldm->coordFields[f], swarm->db->nfields, (const DMSwarmDataField *)swarm->db->field, &flg));
    if (!flg) {
      PetscCall(DMSwarmRegisterPetscDatatypeField(sw, celldm->coordFields[f], dim, PETSC_DOUBLE));
    } else {
      PetscDataType dt;
      PetscInt      bs;

      PetscCall(DMSwarmGetFieldInfo(sw, celldm->coordFields[f], &bs, &dt));
      PetscCheck(bs == dim, comm, PETSC_ERR_ARG_WRONG, "Coordinate field %s has blocksize %" PetscInt_FMT " != %" PetscInt_FMT " spatial dimension", celldm->coordFields[f], bs, dim);
      PetscCheck(dt == PETSC_DOUBLE, comm, PETSC_ERR_ARG_WRONG, "Coordinate field %s has datatype %s != PETSC_DOUBLE", celldm->coordFields[f], PetscDataTypes[dt]);
    }
  }
  // Assume that DMs with the same name share the cellid field
  PetscCall(DMSwarmDataFieldStringInList(celldm->cellid, swarm->db->nfields, (const DMSwarmDataField *)swarm->db->field, &flg));
  if (!flg) {
    PetscBool   isShell, isDummy;
    const char *name;

    // Allow dummy DMSHELL (I don't think we should support this mode)
    PetscCall(PetscObjectTypeCompare((PetscObject)celldm->dm, DMSHELL, &isShell));
    PetscCall(PetscObjectGetName((PetscObject)celldm->dm, &name));
    PetscCall(PetscStrcmp(name, "dummy", &isDummy));
    if (!isShell || !isDummy) PetscCall(DMSwarmRegisterPetscDatatypeField(sw, celldm->cellid, 1, PETSC_INT));
  }
  PetscCall(DMSwarmSetCellDMActive(sw, name));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmGetLocalSize - Retrieves the local length of fields registered

  Not Collective

  Input Parameter:
. dm - a `DMSWARM`

  Output Parameter:
. nlocal - the length of each registered field

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmGetSize()`, `DMSwarmSetLocalSizes()`
@*/
PetscErrorCode DMSwarmGetLocalSize(DM dm, PetscInt *nlocal)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  PetscCall(DMSwarmDataBucketGetSizes(swarm->db, nlocal, NULL, NULL));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmGetSize - Retrieves the total length of fields registered

  Collective

  Input Parameter:
. dm - a `DMSWARM`

  Output Parameter:
. n - the total length of each registered field

  Level: beginner

  Note:
  This calls `MPI_Allreduce()` upon each call (inefficient but safe)

.seealso: `DM`, `DMSWARM`, `DMSwarmGetLocalSize()`, `DMSwarmSetLocalSizes()`
@*/
PetscErrorCode DMSwarmGetSize(DM dm, PetscInt *n)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;
  PetscInt  nlocal;

  PetscFunctionBegin;
  PetscCall(DMSwarmDataBucketGetSizes(swarm->db, &nlocal, NULL, NULL));
  PetscCallMPI(MPIU_Allreduce(&nlocal, n, 1, MPIU_INT, MPI_SUM, PetscObjectComm((PetscObject)dm)));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmRegisterPetscDatatypeField - Register a field to a `DMSWARM` with a native PETSc data type

  Collective

  Input Parameters:
+ dm        - a `DMSWARM`
. fieldname - the textual name to identify this field
. blocksize - the number of each data type
- type      - a valid PETSc data type (`PETSC_CHAR`, `PETSC_SHORT`, `PETSC_INT`, `PETSC_FLOAT`, `PETSC_REAL`, `PETSC_LONG`)

  Level: beginner

  Notes:
  The textual name for each registered field must be unique.

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterUserStructField()`, `DMSwarmRegisterUserDatatypeField()`
@*/
PetscErrorCode DMSwarmRegisterPetscDatatypeField(DM dm, const char fieldname[], PetscInt blocksize, PetscDataType type)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;
  size_t    size;

  PetscFunctionBegin;
  PetscCheck(swarm->field_registration_initialized, PetscObjectComm((PetscObject)dm), PETSC_ERR_USER, "Must call DMSwarmInitializeFieldRegister() first");
  PetscCheck(!swarm->field_registration_finalized, PetscObjectComm((PetscObject)dm), PETSC_ERR_USER, "Cannot register additional fields after calling DMSwarmFinalizeFieldRegister() first");

  PetscCheck(type != PETSC_OBJECT, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Valid for {char,short,int,long,float,double}");
  PetscCheck(type != PETSC_FUNCTION, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Valid for {char,short,int,long,float,double}");
  PetscCheck(type != PETSC_STRING, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Valid for {char,short,int,long,float,double}");
  PetscCheck(type != PETSC_STRUCT, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Valid for {char,short,int,long,float,double}");
  PetscCheck(type != PETSC_DATATYPE_UNKNOWN, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Valid for {char,short,int,long,float,double}");

  PetscCall(PetscDataTypeGetSize(type, &size));
  /* Load a specific data type into data bucket, specifying textual name and its size in bytes */
  PetscCall(DMSwarmDataBucketRegisterField(swarm->db, "DMSwarmRegisterPetscDatatypeField", fieldname, blocksize * size, NULL));
  {
    DMSwarmDataField gfield;

    PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldByName(swarm->db, fieldname, &gfield));
    PetscCall(DMSwarmDataFieldSetBlockSize(gfield, blocksize));
  }
  swarm->db->field[swarm->db->nfields - 1]->petsc_type = type;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmRegisterUserStructField - Register a user defined struct to a `DMSWARM`

  Collective

  Input Parameters:
+ dm        - a `DMSWARM`
. fieldname - the textual name to identify this field
- size      - the size in bytes of the user struct of each data type

  Level: beginner

  Note:
  The textual name for each registered field must be unique.

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmRegisterUserDatatypeField()`
@*/
PetscErrorCode DMSwarmRegisterUserStructField(DM dm, const char fieldname[], size_t size)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  PetscCall(DMSwarmDataBucketRegisterField(swarm->db, "DMSwarmRegisterUserStructField", fieldname, size, NULL));
  swarm->db->field[swarm->db->nfields - 1]->petsc_type = PETSC_STRUCT;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmRegisterUserDatatypeField - Register a user defined data type to a `DMSWARM`

  Collective

  Input Parameters:
+ dm        - a `DMSWARM`
. fieldname - the textual name to identify this field
. size      - the size in bytes of the user data type
- blocksize - the number of each data type

  Level: beginner

  Note:
  The textual name for each registered field must be unique.

.seealso: `DM`, `DMSWARM`, `DMSwarmRegisterPetscDatatypeField()`, `DMSwarmRegisterUserStructField()`
@*/
PetscErrorCode DMSwarmRegisterUserDatatypeField(DM dm, const char fieldname[], size_t size, PetscInt blocksize)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  PetscCall(DMSwarmDataBucketRegisterField(swarm->db, "DMSwarmRegisterUserDatatypeField", fieldname, blocksize * size, NULL));
  {
    DMSwarmDataField gfield;

    PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldByName(swarm->db, fieldname, &gfield));
    PetscCall(DMSwarmDataFieldSetBlockSize(gfield, blocksize));
  }
  swarm->db->field[swarm->db->nfields - 1]->petsc_type = PETSC_DATATYPE_UNKNOWN;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmGetField - Get access to the underlying array storing all entries associated with a registered field

  Not Collective, No Fortran Support

  Input Parameters:
+ dm        - a `DMSWARM`
- fieldname - the textual name to identify this field

  Output Parameters:
+ blocksize - the number of each data type
. type      - the data type
- data      - pointer to raw array

  Level: beginner

  Notes:
  The array must be returned using a matching call to `DMSwarmRestoreField()`.

  Fortran Note:
  Only works for `type` of `PETSC_SCALAR`

.seealso: `DM`, `DMSWARM`, `DMSwarmRestoreField()`
@*/
PetscErrorCode DMSwarmGetField(DM dm, const char fieldname[], PetscInt *blocksize, PetscDataType *type, void **data) PeNS
{
  DM_Swarm        *swarm = (DM_Swarm *)dm->data;
  DMSwarmDataField gfield;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  if (!swarm->issetup) PetscCall(DMSetUp(dm));
  PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldByName(swarm->db, fieldname, &gfield));
  PetscCall(DMSwarmDataFieldGetAccess(gfield));
  PetscCall(DMSwarmDataFieldGetEntries(gfield, data));
  if (blocksize) *blocksize = gfield->bs;
  if (type) *type = gfield->petsc_type;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSwarmRestoreField - Restore access to the underlying array storing all entries associated with a registered field

  Not Collective

  Input Parameters:
+ dm        - a `DMSWARM`
- fieldname - the textual name to identify this field

  Output Parameters:
+ blocksize - the number of each data type
. type      - the data type
- data      - pointer to raw array

  Level: beginner

  Notes:
  The user must call `DMSwarmGetField()` prior to calling `DMSwarmRestoreField()`.

  Fortran Note:
  Only works for `type` of `PETSC_SCALAR`

.seealso: `DM`, `DMSWARM`, `DMSwarmGetField()`
@*/
PetscErrorCode DMSwarmRestoreField(DM dm, const char fieldname[], PetscInt *blocksize, PetscDataType *type, void **data) PeNS
{
  DM_Swarm        *swarm = (DM_Swarm *)dm->data;
  DMSwarmDataField gfield;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldByName(swarm->db, fieldname, &gfield));
  PetscCall(DMSwarmDataFieldRestoreAccess(gfield));
  if (data) *data = NULL;
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode DMSwarmGetFieldInfo(DM dm, const char fieldname[], PetscInt *blocksize, PetscDataType *type)
{
  DM_Swarm        *swarm = (DM_Swarm *)dm->data;
  DMSwarmDataField gfield;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(dm, DM_CLASSID, 1, DMSWARM);
  PetscCall(DMSwarmDataBucketGetDMSwarmDataFieldByName(swarm->db, fieldname, &gfield));
  if (blocksize) *blocksize = gfield->bs;
  if (type) *type = gfield->petsc_type;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmAddPoint - Add space for one new point in the `DMSWARM`

  Not Collective

  Input Parameter:
. dm - a `DMSWARM`

  Level: beginner

  Notes:
  The new point will have all fields initialized to zero.

.seealso: `DM`, `DMSWARM`, `DMSwarmAddNPoints()`
@*/
PetscErrorCode DMSwarmAddPoint(DM dm)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  if (!swarm->issetup) PetscCall(DMSetUp(dm));
  PetscCall(PetscLogEventBegin(DMSWARM_AddPoints, 0, 0, 0, 0));
  PetscCall(DMSwarmDataBucketAddPoint(swarm->db));
  PetscCall(PetscLogEventEnd(DMSWARM_AddPoints, 0, 0, 0, 0));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmAddNPoints - Add space for a number of new points in the `DMSWARM`

  Not Collective

  Input Parameters:
+ dm      - a `DMSWARM`
- npoints - the number of new points to add

  Level: beginner

  Notes:
  The new point will have all fields initialized to zero.

.seealso: `DM`, `DMSWARM`, `DMSwarmAddPoint()`
@*/
PetscErrorCode DMSwarmAddNPoints(DM dm, PetscInt npoints)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;
  PetscInt  nlocal;

  PetscFunctionBegin;
  PetscCall(PetscLogEventBegin(DMSWARM_AddPoints, 0, 0, 0, 0));
  PetscCall(DMSwarmDataBucketGetSizes(swarm->db, &nlocal, NULL, NULL));
  nlocal = PetscMax(nlocal, 0) + npoints;
  PetscCall(DMSwarmDataBucketSetSizes(swarm->db, nlocal, DMSWARM_DATA_BUCKET_BUFFER_DEFAULT));
  PetscCall(PetscLogEventEnd(DMSWARM_AddPoints, 0, 0, 0, 0));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmRemovePoint - Remove the last point from the `DMSWARM`

  Not Collective

  Input Parameter:
. dm - a `DMSWARM`

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmRemovePointAtIndex()`
@*/
PetscErrorCode DMSwarmRemovePoint(DM dm)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  PetscCall(PetscLogEventBegin(DMSWARM_RemovePoints, 0, 0, 0, 0));
  PetscCall(DMSwarmDataBucketRemovePoint(swarm->db));
  PetscCall(PetscLogEventEnd(DMSWARM_RemovePoints, 0, 0, 0, 0));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmRemovePointAtIndex - Removes a specific point from the `DMSWARM`

  Not Collective

  Input Parameters:
+ dm  - a `DMSWARM`
- idx - index of point to remove

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmRemovePoint()`
@*/
PetscErrorCode DMSwarmRemovePointAtIndex(DM dm, PetscInt idx)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  PetscCall(PetscLogEventBegin(DMSWARM_RemovePoints, 0, 0, 0, 0));
  PetscCall(DMSwarmDataBucketRemovePointAtIndex(swarm->db, idx));
  PetscCall(PetscLogEventEnd(DMSWARM_RemovePoints, 0, 0, 0, 0));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmCopyPoint - Copy point pj to point pi in the `DMSWARM`

  Not Collective

  Input Parameters:
+ dm - a `DMSWARM`
. pi - the index of the point to copy
- pj - the point index where the copy should be located

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmRemovePoint()`
@*/
PetscErrorCode DMSwarmCopyPoint(DM dm, PetscInt pi, PetscInt pj)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  if (!swarm->issetup) PetscCall(DMSetUp(dm));
  PetscCall(DMSwarmDataBucketCopyPoint(swarm->db, pi, swarm->db, pj));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmMigrate_Basic(DM dm, PetscBool remove_sent_points)
{
  PetscFunctionBegin;
  PetscCall(DMSwarmMigrate_Push_Basic(dm, remove_sent_points));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmMigrate - Relocates points defined in the `DMSWARM` to other MPI-ranks

  Collective

  Input Parameters:
+ dm                 - the `DMSWARM`
- remove_sent_points - flag indicating if sent points should be removed from the current MPI-rank

  Level: advanced

  Notes:
  The `DM` will be modified to accommodate received points.
  If `remove_sent_points` is `PETSC_TRUE`, any points that were sent will be removed from the `DM`.
  Different styles of migration are supported. See `DMSwarmSetMigrateType()`.

.seealso: `DM`, `DMSWARM`, `DMSwarmSetMigrateType()`
@*/
PetscErrorCode DMSwarmMigrate(DM dm, PetscBool remove_sent_points)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  PetscCall(PetscLogEventBegin(DMSWARM_Migrate, 0, 0, 0, 0));
  switch (swarm->migrate_type) {
  case DMSWARM_MIGRATE_BASIC:
    PetscCall(DMSwarmMigrate_Basic(dm, remove_sent_points));
    break;
  case DMSWARM_MIGRATE_DMCELLNSCATTER:
    PetscCall(DMSwarmMigrate_CellDMScatter(dm, remove_sent_points));
    break;
  case DMSWARM_MIGRATE_DMCELLEXACT:
    SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_SUP, "DMSWARM_MIGRATE_DMCELLEXACT not implemented");
  case DMSWARM_MIGRATE_USER:
    SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_SUP, "DMSWARM_MIGRATE_USER not implemented");
  default:
    SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_SUP, "DMSWARM_MIGRATE type unknown");
  }
  PetscCall(PetscLogEventEnd(DMSWARM_Migrate, 0, 0, 0, 0));
  PetscCall(DMClearGlobalVectors(dm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode DMSwarmMigrate_GlobalToLocal_Basic(DM dm, PetscInt *globalsize);

/*
 DMSwarmCollectViewCreate

 * Applies a collection method and gathers point neighbour points into dm

 Notes:
 Users should call DMSwarmCollectViewDestroy() after
 they have finished computations associated with the collected points
*/

/*@
  DMSwarmCollectViewCreate - Applies a collection method and gathers points
  in neighbour ranks into the `DMSWARM`

  Collective

  Input Parameter:
. dm - the `DMSWARM`

  Level: advanced

  Notes:
  Users should call `DMSwarmCollectViewDestroy()` after
  they have finished computations associated with the collected points

  Different collect methods are supported. See `DMSwarmSetCollectType()`.

  Developer Note:
  Create and Destroy routines create new objects that can get destroyed, they do not change the state
  of the current object.

.seealso: `DM`, `DMSWARM`, `DMSwarmCollectViewDestroy()`, `DMSwarmSetCollectType()`
@*/
PetscErrorCode DMSwarmCollectViewCreate(DM dm)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;
  PetscInt  ng;

  PetscFunctionBegin;
  PetscCheck(!swarm->collect_view_active, PetscObjectComm((PetscObject)dm), PETSC_ERR_USER, "CollectView currently active");
  PetscCall(DMSwarmGetLocalSize(dm, &ng));
  switch (swarm->collect_type) {
  case DMSWARM_COLLECT_BASIC:
    PetscCall(DMSwarmMigrate_GlobalToLocal_Basic(dm, &ng));
    break;
  case DMSWARM_COLLECT_DMDABOUNDINGBOX:
    SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_SUP, "DMSWARM_COLLECT_DMDABOUNDINGBOX not implemented");
  case DMSWARM_COLLECT_GENERAL:
    SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_SUP, "DMSWARM_COLLECT_GENERAL not implemented");
  default:
    SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_SUP, "DMSWARM_COLLECT type unknown");
  }
  swarm->collect_view_active       = PETSC_TRUE;
  swarm->collect_view_reset_nlocal = ng;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmCollectViewDestroy - Resets the `DMSWARM` to the size prior to calling `DMSwarmCollectViewCreate()`

  Collective

  Input Parameters:
. dm - the `DMSWARM`

  Notes:
  Users should call `DMSwarmCollectViewCreate()` before this function is called.

  Level: advanced

  Developer Note:
  Create and Destroy routines create new objects that can get destroyed, they do not change the state
  of the current object.

.seealso: `DM`, `DMSWARM`, `DMSwarmCollectViewCreate()`, `DMSwarmSetCollectType()`
@*/
PetscErrorCode DMSwarmCollectViewDestroy(DM dm)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  PetscCheck(swarm->collect_view_active, PetscObjectComm((PetscObject)dm), PETSC_ERR_USER, "CollectView is currently not active");
  PetscCall(DMSwarmSetLocalSizes(dm, swarm->collect_view_reset_nlocal, -1));
  swarm->collect_view_active = PETSC_FALSE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmSetUpPIC(DM dm)
{
  PetscInt dim;

  PetscFunctionBegin;
  PetscCall(DMSwarmSetNumSpecies(dm, 1));
  PetscCall(DMGetDimension(dm, &dim));
  PetscCheck(dim >= 1, PetscObjectComm((PetscObject)dm), PETSC_ERR_USER, "Dimension must be 1,2,3 - found %" PetscInt_FMT, dim);
  PetscCheck(dim <= 3, PetscObjectComm((PetscObject)dm), PETSC_ERR_USER, "Dimension must be 1,2,3 - found %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmSetPointCoordinatesRandom - Sets initial coordinates for particles in each cell

  Collective

  Input Parameters:
+ dm  - the `DMSWARM`
- Npc - The number of particles per cell in the cell `DM`

  Level: intermediate

  Notes:
  The user must use `DMSwarmSetCellDM()` to set the cell `DM` first. The particles are placed randomly inside each cell. If only
  one particle is in each cell, it is placed at the centroid.

.seealso: `DM`, `DMSWARM`, `DMSwarmSetCellDM()`
@*/
PetscErrorCode DMSwarmSetPointCoordinatesRandom(DM dm, PetscInt Npc)
{
  DM             cdm;
  DMSwarmCellDM  celldm;
  PetscRandom    rnd;
  DMPolytopeType ct;
  PetscBool      simplex;
  PetscReal     *centroid, *coords, *xi0, *v0, *J, *invJ, detJ;
  PetscInt       dim, d, cStart, cEnd, c, p, Nfc;
  const char   **coordFields;

  PetscFunctionBeginUser;
  PetscCall(PetscRandomCreate(PetscObjectComm((PetscObject)dm), &rnd));
  PetscCall(PetscRandomSetInterval(rnd, -1.0, 1.0));
  PetscCall(PetscRandomSetType(rnd, PETSCRAND48));

  PetscCall(DMSwarmGetCellDMActive(dm, &celldm));
  PetscCall(DMSwarmCellDMGetCoordinateFields(celldm, &Nfc, &coordFields));
  PetscCheck(Nfc == 1, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "We only support a single coordinate field right now, not %" PetscInt_FMT, Nfc);
  PetscCall(DMSwarmGetCellDM(dm, &cdm));
  PetscCall(DMGetDimension(cdm, &dim));
  PetscCall(DMPlexGetHeightStratum(cdm, 0, &cStart, &cEnd));
  PetscCall(DMPlexGetCellType(cdm, cStart, &ct));
  simplex = DMPolytopeTypeGetNumVertices(ct) == DMPolytopeTypeGetDim(ct) + 1 ? PETSC_TRUE : PETSC_FALSE;

  PetscCall(PetscMalloc5(dim, &centroid, dim, &xi0, dim, &v0, dim * dim, &J, dim * dim, &invJ));
  for (d = 0; d < dim; ++d) xi0[d] = -1.0;
  PetscCall(DMSwarmGetField(dm, coordFields[0], NULL, NULL, (void **)&coords));
  for (c = cStart; c < cEnd; ++c) {
    if (Npc == 1) {
      PetscCall(DMPlexComputeCellGeometryFVM(cdm, c, NULL, centroid, NULL));
      for (d = 0; d < dim; ++d) coords[c * dim + d] = centroid[d];
    } else {
      PetscCall(DMPlexComputeCellGeometryFEM(cdm, c, NULL, v0, J, invJ, &detJ)); /* affine */
      for (p = 0; p < Npc; ++p) {
        const PetscInt n   = c * Npc + p;
        PetscReal      sum = 0.0, refcoords[3];

        for (d = 0; d < dim; ++d) {
          PetscCall(PetscRandomGetValueReal(rnd, &refcoords[d]));
          sum += refcoords[d];
        }
        if (simplex && sum > 0.0)
          for (d = 0; d < dim; ++d) refcoords[d] -= PetscSqrtReal(dim) * sum;
        CoordinatesRefToReal(dim, dim, xi0, v0, J, refcoords, &coords[n * dim]);
      }
    }
  }
  PetscCall(DMSwarmRestoreField(dm, coordFields[0], NULL, NULL, (void **)&coords));
  PetscCall(PetscFree5(centroid, xi0, v0, J, invJ));
  PetscCall(PetscRandomDestroy(&rnd));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmGetType - Get particular flavor of `DMSWARM`

  Collective

  Input Parameter:
. sw - the `DMSWARM`

  Output Parameter:
. stype - the `DMSWARM` type (e.g. `DMSWARM_PIC`)

  Level: advanced

.seealso: `DM`, `DMSWARM`, `DMSwarmSetMigrateType()`, `DMSwarmSetCollectType()`, `DMSwarmType`, `DMSWARM_PIC`, `DMSWARM_BASIC`
@*/
PetscErrorCode DMSwarmGetType(DM sw, DMSwarmType *stype)
{
  DM_Swarm *swarm = (DM_Swarm *)sw->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscAssertPointer(stype, 2);
  *stype = swarm->swarm_type;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmSetType - Set particular flavor of `DMSWARM`

  Collective

  Input Parameters:
+ sw    - the `DMSWARM`
- stype - the `DMSWARM` type (e.g. `DMSWARM_PIC`)

  Level: advanced

.seealso: `DM`, `DMSWARM`, `DMSwarmSetMigrateType()`, `DMSwarmSetCollectType()`, `DMSwarmType`, `DMSWARM_PIC`, `DMSWARM_BASIC`
@*/
PetscErrorCode DMSwarmSetType(DM sw, DMSwarmType stype)
{
  DM_Swarm *swarm = (DM_Swarm *)sw->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  swarm->swarm_type = stype;
  if (swarm->swarm_type == DMSWARM_PIC) PetscCall(DMSwarmSetUpPIC(sw));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmCreateRemapDM_Private(DM sw, DM *rdm)
{
  PetscFE        fe;
  DMPolytopeType ct;
  PetscInt       dim, cStart;
  const char    *prefix = "remap_";

  PetscFunctionBegin;
  PetscCall(DMCreate(PetscObjectComm((PetscObject)sw), rdm));
  PetscCall(DMSetType(*rdm, DMPLEX));
  PetscCall(DMPlexSetOptionsPrefix(*rdm, prefix));
  PetscCall(DMSetFromOptions(*rdm));
  PetscCall(PetscObjectSetName((PetscObject)*rdm, "remap"));
  PetscCall(DMViewFromOptions(*rdm, NULL, "-dm_view"));

  PetscCall(DMGetDimension(*rdm, &dim));
  PetscCall(DMPlexGetHeightStratum(*rdm, 0, &cStart, NULL));
  PetscCall(DMPlexGetCellType(*rdm, cStart, &ct));
  PetscCall(PetscFECreateByCell(PETSC_COMM_SELF, dim, 1, ct, prefix, PETSC_DETERMINE, &fe));
  PetscCall(PetscObjectSetName((PetscObject)fe, "distribution"));
  PetscCall(DMSetField(*rdm, 0, NULL, (PetscObject)fe));
  PetscCall(DMCreateDS(*rdm));
  PetscCall(PetscFEDestroy(&fe));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSetup_Swarm(DM sw)
{
  DM_Swarm *swarm = (DM_Swarm *)sw->data;

  PetscFunctionBegin;
  if (swarm->issetup) PetscFunctionReturn(PETSC_SUCCESS);
  swarm->issetup = PETSC_TRUE;

  if (swarm->remap_type != DMSWARM_REMAP_NONE) {
    DMSwarmCellDM celldm;
    DM            rdm;
    const char   *fieldnames[2]  = {DMSwarmPICField_coor, "velocity"};
    const char   *vfieldnames[1] = {"w_q"};

    PetscCall(DMSwarmCreateRemapDM_Private(sw, &rdm));
    PetscCall(DMSwarmCellDMCreate(rdm, 1, vfieldnames, 2, fieldnames, &celldm));
    PetscCall(DMSwarmAddCellDM(sw, celldm));
    PetscCall(DMSwarmCellDMDestroy(&celldm));
    PetscCall(DMDestroy(&rdm));
  }

  if (swarm->swarm_type == DMSWARM_PIC) {
    DMSwarmCellDM celldm;

    PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
    PetscCheck(celldm, PetscObjectComm((PetscObject)sw), PETSC_ERR_USER, "No active cell DM. DMSWARM_PIC requires you call DMSwarmSetCellDM() or DMSwarmAddCellDM()");
    if (celldm->dm->ops->locatepointssubdomain) {
      /* check methods exists for exact ownership identificiation */
      PetscCall(PetscInfo(sw, "DMSWARM_PIC: Using method CellDM->ops->LocatePointsSubdomain\n"));
      swarm->migrate_type = DMSWARM_MIGRATE_DMCELLEXACT;
    } else {
      /* check methods exist for point location AND rank neighbor identification */
      if (celldm->dm->ops->locatepoints) {
        PetscCall(PetscInfo(sw, "DMSWARM_PIC: Using method CellDM->LocatePoints\n"));
      } else SETERRQ(PetscObjectComm((PetscObject)sw), PETSC_ERR_USER, "DMSWARM_PIC requires the method CellDM->ops->locatepoints be defined");

      if (celldm->dm->ops->getneighbors) {
        PetscCall(PetscInfo(sw, "DMSWARM_PIC: Using method CellDM->GetNeigbors\n"));
      } else SETERRQ(PetscObjectComm((PetscObject)sw), PETSC_ERR_USER, "DMSWARM_PIC requires the method CellDM->ops->getneighbors be defined");

      swarm->migrate_type = DMSWARM_MIGRATE_DMCELLNSCATTER;
    }
  }

  PetscCall(DMSwarmFinalizeFieldRegister(sw));

  /* check some fields were registered */
  PetscCheck(swarm->db->nfields > 2, PetscObjectComm((PetscObject)sw), PETSC_ERR_USER, "At least one field user must be registered via DMSwarmRegisterXXX()");
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMDestroy_Swarm(DM dm)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  if (--swarm->refct > 0) PetscFunctionReturn(PETSC_SUCCESS);
  PetscCall(PetscObjectListDestroy(&swarm->cellDMs));
  PetscCall(PetscFree(swarm->activeCellDM));
  PetscCall(DMSwarmDataBucketDestroy(&swarm->db));
  PetscCall(PetscFree(swarm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSwarmView_Draw(DM dm, PetscViewer viewer)
{
  DM            cdm;
  DMSwarmCellDM celldm;
  PetscDraw     draw;
  PetscReal    *coords, oldPause, radius = 0.01;
  PetscInt      Np, p, bs, Nfc;
  const char  **coordFields;

  PetscFunctionBegin;
  PetscCall(PetscOptionsGetReal(NULL, ((PetscObject)dm)->prefix, "-dm_view_swarm_radius", &radius, NULL));
  PetscCall(PetscViewerDrawGetDraw(viewer, 0, &draw));
  PetscCall(DMSwarmGetCellDM(dm, &cdm));
  PetscCall(PetscDrawGetPause(draw, &oldPause));
  PetscCall(PetscDrawSetPause(draw, 0.0));
  PetscCall(DMView(cdm, viewer));
  PetscCall(PetscDrawSetPause(draw, oldPause));

  PetscCall(DMSwarmGetCellDMActive(dm, &celldm));
  PetscCall(DMSwarmCellDMGetCoordinateFields(celldm, &Nfc, &coordFields));
  PetscCheck(Nfc == 1, PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "We only support a single coordinate field right now, not %" PetscInt_FMT, Nfc);
  PetscCall(DMSwarmGetLocalSize(dm, &Np));
  PetscCall(DMSwarmGetField(dm, coordFields[0], &bs, NULL, (void **)&coords));
  for (p = 0; p < Np; ++p) {
    const PetscInt i = p * bs;

    PetscCall(PetscDrawEllipse(draw, coords[i], coords[i + 1], radius, radius, PETSC_DRAW_BLUE));
  }
  PetscCall(DMSwarmRestoreField(dm, coordFields[0], &bs, NULL, (void **)&coords));
  PetscCall(PetscDrawFlush(draw));
  PetscCall(PetscDrawPause(draw));
  PetscCall(PetscDrawSave(draw));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMView_Swarm_Ascii(DM dm, PetscViewer viewer)
{
  PetscViewerFormat format;
  PetscInt         *sizes;
  PetscInt          dim, Np, maxSize = 17;
  MPI_Comm          comm;
  PetscMPIInt       rank, size, p;
  const char       *name, *cellid;

  PetscFunctionBegin;
  PetscCall(PetscViewerGetFormat(viewer, &format));
  PetscCall(DMGetDimension(dm, &dim));
  PetscCall(DMSwarmGetLocalSize(dm, &Np));
  PetscCall(PetscObjectGetComm((PetscObject)dm, &comm));
  PetscCallMPI(MPI_Comm_rank(comm, &rank));
  PetscCallMPI(MPI_Comm_size(comm, &size));
  PetscCall(PetscObjectGetName((PetscObject)dm, &name));
  if (name) PetscCall(PetscViewerASCIIPrintf(viewer, "%s in %" PetscInt_FMT " dimension%s:\n", name, dim, dim == 1 ? "" : "s"));
  else PetscCall(PetscViewerASCIIPrintf(viewer, "Swarm in %" PetscInt_FMT " dimension%s:\n", dim, dim == 1 ? "" : "s"));
  if (size < maxSize) PetscCall(PetscCalloc1(size, &sizes));
  else PetscCall(PetscCalloc1(3, &sizes));
  if (size < maxSize) {
    PetscCallMPI(MPI_Gather(&Np, 1, MPIU_INT, sizes, 1, MPIU_INT, 0, comm));
    PetscCall(PetscViewerASCIIPrintf(viewer, "  Number of particles per rank:"));
    for (p = 0; p < size; ++p) {
      if (rank == 0) PetscCall(PetscViewerASCIIPrintf(viewer, " %" PetscInt_FMT, sizes[p]));
    }
  } else {
    PetscInt locMinMax[2] = {Np, Np};

    PetscCall(PetscGlobalMinMaxInt(comm, locMinMax, sizes));
    PetscCall(PetscViewerASCIIPrintf(viewer, "  Min/Max of particles per rank: %" PetscInt_FMT "/%" PetscInt_FMT, sizes[0], sizes[1]));
  }
  PetscCall(PetscViewerASCIIPrintf(viewer, "\n"));
  PetscCall(PetscFree(sizes));
  if (format == PETSC_VIEWER_ASCII_INFO) {
    DMSwarmCellDM celldm;
    PetscInt     *cell;

    PetscCall(PetscViewerASCIIPrintf(viewer, "  Cells containing each particle:\n"));
    PetscCall(PetscViewerASCIIPushSynchronized(viewer));
    PetscCall(DMSwarmGetCellDMActive(dm, &celldm));
    PetscCall(DMSwarmCellDMGetCellID(celldm, &cellid));
    PetscCall(DMSwarmGetField(dm, cellid, NULL, NULL, (void **)&cell));
    for (p = 0; p < Np; ++p) PetscCall(PetscViewerASCIISynchronizedPrintf(viewer, "  p%d: %" PetscInt_FMT "\n", p, cell[p]));
    PetscCall(PetscViewerFlush(viewer));
    PetscCall(PetscViewerASCIIPopSynchronized(viewer));
    PetscCall(DMSwarmRestoreField(dm, cellid, NULL, NULL, (void **)&cell));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMView_Swarm(DM dm, PetscViewer viewer)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;
  PetscBool iascii, ibinary, isvtk, isdraw, ispython;
#if defined(PETSC_HAVE_HDF5)
  PetscBool ishdf5;
#endif

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 2);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERBINARY, &ibinary));
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERVTK, &isvtk));
#if defined(PETSC_HAVE_HDF5)
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERHDF5, &ishdf5));
#endif
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERDRAW, &isdraw));
  PetscCall(PetscObjectHasFunction((PetscObject)viewer, "PetscViewerPythonViewObject_C", &ispython));
  if (iascii) {
    PetscViewerFormat format;

    PetscCall(PetscViewerGetFormat(viewer, &format));
    switch (format) {
    case PETSC_VIEWER_ASCII_INFO_DETAIL:
      PetscCall(DMSwarmDataBucketView(PetscObjectComm((PetscObject)dm), swarm->db, NULL, DATABUCKET_VIEW_STDOUT));
      break;
    default:
      PetscCall(DMView_Swarm_Ascii(dm, viewer));
    }
  } else {
#if defined(PETSC_HAVE_HDF5)
    if (ishdf5) PetscCall(DMSwarmView_HDF5(dm, viewer));
#endif
    if (isdraw) PetscCall(DMSwarmView_Draw(dm, viewer));
    if (ispython) PetscCall(PetscViewerPythonViewObject(viewer, (PetscObject)dm));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmGetCellSwarm - Extracts a single cell from the `DMSWARM` object, returns it as a single cell `DMSWARM`.
  The cell `DM` is filtered for fields of that cell, and the filtered `DM` is used as the cell `DM` of the new swarm object.

  Noncollective

  Input Parameters:
+ sw        - the `DMSWARM`
. cellID    - the integer id of the cell to be extracted and filtered
- cellswarm - The `DMSWARM` to receive the cell

  Level: beginner

  Notes:
  This presently only supports `DMSWARM_PIC` type

  Should be restored with `DMSwarmRestoreCellSwarm()`

  Changes to this cell of the swarm will be lost if they are made prior to restoring this cell.

.seealso: `DM`, `DMSWARM`, `DMSwarmRestoreCellSwarm()`
@*/
PetscErrorCode DMSwarmGetCellSwarm(DM sw, PetscInt cellID, DM cellswarm)
{
  DM_Swarm   *original = (DM_Swarm *)sw->data;
  DMLabel     label;
  DM          dmc, subdmc;
  PetscInt   *pids, particles, dim;
  const char *name;

  PetscFunctionBegin;
  /* Configure new swarm */
  PetscCall(DMSetType(cellswarm, DMSWARM));
  PetscCall(DMGetDimension(sw, &dim));
  PetscCall(DMSetDimension(cellswarm, dim));
  PetscCall(DMSwarmSetType(cellswarm, DMSWARM_PIC));
  /* Destroy the unused, unconfigured data bucket to prevent stragglers in memory */
  PetscCall(DMSwarmDataBucketDestroy(&((DM_Swarm *)cellswarm->data)->db));
  PetscCall(DMSwarmSortGetAccess(sw));
  PetscCall(DMSwarmSortGetNumberOfPointsPerCell(sw, cellID, &particles));
  PetscCall(DMSwarmSortGetPointsPerCell(sw, cellID, &particles, &pids));
  PetscCall(DMSwarmDataBucketCreateFromSubset(original->db, particles, pids, &((DM_Swarm *)cellswarm->data)->db));
  PetscCall(DMSwarmSortRestoreAccess(sw));
  PetscCall(DMSwarmSortRestorePointsPerCell(sw, cellID, &particles, &pids));
  PetscCall(DMSwarmGetCellDM(sw, &dmc));
  PetscCall(DMLabelCreate(PetscObjectComm((PetscObject)sw), "singlecell", &label));
  PetscCall(DMAddLabel(dmc, label));
  PetscCall(DMLabelSetValue(label, cellID, 1));
  PetscCall(DMPlexFilter(dmc, label, 1, PETSC_FALSE, PETSC_FALSE, NULL, &subdmc));
  PetscCall(PetscObjectGetName((PetscObject)dmc, &name));
  PetscCall(PetscObjectSetName((PetscObject)subdmc, name));
  PetscCall(DMSwarmSetCellDM(cellswarm, subdmc));
  PetscCall(DMLabelDestroy(&label));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmRestoreCellSwarm - Restores a `DMSWARM` object obtained with `DMSwarmGetCellSwarm()`. All fields are copied back into the parent swarm.

  Noncollective

  Input Parameters:
+ sw        - the parent `DMSWARM`
. cellID    - the integer id of the cell to be copied back into the parent swarm
- cellswarm - the cell swarm object

  Level: beginner

  Note:
  This only supports `DMSWARM_PIC` types of `DMSWARM`s

.seealso: `DM`, `DMSWARM`, `DMSwarmGetCellSwarm()`
@*/
PetscErrorCode DMSwarmRestoreCellSwarm(DM sw, PetscInt cellID, DM cellswarm)
{
  DM        dmc;
  PetscInt *pids, particles, p;

  PetscFunctionBegin;
  PetscCall(DMSwarmSortGetAccess(sw));
  PetscCall(DMSwarmSortGetPointsPerCell(sw, cellID, &particles, &pids));
  PetscCall(DMSwarmSortRestoreAccess(sw));
  /* Pointwise copy of each particle based on pid. The parent swarm may not be altered during this process. */
  for (p = 0; p < particles; ++p) PetscCall(DMSwarmDataBucketCopyPoint(((DM_Swarm *)cellswarm->data)->db, pids[p], ((DM_Swarm *)sw->data)->db, pids[p]));
  /* Free memory, destroy cell dm */
  PetscCall(DMSwarmGetCellDM(cellswarm, &dmc));
  PetscCall(DMDestroy(&dmc));
  PetscCall(DMSwarmSortRestorePointsPerCell(sw, cellID, &particles, &pids));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmComputeMoments - Compute the first three particle moments for a given field

  Noncollective

  Input Parameters:
+ sw         - the `DMSWARM`
. coordinate - the coordinate field name
- weight     - the weight field name

  Output Parameter:
. moments - the field moments

  Level: intermediate

  Notes:
  The `moments` array should be of length bs + 2, where bs is the block size of the coordinate field.

  The weight field must be a scalar, having blocksize 1.

.seealso: `DM`, `DMSWARM`, `DMPlexComputeMoments()`
@*/
PetscErrorCode DMSwarmComputeMoments(DM sw, const char coordinate[], const char weight[], PetscReal moments[])
{
  const PetscReal *coords;
  const PetscReal *w;
  PetscReal       *mom;
  PetscDataType    dtc, dtw;
  PetscInt         bsc, bsw, Np;
  MPI_Comm         comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sw, DM_CLASSID, 1);
  PetscAssertPointer(coordinate, 2);
  PetscAssertPointer(weight, 3);
  PetscAssertPointer(moments, 4);
  PetscCall(PetscObjectGetComm((PetscObject)sw, &comm));
  PetscCall(DMSwarmGetField(sw, coordinate, &bsc, &dtc, (void **)&coords));
  PetscCall(DMSwarmGetField(sw, weight, &bsw, &dtw, (void **)&w));
  PetscCheck(dtc == PETSC_REAL, comm, PETSC_ERR_ARG_WRONG, "Coordinate field %s must be real, not %s", coordinate, PetscDataTypes[dtc]);
  PetscCheck(dtw == PETSC_REAL, comm, PETSC_ERR_ARG_WRONG, "Weight field %s must be real, not %s", weight, PetscDataTypes[dtw]);
  PetscCheck(bsw == 1, comm, PETSC_ERR_ARG_WRONG, "Weight field %s must be a scalar, not blocksize %" PetscInt_FMT, weight, bsw);
  PetscCall(DMSwarmGetLocalSize(sw, &Np));
  PetscCall(DMGetWorkArray(sw, bsc + 2, MPIU_REAL, &mom));
  PetscCall(PetscArrayzero(mom, bsc + 2));
  for (PetscInt p = 0; p < Np; ++p) {
    const PetscReal *c  = &coords[p * bsc];
    const PetscReal  wp = w[p];

    mom[0] += wp;
    for (PetscInt d = 0; d < bsc; ++d) {
      mom[d + 1] += wp * c[d];
      mom[d + bsc + 1] += wp * PetscSqr(c[d]);
    }
  }
  PetscCall(DMSwarmRestoreField(sw, "velocity", NULL, NULL, (void **)&coords));
  PetscCall(DMSwarmRestoreField(sw, "w_q", NULL, NULL, (void **)&w));
  PetscCallMPI(MPIU_Allreduce(mom, moments, bsc + 2, MPIU_REAL, MPI_SUM, PetscObjectComm((PetscObject)sw)));
  PetscCall(DMRestoreWorkArray(sw, bsc + 2, MPIU_REAL, &mom));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSetFromOptions_Swarm(DM dm, PetscOptionItems PetscOptionsObject)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  PetscOptionsHeadBegin(PetscOptionsObject, "DMSwarm Options");
  PetscCall(PetscOptionsEnum("-dm_swarm_remap_type", "Remap algorithm", "DMSwarmSetRemapType", DMSwarmRemapTypeNames, (PetscEnum)swarm->remap_type, (PetscEnum *)&swarm->remap_type, NULL));
  PetscOptionsHeadEnd();
  PetscFunctionReturn(PETSC_SUCCESS);
}

PETSC_INTERN PetscErrorCode DMClone_Swarm(DM, DM *);

static PetscErrorCode DMInitialize_Swarm(DM sw)
{
  PetscFunctionBegin;
  sw->ops->view                     = DMView_Swarm;
  sw->ops->load                     = NULL;
  sw->ops->setfromoptions           = DMSetFromOptions_Swarm;
  sw->ops->clone                    = DMClone_Swarm;
  sw->ops->setup                    = DMSetup_Swarm;
  sw->ops->createlocalsection       = NULL;
  sw->ops->createsectionpermutation = NULL;
  sw->ops->createdefaultconstraints = NULL;
  sw->ops->createglobalvector       = DMCreateGlobalVector_Swarm;
  sw->ops->createlocalvector        = DMCreateLocalVector_Swarm;
  sw->ops->getlocaltoglobalmapping  = NULL;
  sw->ops->createfieldis            = NULL;
  sw->ops->createcoordinatedm       = NULL;
  sw->ops->createcellcoordinatedm   = NULL;
  sw->ops->getcoloring              = NULL;
  sw->ops->creatematrix             = DMCreateMatrix_Swarm;
  sw->ops->createinterpolation      = NULL;
  sw->ops->createinjection          = NULL;
  sw->ops->createmassmatrix         = DMCreateMassMatrix_Swarm;
  sw->ops->creategradientmatrix     = DMCreateGradientMatrix_Swarm;
  sw->ops->refine                   = NULL;
  sw->ops->coarsen                  = NULL;
  sw->ops->refinehierarchy          = NULL;
  sw->ops->coarsenhierarchy         = NULL;
  sw->ops->globaltolocalbegin       = DMGlobalToLocalBegin_Swarm;
  sw->ops->globaltolocalend         = DMGlobalToLocalEnd_Swarm;
  sw->ops->localtoglobalbegin       = DMLocalToGlobalBegin_Swarm;
  sw->ops->localtoglobalend         = DMLocalToGlobalEnd_Swarm;
  sw->ops->destroy                  = DMDestroy_Swarm;
  sw->ops->createsubdm              = NULL;
  sw->ops->getdimpoints             = NULL;
  sw->ops->locatepoints             = NULL;
  sw->ops->projectfieldlocal        = DMProjectFieldLocal_Swarm;
  PetscFunctionReturn(PETSC_SUCCESS);
}

PETSC_INTERN PetscErrorCode DMClone_Swarm(DM dm, DM *newdm)
{
  DM_Swarm *swarm = (DM_Swarm *)dm->data;

  PetscFunctionBegin;
  swarm->refct++;
  (*newdm)->data = swarm;
  PetscCall(PetscObjectChangeTypeName((PetscObject)*newdm, DMSWARM));
  PetscCall(DMInitialize_Swarm(*newdm));
  (*newdm)->dim = dm->dim;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*MC
 DMSWARM = "swarm" - A `DM` object for particle methods, such as particle-in-cell (PIC), in which the underlying
           data is both (i) dynamic in length, (ii) and of arbitrary data type.

 Level: intermediate

 Notes:
 User data can be represented by `DMSWARM` through a registering "fields" which are to be stored on particles.
 To register a field, the user must provide;
 (a) a unique name;
 (b) the data type (or size in bytes);
 (c) the block size of the data.

 For example, suppose the application requires a unique id, energy, momentum and density to be stored
 on a set of particles. Then the following code could be used
.vb
    DMSwarmInitializeFieldRegister(dm)
    DMSwarmRegisterPetscDatatypeField(dm,"uid",1,PETSC_LONG);
    DMSwarmRegisterPetscDatatypeField(dm,"energy",1,PETSC_REAL);
    DMSwarmRegisterPetscDatatypeField(dm,"momentum",3,PETSC_REAL);
    DMSwarmRegisterPetscDatatypeField(dm,"density",1,PETSC_FLOAT);
    DMSwarmFinalizeFieldRegister(dm)
.ve

 The fields represented by `DMSWARM` are dynamic and can be re-sized at any time.
 The only restriction imposed by `DMSWARM` is that all fields contain the same number of particles.

 To support particle methods, "migration" techniques are provided. These methods migrate data
 between ranks.

 `DMSWARM` supports the methods `DMCreateGlobalVector()` and `DMCreateLocalVector()`.
 As a `DMSWARM` may internally define and store values of different data types,
 before calling `DMCreateGlobalVector()` or `DMCreateLocalVector()`, the user must inform `DMSWARM` which
 fields should be used to define a `Vec` object via `DMSwarmVectorDefineField()`
 The specified field can be changed at any time - thereby permitting vectors
 compatible with different fields to be created.

 A dual representation of fields in the `DMSWARM` and a Vec object is permitted via `DMSwarmCreateGlobalVectorFromField()`
 Here the data defining the field in the `DMSWARM` is shared with a `Vec`.
 This is inherently unsafe if you alter the size of the field at any time between
 calls to `DMSwarmCreateGlobalVectorFromField()` and `DMSwarmDestroyGlobalVectorFromField()`.
 If the local size of the `DMSWARM` does not match the local size of the global vector
 when `DMSwarmDestroyGlobalVectorFromField()` is called, an error is thrown.

 Additional high-level support is provided for Particle-In-Cell methods. Refer to `DMSwarmSetType()`.

.seealso: `DM`, `DMSWARM`, `DMType`, `DMCreate()`, `DMSetType()`, `DMSwarmSetType()`, `DMSwarmType`, `DMSwarmCreateGlobalVectorFromField()`,
         `DMCreateGlobalVector()`, `DMCreateLocalVector()`
M*/

PETSC_EXTERN PetscErrorCode DMCreate_Swarm(DM dm)
{
  DM_Swarm *swarm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(PetscNew(&swarm));
  dm->data = swarm;
  PetscCall(DMSwarmDataBucketCreate(&swarm->db));
  PetscCall(DMSwarmInitializeFieldRegister(dm));
  dm->dim                               = 0;
  swarm->refct                          = 1;
  swarm->issetup                        = PETSC_FALSE;
  swarm->swarm_type                     = DMSWARM_BASIC;
  swarm->migrate_type                   = DMSWARM_MIGRATE_BASIC;
  swarm->collect_type                   = DMSWARM_COLLECT_BASIC;
  swarm->migrate_error_on_missing_point = PETSC_FALSE;
  swarm->collect_view_active            = PETSC_FALSE;
  swarm->collect_view_reset_nlocal      = -1;
  PetscCall(DMInitialize_Swarm(dm));
  if (SwarmDataFieldId == -1) PetscCall(PetscObjectComposedDataRegister(&SwarmDataFieldId));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* Replace dm with the contents of ndm, and then destroy ndm
   - Share the DM_Swarm structure
*/
PetscErrorCode DMSwarmReplace(DM dm, DM *ndm)
{
  DM               dmNew = *ndm;
  const PetscReal *maxCell, *Lstart, *L;
  PetscInt         dim;

  PetscFunctionBegin;
  if (dm == dmNew) {
    PetscCall(DMDestroy(ndm));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  dm->setupcalled = dmNew->setupcalled;
  if (!dm->hdr.name) {
    const char *name;

    PetscCall(PetscObjectGetName((PetscObject)*ndm, &name));
    PetscCall(PetscObjectSetName((PetscObject)dm, name));
  }
  PetscCall(DMGetDimension(dmNew, &dim));
  PetscCall(DMSetDimension(dm, dim));
  PetscCall(DMGetPeriodicity(dmNew, &maxCell, &Lstart, &L));
  PetscCall(DMSetPeriodicity(dm, maxCell, Lstart, L));
  PetscCall(DMDestroy_Swarm(dm));
  PetscCall(DMInitialize_Swarm(dm));
  dm->data = dmNew->data;
  ((DM_Swarm *)dmNew->data)->refct++;
  PetscCall(DMDestroy(ndm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMSwarmDuplicate - Creates a new `DMSWARM` with the same fields and cell `DM`s but no particles

  Collective

  Input Parameter:
. sw - the `DMSWARM`

  Output Parameter:
. nsw - the new `DMSWARM`

  Level: beginner

.seealso: `DM`, `DMSWARM`, `DMSwarmCreate()`, `DMClone()`
@*/
PetscErrorCode DMSwarmDuplicate(DM sw, DM *nsw)
{
  DM_Swarm         *swarm = (DM_Swarm *)sw->data;
  DMSwarmDataField *fields;
  DMSwarmCellDM     celldm, ncelldm;
  DMSwarmType       stype;
  const char       *name, **celldmnames;
  void             *ctx;
  PetscInt          dim, Nf, Ndm;
  PetscBool         flg;

  PetscFunctionBegin;
  PetscCall(DMCreate(PetscObjectComm((PetscObject)sw), nsw));
  PetscCall(DMSetType(*nsw, DMSWARM));
  PetscCall(PetscObjectGetName((PetscObject)sw, &name));
  PetscCall(PetscObjectSetName((PetscObject)*nsw, name));
  PetscCall(DMGetDimension(sw, &dim));
  PetscCall(DMSetDimension(*nsw, dim));
  PetscCall(DMSwarmGetType(sw, &stype));
  PetscCall(DMSwarmSetType(*nsw, stype));
  PetscCall(DMGetApplicationContext(sw, &ctx));
  PetscCall(DMSetApplicationContext(*nsw, ctx));

  PetscCall(DMSwarmDataBucketGetDMSwarmDataFields(swarm->db, &Nf, &fields));
  for (PetscInt f = 0; f < Nf; ++f) {
    PetscCall(DMSwarmDataFieldStringInList(fields[f]->name, ((DM_Swarm *)(*nsw)->data)->db->nfields, (const DMSwarmDataField *)((DM_Swarm *)(*nsw)->data)->db->field, &flg));
    if (!flg) PetscCall(DMSwarmRegisterPetscDatatypeField(*nsw, fields[f]->name, fields[f]->bs, fields[f]->petsc_type));
  }

  PetscCall(DMSwarmGetCellDMNames(sw, &Ndm, &celldmnames));
  for (PetscInt c = 0; c < Ndm; ++c) {
    DM           dm;
    PetscInt     Ncf;
    const char **coordfields, **fields;

    PetscCall(DMSwarmGetCellDMByName(sw, celldmnames[c], &celldm));
    PetscCall(DMSwarmCellDMGetDM(celldm, &dm));
    PetscCall(DMSwarmCellDMGetCoordinateFields(celldm, &Ncf, &coordfields));
    PetscCall(DMSwarmCellDMGetFields(celldm, &Nf, &fields));
    PetscCall(DMSwarmCellDMCreate(dm, Nf, fields, Ncf, coordfields, &ncelldm));
    PetscCall(DMSwarmAddCellDM(*nsw, ncelldm));
    PetscCall(DMSwarmCellDMDestroy(&ncelldm));
  }
  PetscCall(PetscFree(celldmnames));

  PetscCall(DMSetFromOptions(*nsw));
  PetscCall(DMSetUp(*nsw));
  PetscCall(DMSwarmGetCellDMActive(sw, &celldm));
  PetscCall(PetscObjectGetName((PetscObject)celldm, &name));
  PetscCall(DMSwarmSetCellDMActive(*nsw, name));
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode DMLocalToGlobalBegin_Swarm(DM dm, Vec l, InsertMode mode, Vec g)
{
  PetscFunctionBegin;
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode DMLocalToGlobalEnd_Swarm(DM dm, Vec l, InsertMode mode, Vec g)
{
  PetscFunctionBegin;
  switch (mode) {
  case INSERT_VALUES:
    PetscCall(VecCopy(l, g));
    break;
  case ADD_VALUES:
    PetscCall(VecAXPY(g, 1., l));
    break;
  default:
    SETERRQ(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_WRONG, "Mode not supported: %d", mode);
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode DMGlobalToLocalBegin_Swarm(DM dm, Vec g, InsertMode mode, Vec l)
{
  PetscFunctionBegin;
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode DMGlobalToLocalEnd_Swarm(DM dm, Vec g, InsertMode mode, Vec l)
{
  PetscFunctionBegin;
  PetscCall(DMLocalToGlobalEnd_Swarm(dm, g, mode, l));
  PetscFunctionReturn(PETSC_SUCCESS);
}
