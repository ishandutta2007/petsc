#pragma once

#include <petscsnes.h>
#include <petsc/private/petscimpl.h>

/* SUBMANSEC = SNES */

PETSC_EXTERN PetscBool      SNESRegisterAllCalled;
PETSC_EXTERN PetscErrorCode SNESRegisterAll(void);

typedef struct _SNESOps *SNESOps;

struct _SNESOps {
  PetscErrorCode (*computeinitialguess)(SNES, Vec, void *);
  PetscErrorCode (*computescaling)(Vec, Vec, void *);
  PetscErrorCode (*update)(SNES, PetscInt); /* General purpose function for update */
  PetscErrorCode (*converged)(SNES, PetscInt, PetscReal, PetscReal, PetscReal, SNESConvergedReason *, void *);
  PetscErrorCode (*convergeddestroy)(void *);
  PetscErrorCode (*setup)(SNES); /* routine to set up the nonlinear solver */
  PetscErrorCode (*solve)(SNES); /* actual nonlinear solver */
  PetscErrorCode (*view)(SNES, PetscViewer);
  PetscErrorCode (*setfromoptions)(SNES, PetscOptionItems); /* sets options from database */
  PetscErrorCode (*destroy)(SNES);
  PetscErrorCode (*reset)(SNES);
  PetscErrorCode (*usercompute)(SNES, void **);
  PetscCtxDestroyFn *ctxdestroy;
  PetscErrorCode (*computevariablebounds)(SNES, Vec, Vec); /* user provided routine to set box constrained variable bounds */
  PetscErrorCode (*computepfunction)(SNES, Vec, Vec, void *);
  PetscErrorCode (*computepjacobian)(SNES, Vec, Mat, Mat, void *);
  PetscErrorCode (*load)(SNES, PetscViewer);
};

/*
   Nonlinear solver context
 */
#define MAXSNESMONITORS    5
#define MAXSNESREASONVIEWS 5

struct _p_SNES {
  PETSCHEADER(struct _SNESOps);
  DM        dm;
  PetscBool dmAuto; /* SNES created currently used DM automatically */
  SNES      npc;
  PCSide    npcside;
  PetscBool usesnpc; /* type can use a nonlinear preconditioner */

  /*  ------------------------ User-provided stuff -------------------------------*/
  void *ctx; /* user-defined context */

  Vec vec_rhs; /* If non-null, solve F(x) = rhs */
  Vec vec_sol; /* pointer to solution */

  Vec vec_func; /* pointer to function */

  Mat            jacobian;      /* Jacobian matrix */
  Mat            jacobian_pre;  /* matrix used to construct the preconditioner of the Jacobian */
  Mat            picard;        /* copy of jacobian_pre needed for Picard with -snes_mf_operator */
  void          *initialguessP; /* user-defined initial guess context */
  KSP            ksp;           /* linear solver context */
  SNESLineSearch linesearch;    /* line search context */
  PetscBool      usesksp;
  MatStructure   matstruct; /* Used by Picard solver */

  Vec vec_sol_update; /* pointer to solution update */

  Vec   scaling; /* scaling vector */
  void *scaP;    /* scaling context */

  PetscReal precheck_picard_angle; /* For use with SNESLineSearchPreCheckPicard */

  /* ------------------------Time stepping hooks-----------------------------------*/

  /* ---------------- PETSc-provided (or user-provided) stuff ---------------------*/

  PetscErrorCode (*monitor[MAXSNESMONITORS])(SNES, PetscInt, PetscReal, void *); /* monitor routine */
  PetscCtxDestroyFn  *monitordestroy[MAXSNESMONITORS];                           /* monitor context destroy routine */
  void               *monitorcontext[MAXSNESMONITORS];                           /* monitor context */
  PetscInt            numbermonitors;                                            /* number of monitors */
  PetscBool           pauseFinal;                                                /* pause all drawing monitor at the final iterate */
  void               *cnvP;                                                      /* convergence context */
  SNESConvergedReason reason;                                                    /* converged reason */

  PetscViewer       convergedreasonviewer;
  PetscViewerFormat convergedreasonformat;
  PetscErrorCode (*reasonview[MAXSNESREASONVIEWS])(SNES, void *); /* snes converged reason view */
  PetscCtxDestroyFn *reasonviewdestroy[MAXSNESREASONVIEWS];       /* reason view context destroy routine */
  void              *reasonviewcontext[MAXSNESREASONVIEWS];       /* reason view context */
  PetscInt           numberreasonviews;                           /* number of reason views */
  PetscBool          errorifnotconverged;

  /* --- Routines and data that are unique to each particular solver --- */

  PetscBool setupcalled; /* true if setup has been called */
  void     *data;        /* implementation-specific data */

  /* --------------------------  Parameters -------------------------------------- */

  PetscInt  nfuncs;                                 /* number of function evaluations */
  PetscInt  iter;                                   /* global iteration number */
  PetscInt  linear_its;                             /* total number of linear solver iterations */
  PetscReal norm;                                   /* residual norm of current iterate */
  PetscReal ynorm;                                  /* update norm of current iterate */
  PetscReal xnorm;                                  /* solution norm of current iterate */
  PetscBool forceiteration;                         /* Force SNES to take at least one iteration regardless of the initial residual norm */
  PetscInt  lagpreconditioner;                      /* SNESSetLagPreconditioner() */
  PetscInt  lagjacobian;                            /* SNESSetLagJacobian() */
  PetscInt  jac_iter;                               /* The present iteration of the Jacobian lagging */
  PetscBool lagjac_persist;                         /* The jac_iter persists until reset */
  PetscInt  pre_iter;                               /* The present iteration of the Preconditioner lagging */
  PetscBool lagpre_persist;                         /* The pre_iter persists until reset */
  PetscInt  gridsequence;                           /* number of grid sequence steps to take; defaults to zero */
  PetscObjectParameterDeclare(PetscInt, max_its);   /* max number of iterations */
  PetscObjectParameterDeclare(PetscInt, max_funcs); /* max number of function evals */
  PetscObjectParameterDeclare(PetscReal, rtol);     /* relative tolerance */
  PetscObjectParameterDeclare(PetscReal, divtol);   /* relative divergence tolerance */
  PetscObjectParameterDeclare(PetscReal, abstol);   /* absolute tolerance */
  PetscObjectParameterDeclare(PetscReal, stol);     /* step length tolerance*/

  PetscBool vec_func_init_set; /* the initial function has been set */

  SNESNormSchedule normschedule; /* Norm computation type for SNES instance */
  SNESFunctionType functype;     /* Function type for the SNES instance */

  /* ------------------------ Default work-area management ---------------------- */

  PetscInt nwork;
  Vec     *work;

  /* ---------------------------------- Testing --------------------------------- */
  PetscBool testFunc; // Test the function routine
  PetscBool testJac;  // Test the Jacobian routine

  /* ------------------------- Miscellaneous Information ------------------------ */

  PetscInt   setfromoptionscalled;
  PetscReal *conv_hist;       /* If !0, stores function norm (or
                                    gradient norm) at each iteration */
  PetscInt  *conv_hist_its;   /* linear iterations for each Newton step */
  size_t     conv_hist_len;   /* size of convergence history array */
  size_t     conv_hist_max;   /* actual amount of data in conv_history */
  PetscBool  conv_hist_reset; /* reset counter for each new SNES solve */
  PetscBool  conv_hist_alloc;
  PetscBool  counters_reset; /* reset counter for each new SNES solve */

  /* the next two are used for failures in the line search; they should be put elsewhere */
  PetscInt numFailures; /* number of unsuccessful step attempts */
  PetscInt maxFailures; /* maximum number of unsuccessful step attempts */

  PetscInt numLinearSolveFailures;
  PetscInt maxLinearSolveFailures;

  PetscBool domainerror;         /* set with SNESSetFunctionDomainError() */
  PetscBool jacobiandomainerror; /* set with SNESSetJacobianDomainError() */
  PetscBool checkjacdomainerror; /* if or not check Jacobian domain error after Jacobian evaluations */

  PetscBool ksp_ewconv; /* flag indicating use of Eisenstat-Walker KSP convergence criteria */
  void     *kspconvctx; /* Eisenstat-Walker KSP convergence context */

  /* SNESConvergedDefault context: split it off into a separate var/struct to be passed as context to SNESConvergedDefault? */
  PetscReal ttol;   /* rtol*initial_residual_norm */
  PetscReal rnorm0; /* initial residual norm (used for divergence testing) */

  Vec     *vwork; /* more work vectors for Jacobian approx */
  PetscInt nvwork;

  PetscBool mf;          /* -snes_mf OR -snes_mf_operator was used on this snes */
  PetscBool mf_operator; /* -snes_mf_operator was used on this snes */
  PetscInt  mf_version;  /* The version of snes_mf used */

  PetscReal vizerotolerance; /* tolerance for considering an x[] value to be on the bound */
  Vec       xl, xu;          /* upper and lower bounds for box constrained VI problems */
  PetscInt  ntruebounds;     /* number of non-infinite bounds set for VI box constraints */
  PetscBool usersetbounds;   /* bounds have been set via SNESVISetVariableBounds(), rather than via computevariablebounds() callback. */

  PetscBool alwayscomputesfinalresidual; /* Does SNESSolve_XXX always compute the value of the residual at the final
                                             * solution and put it in vec_func?  Used inside SNESSolve_FAS to determine
                                             * if the final residual must be computed before restricting or prolonging
                                             * it. */
};

typedef struct _p_DMSNES  *DMSNES;
typedef struct _DMSNESOps *DMSNESOps;
struct _DMSNESOps {
  SNESFunctionFn *computefunction;
  SNESFunctionFn *computemffunction;
  SNESJacobianFn *computejacobian;

  /* objective */
  SNESObjectiveFn *computeobjective;

  /* Picard iteration functions */
  SNESFunctionFn *computepfunction;
  SNESJacobianFn *computepjacobian;

  /* User-defined smoother */
  SNESNGSFn *computegs;

  PetscErrorCode (*destroy)(DMSNES);
  PetscErrorCode (*duplicate)(DMSNES, DMSNES);
};

/*S
   DMSNES - Object held by a `DM` that contains all the callback functions and their contexts needed by a `SNES`

   Level: developer

   Notes:
   Users provides callback functions and their contexts to `SNES` using, for example, `SNESSetFunction()`. These values are stored
   in a `DMSNES` that is contained in the `DM` associated with the `SNES`. If no `DM` was provided by
   the user with `SNESSetDM()` it is automatically created by `SNESGetDM()` with `DMShellCreate()`.

   Users very rarely need to worked directly with the `DMSNES` object, rather they work with the `SNES` and the `DM` they created

   Multiple `DM` can share a single `DMSNES`, often each `DM` is associated with
   a grid refinement level. `DMGetDMSNES()` returns the `DMSNES` associated with a `DM`. `DMGetDMSNESWrite()` returns a unique
   `DMSNES` that is only associated with the current `DM`, making a copy of the shared `DMSNES` if needed (copy-on-write).

   See `DMKSP` for details on why there is a needed for `DMSNES` instead of simply storing the user callbacks directly in the `DM` or the `TS`

   Developer Note:
   The `originaldm` inside the `DMSNES` is NOT reference counted (to prevent a reference count loop between a `DM` and a `DMSNES`).
   The `DM` on which this context was first created is cached here to implement one-way
   copy-on-write. When `DMGetDMSNESWrite()` sees a request using a different `DM`, it makes a copy of the `TSDM`. Thus, if a user
   only interacts directly with one level, e.g., using `TSSetIFunction()`, then coarse levels of a multilevel item
   integrator are built, then the user changes the routine with another call to `TSSetIFunction()`, it automatically
   propagates to all the levels. If instead, they get out a specific level and set the function on that level,
   subsequent changes to the original level will no longer propagate to that level.

.seealso: [](ch_ts), `SNES`, `SNESCreate()`, `DM`, `DMGetDMSNESWrite()`, `DMGetDMSNES()`,  `DMKSP`, `DMTS`, `DMSNESSetFunction()`, `DMSNESGetFunction()`,
          `DMSNESSetFunctionContextDestroy()`, `DMSNESSetMFFunction()`, `DMSNESSetNGS()`, `DMSNESGetNGS()`, `DMSNESSetJacobian()`, `DMSNESGetJacobian()`,
          `DMSNESSetJacobianContextDestroy()`, `DMSNESSetPicard()`, `DMSNESGetPicard()`, `DMSNESSetObjective()`, `DMSNESGetObjective()`, `DMCopyDMSNES()`
S*/
struct _p_DMSNES {
  PETSCHEADER(struct _DMSNESOps);
  PetscContainer functionctxcontainer;
  PetscContainer jacobianctxcontainer;
  void          *mffunctionctx;
  void          *gsctx;
  void          *pctx;
  void          *objectivectx;

  void *data;

  /* See developer note for DMSNES above */
  DM originaldm;
};
PETSC_EXTERN PetscErrorCode DMGetDMSNES(DM, DMSNES *);
PETSC_EXTERN PetscErrorCode DMSNESView(DMSNES, PetscViewer);
PETSC_EXTERN PetscErrorCode DMSNESLoad(DMSNES, PetscViewer);
PETSC_EXTERN PetscErrorCode DMGetDMSNESWrite(DM, DMSNES *);

/* Context for Eisenstat-Walker convergence criteria for KSP solvers */
typedef struct {
  PetscInt  version;     /* flag indicating version (1,2,3 or 4) */
  PetscReal rtol_0;      /* initial rtol */
  PetscReal rtol_last;   /* last rtol */
  PetscReal rtol_max;    /* maximum rtol */
  PetscReal gamma;       /* mult. factor for version 2 rtol computation */
  PetscReal alpha;       /* power for version 2 rtol computation */
  PetscReal alpha2;      /* power for safeguard */
  PetscReal threshold;   /* threshold for imposing safeguard */
  PetscReal lresid_last; /* linear residual from last iteration */
  PetscReal norm_last;   /* function norm from last iteration */
  PetscReal norm_first;  /* function norm from the beginning of the first iteration. */
  PetscReal rtol_last_2, rk_last, rk_last_2;
  PetscReal v4_p1, v4_p2, v4_p3, v4_m1, v4_m2, v4_m3, v4_m4;
} SNESKSPEW;

static inline PetscErrorCode SNESLogConvergenceHistory(SNES snes, PetscReal res, PetscInt its)
{
  PetscFunctionBegin;
  PetscCall(PetscObjectSAWsTakeAccess((PetscObject)snes));
  if (snes->conv_hist && snes->conv_hist_max > snes->conv_hist_len) {
    if (snes->conv_hist) snes->conv_hist[snes->conv_hist_len] = res;
    if (snes->conv_hist_its) snes->conv_hist_its[snes->conv_hist_len] = its;
    snes->conv_hist_len++;
  }
  PetscCall(PetscObjectSAWsGrantAccess((PetscObject)snes));
  PetscFunctionReturn(PETSC_SUCCESS);
}

PETSC_EXTERN PetscErrorCode SNESVIProjectOntoBounds(SNES, Vec);
PETSC_INTERN PetscErrorCode SNESVICheckLocalMin_Private(SNES, Mat, Vec, Vec, PetscReal, PetscBool *);
PETSC_INTERN PetscErrorCode SNESReset_VI(SNES);
PETSC_INTERN PetscErrorCode SNESDestroy_VI(SNES);
PETSC_INTERN PetscErrorCode SNESView_VI(SNES, PetscViewer);
PETSC_INTERN PetscErrorCode SNESSetFromOptions_VI(SNES, PetscOptionItems);
PETSC_INTERN PetscErrorCode SNESSetUp_VI(SNES);
PETSC_EXTERN_TYPEDEF typedef PetscErrorCode(SNESVIComputeVariableBoundsFn)(SNES, Vec, Vec);
PETSC_EXTERN_TYPEDEF typedef SNESVIComputeVariableBoundsFn *SNESVIComputeVariableBoundsFunction; // deprecated version
PETSC_INTERN PetscErrorCode                                 SNESVISetComputeVariableBounds_VI(SNES, SNESVIComputeVariableBoundsFn);
PETSC_INTERN PetscErrorCode                                 SNESVISetVariableBounds_VI(SNES, Vec, Vec);
PETSC_INTERN PetscErrorCode                                 SNESConvergedDefault_VI(SNES, PetscInt, PetscReal, PetscReal, PetscReal, SNESConvergedReason *, void *);

PETSC_INTERN PetscErrorCode DMSNESUnsetFunctionContext_Internal(DM);
PETSC_EXTERN PetscErrorCode DMSNESUnsetJacobianContext_Internal(DM);

PETSC_EXTERN PetscLogEvent SNES_Solve;
PETSC_EXTERN PetscLogEvent SNES_SetUp;
PETSC_EXTERN PetscLogEvent SNES_FunctionEval;
PETSC_EXTERN PetscLogEvent SNES_JacobianEval;
PETSC_EXTERN PetscLogEvent SNES_NGSEval;
PETSC_EXTERN PetscLogEvent SNES_NGSFuncEval;
PETSC_EXTERN PetscLogEvent SNES_NewtonALEval;
PETSC_EXTERN PetscLogEvent SNES_NPCSolve;
PETSC_EXTERN PetscLogEvent SNES_ObjectiveEval;

PETSC_INTERN PetscBool  SNEScite;
PETSC_INTERN const char SNESCitation[];

/* Used by TAOBNK solvers */
PETSC_SINGLE_LIBRARY_VISIBILITY_INTERNAL PetscErrorCode KSPPostSolve_SNESEW(KSP, Vec, Vec, void *);
PETSC_SINGLE_LIBRARY_VISIBILITY_INTERNAL PetscErrorCode KSPPreSolve_SNESEW(KSP, Vec, Vec, void *);
PETSC_SINGLE_LIBRARY_VISIBILITY_INTERNAL PetscErrorCode SNESEWSetFromOptions_Private(SNESKSPEW *, PetscBool, MPI_Comm, const char *);

/*
    Either generate an error or mark as diverged when a real from a SNES function norm is Nan or Inf.
    domainerror is reset here, once reason is set, to allow subsequent iterations to be feasible (e.g. line search).
*/
#define SNESCheckFunctionNorm(snes, beta) \
  do { \
    if (PetscIsInfOrNanReal(beta)) { \
      PetscCheck(!snes->errorifnotconverged, PetscObjectComm((PetscObject)snes), PETSC_ERR_NOT_CONVERGED, "SNESSolve has not converged due to Nan or Inf norm"); \
      { \
        PetscBool domainerror; \
        PetscCallMPI(MPIU_Allreduce(&snes->domainerror, &domainerror, 1, MPIU_BOOL, MPI_LOR, PetscObjectComm((PetscObject)snes))); \
        if (domainerror) { \
          snes->reason      = SNES_DIVERGED_FUNCTION_DOMAIN; \
          snes->domainerror = PETSC_FALSE; \
        } else snes->reason = SNES_DIVERGED_FNORM_NAN; \
        PetscFunctionReturn(PETSC_SUCCESS); \
      } \
    } \
  } while (0)

#define SNESCheckJacobianDomainerror(snes) \
  do { \
    if (snes->checkjacdomainerror) { \
      PetscBool domainerror; \
      PetscCallMPI(MPIU_Allreduce(&snes->jacobiandomainerror, &domainerror, 1, MPIU_BOOL, MPI_LOR, PetscObjectComm((PetscObject)snes))); \
      if (domainerror) { \
        snes->reason              = SNES_DIVERGED_JACOBIAN_DOMAIN; \
        snes->jacobiandomainerror = PETSC_FALSE; \
        PetscCheck(!snes->errorifnotconverged, PetscObjectComm((PetscObject)snes), PETSC_ERR_NOT_CONVERGED, "SNESSolve has not converged due to Jacobian domain error"); \
        PetscFunctionReturn(PETSC_SUCCESS); \
      } \
    } \
  } while (0)

#define SNESCheckKSPSolve(snes) \
  do { \
    KSPConvergedReason kspreason; \
    PetscInt           lits; \
    PetscCall(KSPGetIterationNumber(snes->ksp, &lits)); \
    snes->linear_its += lits; \
    PetscCall(KSPGetConvergedReason(snes->ksp, &kspreason)); \
    if (kspreason < 0) { \
      if (kspreason == KSP_DIVERGED_NANORINF) { \
        PetscBool domainerror; \
        PetscCallMPI(MPIU_Allreduce(&snes->domainerror, &domainerror, 1, MPIU_BOOL, MPI_LOR, PetscObjectComm((PetscObject)snes))); \
        if (domainerror) snes->reason = SNES_DIVERGED_FUNCTION_DOMAIN; \
        else snes->reason = SNES_DIVERGED_LINEAR_SOLVE; \
        PetscFunctionReturn(PETSC_SUCCESS); \
      } else { \
        if (++snes->numLinearSolveFailures >= snes->maxLinearSolveFailures) { \
          PetscCall(PetscInfo(snes, "iter=%" PetscInt_FMT ", number linear solve failures %" PetscInt_FMT " greater than current SNES allowed %" PetscInt_FMT ", stopping solve\n", snes->iter, snes->numLinearSolveFailures, snes->maxLinearSolveFailures)); \
          snes->reason = SNES_DIVERGED_LINEAR_SOLVE; \
          PetscFunctionReturn(PETSC_SUCCESS); \
        } \
      } \
    } \
  } while (0)

#define SNESNeedNorm_Private(snes, iter) \
  (((iter) == (snes)->max_its && ((snes)->normschedule == SNES_NORM_FINAL_ONLY || (snes)->normschedule == SNES_NORM_INITIAL_FINAL_ONLY)) || ((iter) == 0 && ((snes)->normschedule == SNES_NORM_INITIAL_ONLY || (snes)->normschedule == SNES_NORM_INITIAL_FINAL_ONLY)) || \
   (snes)->normschedule == SNES_NORM_ALWAYS)
