/*
      Preconditioner module.
*/
#pragma once

#include <petscmat.h>
#include <petscdmtypes.h>
#include <petscpctypes.h>

/* MANSEC = KSP */
/* SUBMANSEC = PC */

PETSC_EXTERN PetscErrorCode PCInitializePackage(void);
PETSC_EXTERN PetscErrorCode PCFinalizePackage(void);

/*
    PCList contains the list of preconditioners currently registered
   These are added with PCRegister()
*/
PETSC_EXTERN PetscFunctionList PCList;

/* Logging support */
PETSC_EXTERN PetscClassId PC_CLASSID;

/* Arrays of names for options in implementation PCs */
PETSC_EXTERN const char *const *const PCSides;
PETSC_EXTERN const char *const        PCJacobiTypes[];
PETSC_EXTERN const char *const        PCASMTypes[];
PETSC_EXTERN const char *const        PCGASMTypes[];
PETSC_EXTERN const char *const        PCCompositeTypes[];
PETSC_EXTERN const char *const        PCFieldSplitSchurPreTypes[];
PETSC_EXTERN const char *const        PCFieldSplitSchurFactTypes[];
PETSC_EXTERN const char *const        PCPARMSGlobalTypes[];
PETSC_EXTERN const char *const        PCPARMSLocalTypes[];
PETSC_EXTERN const char *const        PCMGTypes[];
PETSC_EXTERN const char *const        PCMGCycleTypes[];
PETSC_EXTERN const char *const        PCMGGalerkinTypes[];
PETSC_EXTERN const char *const        PCMGCoarseSpaceTypes[];
PETSC_EXTERN const char *const        PCExoticTypes[];
PETSC_EXTERN const char *const        PCPatchConstructTypes[];
PETSC_EXTERN const char *const        PCDeflationTypes[];
PETSC_EXTERN const char *const *const PCFailedReasons;

PETSC_EXTERN PetscErrorCode PCCreate(MPI_Comm, PC *);
PETSC_EXTERN PetscErrorCode PCSetType(PC, PCType);
PETSC_EXTERN PetscErrorCode PCGetType(PC, PCType *);
PETSC_EXTERN PetscErrorCode PCSetUp(PC);

PETSC_EXTERN PetscErrorCode PCSetKSPNestLevel(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGetKSPNestLevel(PC, PetscInt *);

PETSC_EXTERN PetscErrorCode PCSetFailedReason(PC, PCFailedReason);
PETSC_EXTERN PetscErrorCode PCGetFailedReason(PC, PCFailedReason *);
PETSC_DEPRECATED_FUNCTION(3, 11, 0, "PCGetFailedReason()", ) static inline PetscErrorCode PCGetSetUpFailedReason(PC pc, PCFailedReason *reason)
{
  return PCGetFailedReason(pc, reason);
}
PETSC_EXTERN PetscErrorCode PCReduceFailedReason(PC);

PETSC_EXTERN PetscErrorCode PCSetUpOnBlocks(PC);
PETSC_EXTERN PetscErrorCode PCApply(PC, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCMatApply(PC, Mat, Mat);
PETSC_EXTERN PetscErrorCode PCApplySymmetricLeft(PC, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCApplySymmetricRight(PC, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCApplyBAorAB(PC, PCSide, Vec, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCApplyTranspose(PC, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCMatApplyTranspose(PC, Mat, Mat);
PETSC_EXTERN PetscErrorCode PCApplyTransposeExists(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCApplyBAorABTranspose(PC, PCSide, Vec, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCSetReusePreconditioner(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGetReusePreconditioner(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCSetErrorIfFailure(PC, PetscBool);

#define PC_FILE_CLASSID 1211222

PETSC_EXTERN PetscErrorCode PCApplyRichardson(PC, Vec, Vec, Vec, PetscReal, PetscReal, PetscReal, PetscInt, PetscBool, PetscInt *, PCRichardsonConvergedReason *);
PETSC_EXTERN PetscErrorCode PCApplyRichardsonExists(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCSetUseAmat(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGetUseAmat(PC, PetscBool *);

PETSC_EXTERN PetscErrorCode PCRegister(const char[], PetscErrorCode (*)(PC));

PETSC_EXTERN PetscErrorCode PCReset(PC);
PETSC_EXTERN PetscErrorCode PCDestroy(PC *);
PETSC_EXTERN PetscErrorCode PCSetFromOptions(PC);

PETSC_EXTERN PetscErrorCode PCFactorGetMatrix(PC, Mat *);

/*S
  PCModifySubMatricesFn - A prototype of a function used to modify submatrices generated with `PCASM`, `PCBJACOBI`, etc.

  Calling Sequence:
+ pc     - the `PC` preconditioner context
. nsub   - number of index sets
. row    - an array of index sets that contain the global row numbers
         that comprise each local submatrix
. col    - an array of index sets that contain the global column numbers
         that comprise each local submatrix
. submat - array of local submatrices
- ctx    - optional user-defined context for private data for the user-defined func routine (may be `NULL`), provided with `PCSetModifySubMatrices()`

  Level: beginner

.seealso: [](ch_ksp), `PC`, `PCSetModifySubMatrices()`, `PCModifySubMatrices()`, `PCASM`, `PCBJACOBI`
S*/
PETSC_EXTERN_TYPEDEF typedef PetscErrorCode PCModifySubMatricesFn(PC pc, PetscInt nsub, const IS row[], const IS col[], Mat submat[], void *ctx);

PETSC_EXTERN PetscErrorCode        PCSetModifySubMatrices(PC, PCModifySubMatricesFn *, void *);
PETSC_EXTERN PCModifySubMatricesFn PCModifySubMatrices;

PETSC_EXTERN PetscErrorCode PCSetOperators(PC, Mat, Mat);
PETSC_EXTERN PetscErrorCode PCGetOperators(PC, Mat *, Mat *);
PETSC_EXTERN PetscErrorCode PCGetOperatorsSet(PC, PetscBool *, PetscBool *);

PETSC_EXTERN PetscErrorCode PCView(PC, PetscViewer);
PETSC_EXTERN PetscErrorCode PCLoad(PC, PetscViewer);
PETSC_EXTERN PetscErrorCode PCViewFromOptions(PC, PetscObject, const char[]);

PETSC_EXTERN PetscErrorCode PCSetOptionsPrefix(PC, const char[]);
PETSC_EXTERN PetscErrorCode PCAppendOptionsPrefix(PC, const char[]);
PETSC_EXTERN PetscErrorCode PCGetOptionsPrefix(PC, const char *[]);

PETSC_EXTERN PetscErrorCode PCComputeOperator(PC, MatType, Mat *);
PETSC_DEPRECATED_FUNCTION(3, 12, 0, "PCComputeOperator()", ) static inline PetscErrorCode PCComputeExplicitOperator(PC A, Mat *B)
{
  return PCComputeOperator(A, PETSC_NULLPTR, B);
}

PETSC_EXTERN PetscErrorCode PCSetPostSetUp(PC, PetscErrorCode (*)(PC));

/*
      These are used to provide extra scaling of preconditioned
   operator for time-stepping schemes like in SUNDIALS
*/
PETSC_EXTERN PetscErrorCode PCGetDiagonalScale(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCDiagonalScaleLeft(PC, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCDiagonalScaleRight(PC, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCSetDiagonalScale(PC, Vec);

PETSC_EXTERN PetscErrorCode PCSetDM(PC, DM);
PETSC_EXTERN PetscErrorCode PCGetDM(PC, DM *);

PETSC_EXTERN PetscErrorCode PCGetInterpolations(PC, PetscInt *, Mat *[]);
PETSC_EXTERN PetscErrorCode PCGetCoarseOperators(PC, PetscInt *, Mat *[]);

PETSC_EXTERN PetscErrorCode PCSetCoordinates(PC, PetscInt, PetscInt, PetscReal[]);

PETSC_EXTERN PetscErrorCode PCSetApplicationContext(PC, void *);
PETSC_EXTERN PetscErrorCode PCGetApplicationContext(PC, void *);

/* ------------- options specific to particular preconditioners --------- */

PETSC_EXTERN PetscErrorCode PCJacobiSetType(PC, PCJacobiType);
PETSC_EXTERN PetscErrorCode PCJacobiGetType(PC, PCJacobiType *);
PETSC_EXTERN PetscErrorCode PCJacobiSetUseAbs(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCJacobiGetUseAbs(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCJacobiSetFixDiagonal(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCJacobiGetFixDiagonal(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCJacobiGetDiagonal(PC, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCJacobiSetRowl1Scale(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCJacobiGetRowl1Scale(PC, PetscReal *);
PETSC_EXTERN PetscErrorCode PCSORSetSymmetric(PC, MatSORType);
PETSC_EXTERN PetscErrorCode PCSORGetSymmetric(PC, MatSORType *);
PETSC_EXTERN PetscErrorCode PCSORSetOmega(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCSORGetOmega(PC, PetscReal *);
PETSC_EXTERN PetscErrorCode PCSORSetIterations(PC, PetscInt, PetscInt);
PETSC_EXTERN PetscErrorCode PCSORGetIterations(PC, PetscInt *, PetscInt *);

PETSC_EXTERN PetscErrorCode PCEisenstatSetOmega(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCEisenstatGetOmega(PC, PetscReal *);
PETSC_EXTERN PetscErrorCode PCEisenstatSetNoDiagonalScaling(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCEisenstatGetNoDiagonalScaling(PC, PetscBool *);

PETSC_EXTERN PetscErrorCode PCBJacobiSetTotalBlocks(PC, PetscInt, const PetscInt[]);
PETSC_EXTERN PetscErrorCode PCBJacobiGetTotalBlocks(PC, PetscInt *, const PetscInt *[]);
PETSC_EXTERN PetscErrorCode PCBJacobiSetLocalBlocks(PC, PetscInt, const PetscInt[]);
PETSC_EXTERN PetscErrorCode PCBJacobiGetLocalBlocks(PC, PetscInt *, const PetscInt *[]);

PETSC_EXTERN PetscErrorCode PCShellSetApply(PC, PetscErrorCode (*)(PC, Vec, Vec));
PETSC_EXTERN PetscErrorCode PCShellSetMatApply(PC, PetscErrorCode (*)(PC, Mat, Mat));
PETSC_EXTERN PetscErrorCode PCShellSetApplySymmetricLeft(PC, PetscErrorCode (*)(PC, Vec, Vec));
PETSC_EXTERN PetscErrorCode PCShellSetApplySymmetricRight(PC, PetscErrorCode (*)(PC, Vec, Vec));
PETSC_EXTERN PetscErrorCode PCShellSetApplyBA(PC, PetscErrorCode (*)(PC, PCSide, Vec, Vec, Vec));
PETSC_EXTERN PetscErrorCode PCShellSetApplyTranspose(PC, PetscErrorCode (*)(PC, Vec, Vec));
PETSC_EXTERN PetscErrorCode PCShellSetMatApplyTranspose(PC, PetscErrorCode (*)(PC, Mat, Mat));
PETSC_EXTERN PetscErrorCode PCShellSetSetUp(PC, PetscErrorCode (*)(PC));
PETSC_EXTERN PetscErrorCode PCShellSetApplyRichardson(PC, PetscErrorCode (*)(PC, Vec, Vec, Vec, PetscReal, PetscReal, PetscReal, PetscInt, PetscBool, PetscInt *, PCRichardsonConvergedReason *));
PETSC_EXTERN PetscErrorCode PCShellSetView(PC, PetscErrorCode (*)(PC, PetscViewer));
PETSC_EXTERN PetscErrorCode PCShellSetDestroy(PC, PetscErrorCode (*)(PC));
PETSC_EXTERN PetscErrorCode PCShellSetContext(PC, void *);
PETSC_EXTERN PetscErrorCode PCShellGetContext(PC, void *);
PETSC_EXTERN PetscErrorCode PCShellSetName(PC, const char[]);
PETSC_EXTERN PetscErrorCode PCShellGetName(PC, const char *[]);

PETSC_EXTERN PetscErrorCode PCFactorSetZeroPivot(PC, PetscReal);

PETSC_EXTERN PetscErrorCode PCFactorSetShiftType(PC, MatFactorShiftType);
PETSC_EXTERN PetscErrorCode PCFactorSetShiftAmount(PC, PetscReal);

PETSC_EXTERN PetscErrorCode PCFactorSetMatSolverType(PC, MatSolverType);
PETSC_EXTERN PetscErrorCode PCFactorGetMatSolverType(PC, MatSolverType *);
PETSC_EXTERN PetscErrorCode PCFactorSetUpMatSolverType(PC);
PETSC_DEPRECATED_FUNCTION(3, 9, 0, "PCFactorSetMatSolverType()", ) static inline PetscErrorCode PCFactorSetMatSolverPackage(PC pc, MatSolverType stype)
{
  return PCFactorSetMatSolverType(pc, stype);
}
PETSC_DEPRECATED_FUNCTION(3, 9, 0, "PCFactorGetMatSolverType()", ) static inline PetscErrorCode PCFactorGetMatSolverPackage(PC pc, MatSolverType *stype)
{
  return PCFactorGetMatSolverType(pc, stype);
}
PETSC_DEPRECATED_FUNCTION(3, 9, 0, "PCFactorSetUpMatSolverType()", ) static inline PetscErrorCode PCFactorSetUpMatSolverPackage(PC pc)
{
  return PCFactorSetUpMatSolverType(pc);
}

PETSC_EXTERN PetscErrorCode PCFactorSetFill(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCFactorSetColumnPivot(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCFactorReorderForNonzeroDiagonal(PC, PetscReal);

PETSC_EXTERN PetscErrorCode PCFactorSetMatOrderingType(PC, MatOrderingType);
PETSC_EXTERN PetscErrorCode PCFactorSetReuseOrdering(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCFactorSetReuseFill(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCFactorSetUseInPlace(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCFactorGetUseInPlace(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCFactorSetAllowDiagonalFill(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCFactorGetAllowDiagonalFill(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCFactorSetPivotInBlocks(PC, PetscBool);

PETSC_EXTERN PetscErrorCode PCFactorSetLevels(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCFactorGetLevels(PC, PetscInt *);
PETSC_EXTERN PetscErrorCode PCFactorSetDropTolerance(PC, PetscReal, PetscReal, PetscInt);
PETSC_EXTERN PetscErrorCode PCFactorGetZeroPivot(PC, PetscReal *);
PETSC_EXTERN PetscErrorCode PCFactorGetShiftAmount(PC, PetscReal *);
PETSC_EXTERN PetscErrorCode PCFactorGetShiftType(PC, MatFactorShiftType *);

PETSC_EXTERN PetscErrorCode PCASMSetLocalSubdomains(PC, PetscInt, IS[], IS[]);
PETSC_EXTERN PetscErrorCode PCASMSetTotalSubdomains(PC, PetscInt, IS[], IS[]);
PETSC_EXTERN PetscErrorCode PCASMSetOverlap(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCASMSetDMSubdomains(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCASMGetDMSubdomains(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCASMSetSortIndices(PC, PetscBool);

PETSC_EXTERN PetscErrorCode PCASMSetType(PC, PCASMType);
PETSC_EXTERN PetscErrorCode PCASMGetType(PC, PCASMType *);
PETSC_EXTERN PetscErrorCode PCASMSetLocalType(PC, PCCompositeType);
PETSC_EXTERN PetscErrorCode PCASMGetLocalType(PC, PCCompositeType *);
PETSC_EXTERN PetscErrorCode PCASMCreateSubdomains(Mat, PetscInt, IS *[]);
PETSC_EXTERN PetscErrorCode PCASMDestroySubdomains(PetscInt, IS *[], IS *[]);
PETSC_EXTERN PetscErrorCode PCASMCreateSubdomains2D(PetscInt, PetscInt, PetscInt, PetscInt, PetscInt, PetscInt, PetscInt *, IS *[], IS *[]);
PETSC_EXTERN PetscErrorCode PCASMGetLocalSubdomains(PC, PetscInt *, IS *[], IS *[]);
PETSC_EXTERN PetscErrorCode PCASMGetLocalSubmatrices(PC, PetscInt *, Mat *[]);
PETSC_EXTERN PetscErrorCode PCASMGetSubMatType(PC, MatType *);
PETSC_EXTERN PetscErrorCode PCASMSetSubMatType(PC, MatType);

PETSC_EXTERN PetscErrorCode PCGASMSetTotalSubdomains(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGASMSetSubdomains(PC, PetscInt, IS[], IS[]);
PETSC_EXTERN PetscErrorCode PCGASMSetOverlap(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGASMSetUseDMSubdomains(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGASMGetUseDMSubdomains(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCGASMSetSortIndices(PC, PetscBool);

PETSC_EXTERN PetscErrorCode PCGASMSetType(PC, PCGASMType);
PETSC_EXTERN PetscErrorCode PCGASMCreateSubdomains(Mat, PetscInt, PetscInt *, IS *[]);
PETSC_EXTERN PetscErrorCode PCGASMDestroySubdomains(PetscInt, IS *[], IS *[]);
PETSC_EXTERN PetscErrorCode PCGASMCreateSubdomains2D(PC, PetscInt, PetscInt, PetscInt, PetscInt, PetscInt, PetscInt, PetscInt *, IS *[], IS *[]);
PETSC_EXTERN PetscErrorCode PCGASMGetSubdomains(PC, PetscInt *, IS *[], IS *[]);
PETSC_EXTERN PetscErrorCode PCGASMGetSubmatrices(PC, PetscInt *, Mat *[]);

PETSC_EXTERN PetscErrorCode PCCompositeSetType(PC, PCCompositeType);
PETSC_EXTERN PetscErrorCode PCCompositeGetType(PC, PCCompositeType *);
PETSC_EXTERN PetscErrorCode PCCompositeAddPCType(PC, PCType);
PETSC_EXTERN PetscErrorCode PCCompositeAddPC(PC, PC);
PETSC_EXTERN PetscErrorCode PCCompositeGetNumberPC(PC, PetscInt *);
PETSC_EXTERN PetscErrorCode PCCompositeGetPC(PC, PetscInt, PC *);
PETSC_EXTERN PetscErrorCode PCCompositeSpecialSetAlpha(PC, PetscScalar);
PETSC_EXTERN PetscErrorCode PCCompositeSpecialSetAlphaMat(PC, Mat);

PETSC_EXTERN PetscErrorCode PCRedundantSetNumber(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCRedundantSetScatter(PC, VecScatter, VecScatter);
PETSC_EXTERN PetscErrorCode PCRedundantGetOperators(PC, Mat *, Mat *);

PETSC_EXTERN PetscErrorCode PCSPAISetEpsilon(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCSPAISetNBSteps(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCSPAISetMax(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCSPAISetMaxNew(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCSPAISetBlockSize(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCSPAISetCacheSize(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCSPAISetVerbose(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCSPAISetSp(PC, PetscInt);

PETSC_EXTERN PetscErrorCode PCHYPRESetType(PC, const char[]);
PETSC_EXTERN PetscErrorCode PCHYPREGetType(PC, const char *[]);
PETSC_EXTERN PetscErrorCode PCHYPRESetDiscreteGradient(PC, Mat);
PETSC_EXTERN PetscErrorCode PCHYPRESetDiscreteCurl(PC, Mat);
PETSC_EXTERN PetscErrorCode PCHYPRESetInterpolations(PC, PetscInt, Mat, Mat[], Mat, Mat[]);
PETSC_EXTERN PetscErrorCode PCHYPRESetEdgeConstantVectors(PC, Vec, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCHYPREAMSSetInteriorNodes(PC, Vec);
PETSC_EXTERN PetscErrorCode PCHYPRESetAlphaPoissonMatrix(PC, Mat);
PETSC_EXTERN PetscErrorCode PCHYPRESetBetaPoissonMatrix(PC, Mat);
PETSC_EXTERN PetscErrorCode PCHYPREGetCFMarkers(PC pc, PetscInt *[], PetscBT *[]);

PETSC_EXTERN PetscErrorCode PCFieldSplitSetFields(PC, const char[], PetscInt, const PetscInt *, const PetscInt *);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetType(PC, PCCompositeType);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetType(PC, PCCompositeType *);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetBlockSize(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetIS(PC, const char[], IS);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetIS(PC, const char[], IS *);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetISByIndex(PC, PetscInt, IS *);
PETSC_EXTERN PetscErrorCode PCFieldSplitRestrictIS(PC, IS);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetDMSplits(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetDMSplits(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetDiagUseAmat(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetDiagUseAmat(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetOffDiagUseAmat(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetOffDiagUseAmat(PC, PetscBool *);

PETSC_EXTERN PETSC_DEPRECATED_FUNCTION(3, 5, 0, "PCFieldSplitSetSchurPre()", ) PetscErrorCode PCFieldSplitSchurPrecondition(PC, PCFieldSplitSchurPreType, Mat);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetSchurPre(PC, PCFieldSplitSchurPreType, Mat);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetSchurPre(PC, PCFieldSplitSchurPreType *, Mat *);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetSchurFactType(PC, PCFieldSplitSchurFactType);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetSchurScale(PC, PetscScalar);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetSchurBlocks(PC, Mat *, Mat *, Mat *, Mat *);
PETSC_EXTERN PetscErrorCode PCFieldSplitSchurGetS(PC, Mat *);
PETSC_EXTERN PetscErrorCode PCFieldSplitSchurRestoreS(PC, Mat *);
PETSC_EXTERN PetscErrorCode PCFieldSplitGetDetectSaddlePoint(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetDetectSaddlePoint(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetGKBTol(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetGKBNu(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetGKBMaxit(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCFieldSplitSetGKBDelay(PC, PetscInt);

PETSC_EXTERN PetscErrorCode PCGalerkinSetRestriction(PC, Mat);
PETSC_EXTERN PetscErrorCode PCGalerkinSetInterpolation(PC, Mat);
PETSC_EXTERN PetscErrorCode PCGalerkinSetComputeSubmatrix(PC, PetscErrorCode (*)(PC, Mat, Mat, Mat *, void *), void *);

PETSC_EXTERN PetscErrorCode PCPythonSetType(PC, const char[]);
PETSC_EXTERN PetscErrorCode PCPythonGetType(PC, const char *[]);

PETSC_EXTERN PetscErrorCode PCPARMSSetGlobal(PC, PCPARMSGlobalType);
PETSC_EXTERN PetscErrorCode PCPARMSSetLocal(PC, PCPARMSLocalType);
PETSC_EXTERN PetscErrorCode PCPARMSSetSolveTolerances(PC, PetscReal, PetscInt);
PETSC_EXTERN PetscErrorCode PCPARMSSetSolveRestart(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCPARMSSetNonsymPerm(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCPARMSSetFill(PC, PetscInt, PetscInt, PetscInt);

PETSC_EXTERN PetscErrorCode PCGAMGSetType(PC, PCGAMGType);
PETSC_EXTERN PetscErrorCode PCGAMGGetType(PC, PCGAMGType *);
PETSC_EXTERN PetscErrorCode PCGAMGSetProcEqLim(PC, PetscInt);

PETSC_EXTERN PetscErrorCode PCGAMGSetRepartition(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGSetUseSAEstEig(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGSetRecomputeEstEig(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGSetEigenvalues(PC, PetscReal, PetscReal);
PETSC_EXTERN PetscErrorCode PCGAMGASMSetUseAggs(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGSetParallelCoarseGridSolve(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGSetCpuPinCoarseGrids(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGSetCoarseGridLayoutType(PC, PCGAMGLayoutType);
PETSC_EXTERN PetscErrorCode PCGAMGSetThreshold(PC, PetscReal[], PetscInt);
PETSC_EXTERN PetscErrorCode PCGAMGSetRankReductionFactors(PC, PetscInt[], PetscInt);
PETSC_EXTERN PetscErrorCode PCGAMGSetThresholdScale(PC, PetscReal);
PETSC_EXTERN PetscErrorCode PCGAMGSetCoarseEqLim(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGAMGSetNlevels(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGAMGSetNSmooths(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGAMGSetAggressiveLevels(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGAMGSetReuseInterpolation(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGFinalizePackage(void);
PETSC_EXTERN PetscErrorCode PCGAMGInitializePackage(void);
PETSC_EXTERN PetscErrorCode PCGAMGRegister(PCGAMGType, PetscErrorCode (*)(PC));
PETSC_EXTERN PetscErrorCode PCGAMGCreateGraph(PC, Mat, Mat *);
PETSC_EXTERN PetscErrorCode PCGAMGSetAggressiveSquareGraph(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGMISkSetMinDegreeOrdering(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGMISkSetAggressive(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGAMGASMSetHEM(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCGAMGSetLowMemoryFilter(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGSetGraphSymmetrize(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCGAMGSetInjectionIndex(PC, PetscInt, PetscInt[]);

PETSC_EXTERN PetscErrorCode PCGAMGClassicalSetType(PC, PCGAMGClassicalType);
PETSC_EXTERN PetscErrorCode PCGAMGClassicalGetType(PC, PCGAMGClassicalType *);

PETSC_EXTERN PetscErrorCode PCBDDCSetDiscreteGradient(PC, Mat, PetscInt, PetscInt, PetscBool, PetscBool);
PETSC_EXTERN PetscErrorCode PCBDDCSetDivergenceMat(PC, Mat, PetscBool, IS);
PETSC_EXTERN PetscErrorCode PCBDDCSetChangeOfBasisMat(PC, Mat, PetscBool);
PETSC_EXTERN PetscErrorCode PCBDDCSetPrimalVerticesIS(PC, IS);
PETSC_EXTERN PetscErrorCode PCBDDCSetPrimalVerticesLocalIS(PC, IS);
PETSC_EXTERN PetscErrorCode PCBDDCGetPrimalVerticesIS(PC, IS *);
PETSC_EXTERN PetscErrorCode PCBDDCGetPrimalVerticesLocalIS(PC, IS *);
PETSC_EXTERN PetscErrorCode PCBDDCSetCoarseningRatio(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCBDDCSetLevels(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCBDDCSetDirichletBoundaries(PC, IS);
PETSC_EXTERN PetscErrorCode PCBDDCSetDirichletBoundariesLocal(PC, IS);
PETSC_EXTERN PetscErrorCode PCBDDCGetDirichletBoundaries(PC, IS *);
PETSC_EXTERN PetscErrorCode PCBDDCGetDirichletBoundariesLocal(PC, IS *);
PETSC_EXTERN PetscErrorCode PCBDDCSetInterfaceExtType(PC, PCBDDCInterfaceExtType);
PETSC_EXTERN PetscErrorCode PCBDDCSetNeumannBoundaries(PC, IS);
PETSC_EXTERN PetscErrorCode PCBDDCSetNeumannBoundariesLocal(PC, IS);
PETSC_EXTERN PetscErrorCode PCBDDCGetNeumannBoundaries(PC, IS *);
PETSC_EXTERN PetscErrorCode PCBDDCGetNeumannBoundariesLocal(PC, IS *);
PETSC_EXTERN PetscErrorCode PCBDDCSetDofsSplitting(PC, PetscInt, IS[]);
PETSC_EXTERN PetscErrorCode PCBDDCSetDofsSplittingLocal(PC, PetscInt, IS[]);
PETSC_EXTERN PetscErrorCode PCBDDCSetLocalAdjacencyGraph(PC, PetscInt, const PetscInt[], const PetscInt[], PetscCopyMode);
PETSC_EXTERN PetscErrorCode PCBDDCCreateFETIDPOperators(PC, PetscBool, const char *, Mat *, PC *);
PETSC_EXTERN PetscErrorCode PCBDDCMatFETIDPGetRHS(Mat, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCBDDCMatFETIDPGetSolution(Mat, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCBDDCFinalizePackage(void);
PETSC_EXTERN PetscErrorCode PCBDDCInitializePackage(void);

PETSC_EXTERN PetscErrorCode PCISInitialize(PC);
PETSC_EXTERN PetscErrorCode PCISSetUp(PC, PetscBool, PetscBool);
PETSC_EXTERN PetscErrorCode PCISSetUseStiffnessScaling(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCISSetSubdomainScalingFactor(PC, PetscScalar);
PETSC_EXTERN PetscErrorCode PCISSetSubdomainDiagonalScaling(PC, Vec);
PETSC_EXTERN PetscErrorCode PCISScatterArrayNToVecB(PC, PetscScalar *, Vec, InsertMode, ScatterMode);
PETSC_EXTERN PetscErrorCode PCISApplySchur(PC, Vec, Vec, Vec, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCISApplyInvSchur(PC, Vec, Vec, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCISReset(PC);

PETSC_EXTERN PetscInt       PetscMGLevelId;
PETSC_EXTERN PetscErrorCode PCMGSetType(PC, PCMGType);
PETSC_EXTERN PetscErrorCode PCMGGetType(PC, PCMGType *);
PETSC_EXTERN PetscErrorCode PCMGSetLevels(PC, PetscInt, MPI_Comm *);
PETSC_EXTERN PetscErrorCode PCMGGetLevels(PC, PetscInt *);

PETSC_EXTERN PetscErrorCode PCMGSetDistinctSmoothUp(PC);
PETSC_EXTERN PetscErrorCode PCMGSetNumberSmooth(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCMGSetCycleType(PC, PCMGCycleType);
PETSC_EXTERN PetscErrorCode PCMGSetCycleTypeOnLevel(PC, PetscInt, PCMGCycleType);
PETSC_DEPRECATED_FUNCTION(3, 5, 0, "PCMGSetCycleTypeOnLevel()", ) static inline PetscErrorCode PCMGSetCyclesOnLevel(PC pc, PetscInt l, PetscInt t)
{
  return PCMGSetCycleTypeOnLevel(pc, l, (PCMGCycleType)t);
}
PETSC_EXTERN PetscErrorCode PCMGMultiplicativeSetCycles(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCMGSetGalerkin(PC, PCMGGalerkinType);
PETSC_EXTERN PetscErrorCode PCMGGetGalerkin(PC, PCMGGalerkinType *);
PETSC_EXTERN PetscErrorCode PCMGSetAdaptCoarseSpaceType(PC, PCMGCoarseSpaceType);
PETSC_EXTERN PetscErrorCode PCMGGetAdaptCoarseSpaceType(PC, PCMGCoarseSpaceType *);
PETSC_EXTERN PetscErrorCode PCMGSetAdaptCR(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCMGGetAdaptCR(PC, PetscBool *);
/* MATT: Remove? */
PETSC_EXTERN PetscErrorCode PCMGSetAdaptInterpolation(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCMGGetAdaptInterpolation(PC, PetscBool *);

PETSC_EXTERN PetscErrorCode PCMGSetRhs(PC, PetscInt, Vec);
PETSC_EXTERN PetscErrorCode PCMGSetX(PC, PetscInt, Vec);
PETSC_EXTERN PetscErrorCode PCMGSetR(PC, PetscInt, Vec);

PETSC_EXTERN PetscErrorCode PCMGSetRestriction(PC, PetscInt, Mat);
PETSC_EXTERN PetscErrorCode PCMGGetRestriction(PC, PetscInt, Mat *);
PETSC_EXTERN PetscErrorCode PCMGSetInjection(PC, PetscInt, Mat);
PETSC_EXTERN PetscErrorCode PCMGGetInjection(PC, PetscInt, Mat *);
PETSC_EXTERN PetscErrorCode PCMGSetInterpolation(PC, PetscInt, Mat);
PETSC_EXTERN PetscErrorCode PCMGSetOperators(PC, PetscInt, Mat, Mat);
PETSC_EXTERN PetscErrorCode PCMGGetInterpolation(PC, PetscInt, Mat *);
PETSC_EXTERN PetscErrorCode PCMGSetRScale(PC, PetscInt, Vec);
PETSC_EXTERN PetscErrorCode PCMGGetRScale(PC, PetscInt, Vec *);
PETSC_EXTERN PetscErrorCode PCMGSetResidual(PC, PetscInt, PetscErrorCode (*)(Mat, Vec, Vec, Vec), Mat);
PETSC_EXTERN PetscErrorCode PCMGSetResidualTranspose(PC, PetscInt, PetscErrorCode (*)(Mat, Vec, Vec, Vec), Mat);
PETSC_EXTERN PetscErrorCode PCMGResidualDefault(Mat, Vec, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCMGResidualTransposeDefault(Mat, Vec, Vec, Vec);
PETSC_EXTERN PetscErrorCode PCMGMatResidualDefault(Mat, Mat, Mat, Mat);
PETSC_EXTERN PetscErrorCode PCMGMatResidualTransposeDefault(Mat, Mat, Mat, Mat);
PETSC_EXTERN PetscErrorCode PCMGGalerkinSetMatProductAlgorithm(PC, const char[]);
PETSC_EXTERN PetscErrorCode PCMGGalerkinGetMatProductAlgorithm(PC, const char *[]);
PETSC_EXTERN PetscErrorCode PCMGGetGridComplexity(PC, PetscReal *, PetscReal *);

PETSC_EXTERN PetscErrorCode PCHMGSetReuseInterpolation(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCHMGSetUseSubspaceCoarsening(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCHMGSetInnerPCType(PC, PCType);
PETSC_EXTERN PetscErrorCode PCHMGSetCoarseningComponent(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCHMGUseMatMAIJ(PC, PetscBool);

PETSC_EXTERN PetscErrorCode PCTelescopeGetSubcommType(PC, PetscSubcommType *);
PETSC_EXTERN PetscErrorCode PCTelescopeSetSubcommType(PC, PetscSubcommType);
PETSC_EXTERN PetscErrorCode PCTelescopeGetReductionFactor(PC, PetscInt *);
PETSC_EXTERN PetscErrorCode PCTelescopeSetReductionFactor(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCTelescopeGetIgnoreDM(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCTelescopeSetIgnoreDM(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCTelescopeGetUseCoarseDM(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCTelescopeSetUseCoarseDM(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCTelescopeGetIgnoreKSPComputeOperators(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCTelescopeSetIgnoreKSPComputeOperators(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCTelescopeGetDM(PC, DM *);

PETSC_EXTERN PetscErrorCode PCPatchSetSaveOperators(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCPatchGetSaveOperators(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCPatchSetPrecomputeElementTensors(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCPatchGetPrecomputeElementTensors(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCPatchSetPartitionOfUnity(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCPatchGetPartitionOfUnity(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCPatchSetSubMatType(PC, MatType);
PETSC_EXTERN PetscErrorCode PCPatchGetSubMatType(PC, MatType *);
PETSC_EXTERN PetscErrorCode PCPatchSetCellNumbering(PC, PetscSection);
PETSC_EXTERN PetscErrorCode PCPatchGetCellNumbering(PC, PetscSection *);
PETSC_EXTERN PetscErrorCode PCPatchSetConstructType(PC, PCPatchConstructType, PetscErrorCode (*)(PC, PetscInt *, IS **, IS *, void *), void *);
PETSC_EXTERN PetscErrorCode PCPatchGetConstructType(PC, PCPatchConstructType *, PetscErrorCode (**)(PC, PetscInt *, IS **, IS *, void *), void **);
PETSC_EXTERN PetscErrorCode PCPatchSetDiscretisationInfo(PC, PetscInt, DM *, PetscInt *, PetscInt *, const PetscInt **, const PetscInt *, PetscInt, const PetscInt *, PetscInt, const PetscInt *);
PETSC_EXTERN PetscErrorCode PCPatchSetComputeOperator(PC, PetscErrorCode (*)(PC, PetscInt, Vec, Mat, IS, PetscInt, const PetscInt *, const PetscInt *, void *), void *);
PETSC_EXTERN PetscErrorCode PCPatchSetComputeFunction(PC pc, PetscErrorCode (*)(PC, PetscInt, Vec, Vec, IS, PetscInt, const PetscInt *, const PetscInt *, void *), void *ctx);
PETSC_EXTERN PetscErrorCode PCPatchSetComputeOperatorInteriorFacets(PC, PetscErrorCode (*)(PC, PetscInt, Vec, Mat, IS, PetscInt, const PetscInt *, const PetscInt *, void *), void *);
PETSC_EXTERN PetscErrorCode PCPatchSetComputeFunctionInteriorFacets(PC pc, PetscErrorCode (*)(PC, PetscInt, Vec, Vec, IS, PetscInt, const PetscInt *, const PetscInt *, void *), void *ctx);

PETSC_EXTERN PetscErrorCode PCLMVMSetMatLMVM(PC, Mat);
PETSC_EXTERN PetscErrorCode PCLMVMGetMatLMVM(PC, Mat *);
PETSC_EXTERN PetscErrorCode PCLMVMSetIS(PC, IS);
PETSC_EXTERN PetscErrorCode PCLMVMClearIS(PC);
PETSC_EXTERN PetscErrorCode PCLMVMSetUpdateVec(PC, Vec);

PETSC_EXTERN PetscErrorCode PCExoticSetType(PC, PCExoticType);

PETSC_EXTERN PetscErrorCode PCDeflationSetInitOnly(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCDeflationSetLevels(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCDeflationSetReductionFactor(PC, PetscInt);
PETSC_EXTERN PetscErrorCode PCDeflationSetCorrectionFactor(PC, PetscScalar);
PETSC_EXTERN PetscErrorCode PCDeflationSetSpaceToCompute(PC, PCDeflationSpaceType, PetscInt);
PETSC_EXTERN PetscErrorCode PCDeflationSetSpace(PC, Mat, PetscBool);
PETSC_EXTERN PetscErrorCode PCDeflationSetProjectionNullSpaceMat(PC, Mat);
PETSC_EXTERN PetscErrorCode PCDeflationSetCoarseMat(PC, Mat);
PETSC_EXTERN PetscErrorCode PCDeflationGetPC(PC, PC *);

PETSC_EXTERN PetscErrorCode PCHPDDMSetAuxiliaryMat(PC, IS, Mat, PetscErrorCode (*)(Mat, PetscReal, Vec, Vec, PetscReal, IS, void *), void *);
PETSC_EXTERN PetscErrorCode PCHPDDMSetRHSMat(PC, Mat);
PETSC_EXTERN PetscErrorCode PCHPDDMHasNeumannMat(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCHPDDMSetCoarseCorrectionType(PC, PCHPDDMCoarseCorrectionType);
PETSC_EXTERN PetscErrorCode PCHPDDMGetCoarseCorrectionType(PC, PCHPDDMCoarseCorrectionType *);
PETSC_EXTERN PetscErrorCode PCHPDDMSetSTShareSubKSP(PC, PetscBool);
PETSC_EXTERN PetscErrorCode PCHPDDMGetSTShareSubKSP(PC, PetscBool *);
PETSC_EXTERN PetscErrorCode PCHPDDMSetDeflationMat(PC, IS, Mat);
PETSC_EXTERN PetscErrorCode PCHPDDMFinalizePackage(void);
PETSC_EXTERN PetscErrorCode PCHPDDMInitializePackage(void);
PETSC_EXTERN PetscErrorCode PCHPDDMGetComplexities(PC, PetscReal *, PetscReal *);

PETSC_EXTERN PetscErrorCode PCAmgXGetResources(PC, void *);

PETSC_EXTERN PetscErrorCode PCMatSetApplyOperation(PC, MatOperation);
PETSC_EXTERN PetscErrorCode PCMatGetApplyOperation(PC, MatOperation *);
