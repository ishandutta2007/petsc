#pragma once

#include <petscksp.h>
#include <petscpc.h>
#include <petsc/private/petscimpl.h>

PETSC_EXTERN PetscBool      PCRegisterAllCalled;
PETSC_EXTERN PetscErrorCode PCRegisterAll(void);

typedef struct _PCOps *PCOps;
struct _PCOps {
  PetscErrorCode (*setup)(PC);
  PetscErrorCode (*apply)(PC, Vec, Vec);
  PetscErrorCode (*matapply)(PC, Mat, Mat);
  PetscErrorCode (*applyrichardson)(PC, Vec, Vec, Vec, PetscReal, PetscReal, PetscReal, PetscInt, PetscBool, PetscInt *, PCRichardsonConvergedReason *);
  PetscErrorCode (*applyBA)(PC, PCSide, Vec, Vec, Vec);
  PetscErrorCode (*applytranspose)(PC, Vec, Vec);
  PetscErrorCode (*matapplytranspose)(PC, Mat, Mat);
  PetscErrorCode (*applyBAtranspose)(PC, PetscInt, Vec, Vec, Vec);
  PetscErrorCode (*setfromoptions)(PC, PetscOptionItems);
  PetscErrorCode (*presolve)(PC, KSP, Vec, Vec);
  PetscErrorCode (*postsolve)(PC, KSP, Vec, Vec);
  PetscErrorCode (*getfactoredmatrix)(PC, Mat *);
  PetscErrorCode (*applysymmetricleft)(PC, Vec, Vec);
  PetscErrorCode (*applysymmetricright)(PC, Vec, Vec);
  PetscErrorCode (*setuponblocks)(PC);
  PetscErrorCode (*destroy)(PC);
  PetscErrorCode (*view)(PC, PetscViewer);
  PetscErrorCode (*reset)(PC);
  PetscErrorCode (*load)(PC, PetscViewer);
};

/*
   Preconditioner context
*/
struct _p_PC {
  PETSCHEADER(struct _PCOps);
  DM               dm;
  PetscBool        setupcalled;
  PetscObjectState matstate, matnonzerostate; /* last known nonzero state of the pmat associated with this PC */
  PetscBool        reusepreconditioner;
  MatStructure     flag; /* reset each PCSetUp() to indicate to PC implementations if nonzero structure has changed */

  PetscInt  setfromoptionscalled;
  PetscBool erroriffailure; /* Generate an error if FPE detected (for example a zero pivot) instead of returning*/
  Mat       mat, pmat;
  Vec       diagonalscaleright, diagonalscaleleft; /* used for time integration scaling */
  PetscBool diagonalscale;
  PetscBool useAmat;                                                                        /* used by several PC that including applying the operator inside the preconditioner */
  PetscErrorCode (*modifysubmatrices)(PC, PetscInt, const IS[], const IS[], Mat[], void *); /* user provided routine */
  void          *modifysubmatricesP;                                                        /* context for user routine */
  void          *data;
  void          *ctx;              /* optional user-defined context */
  PCFailedReason failedreason;     /* after VecNorm or VecDot contains maximum of all rank failed reasons */
  PCFailedReason failedreasonrank; /* failed reason on this rank */
  PetscInt       presolvedone;
  PetscErrorCode (*postsetup)(PC);
  PetscInt kspnestlevel; /* how many levels of nesting does the KSP have that contains the PC */
};

PETSC_EXTERN PetscLogEvent PC_SetUp;
PETSC_EXTERN PetscLogEvent PC_SetUpOnBlocks;
PETSC_EXTERN PetscLogEvent PC_Apply;
PETSC_EXTERN PetscLogEvent PC_MatApply;
PETSC_EXTERN PetscLogEvent PC_ApplyCoarse;
PETSC_EXTERN PetscLogEvent PC_ApplySymmetricLeft;
PETSC_EXTERN PetscLogEvent PC_ApplySymmetricRight;
PETSC_EXTERN PetscLogEvent PC_ModifySubMatrices;
PETSC_EXTERN PetscLogEvent PC_ApplyOnBlocks;
PETSC_EXTERN PetscLogEvent PC_ApplyTransposeOnBlocks;
PETSC_EXTERN PetscLogStage PCMPIStage;
