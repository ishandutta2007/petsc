#include <petsc/private/dmimpl.h>
#include <petsc/private/snesimpl.h> /*I "petscsnes.h" I*/

typedef struct {
  PetscErrorCode (*objectivelocal)(DM, Vec, PetscReal *, void *);
  PetscErrorCode (*residuallocal)(DM, Vec, Vec, void *);
  PetscErrorCode (*jacobianlocal)(DM, Vec, Mat, Mat, void *);
  PetscErrorCode (*boundarylocal)(DM, Vec, void *);
  void *objectivelocalctx;
  void *residuallocalctx;
  void *jacobianlocalctx;
  void *boundarylocalctx;
} DMSNES_Local;

static PetscErrorCode DMSNESDestroy_DMLocal(DMSNES sdm)
{
  PetscFunctionBegin;
  PetscCall(PetscFree(sdm->data));
  sdm->data = NULL;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMSNESDuplicate_DMLocal(DMSNES oldsdm, DMSNES sdm)
{
  PetscFunctionBegin;
  if (sdm->data != oldsdm->data) {
    PetscCall(PetscFree(sdm->data));
    PetscCall(PetscNew((DMSNES_Local **)&sdm->data));
    if (oldsdm->data) PetscCall(PetscMemcpy(sdm->data, oldsdm->data, sizeof(DMSNES_Local)));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode DMLocalSNESGetContext(DM dm, DMSNES sdm, DMSNES_Local **dmlocalsnes)
{
  PetscFunctionBegin;
  *dmlocalsnes = NULL;
  if (!sdm->data) {
    PetscCall(PetscNew((DMSNES_Local **)&sdm->data));

    sdm->ops->destroy   = DMSNESDestroy_DMLocal;
    sdm->ops->duplicate = DMSNESDuplicate_DMLocal;
  }
  *dmlocalsnes = (DMSNES_Local *)sdm->data;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode SNESComputeObjective_DMLocal(SNES snes, Vec X, PetscReal *obj, void *ctx)
{
  DMSNES_Local *dmlocalsnes = (DMSNES_Local *)ctx;
  DM            dm;
  Vec           Xloc;
  PetscBool     transform;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(snes, SNES_CLASSID, 1);
  PetscValidHeaderSpecific(X, VEC_CLASSID, 2);
  PetscCall(SNESGetDM(snes, &dm));
  PetscCall(DMGetLocalVector(dm, &Xloc));
  PetscCall(VecZeroEntries(Xloc));
  /* Non-conforming routines needs boundary values before G2L */
  if (dmlocalsnes->boundarylocal) PetscCall((*dmlocalsnes->boundarylocal)(dm, Xloc, dmlocalsnes->boundarylocalctx));
  PetscCall(DMGlobalToLocalBegin(dm, X, INSERT_VALUES, Xloc));
  PetscCall(DMGlobalToLocalEnd(dm, X, INSERT_VALUES, Xloc));
  /* Need to reset boundary values if we transformed */
  PetscCall(DMHasBasisTransform(dm, &transform));
  if (transform && dmlocalsnes->boundarylocal) PetscCall((*dmlocalsnes->boundarylocal)(dm, Xloc, dmlocalsnes->boundarylocalctx));
  CHKMEMQ;
  PetscCall((*dmlocalsnes->objectivelocal)(dm, Xloc, obj, dmlocalsnes->objectivelocalctx));
  CHKMEMQ;
  PetscCallMPI(MPIU_Allreduce(MPI_IN_PLACE, obj, 1, MPIU_REAL, MPIU_SUM, PetscObjectComm((PetscObject)snes)));
  PetscCall(DMRestoreLocalVector(dm, &Xloc));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode SNESComputeFunction_DMLocal(SNES snes, Vec X, Vec F, void *ctx)
{
  DMSNES_Local *dmlocalsnes = (DMSNES_Local *)ctx;
  DM            dm;
  Vec           Xloc, Floc;
  PetscBool     transform;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(snes, SNES_CLASSID, 1);
  PetscValidHeaderSpecific(X, VEC_CLASSID, 2);
  PetscValidHeaderSpecific(F, VEC_CLASSID, 3);
  PetscCall(SNESGetDM(snes, &dm));
  PetscCall(DMGetLocalVector(dm, &Xloc));
  PetscCall(DMGetLocalVector(dm, &Floc));
  PetscCall(VecZeroEntries(Xloc));
  PetscCall(VecZeroEntries(Floc));
  /* Non-conforming routines needs boundary values before G2L */
  if (dmlocalsnes->boundarylocal) PetscCall((*dmlocalsnes->boundarylocal)(dm, Xloc, dmlocalsnes->boundarylocalctx));
  PetscCall(DMGlobalToLocalBegin(dm, X, INSERT_VALUES, Xloc));
  PetscCall(DMGlobalToLocalEnd(dm, X, INSERT_VALUES, Xloc));
  /* Need to reset boundary values if we transformed */
  PetscCall(DMHasBasisTransform(dm, &transform));
  if (transform && dmlocalsnes->boundarylocal) PetscCall((*dmlocalsnes->boundarylocal)(dm, Xloc, dmlocalsnes->boundarylocalctx));
  CHKMEMQ;
  PetscCall((*dmlocalsnes->residuallocal)(dm, Xloc, Floc, dmlocalsnes->residuallocalctx));
  CHKMEMQ;
  PetscCall(VecZeroEntries(F));
  PetscCall(DMLocalToGlobalBegin(dm, Floc, ADD_VALUES, F));
  PetscCall(DMLocalToGlobalEnd(dm, Floc, ADD_VALUES, F));
  PetscCall(DMRestoreLocalVector(dm, &Floc));
  PetscCall(DMRestoreLocalVector(dm, &Xloc));
  {
    char        name[PETSC_MAX_PATH_LEN];
    char        oldname[PETSC_MAX_PATH_LEN];
    const char *tmp;
    PetscInt    it;

    PetscCall(SNESGetIterationNumber(snes, &it));
    PetscCall(PetscSNPrintf(name, PETSC_MAX_PATH_LEN, "Solution, Iterate %" PetscInt_FMT, it));
    PetscCall(PetscObjectGetName((PetscObject)X, &tmp));
    PetscCall(PetscStrncpy(oldname, tmp, PETSC_MAX_PATH_LEN - 1));
    PetscCall(PetscObjectSetName((PetscObject)X, name));
    PetscCall(VecViewFromOptions(X, (PetscObject)snes, "-dmsnes_solution_vec_view"));
    PetscCall(PetscObjectSetName((PetscObject)X, oldname));
    PetscCall(PetscSNPrintf(name, PETSC_MAX_PATH_LEN, "Residual, Iterate %" PetscInt_FMT, it));
    PetscCall(PetscObjectSetName((PetscObject)F, name));
    PetscCall(VecViewFromOptions(F, (PetscObject)snes, "-dmsnes_residual_vec_view"));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode SNESComputeJacobian_DMLocal(SNES snes, Vec X, Mat A, Mat B, void *ctx)
{
  DMSNES_Local *dmlocalsnes = (DMSNES_Local *)ctx;
  DM            dm;
  Vec           Xloc;
  PetscBool     transform;

  PetscFunctionBegin;
  PetscCall(SNESGetDM(snes, &dm));
  if (dmlocalsnes->jacobianlocal) {
    PetscCall(DMGetLocalVector(dm, &Xloc));
    PetscCall(VecZeroEntries(Xloc));
    /* Non-conforming routines needs boundary values before G2L */
    if (dmlocalsnes->boundarylocal) PetscCall((*dmlocalsnes->boundarylocal)(dm, Xloc, dmlocalsnes->boundarylocalctx));
    PetscCall(DMGlobalToLocalBegin(dm, X, INSERT_VALUES, Xloc));
    PetscCall(DMGlobalToLocalEnd(dm, X, INSERT_VALUES, Xloc));
    /* Need to reset boundary values if we transformed */
    PetscCall(DMHasBasisTransform(dm, &transform));
    if (transform && dmlocalsnes->boundarylocal) PetscCall((*dmlocalsnes->boundarylocal)(dm, Xloc, dmlocalsnes->boundarylocalctx));
    CHKMEMQ;
    PetscCall((*dmlocalsnes->jacobianlocal)(dm, Xloc, A, B, dmlocalsnes->jacobianlocalctx));
    CHKMEMQ;
    PetscCall(DMRestoreLocalVector(dm, &Xloc));
  } else {
    MatFDColoring fdcoloring;
    PetscCall(PetscObjectQuery((PetscObject)dm, "DMDASNES_FDCOLORING", (PetscObject *)&fdcoloring));
    if (!fdcoloring) {
      ISColoring coloring;

      PetscCall(DMCreateColoring(dm, dm->coloringtype, &coloring));
      PetscCall(MatFDColoringCreate(B, coloring, &fdcoloring));
      PetscCall(ISColoringDestroy(&coloring));
      switch (dm->coloringtype) {
      case IS_COLORING_GLOBAL:
        PetscCall(MatFDColoringSetFunction(fdcoloring, (MatFDColoringFn *)SNESComputeFunction_DMLocal, dmlocalsnes));
        break;
      default:
        SETERRQ(PetscObjectComm((PetscObject)snes), PETSC_ERR_SUP, "No support for coloring type '%s'", ISColoringTypes[dm->coloringtype]);
      }
      PetscCall(PetscObjectSetOptionsPrefix((PetscObject)fdcoloring, ((PetscObject)dm)->prefix));
      PetscCall(MatFDColoringSetFromOptions(fdcoloring));
      PetscCall(MatFDColoringSetUp(B, coloring, fdcoloring));
      PetscCall(PetscObjectCompose((PetscObject)dm, "DMDASNES_FDCOLORING", (PetscObject)fdcoloring));
      PetscCall(PetscObjectDereference((PetscObject)fdcoloring));

      /* The following breaks an ugly reference counting loop that deserves a paragraph. MatFDColoringApply() will call
       * VecDuplicate() with the state Vec and store inside the MatFDColoring. This Vec will duplicate the Vec, but the
       * MatFDColoring is composed with the DM. We dereference the DM here so that the reference count will eventually
       * drop to 0. Note the code in DMDestroy() that exits early for a negative reference count. That code path will be
       * taken when the PetscObjectList for the Vec inside MatFDColoring is destroyed.
       */
      PetscCall(PetscObjectDereference((PetscObject)dm));
    }
    PetscCall(MatFDColoringApply(B, fdcoloring, X, snes));
  }
  /* This will be redundant if the user called both, but it's too common to forget. */
  if (A != B) {
    PetscCall(MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY));
    PetscCall(MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSNESSetObjectiveLocal - set a local objective evaluation function. This function is called with local vector
  containing the local vector information PLUS ghost point information. It should compute a result for all local
  elements and `DMSNES` will automatically accumulate the overlapping values.

  Logically Collective

  Input Parameters:
+ dm   - `DM` to associate callback with
. func - local objective evaluation
- ctx  - optional context for local residual evaluation

  Level: advanced

.seealso: `DMSNESSetFunctionLocal()`, `DMSNESSetJacobianLocal()`
@*/
PetscErrorCode DMSNESSetObjectiveLocal(DM dm, PetscErrorCode (*func)(DM, Vec, PetscReal *, void *), void *ctx)
{
  DMSNES        sdm;
  DMSNES_Local *dmlocalsnes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(DMGetDMSNESWrite(dm, &sdm));
  PetscCall(DMLocalSNESGetContext(dm, sdm, &dmlocalsnes));

  dmlocalsnes->objectivelocal    = func;
  dmlocalsnes->objectivelocalctx = ctx;

  PetscCall(DMSNESSetObjective(dm, SNESComputeObjective_DMLocal, dmlocalsnes));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSNESSetFunctionLocal - set a local residual evaluation function. This function is called with local vector
  containing the local vector information PLUS ghost point information. It should compute a result for all local
  elements and `DMSNES` will automatically accumulate the overlapping values.

  Logically Collective

  Input Parameters:
+ dm   - `DM` to associate callback with
. func - local residual evaluation
- ctx  - optional context for local residual evaluation

  Calling sequence of `func`:
+ dm  - `DM` for the function
. x   - vector to state at which to evaluate residual
. f   - vector to hold the function evaluation
- ctx - optional context passed above

  Level: advanced

.seealso: [](ch_snes), `DMSNESSetFunction()`, `DMSNESSetJacobianLocal()`
@*/
PetscErrorCode DMSNESSetFunctionLocal(DM dm, PetscErrorCode (*func)(DM dm, Vec x, Vec f, void *ctx), void *ctx)
{
  DMSNES        sdm;
  DMSNES_Local *dmlocalsnes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(DMGetDMSNESWrite(dm, &sdm));
  PetscCall(DMLocalSNESGetContext(dm, sdm, &dmlocalsnes));

  dmlocalsnes->residuallocal    = func;
  dmlocalsnes->residuallocalctx = ctx;

  PetscCall(DMSNESSetFunction(dm, SNESComputeFunction_DMLocal, dmlocalsnes));
  if (!sdm->ops->computejacobian) { /* Call us for the Jacobian too, can be overridden by the user. */
    PetscCall(DMSNESSetJacobian(dm, SNESComputeJacobian_DMLocal, dmlocalsnes));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSNESSetBoundaryLocal - set a function to insert, for example, essential boundary conditions into a ghosted solution vector

  Logically Collective

  Input Parameters:
+ dm   - `DM` to associate callback with
. func - local boundary value evaluation
- ctx  - optional context for local boundary value evaluation

  Calling sequence of `func`:
+ dm  - the `DM` context
. X   - ghosted solution vector, appropriate locations (such as essential boundary condition nodes) should be filled
- ctx - option context passed in `DMSNESSetBoundaryLocal()`

  Level: advanced

.seealso: [](ch_snes), `DMSNESSetObjectiveLocal()`, `DMSNESSetFunctionLocal()`, `DMSNESSetJacobianLocal()`
@*/
PetscErrorCode DMSNESSetBoundaryLocal(DM dm, PetscErrorCode (*func)(DM dm, Vec X, void *ctx), void *ctx)
{
  DMSNES        sdm;
  DMSNES_Local *dmlocalsnes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(DMGetDMSNESWrite(dm, &sdm));
  PetscCall(DMLocalSNESGetContext(dm, sdm, &dmlocalsnes));

  dmlocalsnes->boundarylocal    = func;
  dmlocalsnes->boundarylocalctx = ctx;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSNESSetJacobianLocal - set a local Jacobian evaluation function

  Logically Collective

  Input Parameters:
+ dm   - `DM` to associate callback with
. func - local Jacobian evaluation
- ctx  - optional context for local Jacobian evaluation

  Calling sequence of `func`:
+ dm  - the `DM` context
. X   - current solution vector (ghosted or not?)
. J   - the Jacobian
. Jp  - approximate Jacobian used to compute the preconditioner, often `J`
- ctx - a user provided context

  Level: advanced

.seealso: [](ch_snes), `DMSNESSetObjectiveLocal()`, `DMSNESSetFunctionLocal()`, `DMSNESSetBoundaryLocal()`
@*/
PetscErrorCode DMSNESSetJacobianLocal(DM dm, PetscErrorCode (*func)(DM dm, Vec X, Mat J, Mat Jp, void *ctx), void *ctx)
{
  DMSNES        sdm;
  DMSNES_Local *dmlocalsnes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(DMGetDMSNESWrite(dm, &sdm));
  PetscCall(DMLocalSNESGetContext(dm, sdm, &dmlocalsnes));

  dmlocalsnes->jacobianlocal    = func;
  dmlocalsnes->jacobianlocalctx = ctx;

  PetscCall(DMSNESSetJacobian(dm, SNESComputeJacobian_DMLocal, dmlocalsnes));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSNESGetObjectiveLocal - get the local objective evaluation function information set with `DMSNESSetObjectiveLocal()`.

  Not Collective

  Input Parameter:
. dm - `DM` with the associated callback

  Output Parameters:
+ func - local objective evaluation
- ctx  - context for local residual evaluation

  Level: beginner

.seealso: `DMSNESSetObjective()`, `DMSNESSetObjectiveLocal()`, `DMSNESSetFunctionLocal()`
@*/
PetscErrorCode DMSNESGetObjectiveLocal(DM dm, PetscErrorCode (**func)(DM, Vec, PetscReal *, void *), void **ctx)
{
  DMSNES        sdm;
  DMSNES_Local *dmlocalsnes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(DMGetDMSNES(dm, &sdm));
  PetscCall(DMLocalSNESGetContext(dm, sdm, &dmlocalsnes));
  if (func) *func = dmlocalsnes->objectivelocal;
  if (ctx) *ctx = dmlocalsnes->objectivelocalctx;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSNESGetFunctionLocal - get the local residual evaluation function information set with `DMSNESSetFunctionLocal()`.

  Not Collective

  Input Parameter:
. dm - `DM` with the associated callback

  Output Parameters:
+ func - local residual evaluation
- ctx  - context for local residual evaluation

  Level: beginner

.seealso: [](ch_snes), `DMSNESSetFunction()`, `DMSNESSetFunctionLocal()`, `DMSNESSetJacobianLocal()`
@*/
PetscErrorCode DMSNESGetFunctionLocal(DM dm, PetscErrorCode (**func)(DM, Vec, Vec, void *), void **ctx)
{
  DMSNES        sdm;
  DMSNES_Local *dmlocalsnes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(DMGetDMSNES(dm, &sdm));
  PetscCall(DMLocalSNESGetContext(dm, sdm, &dmlocalsnes));
  if (func) *func = dmlocalsnes->residuallocal;
  if (ctx) *ctx = dmlocalsnes->residuallocalctx;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSNESGetBoundaryLocal - get the local boundary value function set with `DMSNESSetBoundaryLocal()`.

  Not Collective

  Input Parameter:
. dm - `DM` with the associated callback

  Output Parameters:
+ func - local boundary value evaluation
- ctx  - context for local boundary value evaluation

  Level: intermediate

.seealso: [](ch_snes), `DMSNESSetFunctionLocal()`, `DMSNESSetBoundaryLocal()`, `DMSNESSetJacobianLocal()`
@*/
PetscErrorCode DMSNESGetBoundaryLocal(DM dm, PetscErrorCode (**func)(DM, Vec, void *), void **ctx)
{
  DMSNES        sdm;
  DMSNES_Local *dmlocalsnes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(DMGetDMSNES(dm, &sdm));
  PetscCall(DMLocalSNESGetContext(dm, sdm, &dmlocalsnes));
  if (func) *func = dmlocalsnes->boundarylocal;
  if (ctx) *ctx = dmlocalsnes->boundarylocalctx;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMSNESGetJacobianLocal - the local Jacobian evaluation function set with `DMSNESSetJacobianLocal()`.

  Logically Collective

  Input Parameter:
. dm - `DM` with the associated callback

  Output Parameters:
+ func - local Jacobian evaluation
- ctx  - context for local Jacobian evaluation

  Level: beginner

.seealso: [](ch_snes), `DMSNESSetJacobianLocal()`, `DMSNESSetJacobian()`
@*/
PetscErrorCode DMSNESGetJacobianLocal(DM dm, PetscErrorCode (**func)(DM, Vec, Mat, Mat, void *), void **ctx)
{
  DMSNES        sdm;
  DMSNES_Local *dmlocalsnes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscCall(DMGetDMSNES(dm, &sdm));
  PetscCall(DMLocalSNESGetContext(dm, sdm, &dmlocalsnes));
  if (func) *func = dmlocalsnes->jacobianlocal;
  if (ctx) *ctx = dmlocalsnes->jacobianlocalctx;
  PetscFunctionReturn(PETSC_SUCCESS);
}
