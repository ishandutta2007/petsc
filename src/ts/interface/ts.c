#include <petsc/private/tsimpl.h> /*I "petscts.h"  I*/
#include <petscdmda.h>
#include <petscdmshell.h>
#include <petscdmplex.h>  // For TSSetFromOptions()
#include <petscdmswarm.h> // For TSSetFromOptions()
#include <petscviewer.h>
#include <petscdraw.h>
#include <petscconvest.h>

/* Logging support */
PetscClassId  TS_CLASSID, DMTS_CLASSID;
PetscLogEvent TS_Step, TS_PseudoComputeTimeStep, TS_FunctionEval, TS_JacobianEval;

const char *const TSExactFinalTimeOptions[] = {"UNSPECIFIED", "STEPOVER", "INTERPOLATE", "MATCHSTEP", "TSExactFinalTimeOption", "TS_EXACTFINALTIME_", NULL};

static PetscErrorCode TSAdaptSetDefaultType(TSAdapt adapt, TSAdaptType default_type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(adapt, TSADAPT_CLASSID, 1);
  PetscAssertPointer(default_type, 2);
  if (!((PetscObject)adapt)->type_name) PetscCall(TSAdaptSetType(adapt, default_type));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetFromOptions - Sets various `TS` parameters from the options database

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Options Database Keys:
+ -ts_type <type>                                                    - EULER, BEULER, SUNDIALS, PSEUDO, CN, RK, THETA, ALPHA, GLLE,  SSP, GLEE, BSYMP, IRK, see `TSType`
. -ts_save_trajectory                                                - checkpoint the solution at each time-step
. -ts_max_time <time>                                                - maximum time to compute to
. -ts_time_span <t0,...tf>                                           - sets the time span, solutions are computed and stored for each indicated time, init_time and max_time are set
. -ts_eval_times <t0,...tn>                                          - time points where solutions are computed and stored for each indicated time
. -ts_max_steps <steps>                                              - maximum time-step number to execute until (possibly with nonzero starting value)
. -ts_run_steps <steps>                                              - maximum number of time steps for TSSolve to take on each call
. -ts_init_time <time>                                               - initial time to start computation
. -ts_final_time <time>                                              - final time to compute to (deprecated: use `-ts_max_time`)
. -ts_dt <dt>                                                        - initial time step
. -ts_exact_final_time <stepover,interpolate,matchstep>              - whether to stop at the exact given final time and how to compute the solution at that time
. -ts_max_snes_failures <maxfailures>                                - Maximum number of nonlinear solve failures allowed
. -ts_max_reject <maxrejects>                                        - Maximum number of step rejections before step fails
. -ts_error_if_step_fails <true,false>                               - Error if no step succeeds
. -ts_rtol <rtol>                                                    - relative tolerance for local truncation error
. -ts_atol <atol>                                                    - Absolute tolerance for local truncation error
. -ts_rhs_jacobian_test_mult -mat_shell_test_mult_view               - test the Jacobian at each iteration against finite difference with RHS function
. -ts_rhs_jacobian_test_mult_transpose                               - test the Jacobian at each iteration against finite difference with RHS function
. -ts_adjoint_solve <yes,no>                                         - After solving the ODE/DAE solve the adjoint problem (requires `-ts_save_trajectory`)
. -ts_fd_color                                                       - Use finite differences with coloring to compute IJacobian
. -ts_monitor                                                        - print information at each timestep
. -ts_monitor_cancel                                                 - Cancel all monitors
. -ts_monitor_wall_clock_time                                        - Monitor wall-clock time, KSP iterations, and SNES iterations per step
. -ts_monitor_lg_solution                                            - Monitor solution graphically
. -ts_monitor_lg_error                                               - Monitor error graphically
. -ts_monitor_error                                                  - Monitors norm of error
. -ts_monitor_lg_timestep                                            - Monitor timestep size graphically
. -ts_monitor_lg_timestep_log                                        - Monitor log timestep size graphically
. -ts_monitor_lg_snes_iterations                                     - Monitor number nonlinear iterations for each timestep graphically
. -ts_monitor_lg_ksp_iterations                                      - Monitor number nonlinear iterations for each timestep graphically
. -ts_monitor_sp_eig                                                 - Monitor eigenvalues of linearized operator graphically
. -ts_monitor_draw_solution                                          - Monitor solution graphically
. -ts_monitor_draw_solution_phase  <xleft,yleft,xright,yright>       - Monitor solution graphically with phase diagram, requires problem with exactly 2 degrees of freedom
. -ts_monitor_draw_error                                             - Monitor error graphically, requires use to have provided TSSetSolutionFunction()
. -ts_monitor_solution [ascii binary draw][:filename][:viewerformat] - monitors the solution at each timestep
. -ts_monitor_solution_interval <interval>                           - output once every interval (default=1) time steps. Use -1 to only output at the end of the simulation
. -ts_monitor_solution_skip_initial                                  - skip writing of initial condition
. -ts_monitor_solution_vtk <filename.vts,filename.vtu>               - Save each time step to a binary file, use filename-%%03" PetscInt_FMT ".vts (filename-%%03" PetscInt_FMT ".vtu)
. -ts_monitor_solution_vtk_interval <interval>                       - output once every interval (default=1) time steps. Use -1 to only output at the end of the simulation
- -ts_monitor_envelope                                               - determine maximum and minimum value of each component of the solution over the solution time

  Level: beginner

  Notes:
  See `SNESSetFromOptions()` and `KSPSetFromOptions()` for how to control the nonlinear and linear solves used by the time-stepper.

  Certain `SNES` options get reset for each new nonlinear solver, for example `-snes_lag_jacobian its` and `-snes_lag_preconditioner its`, in order
  to retain them over the multiple nonlinear solves that `TS` uses you must also provide `-snes_lag_jacobian_persists true` and
  `-snes_lag_preconditioner_persists true`

  Developer Notes:
  We should unify all the -ts_monitor options in the way that -xxx_view has been unified

.seealso: [](ch_ts), `TS`, `TSGetType()`
@*/
PetscErrorCode TSSetFromOptions(TS ts)
{
  PetscBool              opt, flg, tflg;
  char                   monfilename[PETSC_MAX_PATH_LEN];
  PetscReal              time_step, eval_times[100];
  PetscInt               num_eval_times = PETSC_STATIC_ARRAY_LENGTH(eval_times);
  TSExactFinalTimeOption eftopt;
  char                   dir[16];
  TSIFunctionFn         *ifun;
  const char            *defaultType;
  char                   typeName[256];

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);

  PetscCall(TSRegisterAll());
  PetscCall(TSGetIFunction(ts, NULL, &ifun, NULL));

  PetscObjectOptionsBegin((PetscObject)ts);
  if (((PetscObject)ts)->type_name) defaultType = ((PetscObject)ts)->type_name;
  else defaultType = ifun ? TSBEULER : TSEULER;
  PetscCall(PetscOptionsFList("-ts_type", "TS method", "TSSetType", TSList, defaultType, typeName, 256, &opt));
  if (opt) PetscCall(TSSetType(ts, typeName));
  else PetscCall(TSSetType(ts, defaultType));

  /* Handle generic TS options */
  PetscCall(PetscOptionsDeprecated("-ts_final_time", "-ts_max_time", "3.10", NULL));
  PetscCall(PetscOptionsReal("-ts_max_time", "Maximum time to run to", "TSSetMaxTime", ts->max_time, &ts->max_time, NULL));
  PetscCall(PetscOptionsRealArray("-ts_time_span", "Time span", "TSSetTimeSpan", eval_times, &num_eval_times, &flg));
  if (flg) PetscCall(TSSetTimeSpan(ts, num_eval_times, eval_times));
  num_eval_times = PETSC_STATIC_ARRAY_LENGTH(eval_times);
  PetscCall(PetscOptionsRealArray("-ts_eval_times", "Evaluation time points", "TSSetEvaluationTimes", eval_times, &num_eval_times, &opt));
  PetscCheck(flg != opt || (!flg && !opt), PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "May not provide -ts_time_span and -ts_eval_times simultaneously");
  if (opt) PetscCall(TSSetEvaluationTimes(ts, num_eval_times, eval_times));
  PetscCall(PetscOptionsInt("-ts_max_steps", "Maximum time step number to execute to (possibly with non-zero starting value)", "TSSetMaxSteps", ts->max_steps, &ts->max_steps, NULL));
  PetscCall(PetscOptionsInt("-ts_run_steps", "Maximum number of time steps to take on each call to TSSolve()", "TSSetRunSteps", ts->run_steps, &ts->run_steps, NULL));
  PetscCall(PetscOptionsReal("-ts_init_time", "Initial time", "TSSetTime", ts->ptime, &ts->ptime, NULL));
  PetscCall(PetscOptionsReal("-ts_dt", "Initial time step", "TSSetTimeStep", ts->time_step, &time_step, &flg));
  if (flg) PetscCall(TSSetTimeStep(ts, time_step));
  PetscCall(PetscOptionsEnum("-ts_exact_final_time", "Option for handling of final time step", "TSSetExactFinalTime", TSExactFinalTimeOptions, (PetscEnum)ts->exact_final_time, (PetscEnum *)&eftopt, &flg));
  if (flg) PetscCall(TSSetExactFinalTime(ts, eftopt));
  PetscCall(PetscOptionsInt("-ts_max_snes_failures", "Maximum number of nonlinear solve failures", "TSSetMaxSNESFailures", ts->max_snes_failures, &ts->max_snes_failures, &flg));
  if (flg) PetscCall(TSSetMaxSNESFailures(ts, ts->max_snes_failures));
  PetscCall(PetscOptionsInt("-ts_max_reject", "Maximum number of step rejections before step fails", "TSSetMaxStepRejections", ts->max_reject, &ts->max_reject, &flg));
  if (flg) PetscCall(TSSetMaxStepRejections(ts, ts->max_reject));
  PetscCall(PetscOptionsBool("-ts_error_if_step_fails", "Error if no step succeeds", "TSSetErrorIfStepFails", ts->errorifstepfailed, &ts->errorifstepfailed, NULL));
  PetscCall(PetscOptionsBoundedReal("-ts_rtol", "Relative tolerance for local truncation error", "TSSetTolerances", ts->rtol, &ts->rtol, NULL, 0));
  PetscCall(PetscOptionsBoundedReal("-ts_atol", "Absolute tolerance for local truncation error", "TSSetTolerances", ts->atol, &ts->atol, NULL, 0));

  PetscCall(PetscOptionsBool("-ts_rhs_jacobian_test_mult", "Test the RHS Jacobian for consistency with RHS at each solve ", "None", ts->testjacobian, &ts->testjacobian, NULL));
  PetscCall(PetscOptionsBool("-ts_rhs_jacobian_test_mult_transpose", "Test the RHS Jacobian transpose for consistency with RHS at each solve ", "None", ts->testjacobiantranspose, &ts->testjacobiantranspose, NULL));
  PetscCall(PetscOptionsBool("-ts_use_splitrhsfunction", "Use the split RHS function for multirate solvers ", "TSSetUseSplitRHSFunction", ts->use_splitrhsfunction, &ts->use_splitrhsfunction, NULL));
#if defined(PETSC_HAVE_SAWS)
  {
    PetscBool set;
    flg = PETSC_FALSE;
    PetscCall(PetscOptionsBool("-ts_saws_block", "Block for SAWs memory snooper at end of TSSolve", "PetscObjectSAWsBlock", ((PetscObject)ts)->amspublishblock, &flg, &set));
    if (set) PetscCall(PetscObjectSAWsSetBlock((PetscObject)ts, flg));
  }
#endif

  /* Monitor options */
  PetscCall(PetscOptionsDeprecated("-ts_monitor_frequency", "-ts_dmswarm_monitor_moments_interval", "3.24", "Retired in favor of monitor-specific intervals (ts_dmswarm_monitor_moments was the only monitor to use ts_monitor_frequency)"));
  PetscCall(TSMonitorSetFromOptions(ts, "-ts_monitor", "Monitor time and timestep size", "TSMonitorDefault", TSMonitorDefault, NULL));
  PetscCall(TSMonitorSetFromOptions(ts, "-ts_monitor_wall_clock_time", "Monitor wall-clock time, KSP iterations, and SNES iterations per step", "TSMonitorWallClockTime", TSMonitorWallClockTime, TSMonitorWallClockTimeSetUp));
  PetscCall(TSMonitorSetFromOptions(ts, "-ts_monitor_extreme", "Monitor extreme values of the solution", "TSMonitorExtreme", TSMonitorExtreme, NULL));
  PetscCall(TSMonitorSetFromOptions(ts, "-ts_monitor_solution", "View the solution at each timestep", "TSMonitorSolution", TSMonitorSolution, TSMonitorSolutionSetup));
  PetscCall(TSMonitorSetFromOptions(ts, "-ts_dmswarm_monitor_moments", "Monitor moments of particle distribution", "TSDMSwarmMonitorMoments", TSDMSwarmMonitorMoments, NULL));
  PetscCall(PetscOptionsString("-ts_monitor_python", "Use Python function", "TSMonitorSet", NULL, monfilename, sizeof(monfilename), &flg));
  if (flg) PetscCall(PetscPythonMonitorSet((PetscObject)ts, monfilename));

  PetscCall(PetscOptionsName("-ts_monitor_lg_solution", "Monitor solution graphically", "TSMonitorLGSolution", &opt));
  if (opt) {
    PetscInt  howoften = 1;
    DM        dm;
    PetscBool net;

    PetscCall(PetscOptionsInt("-ts_monitor_lg_solution", "Monitor solution graphically", "TSMonitorLGSolution", howoften, &howoften, NULL));
    PetscCall(TSGetDM(ts, &dm));
    PetscCall(PetscObjectTypeCompare((PetscObject)dm, DMNETWORK, &net));
    if (net) {
      TSMonitorLGCtxNetwork ctx;
      PetscCall(TSMonitorLGCtxNetworkCreate(ts, NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 600, 400, howoften, &ctx));
      PetscCall(TSMonitorSet(ts, TSMonitorLGCtxNetworkSolution, ctx, (PetscCtxDestroyFn *)TSMonitorLGCtxNetworkDestroy));
      PetscCall(PetscOptionsBool("-ts_monitor_lg_solution_semilogy", "Plot the solution with a semi-log axis", "", ctx->semilogy, &ctx->semilogy, NULL));
    } else {
      TSMonitorLGCtx ctx;
      PetscCall(TSMonitorLGCtxCreate(PETSC_COMM_SELF, NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 400, 300, howoften, &ctx));
      PetscCall(TSMonitorSet(ts, TSMonitorLGSolution, ctx, (PetscCtxDestroyFn *)TSMonitorLGCtxDestroy));
    }
  }

  PetscCall(PetscOptionsName("-ts_monitor_lg_error", "Monitor error graphically", "TSMonitorLGError", &opt));
  if (opt) {
    TSMonitorLGCtx ctx;
    PetscInt       howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_lg_error", "Monitor error graphically", "TSMonitorLGError", howoften, &howoften, NULL));
    PetscCall(TSMonitorLGCtxCreate(PETSC_COMM_SELF, NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 400, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorLGError, ctx, (PetscCtxDestroyFn *)TSMonitorLGCtxDestroy));
  }
  PetscCall(TSMonitorSetFromOptions(ts, "-ts_monitor_error", "View the error at each timestep", "TSMonitorError", TSMonitorError, NULL));

  PetscCall(PetscOptionsName("-ts_monitor_lg_timestep", "Monitor timestep size graphically", "TSMonitorLGTimeStep", &opt));
  if (opt) {
    TSMonitorLGCtx ctx;
    PetscInt       howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_lg_timestep", "Monitor timestep size graphically", "TSMonitorLGTimeStep", howoften, &howoften, NULL));
    PetscCall(TSMonitorLGCtxCreate(PetscObjectComm((PetscObject)ts), NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 400, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorLGTimeStep, ctx, (PetscCtxDestroyFn *)TSMonitorLGCtxDestroy));
  }
  PetscCall(PetscOptionsName("-ts_monitor_lg_timestep_log", "Monitor log timestep size graphically", "TSMonitorLGTimeStep", &opt));
  if (opt) {
    TSMonitorLGCtx ctx;
    PetscInt       howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_lg_timestep_log", "Monitor log timestep size graphically", "TSMonitorLGTimeStep", howoften, &howoften, NULL));
    PetscCall(TSMonitorLGCtxCreate(PetscObjectComm((PetscObject)ts), NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 400, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorLGTimeStep, ctx, (PetscCtxDestroyFn *)TSMonitorLGCtxDestroy));
    ctx->semilogy = PETSC_TRUE;
  }

  PetscCall(PetscOptionsName("-ts_monitor_lg_snes_iterations", "Monitor number nonlinear iterations for each timestep graphically", "TSMonitorLGSNESIterations", &opt));
  if (opt) {
    TSMonitorLGCtx ctx;
    PetscInt       howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_lg_snes_iterations", "Monitor number nonlinear iterations for each timestep graphically", "TSMonitorLGSNESIterations", howoften, &howoften, NULL));
    PetscCall(TSMonitorLGCtxCreate(PetscObjectComm((PetscObject)ts), NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 400, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorLGSNESIterations, ctx, (PetscCtxDestroyFn *)TSMonitorLGCtxDestroy));
  }
  PetscCall(PetscOptionsName("-ts_monitor_lg_ksp_iterations", "Monitor number nonlinear iterations for each timestep graphically", "TSMonitorLGKSPIterations", &opt));
  if (opt) {
    TSMonitorLGCtx ctx;
    PetscInt       howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_lg_ksp_iterations", "Monitor number nonlinear iterations for each timestep graphically", "TSMonitorLGKSPIterations", howoften, &howoften, NULL));
    PetscCall(TSMonitorLGCtxCreate(PetscObjectComm((PetscObject)ts), NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 400, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorLGKSPIterations, ctx, (PetscCtxDestroyFn *)TSMonitorLGCtxDestroy));
  }
  PetscCall(PetscOptionsName("-ts_monitor_sp_eig", "Monitor eigenvalues of linearized operator graphically", "TSMonitorSPEig", &opt));
  if (opt) {
    TSMonitorSPEigCtx ctx;
    PetscInt          howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_sp_eig", "Monitor eigenvalues of linearized operator graphically", "TSMonitorSPEig", howoften, &howoften, NULL));
    PetscCall(TSMonitorSPEigCtxCreate(PETSC_COMM_SELF, NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 300, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorSPEig, ctx, (PetscCtxDestroyFn *)TSMonitorSPEigCtxDestroy));
  }
  PetscCall(PetscOptionsName("-ts_monitor_sp_swarm", "Display particle phase space from the DMSwarm", "TSMonitorSPSwarm", &opt));
  if (opt) {
    TSMonitorSPCtx ctx;
    PetscInt       howoften = 1, retain = 0;
    PetscBool      phase = PETSC_TRUE, create = PETSC_TRUE, multispecies = PETSC_FALSE;

    for (PetscInt i = 0; i < ts->numbermonitors; ++i)
      if (ts->monitor[i] == TSMonitorSPSwarmSolution) {
        create = PETSC_FALSE;
        break;
      }
    if (create) {
      PetscCall(PetscOptionsInt("-ts_monitor_sp_swarm", "Display particles phase space from the DMSwarm", "TSMonitorSPSwarm", howoften, &howoften, NULL));
      PetscCall(PetscOptionsInt("-ts_monitor_sp_swarm_retain", "Retain n points plotted to show trajectory, -1 for all points", "TSMonitorSPSwarm", retain, &retain, NULL));
      PetscCall(PetscOptionsBool("-ts_monitor_sp_swarm_phase", "Plot in phase space rather than coordinate space", "TSMonitorSPSwarm", phase, &phase, NULL));
      PetscCall(PetscOptionsBool("-ts_monitor_sp_swarm_multi_species", "Color particles by particle species", "TSMonitorSPSwarm", multispecies, &multispecies, NULL));
      PetscCall(TSMonitorSPCtxCreate(PetscObjectComm((PetscObject)ts), NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 300, 300, howoften, retain, phase, multispecies, &ctx));
      PetscCall(TSMonitorSet(ts, TSMonitorSPSwarmSolution, ctx, (PetscCtxDestroyFn *)TSMonitorSPCtxDestroy));
    }
  }
  PetscCall(PetscOptionsName("-ts_monitor_hg_swarm", "Display particle histogram from the DMSwarm", "TSMonitorHGSwarm", &opt));
  if (opt) {
    TSMonitorHGCtx ctx;
    PetscInt       howoften = 1, Ns = 1;
    PetscBool      velocity = PETSC_FALSE, create = PETSC_TRUE;

    for (PetscInt i = 0; i < ts->numbermonitors; ++i)
      if (ts->monitor[i] == TSMonitorHGSwarmSolution) {
        create = PETSC_FALSE;
        break;
      }
    if (create) {
      DM       sw, dm;
      PetscInt Nc, Nb;

      PetscCall(TSGetDM(ts, &sw));
      PetscCall(DMSwarmGetCellDM(sw, &dm));
      PetscCall(DMPlexGetHeightStratum(dm, 0, NULL, &Nc));
      Nb = PetscMin(20, PetscMax(10, Nc));
      PetscCall(PetscOptionsInt("-ts_monitor_hg_swarm", "Display particles histogram from the DMSwarm", "TSMonitorHGSwarm", howoften, &howoften, NULL));
      PetscCall(PetscOptionsBool("-ts_monitor_hg_swarm_velocity", "Plot in velocity space rather than coordinate space", "TSMonitorHGSwarm", velocity, &velocity, NULL));
      PetscCall(PetscOptionsInt("-ts_monitor_hg_swarm_species", "Number of species to histogram", "TSMonitorHGSwarm", Ns, &Ns, NULL));
      PetscCall(PetscOptionsInt("-ts_monitor_hg_swarm_bins", "Number of histogram bins", "TSMonitorHGSwarm", Nb, &Nb, NULL));
      PetscCall(TSMonitorHGCtxCreate(PetscObjectComm((PetscObject)ts), NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 300, 300, howoften, Ns, Nb, velocity, &ctx));
      PetscCall(TSMonitorSet(ts, TSMonitorHGSwarmSolution, ctx, (PetscCtxDestroyFn *)TSMonitorHGCtxDestroy));
    }
  }
  opt = PETSC_FALSE;
  PetscCall(PetscOptionsName("-ts_monitor_draw_solution", "Monitor solution graphically", "TSMonitorDrawSolution", &opt));
  if (opt) {
    TSMonitorDrawCtx ctx;
    PetscInt         howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_draw_solution", "Monitor solution graphically", "TSMonitorDrawSolution", howoften, &howoften, NULL));
    PetscCall(TSMonitorDrawCtxCreate(PetscObjectComm((PetscObject)ts), NULL, "Computed Solution", PETSC_DECIDE, PETSC_DECIDE, 300, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorDrawSolution, ctx, (PetscCtxDestroyFn *)TSMonitorDrawCtxDestroy));
  }
  opt = PETSC_FALSE;
  PetscCall(PetscOptionsName("-ts_monitor_draw_solution_phase", "Monitor solution graphically", "TSMonitorDrawSolutionPhase", &opt));
  if (opt) {
    TSMonitorDrawCtx ctx;
    PetscReal        bounds[4];
    PetscInt         n = 4;
    PetscDraw        draw;
    PetscDrawAxis    axis;

    PetscCall(PetscOptionsRealArray("-ts_monitor_draw_solution_phase", "Monitor solution graphically", "TSMonitorDrawSolutionPhase", bounds, &n, NULL));
    PetscCheck(n == 4, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "Must provide bounding box of phase field");
    PetscCall(TSMonitorDrawCtxCreate(PetscObjectComm((PetscObject)ts), NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 300, 300, 1, &ctx));
    PetscCall(PetscViewerDrawGetDraw(ctx->viewer, 0, &draw));
    PetscCall(PetscViewerDrawGetDrawAxis(ctx->viewer, 0, &axis));
    PetscCall(PetscDrawAxisSetLimits(axis, bounds[0], bounds[2], bounds[1], bounds[3]));
    PetscCall(PetscDrawAxisSetLabels(axis, "Phase Diagram", "Variable 1", "Variable 2"));
    PetscCall(TSMonitorSet(ts, TSMonitorDrawSolutionPhase, ctx, (PetscCtxDestroyFn *)TSMonitorDrawCtxDestroy));
  }
  opt = PETSC_FALSE;
  PetscCall(PetscOptionsName("-ts_monitor_draw_error", "Monitor error graphically", "TSMonitorDrawError", &opt));
  if (opt) {
    TSMonitorDrawCtx ctx;
    PetscInt         howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_draw_error", "Monitor error graphically", "TSMonitorDrawError", howoften, &howoften, NULL));
    PetscCall(TSMonitorDrawCtxCreate(PetscObjectComm((PetscObject)ts), NULL, "Error", PETSC_DECIDE, PETSC_DECIDE, 300, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorDrawError, ctx, (PetscCtxDestroyFn *)TSMonitorDrawCtxDestroy));
  }
  opt = PETSC_FALSE;
  PetscCall(PetscOptionsName("-ts_monitor_draw_solution_function", "Monitor solution provided by TSMonitorSetSolutionFunction() graphically", "TSMonitorDrawSolutionFunction", &opt));
  if (opt) {
    TSMonitorDrawCtx ctx;
    PetscInt         howoften = 1;

    PetscCall(PetscOptionsInt("-ts_monitor_draw_solution_function", "Monitor solution provided by TSMonitorSetSolutionFunction() graphically", "TSMonitorDrawSolutionFunction", howoften, &howoften, NULL));
    PetscCall(TSMonitorDrawCtxCreate(PetscObjectComm((PetscObject)ts), NULL, "Solution provided by user function", PETSC_DECIDE, PETSC_DECIDE, 300, 300, howoften, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorDrawSolutionFunction, ctx, (PetscCtxDestroyFn *)TSMonitorDrawCtxDestroy));
  }

  opt = PETSC_FALSE;
  PetscCall(PetscOptionsString("-ts_monitor_solution_vtk", "Save each time step to a binary file, use filename-%%03" PetscInt_FMT ".vts", "TSMonitorSolutionVTK", NULL, monfilename, sizeof(monfilename), &flg));
  if (flg) {
    TSMonitorVTKCtx ctx;

    PetscCall(TSMonitorSolutionVTKCtxCreate(monfilename, &ctx));
    PetscCall(PetscOptionsInt("-ts_monitor_solution_vtk_interval", "Save every interval time step (-1 for last step only)", NULL, ctx->interval, &ctx->interval, NULL));
    PetscCall(TSMonitorSet(ts, (PetscErrorCode (*)(TS, PetscInt, PetscReal, Vec, void *))TSMonitorSolutionVTK, ctx, (PetscCtxDestroyFn *)TSMonitorSolutionVTKDestroy));
  }

  PetscCall(PetscOptionsString("-ts_monitor_dmda_ray", "Display a ray of the solution", "None", "y=0", dir, sizeof(dir), &flg));
  if (flg) {
    TSMonitorDMDARayCtx *rayctx;
    int                  ray = 0;
    DMDirection          ddir;
    DM                   da;
    PetscMPIInt          rank;

    PetscCheck(dir[1] == '=', PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "Unknown ray %s", dir);
    if (dir[0] == 'x') ddir = DM_X;
    else if (dir[0] == 'y') ddir = DM_Y;
    else SETERRQ(PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "Unknown ray %s", dir);
    sscanf(dir + 2, "%d", &ray);

    PetscCall(PetscInfo(ts, "Displaying DMDA ray %c = %d\n", dir[0], ray));
    PetscCall(PetscNew(&rayctx));
    PetscCall(TSGetDM(ts, &da));
    PetscCall(DMDAGetRay(da, ddir, ray, &rayctx->ray, &rayctx->scatter));
    PetscCallMPI(MPI_Comm_rank(PetscObjectComm((PetscObject)ts), &rank));
    if (rank == 0) PetscCall(PetscViewerDrawOpen(PETSC_COMM_SELF, NULL, NULL, 0, 0, 600, 300, &rayctx->viewer));
    rayctx->lgctx = NULL;
    PetscCall(TSMonitorSet(ts, TSMonitorDMDARay, rayctx, TSMonitorDMDARayDestroy));
  }
  PetscCall(PetscOptionsString("-ts_monitor_lg_dmda_ray", "Display a ray of the solution", "None", "x=0", dir, sizeof(dir), &flg));
  if (flg) {
    TSMonitorDMDARayCtx *rayctx;
    int                  ray = 0;
    DMDirection          ddir;
    DM                   da;
    PetscInt             howoften = 1;

    PetscCheck(dir[1] == '=', PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "Malformed ray %s", dir);
    if (dir[0] == 'x') ddir = DM_X;
    else if (dir[0] == 'y') ddir = DM_Y;
    else SETERRQ(PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "Unknown ray direction %s", dir);
    sscanf(dir + 2, "%d", &ray);

    PetscCall(PetscInfo(ts, "Displaying LG DMDA ray %c = %d\n", dir[0], ray));
    PetscCall(PetscNew(&rayctx));
    PetscCall(TSGetDM(ts, &da));
    PetscCall(DMDAGetRay(da, ddir, ray, &rayctx->ray, &rayctx->scatter));
    PetscCall(TSMonitorLGCtxCreate(PETSC_COMM_SELF, NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 600, 400, howoften, &rayctx->lgctx));
    PetscCall(TSMonitorSet(ts, TSMonitorLGDMDARay, rayctx, TSMonitorDMDARayDestroy));
  }

  PetscCall(PetscOptionsName("-ts_monitor_envelope", "Monitor maximum and minimum value of each component of the solution", "TSMonitorEnvelope", &opt));
  if (opt) {
    TSMonitorEnvelopeCtx ctx;

    PetscCall(TSMonitorEnvelopeCtxCreate(ts, &ctx));
    PetscCall(TSMonitorSet(ts, TSMonitorEnvelope, ctx, (PetscCtxDestroyFn *)TSMonitorEnvelopeCtxDestroy));
  }
  flg = PETSC_FALSE;
  PetscCall(PetscOptionsBool("-ts_monitor_cancel", "Remove all monitors", "TSMonitorCancel", flg, &flg, &opt));
  if (opt && flg) PetscCall(TSMonitorCancel(ts));

  flg = PETSC_FALSE;
  PetscCall(PetscOptionsBool("-ts_fd_color", "Use finite differences with coloring to compute IJacobian", "TSComputeIJacobianDefaultColor", flg, &flg, NULL));
  if (flg) {
    DM dm;

    PetscCall(TSGetDM(ts, &dm));
    PetscCall(DMTSUnsetIJacobianContext_Internal(dm));
    PetscCall(TSSetIJacobian(ts, NULL, NULL, TSComputeIJacobianDefaultColor, NULL));
    PetscCall(PetscInfo(ts, "Setting default finite difference coloring Jacobian matrix\n"));
  }

  /* Handle specific TS options */
  PetscTryTypeMethod(ts, setfromoptions, PetscOptionsObject);

  /* Handle TSAdapt options */
  PetscCall(TSGetAdapt(ts, &ts->adapt));
  PetscCall(TSAdaptSetDefaultType(ts->adapt, ts->default_adapt_type));
  PetscCall(TSAdaptSetFromOptions(ts->adapt, PetscOptionsObject));

  /* TS trajectory must be set after TS, since it may use some TS options above */
  tflg = ts->trajectory ? PETSC_TRUE : PETSC_FALSE;
  PetscCall(PetscOptionsBool("-ts_save_trajectory", "Save the solution at each timestep", "TSSetSaveTrajectory", tflg, &tflg, NULL));
  if (tflg) PetscCall(TSSetSaveTrajectory(ts));

  PetscCall(TSAdjointSetFromOptions(ts, PetscOptionsObject));

  /* process any options handlers added with PetscObjectAddOptionsHandler() */
  PetscCall(PetscObjectProcessOptionsHandlers((PetscObject)ts, PetscOptionsObject));
  PetscOptionsEnd();

  if (ts->trajectory) PetscCall(TSTrajectorySetFromOptions(ts->trajectory, ts));

  /* why do we have to do this here and not during TSSetUp? */
  PetscCall(TSGetSNES(ts, &ts->snes));
  if (ts->problem_type == TS_LINEAR) {
    PetscCall(PetscObjectTypeCompareAny((PetscObject)ts->snes, &flg, SNESKSPONLY, SNESKSPTRANSPOSEONLY, ""));
    if (!flg) PetscCall(SNESSetType(ts->snes, SNESKSPONLY));
  }
  PetscCall(SNESSetFromOptions(ts->snes));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetTrajectory - Gets the trajectory from a `TS` if it exists

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. tr - the `TSTrajectory` object, if it exists

  Level: advanced

  Note:
  This routine should be called after all `TS` options have been set

.seealso: [](ch_ts), `TS`, `TSTrajectory`, `TSAdjointSolve()`, `TSTrajectoryCreate()`
@*/
PetscErrorCode TSGetTrajectory(TS ts, TSTrajectory *tr)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  *tr = ts->trajectory;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetSaveTrajectory - Causes the `TS` to save its solutions as it iterates forward in time in a `TSTrajectory` object

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Options Database Keys:
+ -ts_save_trajectory      - saves the trajectory to a file
- -ts_trajectory_type type - set trajectory type

  Level: intermediate

  Notes:
  This routine should be called after all `TS` options have been set

  The `TSTRAJECTORYVISUALIZATION` files can be loaded into Python with $PETSC_DIR/lib/petsc/bin/PetscBinaryIOTrajectory.py and
  MATLAB with $PETSC_DIR/share/petsc/matlab/PetscReadBinaryTrajectory.m

.seealso: [](ch_ts), `TS`, `TSTrajectory`, `TSGetTrajectory()`, `TSAdjointSolve()`
@*/
PetscErrorCode TSSetSaveTrajectory(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (!ts->trajectory) PetscCall(TSTrajectoryCreate(PetscObjectComm((PetscObject)ts), &ts->trajectory));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSResetTrajectory - Destroys and recreates the internal `TSTrajectory` object

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Level: intermediate

.seealso: [](ch_ts), `TSTrajectory`, `TSGetTrajectory()`, `TSAdjointSolve()`, `TSRemoveTrajectory()`
@*/
PetscErrorCode TSResetTrajectory(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->trajectory) {
    PetscCall(TSTrajectoryDestroy(&ts->trajectory));
    PetscCall(TSTrajectoryCreate(PetscObjectComm((PetscObject)ts), &ts->trajectory));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSRemoveTrajectory - Destroys and removes the internal `TSTrajectory` object from a `TS`

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Level: intermediate

.seealso: [](ch_ts), `TSTrajectory`, `TSResetTrajectory()`, `TSAdjointSolve()`
@*/
PetscErrorCode TSRemoveTrajectory(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->trajectory) PetscCall(TSTrajectoryDestroy(&ts->trajectory));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeRHSJacobian - Computes the Jacobian matrix that has been
  set with `TSSetRHSJacobian()`.

  Collective

  Input Parameters:
+ ts - the `TS` context
. t  - current timestep
- U  - input vector

  Output Parameters:
+ A - Jacobian matrix
- B - optional matrix used to compute the preconditioner, often the same as `A`

  Level: developer

  Note:
  Most users should not need to explicitly call this routine, as it
  is used internally within the ODE integrators.

.seealso: [](ch_ts), `TS`, `TSSetRHSJacobian()`, `KSPSetOperators()`
@*/
PetscErrorCode TSComputeRHSJacobian(TS ts, PetscReal t, Vec U, Mat A, Mat B)
{
  PetscObjectState Ustate;
  PetscObjectId    Uid;
  DM               dm;
  DMTS             tsdm;
  TSRHSJacobianFn *rhsjacobianfunc;
  void            *ctx;
  TSRHSFunctionFn *rhsfunction;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscCheckSameComm(ts, 1, U, 3);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMGetDMTS(dm, &tsdm));
  PetscCall(DMTSGetRHSFunction(dm, &rhsfunction, NULL));
  PetscCall(DMTSGetRHSJacobian(dm, &rhsjacobianfunc, &ctx));
  PetscCall(PetscObjectStateGet((PetscObject)U, &Ustate));
  PetscCall(PetscObjectGetId((PetscObject)U, &Uid));

  if (ts->rhsjacobian.time == t && (ts->problem_type == TS_LINEAR || (ts->rhsjacobian.Xid == Uid && ts->rhsjacobian.Xstate == Ustate)) && (rhsfunction != TSComputeRHSFunctionLinear)) PetscFunctionReturn(PETSC_SUCCESS);

  PetscCheck(ts->rhsjacobian.shift == 0.0 || !ts->rhsjacobian.reuse, PetscObjectComm((PetscObject)ts), PETSC_ERR_USER, "Should not call TSComputeRHSJacobian() on a shifted matrix (shift=%lf) when RHSJacobian is reusable.", (double)ts->rhsjacobian.shift);
  if (rhsjacobianfunc) {
    PetscCall(PetscLogEventBegin(TS_JacobianEval, U, ts, A, B));
    PetscCallBack("TS callback Jacobian", (*rhsjacobianfunc)(ts, t, U, A, B, ctx));
    ts->rhsjacs++;
    PetscCall(PetscLogEventEnd(TS_JacobianEval, U, ts, A, B));
  } else {
    PetscCall(MatZeroEntries(A));
    if (B && A != B) PetscCall(MatZeroEntries(B));
  }
  ts->rhsjacobian.time  = t;
  ts->rhsjacobian.shift = 0;
  ts->rhsjacobian.scale = 1.;
  PetscCall(PetscObjectGetId((PetscObject)U, &ts->rhsjacobian.Xid));
  PetscCall(PetscObjectStateGet((PetscObject)U, &ts->rhsjacobian.Xstate));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeRHSFunction - Evaluates the right-hand-side function for a `TS`

  Collective

  Input Parameters:
+ ts - the `TS` context
. t  - current time
- U  - state vector

  Output Parameter:
. y - right-hand side

  Level: developer

  Note:
  Most users should not need to explicitly call this routine, as it
  is used internally within the nonlinear solvers.

.seealso: [](ch_ts), `TS`, `TSSetRHSFunction()`, `TSComputeIFunction()`
@*/
PetscErrorCode TSComputeRHSFunction(TS ts, PetscReal t, Vec U, Vec y)
{
  TSRHSFunctionFn *rhsfunction;
  TSIFunctionFn   *ifunction;
  void            *ctx;
  DM               dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscValidHeaderSpecific(y, VEC_CLASSID, 4);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetRHSFunction(dm, &rhsfunction, &ctx));
  PetscCall(DMTSGetIFunction(dm, &ifunction, NULL));

  PetscCheck(rhsfunction || ifunction, PetscObjectComm((PetscObject)ts), PETSC_ERR_USER, "Must call TSSetRHSFunction() and / or TSSetIFunction()");

  if (rhsfunction) {
    PetscCall(PetscLogEventBegin(TS_FunctionEval, U, ts, y, 0));
    PetscCall(VecLockReadPush(U));
    PetscCallBack("TS callback right-hand-side", (*rhsfunction)(ts, t, U, y, ctx));
    PetscCall(VecLockReadPop(U));
    ts->rhsfuncs++;
    PetscCall(PetscLogEventEnd(TS_FunctionEval, U, ts, y, 0));
  } else PetscCall(VecZeroEntries(y));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeSolutionFunction - Evaluates the solution function.

  Collective

  Input Parameters:
+ ts - the `TS` context
- t  - current time

  Output Parameter:
. U - the solution

  Level: developer

.seealso: [](ch_ts), `TS`, `TSSetSolutionFunction()`, `TSSetRHSFunction()`, `TSComputeIFunction()`
@*/
PetscErrorCode TSComputeSolutionFunction(TS ts, PetscReal t, Vec U)
{
  TSSolutionFn *solutionfunction;
  void         *ctx;
  DM            dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetSolutionFunction(dm, &solutionfunction, &ctx));
  if (solutionfunction) PetscCallBack("TS callback solution", (*solutionfunction)(ts, t, U, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}
/*@
  TSComputeForcingFunction - Evaluates the forcing function.

  Collective

  Input Parameters:
+ ts - the `TS` context
- t  - current time

  Output Parameter:
. U - the function value

  Level: developer

.seealso: [](ch_ts), `TS`, `TSSetSolutionFunction()`, `TSSetRHSFunction()`, `TSComputeIFunction()`
@*/
PetscErrorCode TSComputeForcingFunction(TS ts, PetscReal t, Vec U)
{
  void        *ctx;
  DM           dm;
  TSForcingFn *forcing;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetForcingFunction(dm, &forcing, &ctx));

  if (forcing) PetscCallBack("TS callback forcing function", (*forcing)(ts, t, U, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode TSGetRHSMats_Private(TS ts, Mat *Arhs, Mat *Brhs)
{
  Mat            A, B;
  TSIJacobianFn *ijacobian;

  PetscFunctionBegin;
  if (Arhs) *Arhs = NULL;
  if (Brhs) *Brhs = NULL;
  PetscCall(TSGetIJacobian(ts, &A, &B, &ijacobian, NULL));
  if (Arhs) {
    if (!ts->Arhs) {
      if (ijacobian) {
        PetscCall(MatDuplicate(A, MAT_DO_NOT_COPY_VALUES, &ts->Arhs));
        PetscCall(TSSetMatStructure(ts, SAME_NONZERO_PATTERN));
      } else {
        ts->Arhs = A;
        PetscCall(PetscObjectReference((PetscObject)A));
      }
    } else {
      PetscBool flg;
      PetscCall(SNESGetUseMatrixFree(ts->snes, NULL, &flg));
      /* Handle case where user provided only RHSJacobian and used -snes_mf_operator */
      if (flg && !ijacobian && ts->Arhs == ts->Brhs) {
        PetscCall(PetscObjectDereference((PetscObject)ts->Arhs));
        ts->Arhs = A;
        PetscCall(PetscObjectReference((PetscObject)A));
      }
    }
    *Arhs = ts->Arhs;
  }
  if (Brhs) {
    if (!ts->Brhs) {
      if (A != B) {
        if (ijacobian) {
          PetscCall(MatDuplicate(B, MAT_DO_NOT_COPY_VALUES, &ts->Brhs));
        } else {
          ts->Brhs = B;
          PetscCall(PetscObjectReference((PetscObject)B));
        }
      } else {
        PetscCall(PetscObjectReference((PetscObject)ts->Arhs));
        ts->Brhs = ts->Arhs;
      }
    }
    *Brhs = ts->Brhs;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeIFunction - Evaluates the DAE residual written in the implicit form F(t,U,Udot)=0

  Collective

  Input Parameters:
+ ts   - the `TS` context
. t    - current time
. U    - state vector
. Udot - time derivative of state vector
- imex - flag indicates if the method is `TSARKIMEX` so that the RHSFunction should be kept separate

  Output Parameter:
. Y - right-hand side

  Level: developer

  Note:
  Most users should not need to explicitly call this routine, as it
  is used internally within the nonlinear solvers.

  If the user did not write their equations in implicit form, this
  function recasts them in implicit form.

.seealso: [](ch_ts), `TS`, `TSSetIFunction()`, `TSComputeRHSFunction()`
@*/
PetscErrorCode TSComputeIFunction(TS ts, PetscReal t, Vec U, Vec Udot, Vec Y, PetscBool imex)
{
  TSIFunctionFn   *ifunction;
  TSRHSFunctionFn *rhsfunction;
  void            *ctx;
  DM               dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscValidHeaderSpecific(Udot, VEC_CLASSID, 4);
  PetscValidHeaderSpecific(Y, VEC_CLASSID, 5);

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetIFunction(dm, &ifunction, &ctx));
  PetscCall(DMTSGetRHSFunction(dm, &rhsfunction, NULL));

  PetscCheck(rhsfunction || ifunction, PetscObjectComm((PetscObject)ts), PETSC_ERR_USER, "Must call TSSetRHSFunction() and / or TSSetIFunction()");

  PetscCall(PetscLogEventBegin(TS_FunctionEval, U, ts, Udot, Y));
  if (ifunction) {
    PetscCallBack("TS callback implicit function", (*ifunction)(ts, t, U, Udot, Y, ctx));
    ts->ifuncs++;
  }
  if (imex) {
    if (!ifunction) PetscCall(VecCopy(Udot, Y));
  } else if (rhsfunction) {
    if (ifunction) {
      Vec Frhs;

      PetscCall(DMGetGlobalVector(dm, &Frhs));
      PetscCall(TSComputeRHSFunction(ts, t, U, Frhs));
      PetscCall(VecAXPY(Y, -1, Frhs));
      PetscCall(DMRestoreGlobalVector(dm, &Frhs));
    } else {
      PetscCall(TSComputeRHSFunction(ts, t, U, Y));
      PetscCall(VecAYPX(Y, -1, Udot));
    }
  }
  PetscCall(PetscLogEventEnd(TS_FunctionEval, U, ts, Udot, Y));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*
   TSRecoverRHSJacobian - Recover the Jacobian matrix so that one can call `TSComputeRHSJacobian()` on it.

   Note:
   This routine is needed when one switches from `TSComputeIJacobian()` to `TSComputeRHSJacobian()` because the Jacobian matrix may be shifted or scaled in `TSComputeIJacobian()`.

*/
static PetscErrorCode TSRecoverRHSJacobian(TS ts, Mat A, Mat B)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCheck(A == ts->Arhs, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "Invalid Amat");
  PetscCheck(B == ts->Brhs, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "Invalid Bmat");

  if (ts->rhsjacobian.shift) PetscCall(MatShift(A, -ts->rhsjacobian.shift));
  if (ts->rhsjacobian.scale == -1.) PetscCall(MatScale(A, -1));
  if (B && B == ts->Brhs && A != B) {
    if (ts->rhsjacobian.shift) PetscCall(MatShift(B, -ts->rhsjacobian.shift));
    if (ts->rhsjacobian.scale == -1.) PetscCall(MatScale(B, -1));
  }
  ts->rhsjacobian.shift = 0;
  ts->rhsjacobian.scale = 1.;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeIJacobian - Evaluates the Jacobian of the DAE

  Collective

  Input Parameters:
+ ts    - the `TS` context
. t     - current timestep
. U     - state vector
. Udot  - time derivative of state vector
. shift - shift to apply, see note below
- imex  - flag indicates if the method is `TSARKIMEX` so that the RHSJacobian should be kept separate

  Output Parameters:
+ A - Jacobian matrix
- B - matrix from which the preconditioner is constructed; often the same as `A`

  Level: developer

  Notes:
  If F(t,U,Udot)=0 is the DAE, the required Jacobian is
.vb
   dF/dU + shift*dF/dUdot
.ve
  Most users should not need to explicitly call this routine, as it
  is used internally within the nonlinear solvers.

.seealso: [](ch_ts), `TS`, `TSSetIJacobian()`
@*/
PetscErrorCode TSComputeIJacobian(TS ts, PetscReal t, Vec U, Vec Udot, PetscReal shift, Mat A, Mat B, PetscBool imex)
{
  TSIJacobianFn   *ijacobian;
  TSRHSJacobianFn *rhsjacobian;
  DM               dm;
  void            *ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscValidHeaderSpecific(Udot, VEC_CLASSID, 4);
  PetscValidHeaderSpecific(A, MAT_CLASSID, 6);
  PetscValidHeaderSpecific(B, MAT_CLASSID, 7);

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetIJacobian(dm, &ijacobian, &ctx));
  PetscCall(DMTSGetRHSJacobian(dm, &rhsjacobian, NULL));

  PetscCheck(rhsjacobian || ijacobian, PetscObjectComm((PetscObject)ts), PETSC_ERR_USER, "Must call TSSetRHSJacobian() and / or TSSetIJacobian()");

  PetscCall(PetscLogEventBegin(TS_JacobianEval, U, ts, A, B));
  if (ijacobian) {
    PetscCallBack("TS callback implicit Jacobian", (*ijacobian)(ts, t, U, Udot, shift, A, B, ctx));
    ts->ijacs++;
  }
  if (imex) {
    if (!ijacobian) { /* system was written as Udot = G(t,U) */
      PetscBool assembled;
      if (rhsjacobian) {
        Mat Arhs = NULL;
        PetscCall(TSGetRHSMats_Private(ts, &Arhs, NULL));
        if (A == Arhs) {
          PetscCheck(rhsjacobian != TSComputeRHSJacobianConstant, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "Unsupported operation! cannot use TSComputeRHSJacobianConstant"); /* there is no way to reconstruct shift*M-J since J cannot be reevaluated */
          ts->rhsjacobian.time = PETSC_MIN_REAL;
        }
      }
      PetscCall(MatZeroEntries(A));
      PetscCall(MatAssembled(A, &assembled));
      if (!assembled) {
        PetscCall(MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY));
        PetscCall(MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY));
      }
      PetscCall(MatShift(A, shift));
      if (A != B) {
        PetscCall(MatZeroEntries(B));
        PetscCall(MatAssembled(B, &assembled));
        if (!assembled) {
          PetscCall(MatAssemblyBegin(B, MAT_FINAL_ASSEMBLY));
          PetscCall(MatAssemblyEnd(B, MAT_FINAL_ASSEMBLY));
        }
        PetscCall(MatShift(B, shift));
      }
    }
  } else {
    Mat Arhs = NULL, Brhs = NULL;

    /* RHSJacobian needs to be converted to part of IJacobian if exists */
    if (rhsjacobian) PetscCall(TSGetRHSMats_Private(ts, &Arhs, &Brhs));
    if (Arhs == A) { /* No IJacobian matrix, so we only have the RHS matrix */
      PetscObjectState Ustate;
      PetscObjectId    Uid;
      TSRHSFunctionFn *rhsfunction;

      PetscCall(DMTSGetRHSFunction(dm, &rhsfunction, NULL));
      PetscCall(PetscObjectStateGet((PetscObject)U, &Ustate));
      PetscCall(PetscObjectGetId((PetscObject)U, &Uid));
      if ((rhsjacobian == TSComputeRHSJacobianConstant || (ts->rhsjacobian.time == t && (ts->problem_type == TS_LINEAR || (ts->rhsjacobian.Xid == Uid && ts->rhsjacobian.Xstate == Ustate)) && rhsfunction != TSComputeRHSFunctionLinear)) &&
          ts->rhsjacobian.scale == -1.) {                      /* No need to recompute RHSJacobian */
        PetscCall(MatShift(A, shift - ts->rhsjacobian.shift)); /* revert the old shift and add the new shift with a single call to MatShift */
        if (A != B) PetscCall(MatShift(B, shift - ts->rhsjacobian.shift));
      } else {
        PetscBool flg;

        if (ts->rhsjacobian.reuse) { /* Undo the damage */
          /* MatScale has a short path for this case.
             However, this code path is taken the first time TSComputeRHSJacobian is called
             and the matrices have not been assembled yet */
          PetscCall(TSRecoverRHSJacobian(ts, A, B));
        }
        PetscCall(TSComputeRHSJacobian(ts, t, U, A, B));
        PetscCall(SNESGetUseMatrixFree(ts->snes, NULL, &flg));
        /* since -snes_mf_operator uses the full SNES function it does not need to be shifted or scaled here */
        if (!flg) {
          PetscCall(MatScale(A, -1));
          PetscCall(MatShift(A, shift));
        }
        if (A != B) {
          PetscCall(MatScale(B, -1));
          PetscCall(MatShift(B, shift));
        }
      }
      ts->rhsjacobian.scale = -1;
      ts->rhsjacobian.shift = shift;
    } else if (Arhs) {  /* Both IJacobian and RHSJacobian */
      if (!ijacobian) { /* No IJacobian provided, but we have a separate RHS matrix */
        PetscCall(MatZeroEntries(A));
        PetscCall(MatShift(A, shift));
        if (A != B) {
          PetscCall(MatZeroEntries(B));
          PetscCall(MatShift(B, shift));
        }
      }
      PetscCall(TSComputeRHSJacobian(ts, t, U, Arhs, Brhs));
      PetscCall(MatAXPY(A, -1, Arhs, ts->axpy_pattern));
      if (A != B) PetscCall(MatAXPY(B, -1, Brhs, ts->axpy_pattern));
    }
  }
  PetscCall(PetscLogEventEnd(TS_JacobianEval, U, ts, A, B));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetRHSFunction - Sets the routine for evaluating the function,
  where U_t = G(t,u).

  Logically Collective

  Input Parameters:
+ ts  - the `TS` context obtained from `TSCreate()`
. r   - vector to put the computed right-hand side (or `NULL` to have it created)
. f   - routine for evaluating the right-hand-side function
- ctx - [optional] user-defined context for private data for the function evaluation routine (may be `NULL`)

  Level: beginner

  Note:
  You must call this function or `TSSetIFunction()` to define your ODE. You cannot use this function when solving a DAE.

.seealso: [](ch_ts), `TS`, `TSRHSFunctionFn`, `TSSetRHSJacobian()`, `TSSetIJacobian()`, `TSSetIFunction()`
@*/
PetscErrorCode TSSetRHSFunction(TS ts, Vec r, TSRHSFunctionFn *f, void *ctx)
{
  SNES snes;
  Vec  ralloc = NULL;
  DM   dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (r) PetscValidHeaderSpecific(r, VEC_CLASSID, 2);

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetRHSFunction(dm, f, ctx));
  PetscCall(TSGetSNES(ts, &snes));
  if (!r && !ts->dm && ts->vec_sol) {
    PetscCall(VecDuplicate(ts->vec_sol, &ralloc));
    r = ralloc;
  }
  PetscCall(SNESSetFunction(snes, r, SNESTSFormFunction, ts));
  PetscCall(VecDestroy(&ralloc));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetSolutionFunction - Provide a function that computes the solution of the ODE or DAE

  Logically Collective

  Input Parameters:
+ ts  - the `TS` context obtained from `TSCreate()`
. f   - routine for evaluating the solution
- ctx - [optional] user-defined context for private data for the
          function evaluation routine (may be `NULL`)

  Options Database Keys:
+ -ts_monitor_lg_error   - create a graphical monitor of error history, requires user to have provided `TSSetSolutionFunction()`
- -ts_monitor_draw_error - Monitor error graphically, requires user to have provided `TSSetSolutionFunction()`

  Level: intermediate

  Notes:
  This routine is used for testing accuracy of time integration schemes when you already know the solution.
  If analytic solutions are not known for your system, consider using the Method of Manufactured Solutions to
  create closed-form solutions with non-physical forcing terms.

  For low-dimensional problems solved in serial, such as small discrete systems, `TSMonitorLGError()` can be used to monitor the error history.

.seealso: [](ch_ts), `TS`, `TSSolutionFn`, `TSSetRHSJacobian()`, `TSSetIJacobian()`, `TSComputeSolutionFunction()`, `TSSetForcingFunction()`, `TSSetSolution()`, `TSGetSolution()`, `TSMonitorLGError()`, `TSMonitorDrawError()`
@*/
PetscErrorCode TSSetSolutionFunction(TS ts, TSSolutionFn *f, void *ctx)
{
  DM dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetSolutionFunction(dm, f, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetForcingFunction - Provide a function that computes a forcing term for a ODE or PDE

  Logically Collective

  Input Parameters:
+ ts   - the `TS` context obtained from `TSCreate()`
. func - routine for evaluating the forcing function
- ctx  - [optional] user-defined context for private data for the function evaluation routine
         (may be `NULL`)

  Level: intermediate

  Notes:
  This routine is useful for testing accuracy of time integration schemes when using the Method of Manufactured Solutions to
  create closed-form solutions with a non-physical forcing term. It allows you to use the Method of Manufactored Solution without directly editing the
  definition of the problem you are solving and hence possibly introducing bugs.

  This replaces the ODE F(u,u_t,t) = 0 the `TS` is solving with F(u,u_t,t) - func(t) = 0

  This forcing function does not depend on the solution to the equations, it can only depend on spatial location, time, and possibly parameters, the
  parameters can be passed in the ctx variable.

  For low-dimensional problems solved in serial, such as small discrete systems, `TSMonitorLGError()` can be used to monitor the error history.

.seealso: [](ch_ts), `TS`, `TSForcingFn`, `TSSetRHSJacobian()`, `TSSetIJacobian()`,
`TSComputeSolutionFunction()`, `TSSetSolutionFunction()`
@*/
PetscErrorCode TSSetForcingFunction(TS ts, TSForcingFn *func, void *ctx)
{
  DM dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetForcingFunction(dm, func, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetRHSJacobian - Sets the function to compute the Jacobian of G,
  where U_t = G(U,t), as well as the location to store the matrix.

  Logically Collective

  Input Parameters:
+ ts   - the `TS` context obtained from `TSCreate()`
. Amat - (approximate) location to store Jacobian matrix entries computed by `f`
. Pmat - matrix from which preconditioner is to be constructed (usually the same as `Amat`)
. f    - the Jacobian evaluation routine
- ctx  - [optional] user-defined context for private data for the Jacobian evaluation routine (may be `NULL`)

  Level: beginner

  Notes:
  You must set all the diagonal entries of the matrices, if they are zero you must still set them with a zero value

  The `TS` solver may modify the nonzero structure and the entries of the matrices `Amat` and `Pmat` between the calls to `f()`
  You should not assume the values are the same in the next call to f() as you set them in the previous call.

.seealso: [](ch_ts), `TS`, `TSRHSJacobianFn`, `SNESComputeJacobianDefaultColor()`,
`TSSetRHSFunction()`, `TSRHSJacobianSetReuse()`, `TSSetIJacobian()`, `TSRHSFunctionFn`, `TSIFunctionFn`
@*/
PetscErrorCode TSSetRHSJacobian(TS ts, Mat Amat, Mat Pmat, TSRHSJacobianFn *f, void *ctx)
{
  SNES           snes;
  DM             dm;
  TSIJacobianFn *ijacobian;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (Amat) PetscValidHeaderSpecific(Amat, MAT_CLASSID, 2);
  if (Pmat) PetscValidHeaderSpecific(Pmat, MAT_CLASSID, 3);
  if (Amat) PetscCheckSameComm(ts, 1, Amat, 2);
  if (Pmat) PetscCheckSameComm(ts, 1, Pmat, 3);

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetRHSJacobian(dm, f, ctx));
  PetscCall(DMTSGetIJacobian(dm, &ijacobian, NULL));
  PetscCall(TSGetSNES(ts, &snes));
  if (!ijacobian) PetscCall(SNESSetJacobian(snes, Amat, Pmat, SNESTSFormJacobian, ts));
  if (Amat) {
    PetscCall(PetscObjectReference((PetscObject)Amat));
    PetscCall(MatDestroy(&ts->Arhs));
    ts->Arhs = Amat;
  }
  if (Pmat) {
    PetscCall(PetscObjectReference((PetscObject)Pmat));
    PetscCall(MatDestroy(&ts->Brhs));
    ts->Brhs = Pmat;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetIFunction - Set the function to compute F(t,U,U_t) where F() = 0 is the DAE to be solved.

  Logically Collective

  Input Parameters:
+ ts  - the `TS` context obtained from `TSCreate()`
. r   - vector to hold the residual (or `NULL` to have it created internally)
. f   - the function evaluation routine
- ctx - user-defined context for private data for the function evaluation routine (may be `NULL`)

  Level: beginner

  Note:
  The user MUST call either this routine or `TSSetRHSFunction()` to define the ODE.  When solving DAEs you must use this function.

.seealso: [](ch_ts), `TS`, `TSIFunctionFn`, `TSSetRHSJacobian()`, `TSSetRHSFunction()`,
`TSSetIJacobian()`
@*/
PetscErrorCode TSSetIFunction(TS ts, Vec r, TSIFunctionFn *f, void *ctx)
{
  SNES snes;
  Vec  ralloc = NULL;
  DM   dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (r) PetscValidHeaderSpecific(r, VEC_CLASSID, 2);

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetIFunction(dm, f, ctx));

  PetscCall(TSGetSNES(ts, &snes));
  if (!r && !ts->dm && ts->vec_sol) {
    PetscCall(VecDuplicate(ts->vec_sol, &ralloc));
    r = ralloc;
  }
  PetscCall(SNESSetFunction(snes, r, SNESTSFormFunction, ts));
  PetscCall(VecDestroy(&ralloc));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetIFunction - Returns the vector where the implicit residual is stored and the function/context to compute it.

  Not Collective

  Input Parameter:
. ts - the `TS` context

  Output Parameters:
+ r    - vector to hold residual (or `NULL`)
. func - the function to compute residual (or `NULL`)
- ctx  - the function context (or `NULL`)

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSSetIFunction()`, `SNESGetFunction()`
@*/
PetscErrorCode TSGetIFunction(TS ts, Vec *r, TSIFunctionFn **func, void **ctx)
{
  SNES snes;
  DM   dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESGetFunction(snes, r, NULL, NULL));
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetIFunction(dm, func, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetRHSFunction - Returns the vector where the right-hand side is stored and the function/context to compute it.

  Not Collective

  Input Parameter:
. ts - the `TS` context

  Output Parameters:
+ r    - vector to hold computed right-hand side (or `NULL`)
. func - the function to compute right-hand side (or `NULL`)
- ctx  - the function context (or `NULL`)

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSSetRHSFunction()`, `SNESGetFunction()`
@*/
PetscErrorCode TSGetRHSFunction(TS ts, Vec *r, TSRHSFunctionFn **func, void **ctx)
{
  SNES snes;
  DM   dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESGetFunction(snes, r, NULL, NULL));
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetRHSFunction(dm, func, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetIJacobian - Set the function to compute the matrix dF/dU + a*dF/dU_t where F(t,U,U_t) is the function
  provided with `TSSetIFunction()`.

  Logically Collective

  Input Parameters:
+ ts   - the `TS` context obtained from `TSCreate()`
. Amat - (approximate) matrix to store Jacobian entries computed by `f`
. Pmat - matrix used to compute preconditioner (usually the same as `Amat`)
. f    - the Jacobian evaluation routine
- ctx  - user-defined context for private data for the Jacobian evaluation routine (may be `NULL`)

  Level: beginner

  Notes:
  The matrices `Amat` and `Pmat` are exactly the matrices that are used by `SNES` for the nonlinear solve.

  If you know the operator Amat has a null space you can use `MatSetNullSpace()` and `MatSetTransposeNullSpace()` to supply the null
  space to `Amat` and the `KSP` solvers will automatically use that null space as needed during the solution process.

  The matrix dF/dU + a*dF/dU_t you provide turns out to be
  the Jacobian of F(t,U,W+a*U) where F(t,U,U_t) = 0 is the DAE to be solved.
  The time integrator internally approximates U_t by W+a*U where the positive "shift"
  a and vector W depend on the integration method, step size, and past states. For example with
  the backward Euler method a = 1/dt and W = -a*U(previous timestep) so
  W + a*U = a*(U - U(previous timestep)) = (U - U(previous timestep))/dt

  You must set all the diagonal entries of the matrices, if they are zero you must still set them with a zero value

  The TS solver may modify the nonzero structure and the entries of the matrices `Amat` and `Pmat` between the calls to `f`
  You should not assume the values are the same in the next call to `f` as you set them in the previous call.

  In case `TSSetRHSJacobian()` is also used in conjunction with a fully-implicit solver,
  multilevel linear solvers, e.g. `PCMG`, will likely not work due to the way `TS` handles rhs matrices.

.seealso: [](ch_ts), `TS`, `TSIJacobianFn`, `TSSetIFunction()`, `TSSetRHSJacobian()`,
`SNESComputeJacobianDefaultColor()`, `SNESComputeJacobianDefault()`, `TSSetRHSFunction()`
@*/
PetscErrorCode TSSetIJacobian(TS ts, Mat Amat, Mat Pmat, TSIJacobianFn *f, void *ctx)
{
  SNES snes;
  DM   dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (Amat) PetscValidHeaderSpecific(Amat, MAT_CLASSID, 2);
  if (Pmat) PetscValidHeaderSpecific(Pmat, MAT_CLASSID, 3);
  if (Amat) PetscCheckSameComm(ts, 1, Amat, 2);
  if (Pmat) PetscCheckSameComm(ts, 1, Pmat, 3);

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetIJacobian(dm, f, ctx));

  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESSetJacobian(snes, Amat, Pmat, SNESTSFormJacobian, ts));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSRHSJacobianSetReuse - restore the RHS Jacobian before calling the user-provided `TSRHSJacobianFn` function again

  Logically Collective

  Input Parameters:
+ ts    - `TS` context obtained from `TSCreate()`
- reuse - `PETSC_TRUE` if the RHS Jacobian

  Level: intermediate

  Notes:
  Without this flag, `TS` will change the sign and shift the RHS Jacobian for a
  finite-time-step implicit solve, in which case the user function will need to recompute the
  entire Jacobian.  The `reuse `flag must be set if the evaluation function assumes that the
  matrix entries have not been changed by the `TS`.

.seealso: [](ch_ts), `TS`, `TSSetRHSJacobian()`, `TSComputeRHSJacobianConstant()`
@*/
PetscErrorCode TSRHSJacobianSetReuse(TS ts, PetscBool reuse)
{
  PetscFunctionBegin;
  ts->rhsjacobian.reuse = reuse;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetI2Function - Set the function to compute F(t,U,U_t,U_tt) where F = 0 is the DAE to be solved.

  Logically Collective

  Input Parameters:
+ ts  - the `TS` context obtained from `TSCreate()`
. F   - vector to hold the residual (or `NULL` to have it created internally)
. fun - the function evaluation routine
- ctx - user-defined context for private data for the function evaluation routine (may be `NULL`)

  Level: beginner

.seealso: [](ch_ts), `TS`, `TSI2FunctionFn`, `TSSetI2Jacobian()`, `TSSetIFunction()`,
`TSCreate()`, `TSSetRHSFunction()`
@*/
PetscErrorCode TSSetI2Function(TS ts, Vec F, TSI2FunctionFn *fun, void *ctx)
{
  DM dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (F) PetscValidHeaderSpecific(F, VEC_CLASSID, 2);
  PetscCall(TSSetIFunction(ts, F, NULL, NULL));
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetI2Function(dm, fun, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetI2Function - Returns the vector where the implicit residual is stored and the function/context to compute it.

  Not Collective

  Input Parameter:
. ts - the `TS` context

  Output Parameters:
+ r   - vector to hold residual (or `NULL`)
. fun - the function to compute residual (or `NULL`)
- ctx - the function context (or `NULL`)

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSSetIFunction()`, `SNESGetFunction()`, `TSCreate()`
@*/
PetscErrorCode TSGetI2Function(TS ts, Vec *r, TSI2FunctionFn **fun, void **ctx)
{
  SNES snes;
  DM   dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESGetFunction(snes, r, NULL, NULL));
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetI2Function(dm, fun, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetI2Jacobian - Set the function to compute the matrix dF/dU + v*dF/dU_t  + a*dF/dU_tt
  where F(t,U,U_t,U_tt) is the function you provided with `TSSetI2Function()`.

  Logically Collective

  Input Parameters:
+ ts  - the `TS` context obtained from `TSCreate()`
. J   - matrix to hold the Jacobian values
. P   - matrix for constructing the preconditioner (may be same as `J`)
. jac - the Jacobian evaluation routine, see `TSI2JacobianFn` for the calling sequence
- ctx - user-defined context for private data for the Jacobian evaluation routine (may be `NULL`)

  Level: beginner

  Notes:
  The matrices `J` and `P` are exactly the matrices that are used by `SNES` for the nonlinear solve.

  The matrix dF/dU + v*dF/dU_t + a*dF/dU_tt you provide turns out to be
  the Jacobian of G(U) = F(t,U,W+v*U,W'+a*U) where F(t,U,U_t,U_tt) = 0 is the DAE to be solved.
  The time integrator internally approximates U_t by W+v*U and U_tt by W'+a*U  where the positive "shift"
  parameters 'v' and 'a' and vectors W, W' depend on the integration method, step size, and past states.

.seealso: [](ch_ts), `TS`, `TSI2JacobianFn`, `TSSetI2Function()`, `TSGetI2Jacobian()`
@*/
PetscErrorCode TSSetI2Jacobian(TS ts, Mat J, Mat P, TSI2JacobianFn *jac, void *ctx)
{
  DM dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (J) PetscValidHeaderSpecific(J, MAT_CLASSID, 2);
  if (P) PetscValidHeaderSpecific(P, MAT_CLASSID, 3);
  PetscCall(TSSetIJacobian(ts, J, P, NULL, NULL));
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetI2Jacobian(dm, jac, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetI2Jacobian - Returns the implicit Jacobian at the present timestep.

  Not Collective, but parallel objects are returned if `TS` is parallel

  Input Parameter:
. ts - The `TS` context obtained from `TSCreate()`

  Output Parameters:
+ J   - The (approximate) Jacobian of F(t,U,U_t,U_tt)
. P   - The matrix from which the preconditioner is constructed, often the same as `J`
. jac - The function to compute the Jacobian matrices
- ctx - User-defined context for Jacobian evaluation routine

  Level: advanced

  Note:
  You can pass in `NULL` for any return argument you do not need.

.seealso: [](ch_ts), `TS`, `TSGetTimeStep()`, `TSGetMatrices()`, `TSGetTime()`, `TSGetStepNumber()`, `TSSetI2Jacobian()`, `TSGetI2Function()`, `TSCreate()`
@*/
PetscErrorCode TSGetI2Jacobian(TS ts, Mat *J, Mat *P, TSI2JacobianFn **jac, void **ctx)
{
  SNES snes;
  DM   dm;

  PetscFunctionBegin;
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESSetUpMatrices(snes));
  PetscCall(SNESGetJacobian(snes, J, P, NULL, NULL));
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetI2Jacobian(dm, jac, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeI2Function - Evaluates the DAE residual written in implicit form F(t,U,U_t,U_tt) = 0

  Collective

  Input Parameters:
+ ts - the `TS` context
. t  - current time
. U  - state vector
. V  - time derivative of state vector (U_t)
- A  - second time derivative of state vector (U_tt)

  Output Parameter:
. F - the residual vector

  Level: developer

  Note:
  Most users should not need to explicitly call this routine, as it
  is used internally within the nonlinear solvers.

.seealso: [](ch_ts), `TS`, `TSSetI2Function()`, `TSGetI2Function()`
@*/
PetscErrorCode TSComputeI2Function(TS ts, PetscReal t, Vec U, Vec V, Vec A, Vec F)
{
  DM               dm;
  TSI2FunctionFn  *I2Function;
  void            *ctx;
  TSRHSFunctionFn *rhsfunction;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscValidHeaderSpecific(V, VEC_CLASSID, 4);
  PetscValidHeaderSpecific(A, VEC_CLASSID, 5);
  PetscValidHeaderSpecific(F, VEC_CLASSID, 6);

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetI2Function(dm, &I2Function, &ctx));
  PetscCall(DMTSGetRHSFunction(dm, &rhsfunction, NULL));

  if (!I2Function) {
    PetscCall(TSComputeIFunction(ts, t, U, A, F, PETSC_FALSE));
    PetscFunctionReturn(PETSC_SUCCESS);
  }

  PetscCall(PetscLogEventBegin(TS_FunctionEval, U, ts, V, F));

  PetscCallBack("TS callback implicit function", I2Function(ts, t, U, V, A, F, ctx));

  if (rhsfunction) {
    Vec Frhs;

    PetscCall(DMGetGlobalVector(dm, &Frhs));
    PetscCall(TSComputeRHSFunction(ts, t, U, Frhs));
    PetscCall(VecAXPY(F, -1, Frhs));
    PetscCall(DMRestoreGlobalVector(dm, &Frhs));
  }

  PetscCall(PetscLogEventEnd(TS_FunctionEval, U, ts, V, F));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeI2Jacobian - Evaluates the Jacobian of the DAE

  Collective

  Input Parameters:
+ ts     - the `TS` context
. t      - current timestep
. U      - state vector
. V      - time derivative of state vector
. A      - second time derivative of state vector
. shiftV - shift to apply, see note below
- shiftA - shift to apply, see note below

  Output Parameters:
+ J - Jacobian matrix
- P - optional matrix used to construct the preconditioner

  Level: developer

  Notes:
  If $F(t,U,V,A) = 0$ is the DAE, the required Jacobian is

$$
  dF/dU + shiftV*dF/dV + shiftA*dF/dA
$$

  Most users should not need to explicitly call this routine, as it
  is used internally within the ODE integrators.

.seealso: [](ch_ts), `TS`, `TSSetI2Jacobian()`
@*/
PetscErrorCode TSComputeI2Jacobian(TS ts, PetscReal t, Vec U, Vec V, Vec A, PetscReal shiftV, PetscReal shiftA, Mat J, Mat P)
{
  DM               dm;
  TSI2JacobianFn  *I2Jacobian;
  void            *ctx;
  TSRHSJacobianFn *rhsjacobian;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscValidHeaderSpecific(V, VEC_CLASSID, 4);
  PetscValidHeaderSpecific(A, VEC_CLASSID, 5);
  PetscValidHeaderSpecific(J, MAT_CLASSID, 8);
  PetscValidHeaderSpecific(P, MAT_CLASSID, 9);

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetI2Jacobian(dm, &I2Jacobian, &ctx));
  PetscCall(DMTSGetRHSJacobian(dm, &rhsjacobian, NULL));

  if (!I2Jacobian) {
    PetscCall(TSComputeIJacobian(ts, t, U, A, shiftA, J, P, PETSC_FALSE));
    PetscFunctionReturn(PETSC_SUCCESS);
  }

  PetscCall(PetscLogEventBegin(TS_JacobianEval, U, ts, J, P));
  PetscCallBack("TS callback implicit Jacobian", I2Jacobian(ts, t, U, V, A, shiftV, shiftA, J, P, ctx));
  if (rhsjacobian) {
    Mat Jrhs, Prhs;
    PetscCall(TSGetRHSMats_Private(ts, &Jrhs, &Prhs));
    PetscCall(TSComputeRHSJacobian(ts, t, U, Jrhs, Prhs));
    PetscCall(MatAXPY(J, -1, Jrhs, ts->axpy_pattern));
    if (P != J) PetscCall(MatAXPY(P, -1, Prhs, ts->axpy_pattern));
  }

  PetscCall(PetscLogEventEnd(TS_JacobianEval, U, ts, J, P));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetTransientVariable - sets function to transform from state to transient variables

  Logically Collective

  Input Parameters:
+ ts   - time stepping context on which to change the transient variable
. tvar - a function that transforms to transient variables, see `TSTransientVariableFn` for the calling sequence
- ctx  - a context for tvar

  Level: advanced

  Notes:
  This is typically used to transform from primitive to conservative variables so that a time integrator (e.g., `TSBDF`)
  can be conservative.  In this context, primitive variables P are used to model the state (e.g., because they lead to
  well-conditioned formulations even in limiting cases such as low-Mach or zero porosity).  The transient variable is
  C(P), specified by calling this function.  An IFunction thus receives arguments (P, Cdot) and the IJacobian must be
  evaluated via the chain rule, as in
.vb
     dF/dP + shift * dF/dCdot dC/dP.
.ve

.seealso: [](ch_ts), `TS`, `TSBDF`, `TSTransientVariableFn`, `DMTSSetTransientVariable()`, `DMTSGetTransientVariable()`, `TSSetIFunction()`, `TSSetIJacobian()`
@*/
PetscErrorCode TSSetTransientVariable(TS ts, TSTransientVariableFn *tvar, void *ctx)
{
  DM dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSSetTransientVariable(dm, tvar, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeTransientVariable - transforms state (primitive) variables to transient (conservative) variables

  Logically Collective

  Input Parameters:
+ ts - TS on which to compute
- U  - state vector to be transformed to transient variables

  Output Parameter:
. C - transient (conservative) variable

  Level: developer

  Developer Notes:
  If `DMTSSetTransientVariable()` has not been called, then C is not modified in this routine and C = `NULL` is allowed.
  This makes it safe to call without a guard.  One can use `TSHasTransientVariable()` to check if transient variables are
  being used.

.seealso: [](ch_ts), `TS`, `TSBDF`, `DMTSSetTransientVariable()`, `TSComputeIFunction()`, `TSComputeIJacobian()`
@*/
PetscErrorCode TSComputeTransientVariable(TS ts, Vec U, Vec C)
{
  DM   dm;
  DMTS dmts;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 2);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMGetDMTS(dm, &dmts));
  if (dmts->ops->transientvar) {
    PetscValidHeaderSpecific(C, VEC_CLASSID, 3);
    PetscCall((*dmts->ops->transientvar)(ts, U, C, dmts->transientvarctx));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSHasTransientVariable - determine whether transient variables have been set

  Logically Collective

  Input Parameter:
. ts - `TS` on which to compute

  Output Parameter:
. has - `PETSC_TRUE` if transient variables have been set

  Level: developer

.seealso: [](ch_ts), `TS`, `TSBDF`, `DMTSSetTransientVariable()`, `TSComputeTransientVariable()`
@*/
PetscErrorCode TSHasTransientVariable(TS ts, PetscBool *has)
{
  DM   dm;
  DMTS dmts;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMGetDMTS(dm, &dmts));
  *has = dmts->ops->transientvar ? PETSC_TRUE : PETSC_FALSE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TS2SetSolution - Sets the initial solution and time derivative vectors
  for use by the `TS` routines handling second order equations.

  Logically Collective

  Input Parameters:
+ ts - the `TS` context obtained from `TSCreate()`
. u  - the solution vector
- v  - the time derivative vector

  Level: beginner

.seealso: [](ch_ts), `TS`
@*/
PetscErrorCode TS2SetSolution(TS ts, Vec u, Vec v)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(u, VEC_CLASSID, 2);
  PetscValidHeaderSpecific(v, VEC_CLASSID, 3);
  PetscCall(TSSetSolution(ts, u));
  PetscCall(PetscObjectReference((PetscObject)v));
  PetscCall(VecDestroy(&ts->vec_dot));
  ts->vec_dot = v;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TS2GetSolution - Returns the solution and time derivative at the present timestep
  for second order equations.

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameters:
+ u - the vector containing the solution
- v - the vector containing the time derivative

  Level: intermediate

  Notes:
  It is valid to call this routine inside the function
  that you are evaluating in order to move to the new timestep. This vector not
  changed until the solution at the next timestep has been calculated.

.seealso: [](ch_ts), `TS`, `TS2SetSolution()`, `TSGetTimeStep()`, `TSGetTime()`
@*/
PetscErrorCode TS2GetSolution(TS ts, Vec *u, Vec *v)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (u) PetscAssertPointer(u, 2);
  if (v) PetscAssertPointer(v, 3);
  if (u) *u = ts->vec_sol;
  if (v) *v = ts->vec_dot;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSLoad - Loads a `TS` that has been stored in binary  with `TSView()`.

  Collective

  Input Parameters:
+ ts     - the newly loaded `TS`, this needs to have been created with `TSCreate()` or
           some related function before a call to `TSLoad()`.
- viewer - binary file viewer, obtained from `PetscViewerBinaryOpen()`

  Level: intermediate

  Note:
  The type is determined by the data in the file, any type set into the `TS` before this call is ignored.

.seealso: [](ch_ts), `TS`, `PetscViewer`, `PetscViewerBinaryOpen()`, `TSView()`, `MatLoad()`, `VecLoad()`
@*/
PetscErrorCode TSLoad(TS ts, PetscViewer viewer)
{
  PetscBool isbinary;
  PetscInt  classid;
  char      type[256];
  DMTS      sdm;
  DM        dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 2);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERBINARY, &isbinary));
  PetscCheck(isbinary, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Invalid viewer; open viewer with PetscViewerBinaryOpen()");

  PetscCall(PetscViewerBinaryRead(viewer, &classid, 1, NULL, PETSC_INT));
  PetscCheck(classid == TS_FILE_CLASSID, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "Not TS next in file");
  PetscCall(PetscViewerBinaryRead(viewer, type, 256, NULL, PETSC_CHAR));
  PetscCall(TSSetType(ts, type));
  PetscTryTypeMethod(ts, load, viewer);
  PetscCall(DMCreate(PetscObjectComm((PetscObject)ts), &dm));
  PetscCall(DMLoad(dm, viewer));
  PetscCall(TSSetDM(ts, dm));
  PetscCall(DMCreateGlobalVector(ts->dm, &ts->vec_sol));
  PetscCall(VecLoad(ts->vec_sol, viewer));
  PetscCall(DMGetDMTS(ts->dm, &sdm));
  PetscCall(DMTSLoad(sdm, viewer));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#include <petscdraw.h>
#if defined(PETSC_HAVE_SAWS)
  #include <petscviewersaws.h>
#endif

/*@
  TSViewFromOptions - View a `TS` based on values in the options database

  Collective

  Input Parameters:
+ ts   - the `TS` context
. obj  - Optional object that provides the prefix for the options database keys
- name - command line option string to be passed by user

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSView`, `PetscObjectViewFromOptions()`, `TSCreate()`
@*/
PetscErrorCode TSViewFromOptions(TS ts, PetscObject obj, const char name[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(PetscObjectViewFromOptions((PetscObject)ts, obj, name));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSView - Prints the `TS` data structure.

  Collective

  Input Parameters:
+ ts     - the `TS` context obtained from `TSCreate()`
- viewer - visualization context

  Options Database Key:
. -ts_view - calls `TSView()` at end of `TSStep()`

  Level: beginner

  Notes:
  The available visualization contexts include
+     `PETSC_VIEWER_STDOUT_SELF` - standard output (default)
-     `PETSC_VIEWER_STDOUT_WORLD` - synchronized standard
  output where only the first processor opens
  the file.  All other processors send their
  data to the first processor to print.

  The user can open an alternative visualization context with
  `PetscViewerASCIIOpen()` - output to a specified file.

  In the debugger you can do call `TSView`(ts,0) to display the `TS` solver. (The same holds for any PETSc object viewer).

.seealso: [](ch_ts), `TS`, `PetscViewer`, `PetscViewerASCIIOpen()`
@*/
PetscErrorCode TSView(TS ts, PetscViewer viewer)
{
  TSType    type;
  PetscBool iascii, isstring, isundials, isbinary, isdraw;
  DMTS      sdm;
#if defined(PETSC_HAVE_SAWS)
  PetscBool issaws;
#endif

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (!viewer) PetscCall(PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject)ts), &viewer));
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 2);
  PetscCheckSameComm(ts, 1, viewer, 2);

  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERSTRING, &isstring));
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERBINARY, &isbinary));
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERDRAW, &isdraw));
#if defined(PETSC_HAVE_SAWS)
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERSAWS, &issaws));
#endif
  if (iascii) {
    PetscCall(PetscObjectPrintClassNamePrefixType((PetscObject)ts, viewer));
    if (ts->ops->view) {
      PetscCall(PetscViewerASCIIPushTab(viewer));
      PetscUseTypeMethod(ts, view, viewer);
      PetscCall(PetscViewerASCIIPopTab(viewer));
    }
    if (ts->max_steps < PETSC_INT_MAX) PetscCall(PetscViewerASCIIPrintf(viewer, "  maximum steps=%" PetscInt_FMT "\n", ts->max_steps));
    if (ts->run_steps < PETSC_INT_MAX) PetscCall(PetscViewerASCIIPrintf(viewer, "  run steps=%" PetscInt_FMT "\n", ts->run_steps));
    if (ts->max_time < PETSC_MAX_REAL) PetscCall(PetscViewerASCIIPrintf(viewer, "  maximum time=%g\n", (double)ts->max_time));
    if (ts->ifuncs) PetscCall(PetscViewerASCIIPrintf(viewer, "  total number of I function evaluations=%" PetscInt_FMT "\n", ts->ifuncs));
    if (ts->ijacs) PetscCall(PetscViewerASCIIPrintf(viewer, "  total number of I Jacobian evaluations=%" PetscInt_FMT "\n", ts->ijacs));
    if (ts->rhsfuncs) PetscCall(PetscViewerASCIIPrintf(viewer, "  total number of RHS function evaluations=%" PetscInt_FMT "\n", ts->rhsfuncs));
    if (ts->rhsjacs) PetscCall(PetscViewerASCIIPrintf(viewer, "  total number of RHS Jacobian evaluations=%" PetscInt_FMT "\n", ts->rhsjacs));
    if (ts->usessnes) {
      PetscBool lin;
      if (ts->problem_type == TS_NONLINEAR) PetscCall(PetscViewerASCIIPrintf(viewer, "  total number of nonlinear solver iterations=%" PetscInt_FMT "\n", ts->snes_its));
      PetscCall(PetscViewerASCIIPrintf(viewer, "  total number of linear solver iterations=%" PetscInt_FMT "\n", ts->ksp_its));
      PetscCall(PetscObjectTypeCompareAny((PetscObject)ts->snes, &lin, SNESKSPONLY, SNESKSPTRANSPOSEONLY, ""));
      PetscCall(PetscViewerASCIIPrintf(viewer, "  total number of %slinear solve failures=%" PetscInt_FMT "\n", lin ? "" : "non", ts->num_snes_failures));
    }
    PetscCall(PetscViewerASCIIPrintf(viewer, "  total number of rejected steps=%" PetscInt_FMT "\n", ts->reject));
    if (ts->vrtol) PetscCall(PetscViewerASCIIPrintf(viewer, "  using vector of relative error tolerances, "));
    else PetscCall(PetscViewerASCIIPrintf(viewer, "  using relative error tolerance of %g, ", (double)ts->rtol));
    if (ts->vatol) PetscCall(PetscViewerASCIIPrintf(viewer, "  using vector of absolute error tolerances\n"));
    else PetscCall(PetscViewerASCIIPrintf(viewer, "  using absolute error tolerance of %g\n", (double)ts->atol));
    PetscCall(PetscViewerASCIIPushTab(viewer));
    PetscCall(TSAdaptView(ts->adapt, viewer));
    PetscCall(PetscViewerASCIIPopTab(viewer));
  } else if (isstring) {
    PetscCall(TSGetType(ts, &type));
    PetscCall(PetscViewerStringSPrintf(viewer, " TSType: %-7.7s", type));
    PetscTryTypeMethod(ts, view, viewer);
  } else if (isbinary) {
    PetscInt    classid = TS_FILE_CLASSID;
    MPI_Comm    comm;
    PetscMPIInt rank;
    char        type[256];

    PetscCall(PetscObjectGetComm((PetscObject)ts, &comm));
    PetscCallMPI(MPI_Comm_rank(comm, &rank));
    if (rank == 0) {
      PetscCall(PetscViewerBinaryWrite(viewer, &classid, 1, PETSC_INT));
      PetscCall(PetscStrncpy(type, ((PetscObject)ts)->type_name, 256));
      PetscCall(PetscViewerBinaryWrite(viewer, type, 256, PETSC_CHAR));
    }
    PetscTryTypeMethod(ts, view, viewer);
    if (ts->adapt) PetscCall(TSAdaptView(ts->adapt, viewer));
    PetscCall(DMView(ts->dm, viewer));
    PetscCall(VecView(ts->vec_sol, viewer));
    PetscCall(DMGetDMTS(ts->dm, &sdm));
    PetscCall(DMTSView(sdm, viewer));
  } else if (isdraw) {
    PetscDraw draw;
    char      str[36];
    PetscReal x, y, bottom, h;

    PetscCall(PetscViewerDrawGetDraw(viewer, 0, &draw));
    PetscCall(PetscDrawGetCurrentPoint(draw, &x, &y));
    PetscCall(PetscStrncpy(str, "TS: ", sizeof(str)));
    PetscCall(PetscStrlcat(str, ((PetscObject)ts)->type_name, sizeof(str)));
    PetscCall(PetscDrawStringBoxed(draw, x, y, PETSC_DRAW_BLACK, PETSC_DRAW_BLACK, str, NULL, &h));
    bottom = y - h;
    PetscCall(PetscDrawPushCurrentPoint(draw, x, bottom));
    PetscTryTypeMethod(ts, view, viewer);
    if (ts->adapt) PetscCall(TSAdaptView(ts->adapt, viewer));
    if (ts->snes) PetscCall(SNESView(ts->snes, viewer));
    PetscCall(PetscDrawPopCurrentPoint(draw));
#if defined(PETSC_HAVE_SAWS)
  } else if (issaws) {
    PetscMPIInt rank;
    const char *name;

    PetscCall(PetscObjectGetName((PetscObject)ts, &name));
    PetscCallMPI(MPI_Comm_rank(PETSC_COMM_WORLD, &rank));
    if (!((PetscObject)ts)->amsmem && rank == 0) {
      char dir[1024];

      PetscCall(PetscObjectViewSAWs((PetscObject)ts, viewer));
      PetscCall(PetscSNPrintf(dir, 1024, "/PETSc/Objects/%s/time_step", name));
      PetscCallSAWs(SAWs_Register, (dir, &ts->steps, 1, SAWs_READ, SAWs_INT));
      PetscCall(PetscSNPrintf(dir, 1024, "/PETSc/Objects/%s/time", name));
      PetscCallSAWs(SAWs_Register, (dir, &ts->ptime, 1, SAWs_READ, SAWs_DOUBLE));
    }
    PetscTryTypeMethod(ts, view, viewer);
#endif
  }
  if (ts->snes && ts->usessnes) {
    PetscCall(PetscViewerASCIIPushTab(viewer));
    PetscCall(SNESView(ts->snes, viewer));
    PetscCall(PetscViewerASCIIPopTab(viewer));
  }
  PetscCall(DMGetDMTS(ts->dm, &sdm));
  PetscCall(DMTSView(sdm, viewer));

  PetscCall(PetscViewerASCIIPushTab(viewer));
  PetscCall(PetscObjectTypeCompare((PetscObject)ts, TSSUNDIALS, &isundials));
  PetscCall(PetscViewerASCIIPopTab(viewer));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetApplicationContext - Sets an optional user-defined context for the timesteppers that may be accessed, for example inside the user provided
  `TS` callbacks with `TSGetApplicationContext()`

  Logically Collective

  Input Parameters:
+ ts  - the `TS` context obtained from `TSCreate()`
- ctx - user context

  Level: intermediate

  Fortran Note:
  This only works when `ctx` is a Fortran derived type (it cannot be a `PetscObject`), we recommend writing a Fortran interface definition for this
  function that tells the Fortran compiler the derived data type that is passed in as the `ctx` argument. See `TSGetApplicationContext()` for
  an example.

.seealso: [](ch_ts), `TS`, `TSGetApplicationContext()`
@*/
PetscErrorCode TSSetApplicationContext(TS ts, PeCtx ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->ctx = ctx;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetApplicationContext - Gets the user-defined context for the
  timestepper that was set with `TSSetApplicationContext()`

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. ctx - a pointer to the user context

  Level: intermediate

  Fortran Notes:
  This only works when the context is a Fortran derived type (it cannot be a `PetscObject`) and you **must** write a Fortran interface definition for this
  function that tells the Fortran compiler the derived data type that is returned as the `ctx` argument. For example,
.vb
  Interface TSGetApplicationContext
    Subroutine TSGetApplicationContext(ts,ctx,ierr)
  #include <petsc/finclude/petscts.h>
      use petscts
      TS ts
      type(tUsertype), pointer :: ctx
      PetscErrorCode ierr
    End Subroutine
  End Interface TSGetApplicationContext
.ve

  The prototype for `ctx` must be
.vb
  type(tUsertype), pointer :: ctx
.ve

.seealso: [](ch_ts), `TS`, `TSSetApplicationContext()`
@*/
PetscErrorCode TSGetApplicationContext(TS ts, void *ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  *(void **)ctx = ts->ctx;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetStepNumber - Gets the number of time steps completed.

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. steps - number of steps completed so far

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSGetTime()`, `TSGetTimeStep()`, `TSSetPreStep()`, `TSSetPreStage()`, `TSSetPostStage()`, `TSSetPostStep()`
@*/
PetscErrorCode TSGetStepNumber(TS ts, PetscInt *steps)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(steps, 2);
  *steps = ts->steps;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetStepNumber - Sets the number of steps completed.

  Logically Collective

  Input Parameters:
+ ts    - the `TS` context
- steps - number of steps completed so far

  Level: developer

  Note:
  For most uses of the `TS` solvers the user need not explicitly call
  `TSSetStepNumber()`, as the step counter is appropriately updated in
  `TSSolve()`/`TSStep()`/`TSRollBack()`. Power users may call this routine to
  reinitialize timestepping by setting the step counter to zero (and time
  to the initial time) to solve a similar problem with different initial
  conditions or parameters. Other possible use case is to continue
  timestepping from a previously interrupted run in such a way that `TS`
  monitors will be called with a initial nonzero step counter.

.seealso: [](ch_ts), `TS`, `TSGetStepNumber()`, `TSSetTime()`, `TSSetTimeStep()`, `TSSetSolution()`
@*/
PetscErrorCode TSSetStepNumber(TS ts, PetscInt steps)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidLogicalCollectiveInt(ts, steps, 2);
  PetscCheck(steps >= 0, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_OUTOFRANGE, "Step number must be non-negative");
  ts->steps = steps;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetTimeStep - Allows one to reset the timestep at any time,
  useful for simple pseudo-timestepping codes.

  Logically Collective

  Input Parameters:
+ ts        - the `TS` context obtained from `TSCreate()`
- time_step - the size of the timestep

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSPSEUDO`, `TSGetTimeStep()`, `TSSetTime()`
@*/
PetscErrorCode TSSetTimeStep(TS ts, PetscReal time_step)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidLogicalCollectiveReal(ts, time_step, 2);
  ts->time_step = time_step;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetExactFinalTime - Determines whether to adapt the final time step to
  match the exact final time, to interpolate the solution to the exact final time,
  or to just return at the final time `TS` computed (which may be slightly larger
  than the requested final time).

  Logically Collective

  Input Parameters:
+ ts     - the time-step context
- eftopt - exact final time option
.vb
  TS_EXACTFINALTIME_STEPOVER    - Don't do anything if final time is exceeded, just use it
  TS_EXACTFINALTIME_INTERPOLATE - Interpolate back to final time if the final time is exceeded
  TS_EXACTFINALTIME_MATCHSTEP   - Adapt final time step to ensure the computed final time exactly equals the requested final time
.ve

  Options Database Key:
. -ts_exact_final_time <stepover,interpolate,matchstep> - select the final step approach at runtime

  Level: beginner

  Note:
  If you use the option `TS_EXACTFINALTIME_STEPOVER` the solution may be at a very different time
  then the final time you selected.

.seealso: [](ch_ts), `TS`, `TSExactFinalTimeOption`, `TSGetExactFinalTime()`
@*/
PetscErrorCode TSSetExactFinalTime(TS ts, TSExactFinalTimeOption eftopt)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidLogicalCollectiveEnum(ts, eftopt, 2);
  ts->exact_final_time = eftopt;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetExactFinalTime - Gets the exact final time option set with `TSSetExactFinalTime()`

  Not Collective

  Input Parameter:
. ts - the `TS` context

  Output Parameter:
. eftopt - exact final time option

  Level: beginner

.seealso: [](ch_ts), `TS`, `TSExactFinalTimeOption`, `TSSetExactFinalTime()`
@*/
PetscErrorCode TSGetExactFinalTime(TS ts, TSExactFinalTimeOption *eftopt)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(eftopt, 2);
  *eftopt = ts->exact_final_time;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetTimeStep - Gets the current timestep size.

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. dt - the current timestep size

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSSetTimeStep()`, `TSGetTime()`
@*/
PetscErrorCode TSGetTimeStep(TS ts, PetscReal *dt)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(dt, 2);
  *dt = ts->time_step;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetSolution - Returns the solution at the present timestep. It
  is valid to call this routine inside the function that you are evaluating
  in order to move to the new timestep. This vector not changed until
  the solution at the next timestep has been calculated.

  Not Collective, but v returned is parallel if ts is parallel

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. v - the vector containing the solution

  Level: intermediate

  Note:
  If you used `TSSetExactFinalTime`(ts,`TS_EXACTFINALTIME_MATCHSTEP`); this does not return the solution at the requested
  final time. It returns the solution at the next timestep.

.seealso: [](ch_ts), `TS`, `TSGetTimeStep()`, `TSGetTime()`, `TSGetSolveTime()`, `TSGetSolutionComponents()`, `TSSetSolutionFunction()`
@*/
PetscErrorCode TSGetSolution(TS ts, Vec *v)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(v, 2);
  *v = ts->vec_sol;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetSolutionComponents - Returns any solution components at the present
  timestep, if available for the time integration method being used.
  Solution components are quantities that share the same size and
  structure as the solution vector.

  Not Collective, but v returned is parallel if ts is parallel

  Input Parameters:
+ ts - the `TS` context obtained from `TSCreate()` (input parameter).
. n  - If v is `NULL`, then the number of solution components is
       returned through n, else the n-th solution component is
       returned in v.
- v  - the vector containing the n-th solution component
       (may be `NULL` to use this function to find out
        the number of solutions components).

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSGetSolution()`
@*/
PetscErrorCode TSGetSolutionComponents(TS ts, PetscInt *n, Vec *v)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (!ts->ops->getsolutioncomponents) *n = 0;
  else PetscUseTypeMethod(ts, getsolutioncomponents, n, v);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetAuxSolution - Returns an auxiliary solution at the present
  timestep, if available for the time integration method being used.

  Not Collective, but v returned is parallel if ts is parallel

  Input Parameters:
+ ts - the `TS` context obtained from `TSCreate()` (input parameter).
- v  - the vector containing the auxiliary solution

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSGetSolution()`
@*/
PetscErrorCode TSGetAuxSolution(TS ts, Vec *v)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->ops->getauxsolution) PetscUseTypeMethod(ts, getauxsolution, v);
  else PetscCall(VecZeroEntries(*v));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetTimeError - Returns the estimated error vector, if the chosen
  `TSType` has an error estimation functionality and `TSSetTimeError()` was called

  Not Collective, but v returned is parallel if ts is parallel

  Input Parameters:
+ ts - the `TS` context obtained from `TSCreate()` (input parameter).
. n  - current estimate (n=0) or previous one (n=-1)
- v  - the vector containing the error (same size as the solution).

  Level: intermediate

  Note:
  MUST call after `TSSetUp()`

.seealso: [](ch_ts), `TSGetSolution()`, `TSSetTimeError()`
@*/
PetscErrorCode TSGetTimeError(TS ts, PetscInt n, Vec *v)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->ops->gettimeerror) PetscUseTypeMethod(ts, gettimeerror, n, v);
  else PetscCall(VecZeroEntries(*v));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetTimeError - Sets the estimated error vector, if the chosen
  `TSType` has an error estimation functionality. This can be used
  to restart such a time integrator with a given error vector.

  Not Collective, but v returned is parallel if ts is parallel

  Input Parameters:
+ ts - the `TS` context obtained from `TSCreate()` (input parameter).
- v  - the vector containing the error (same size as the solution).

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSSetSolution()`, `TSGetTimeError()`
@*/
PetscErrorCode TSSetTimeError(TS ts, Vec v)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCheck(ts->setupcalled, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONGSTATE, "Must call TSSetUp() first");
  PetscTryTypeMethod(ts, settimeerror, v);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* ----- Routines to initialize and destroy a timestepper ---- */
/*@
  TSSetProblemType - Sets the type of problem to be solved.

  Not collective

  Input Parameters:
+ ts   - The `TS`
- type - One of `TS_LINEAR`, `TS_NONLINEAR` where these types refer to problems of the forms
.vb
         U_t - A U = 0      (linear)
         U_t - A(t) U = 0   (linear)
         F(t,U,U_t) = 0     (nonlinear)
.ve

  Level: beginner

.seealso: [](ch_ts), `TSSetUp()`, `TSProblemType`, `TS`
@*/
PetscErrorCode TSSetProblemType(TS ts, TSProblemType type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->problem_type = type;
  if (type == TS_LINEAR) {
    SNES snes;
    PetscCall(TSGetSNES(ts, &snes));
    PetscCall(SNESSetType(snes, SNESKSPONLY));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetProblemType - Gets the type of problem to be solved.

  Not collective

  Input Parameter:
. ts - The `TS`

  Output Parameter:
. type - One of `TS_LINEAR`, `TS_NONLINEAR` where these types refer to problems of the forms
.vb
         M U_t = A U
         M(t) U_t = A(t) U
         F(t,U,U_t)
.ve

  Level: beginner

.seealso: [](ch_ts), `TSSetUp()`, `TSProblemType`, `TS`
@*/
PetscErrorCode TSGetProblemType(TS ts, TSProblemType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(type, 2);
  *type = ts->problem_type;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*
    Attempt to check/preset a default value for the exact final time option. This is needed at the beginning of TSSolve() and in TSSetUp()
*/
static PetscErrorCode TSSetExactFinalTimeDefault(TS ts)
{
  PetscBool isnone;

  PetscFunctionBegin;
  PetscCall(TSGetAdapt(ts, &ts->adapt));
  PetscCall(TSAdaptSetDefaultType(ts->adapt, ts->default_adapt_type));

  PetscCall(PetscObjectTypeCompare((PetscObject)ts->adapt, TSADAPTNONE, &isnone));
  if (!isnone && ts->exact_final_time == TS_EXACTFINALTIME_UNSPECIFIED) ts->exact_final_time = TS_EXACTFINALTIME_MATCHSTEP;
  else if (ts->exact_final_time == TS_EXACTFINALTIME_UNSPECIFIED) ts->exact_final_time = TS_EXACTFINALTIME_INTERPOLATE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetUp - Sets up the internal data structures for the later use of a timestepper.

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Level: advanced

  Note:
  For basic use of the `TS` solvers the user need not explicitly call
  `TSSetUp()`, since these actions will automatically occur during
  the call to `TSStep()` or `TSSolve()`.  However, if one wishes to control this
  phase separately, `TSSetUp()` should be called after `TSCreate()`
  and optional routines of the form TSSetXXX(), but before `TSStep()` and `TSSolve()`.

.seealso: [](ch_ts), `TSCreate()`, `TS`, `TSStep()`, `TSDestroy()`, `TSSolve()`
@*/
PetscErrorCode TSSetUp(TS ts)
{
  DM dm;
  PetscErrorCode (*func)(SNES, Vec, Vec, void *);
  PetscErrorCode (*jac)(SNES, Vec, Mat, Mat, void *);
  TSIFunctionFn   *ifun;
  TSIJacobianFn   *ijac;
  TSI2JacobianFn  *i2jac;
  TSRHSJacobianFn *rhsjac;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->setupcalled) PetscFunctionReturn(PETSC_SUCCESS);

  if (!((PetscObject)ts)->type_name) {
    PetscCall(TSGetIFunction(ts, NULL, &ifun, NULL));
    PetscCall(TSSetType(ts, ifun ? TSBEULER : TSEULER));
  }

  if (!ts->vec_sol) {
    PetscCheck(ts->dm, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONGSTATE, "Must call TSSetSolution() first");
    PetscCall(DMCreateGlobalVector(ts->dm, &ts->vec_sol));
  }

  if (ts->eval_times) {
    if (!ts->eval_times->sol_vecs) PetscCall(VecDuplicateVecs(ts->vec_sol, ts->eval_times->num_time_points, &ts->eval_times->sol_vecs));
    if (!ts->eval_times->sol_times) PetscCall(PetscMalloc1(ts->eval_times->num_time_points, &ts->eval_times->sol_times));
  }
  if (!ts->Jacp && ts->Jacprhs) { /* IJacobianP shares the same matrix with RHSJacobianP if only RHSJacobianP is provided */
    PetscCall(PetscObjectReference((PetscObject)ts->Jacprhs));
    ts->Jacp = ts->Jacprhs;
  }

  if (ts->quadraturets) {
    PetscCall(TSSetUp(ts->quadraturets));
    PetscCall(VecDestroy(&ts->vec_costintegrand));
    PetscCall(VecDuplicate(ts->quadraturets->vec_sol, &ts->vec_costintegrand));
  }

  PetscCall(TSGetRHSJacobian(ts, NULL, NULL, &rhsjac, NULL));
  if (rhsjac == TSComputeRHSJacobianConstant) {
    Mat  Amat, Pmat;
    SNES snes;
    PetscCall(TSGetSNES(ts, &snes));
    PetscCall(SNESGetJacobian(snes, &Amat, &Pmat, NULL, NULL));
    /* Matching matrices implies that an IJacobian is NOT set, because if it had been set, the IJacobian's matrix would
     * have displaced the RHS matrix */
    if (Amat && Amat == ts->Arhs) {
      /* we need to copy the values of the matrix because for the constant Jacobian case the user will never set the numerical values in this new location */
      PetscCall(MatDuplicate(ts->Arhs, MAT_COPY_VALUES, &Amat));
      PetscCall(SNESSetJacobian(snes, Amat, NULL, NULL, NULL));
      PetscCall(MatDestroy(&Amat));
    }
    if (Pmat && Pmat == ts->Brhs) {
      PetscCall(MatDuplicate(ts->Brhs, MAT_COPY_VALUES, &Pmat));
      PetscCall(SNESSetJacobian(snes, NULL, Pmat, NULL, NULL));
      PetscCall(MatDestroy(&Pmat));
    }
  }

  PetscCall(TSGetAdapt(ts, &ts->adapt));
  PetscCall(TSAdaptSetDefaultType(ts->adapt, ts->default_adapt_type));

  PetscTryTypeMethod(ts, setup);

  PetscCall(TSSetExactFinalTimeDefault(ts));

  /* In the case where we've set a DMTSFunction or what have you, we need the default SNESFunction
     to be set right but can't do it elsewhere due to the overreliance on ctx=ts.
   */
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMSNESGetFunction(dm, &func, NULL));
  if (!func) PetscCall(DMSNESSetFunction(dm, SNESTSFormFunction, ts));

  /* If the SNES doesn't have a jacobian set and the TS has an ijacobian or rhsjacobian set, set the SNES to use it.
     Otherwise, the SNES will use coloring internally to form the Jacobian.
   */
  PetscCall(DMSNESGetJacobian(dm, &jac, NULL));
  PetscCall(DMTSGetIJacobian(dm, &ijac, NULL));
  PetscCall(DMTSGetI2Jacobian(dm, &i2jac, NULL));
  PetscCall(DMTSGetRHSJacobian(dm, &rhsjac, NULL));
  if (!jac && (ijac || i2jac || rhsjac)) PetscCall(DMSNESSetJacobian(dm, SNESTSFormJacobian, ts));

  /* if time integration scheme has a starting method, call it */
  PetscTryTypeMethod(ts, startingmethod);

  ts->setupcalled = PETSC_TRUE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSReset - Resets a `TS` context to the state it was in before `TSSetUp()` was called and removes any allocated `Vec` and `Mat` from its data structures

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Level: developer

  Notes:
  Any options set on the `TS` object, including those set with `TSSetFromOptions()` remain.

  See also `TSSetResize()` to change the size of the system being integrated (for example by adaptive mesh refinement) during the time integration.

.seealso: [](ch_ts), `TS`, `TSCreate()`, `TSSetUp()`, `TSDestroy()`, `TSSetResize()`
@*/
PetscErrorCode TSReset(TS ts)
{
  TS_RHSSplitLink ilink = ts->tsrhssplit, next;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);

  PetscTryTypeMethod(ts, reset);
  if (ts->snes) PetscCall(SNESReset(ts->snes));
  if (ts->adapt) PetscCall(TSAdaptReset(ts->adapt));

  PetscCall(MatDestroy(&ts->Arhs));
  PetscCall(MatDestroy(&ts->Brhs));
  PetscCall(VecDestroy(&ts->Frhs));
  PetscCall(VecDestroy(&ts->vec_sol));
  PetscCall(VecDestroy(&ts->vec_sol0));
  PetscCall(VecDestroy(&ts->vec_dot));
  PetscCall(VecDestroy(&ts->vatol));
  PetscCall(VecDestroy(&ts->vrtol));
  PetscCall(VecDestroyVecs(ts->nwork, &ts->work));

  PetscCall(MatDestroy(&ts->Jacprhs));
  PetscCall(MatDestroy(&ts->Jacp));
  if (ts->forward_solve) PetscCall(TSForwardReset(ts));
  if (ts->quadraturets) {
    PetscCall(TSReset(ts->quadraturets));
    PetscCall(VecDestroy(&ts->vec_costintegrand));
  }
  while (ilink) {
    next = ilink->next;
    PetscCall(TSDestroy(&ilink->ts));
    PetscCall(PetscFree(ilink->splitname));
    PetscCall(ISDestroy(&ilink->is));
    PetscCall(PetscFree(ilink));
    ilink = next;
  }
  ts->tsrhssplit     = NULL;
  ts->num_rhs_splits = 0;
  if (ts->eval_times) {
    PetscCall(PetscFree(ts->eval_times->time_points));
    PetscCall(PetscFree(ts->eval_times->sol_times));
    PetscCall(VecDestroyVecs(ts->eval_times->num_time_points, &ts->eval_times->sol_vecs));
    PetscCall(PetscFree(ts->eval_times));
  }
  ts->rhsjacobian.time  = PETSC_MIN_REAL;
  ts->rhsjacobian.scale = 1.0;
  ts->ijacobian.shift   = 1.0;
  ts->setupcalled       = PETSC_FALSE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode TSResizeReset(TS);

/*@
  TSDestroy - Destroys the timestepper context that was created
  with `TSCreate()`.

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Level: beginner

.seealso: [](ch_ts), `TS`, `TSCreate()`, `TSSetUp()`, `TSSolve()`
@*/
PetscErrorCode TSDestroy(TS *ts)
{
  PetscFunctionBegin;
  if (!*ts) PetscFunctionReturn(PETSC_SUCCESS);
  PetscValidHeaderSpecific(*ts, TS_CLASSID, 1);
  if (--((PetscObject)*ts)->refct > 0) {
    *ts = NULL;
    PetscFunctionReturn(PETSC_SUCCESS);
  }

  PetscCall(TSReset(*ts));
  PetscCall(TSAdjointReset(*ts));
  if ((*ts)->forward_solve) PetscCall(TSForwardReset(*ts));
  PetscCall(TSResizeReset(*ts));

  /* if memory was published with SAWs then destroy it */
  PetscCall(PetscObjectSAWsViewOff((PetscObject)*ts));
  PetscTryTypeMethod(*ts, destroy);

  PetscCall(TSTrajectoryDestroy(&(*ts)->trajectory));

  PetscCall(TSAdaptDestroy(&(*ts)->adapt));
  PetscCall(TSEventDestroy(&(*ts)->event));

  PetscCall(SNESDestroy(&(*ts)->snes));
  PetscCall(SNESDestroy(&(*ts)->snesrhssplit));
  PetscCall(DMDestroy(&(*ts)->dm));
  PetscCall(TSMonitorCancel(*ts));
  PetscCall(TSAdjointMonitorCancel(*ts));

  PetscCall(TSDestroy(&(*ts)->quadraturets));
  PetscCall(PetscHeaderDestroy(ts));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetSNES - Returns the `SNES` (nonlinear solver) associated with
  a `TS` (timestepper) context. Valid only for nonlinear problems.

  Not Collective, but snes is parallel if ts is parallel

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. snes - the nonlinear solver context

  Level: beginner

  Notes:
  The user can then directly manipulate the `SNES` context to set various
  options, etc.  Likewise, the user can then extract and manipulate the
  `KSP`, and `PC` contexts as well.

  `TSGetSNES()` does not work for integrators that do not use `SNES`; in
  this case `TSGetSNES()` returns `NULL` in `snes`.

.seealso: [](ch_ts), `TS`, `SNES`, `TSCreate()`, `TSSetUp()`, `TSSolve()`
@*/
PetscErrorCode TSGetSNES(TS ts, SNES *snes)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(snes, 2);
  if (!ts->snes) {
    PetscCall(SNESCreate(PetscObjectComm((PetscObject)ts), &ts->snes));
    PetscCall(PetscObjectSetOptions((PetscObject)ts->snes, ((PetscObject)ts)->options));
    PetscCall(SNESSetFunction(ts->snes, NULL, SNESTSFormFunction, ts));
    PetscCall(PetscObjectIncrementTabLevel((PetscObject)ts->snes, (PetscObject)ts, 1));
    if (ts->dm) PetscCall(SNESSetDM(ts->snes, ts->dm));
    if (ts->problem_type == TS_LINEAR) PetscCall(SNESSetType(ts->snes, SNESKSPONLY));
  }
  *snes = ts->snes;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetSNES - Set the `SNES` (nonlinear solver) to be used by the `TS` timestepping context

  Collective

  Input Parameters:
+ ts   - the `TS` context obtained from `TSCreate()`
- snes - the nonlinear solver context

  Level: developer

  Note:
  Most users should have the `TS` created by calling `TSGetSNES()`

.seealso: [](ch_ts), `TS`, `SNES`, `TSCreate()`, `TSSetUp()`, `TSSolve()`, `TSGetSNES()`
@*/
PetscErrorCode TSSetSNES(TS ts, SNES snes)
{
  PetscErrorCode (*func)(SNES, Vec, Mat, Mat, void *);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(snes, SNES_CLASSID, 2);
  PetscCall(PetscObjectReference((PetscObject)snes));
  PetscCall(SNESDestroy(&ts->snes));

  ts->snes = snes;

  PetscCall(SNESSetFunction(ts->snes, NULL, SNESTSFormFunction, ts));
  PetscCall(SNESGetJacobian(ts->snes, NULL, NULL, &func, NULL));
  if (func == SNESTSFormJacobian) PetscCall(SNESSetJacobian(ts->snes, NULL, NULL, SNESTSFormJacobian, ts));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetKSP - Returns the `KSP` (linear solver) associated with
  a `TS` (timestepper) context.

  Not Collective, but `ksp` is parallel if `ts` is parallel

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. ksp - the nonlinear solver context

  Level: beginner

  Notes:
  The user can then directly manipulate the `KSP` context to set various
  options, etc.  Likewise, the user can then extract and manipulate the
  `PC` context as well.

  `TSGetKSP()` does not work for integrators that do not use `KSP`;
  in this case `TSGetKSP()` returns `NULL` in `ksp`.

.seealso: [](ch_ts), `TS`, `SNES`, `KSP`, `TSCreate()`, `TSSetUp()`, `TSSolve()`, `TSGetSNES()`
@*/
PetscErrorCode TSGetKSP(TS ts, KSP *ksp)
{
  SNES snes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(ksp, 2);
  PetscCheck(((PetscObject)ts)->type_name, PETSC_COMM_SELF, PETSC_ERR_ARG_NULL, "KSP is not created yet. Call TSSetType() first");
  PetscCheck(ts->problem_type == TS_LINEAR, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Linear only; use TSGetSNES()");
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESGetKSP(snes, ksp));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* ----------- Routines to set solver parameters ---------- */

/*@
  TSSetMaxSteps - Sets the maximum number of steps to use.

  Logically Collective

  Input Parameters:
+ ts       - the `TS` context obtained from `TSCreate()`
- maxsteps - maximum number of steps to use

  Options Database Key:
. -ts_max_steps <maxsteps> - Sets maxsteps

  Level: intermediate

  Note:
  Use `PETSC_DETERMINE` to reset the maximum number of steps to the default from when the object's type was set

  The default maximum number of steps is 5,000

  Fortran Note:
  Use `PETSC_DETERMINE_INTEGER`

.seealso: [](ch_ts), `TS`, `TSGetMaxSteps()`, `TSSetMaxTime()`, `TSSetExactFinalTime()`
@*/
PetscErrorCode TSSetMaxSteps(TS ts, PetscInt maxsteps)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidLogicalCollectiveInt(ts, maxsteps, 2);
  if (maxsteps == PETSC_DETERMINE) {
    ts->max_steps = ts->default_max_steps;
  } else {
    PetscCheck(maxsteps >= 0, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_OUTOFRANGE, "Maximum number of steps must be non-negative");
    ts->max_steps = maxsteps;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetMaxSteps - Gets the maximum number of steps to use.

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. maxsteps - maximum number of steps to use

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSSetMaxSteps()`, `TSGetMaxTime()`, `TSSetMaxTime()`
@*/
PetscErrorCode TSGetMaxSteps(TS ts, PetscInt *maxsteps)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(maxsteps, 2);
  *maxsteps = ts->max_steps;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetRunSteps - Sets the maximum number of steps to take in each call to `TSSolve()`.

  If the step count when `TSSolve()` is `start_step`, this will stop the simulation once `current_step - start_step >= run_steps`.
  Comparatively, `TSSetMaxSteps()` will stop if `current_step >= max_steps`.
  The simulation will stop when either condition is reached.

  Logically Collective

  Input Parameters:
+ ts       - the `TS` context obtained from `TSCreate()`
- runsteps - maximum number of steps to take in each call to `TSSolve()`;

  Options Database Key:
. -ts_run_steps <runsteps> - Sets runsteps

  Level: intermediate

  Note:
  The default is `PETSC_UNLIMITED`

.seealso: [](ch_ts), `TS`, `TSGetRunSteps()`, `TSSetMaxTime()`, `TSSetExactFinalTime()`, `TSSetMaxSteps()`
@*/
PetscErrorCode TSSetRunSteps(TS ts, PetscInt runsteps)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidLogicalCollectiveInt(ts, runsteps, 2);
  if (runsteps == PETSC_DETERMINE) {
    ts->run_steps = PETSC_UNLIMITED;
  } else {
    PetscCheck(runsteps >= 0, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_OUTOFRANGE, "Max number of steps to take in each call to TSSolve must be non-negative");
    ts->run_steps = runsteps;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetRunSteps - Gets the maximum number of steps to take in each call to `TSSolve()`.

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. runsteps - maximum number of steps to take in each call to `TSSolve`.

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSSetRunSteps()`, `TSGetMaxTime()`, `TSSetMaxTime()`, `TSGetMaxSteps()`
@*/
PetscErrorCode TSGetRunSteps(TS ts, PetscInt *runsteps)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(runsteps, 2);
  *runsteps = ts->run_steps;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetMaxTime - Sets the maximum (or final) time for timestepping.

  Logically Collective

  Input Parameters:
+ ts      - the `TS` context obtained from `TSCreate()`
- maxtime - final time to step to

  Options Database Key:
. -ts_max_time <maxtime> - Sets maxtime

  Level: intermediate

  Notes:
  Use `PETSC_DETERMINE` to reset the maximum time to the default from when the object's type was set

  The default maximum time is 5.0

  Fortran Note:
  Use `PETSC_DETERMINE_REAL`

.seealso: [](ch_ts), `TS`, `TSGetMaxTime()`, `TSSetMaxSteps()`, `TSSetExactFinalTime()`
@*/
PetscErrorCode TSSetMaxTime(TS ts, PetscReal maxtime)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidLogicalCollectiveReal(ts, maxtime, 2);
  if (maxtime == PETSC_DETERMINE) {
    ts->max_time = ts->default_max_time;
  } else {
    ts->max_time = maxtime;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetMaxTime - Gets the maximum (or final) time for timestepping.

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. maxtime - final time to step to

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSSetMaxTime()`, `TSGetMaxSteps()`, `TSSetMaxSteps()`
@*/
PetscErrorCode TSGetMaxTime(TS ts, PetscReal *maxtime)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(maxtime, 2);
  *maxtime = ts->max_time;
  PetscFunctionReturn(PETSC_SUCCESS);
}

// PetscClangLinter pragma disable: -fdoc-*
/*@
  TSSetInitialTimeStep - Deprecated, use `TSSetTime()` and `TSSetTimeStep()`.

  Level: deprecated

@*/
PetscErrorCode TSSetInitialTimeStep(TS ts, PetscReal initial_time, PetscReal time_step)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(TSSetTime(ts, initial_time));
  PetscCall(TSSetTimeStep(ts, time_step));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// PetscClangLinter pragma disable: -fdoc-*
/*@
  TSGetDuration - Deprecated, use `TSGetMaxSteps()` and `TSGetMaxTime()`.

  Level: deprecated

@*/
PetscErrorCode TSGetDuration(TS ts, PetscInt *maxsteps, PetscReal *maxtime)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (maxsteps) {
    PetscAssertPointer(maxsteps, 2);
    *maxsteps = ts->max_steps;
  }
  if (maxtime) {
    PetscAssertPointer(maxtime, 3);
    *maxtime = ts->max_time;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// PetscClangLinter pragma disable: -fdoc-*
/*@
  TSSetDuration - Deprecated, use `TSSetMaxSteps()` and `TSSetMaxTime()`.

  Level: deprecated

@*/
PetscErrorCode TSSetDuration(TS ts, PetscInt maxsteps, PetscReal maxtime)
{
  PetscFunctionBegin;
  if (maxsteps != PETSC_CURRENT) PetscCall(TSSetMaxSteps(ts, maxsteps));
  if (maxtime != (PetscReal)PETSC_CURRENT) PetscCall(TSSetMaxTime(ts, maxtime));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// PetscClangLinter pragma disable: -fdoc-*
/*@
  TSGetTimeStepNumber - Deprecated, use `TSGetStepNumber()`.

  Level: deprecated

@*/
PetscErrorCode TSGetTimeStepNumber(TS ts, PetscInt *steps)
{
  return TSGetStepNumber(ts, steps);
}

// PetscClangLinter pragma disable: -fdoc-*
/*@
  TSGetTotalSteps - Deprecated, use `TSGetStepNumber()`.

  Level: deprecated

@*/
PetscErrorCode TSGetTotalSteps(TS ts, PetscInt *steps)
{
  return TSGetStepNumber(ts, steps);
}

/*@
  TSSetSolution - Sets the initial solution vector
  for use by the `TS` routines.

  Logically Collective

  Input Parameters:
+ ts - the `TS` context obtained from `TSCreate()`
- u  - the solution vector

  Level: beginner

.seealso: [](ch_ts), `TS`, `TSSetSolutionFunction()`, `TSGetSolution()`, `TSCreate()`
@*/
PetscErrorCode TSSetSolution(TS ts, Vec u)
{
  DM dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(u, VEC_CLASSID, 2);
  PetscCall(PetscObjectReference((PetscObject)u));
  PetscCall(VecDestroy(&ts->vec_sol));
  ts->vec_sol = u;

  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMShellSetGlobalVector(dm, u));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetPreStep - Sets the general-purpose function
  called once at the beginning of each time step.

  Logically Collective

  Input Parameters:
+ ts   - The `TS` context obtained from `TSCreate()`
- func - The function

  Calling sequence of `func`:
. ts - the `TS` context

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSSetPreStage()`, `TSSetPostStage()`, `TSSetPostStep()`, `TSStep()`, `TSRestartStep()`
@*/
PetscErrorCode TSSetPreStep(TS ts, PetscErrorCode (*func)(TS ts))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->prestep = func;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSPreStep - Runs the user-defined pre-step function provided with `TSSetPreStep()`

  Collective

  Input Parameter:
. ts - The `TS` context obtained from `TSCreate()`

  Level: developer

  Note:
  `TSPreStep()` is typically used within time stepping implementations,
  so most users would not generally call this routine themselves.

.seealso: [](ch_ts), `TS`, `TSSetPreStep()`, `TSPreStage()`, `TSPostStage()`, `TSPostStep()`
@*/
PetscErrorCode TSPreStep(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->prestep) {
    Vec              U;
    PetscObjectId    idprev;
    PetscBool        sameObject;
    PetscObjectState sprev, spost;

    PetscCall(TSGetSolution(ts, &U));
    PetscCall(PetscObjectGetId((PetscObject)U, &idprev));
    PetscCall(PetscObjectStateGet((PetscObject)U, &sprev));
    PetscCallBack("TS callback preset", (*ts->prestep)(ts));
    PetscCall(TSGetSolution(ts, &U));
    PetscCall(PetscObjectCompareId((PetscObject)U, idprev, &sameObject));
    PetscCall(PetscObjectStateGet((PetscObject)U, &spost));
    if (!sameObject || sprev != spost) PetscCall(TSRestartStep(ts));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetPreStage - Sets the general-purpose function
  called once at the beginning of each stage.

  Logically Collective

  Input Parameters:
+ ts   - The `TS` context obtained from `TSCreate()`
- func - The function

  Calling sequence of `func`:
+ ts        - the `TS` context
- stagetime - the stage time

  Level: intermediate

  Note:
  There may be several stages per time step. If the solve for a given stage fails, the step may be rejected and retried.
  The time step number being computed can be queried using `TSGetStepNumber()` and the total size of the step being
  attempted can be obtained using `TSGetTimeStep()`. The time at the start of the step is available via `TSGetTime()`.

.seealso: [](ch_ts), `TS`, `TSSetPostStage()`, `TSSetPreStep()`, `TSSetPostStep()`, `TSGetApplicationContext()`
@*/
PetscErrorCode TSSetPreStage(TS ts, PetscErrorCode (*func)(TS ts, PetscReal stagetime))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->prestage = func;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetPostStage - Sets the general-purpose function
  called once at the end of each stage.

  Logically Collective

  Input Parameters:
+ ts   - The `TS` context obtained from `TSCreate()`
- func - The function

  Calling sequence of `func`:
+ ts         - the `TS` context
. stagetime  - the stage time
. stageindex - the stage index
- Y          - Array of vectors (of size = total number of stages) with the stage solutions

  Level: intermediate

  Note:
  There may be several stages per time step. If the solve for a given stage fails, the step may be rejected and retried.
  The time step number being computed can be queried using `TSGetStepNumber()` and the total size of the step being
  attempted can be obtained using `TSGetTimeStep()`. The time at the start of the step is available via `TSGetTime()`.

.seealso: [](ch_ts), `TS`, `TSSetPreStage()`, `TSSetPreStep()`, `TSSetPostStep()`, `TSGetApplicationContext()`
@*/
PetscErrorCode TSSetPostStage(TS ts, PetscErrorCode (*func)(TS ts, PetscReal stagetime, PetscInt stageindex, Vec *Y))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->poststage = func;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetPostEvaluate - Sets the general-purpose function
  called at the end of each step evaluation.

  Logically Collective

  Input Parameters:
+ ts   - The `TS` context obtained from `TSCreate()`
- func - The function

  Calling sequence of `func`:
. ts - the `TS` context

  Level: intermediate

  Note:
  The function set by `TSSetPostEvaluate()` is called after the solution is evaluated, or after the step rollback.
  Inside the `func` callback, the solution vector can be obtained with `TSGetSolution()`, and modified, if need be.
  The time step can be obtained with `TSGetTimeStep()`, and the time at the start of the step - via `TSGetTime()`.
  The potential changes to the solution vector introduced by event handling (`postevent()`) are not relevant for `TSSetPostEvaluate()`,
  but are relevant for `TSSetPostStep()`, according to the function call scheme in `TSSolve()`, as shown below
.vb
  ...
  Step()
  PostEvaluate()
  EventHandling()
  step_rollback ? PostEvaluate() : PostStep()
  ...
.ve
  where EventHandling() may result in one of the following three outcomes
.vb
  (1) | successful step | solution intact
  (2) | successful step | solution modified by `postevent()`
  (3) | step_rollback   | solution rolled back
.ve

.seealso: [](ch_ts), `TS`, `TSSetPreStage()`, `TSSetPreStep()`, `TSSetPostStep()`, `TSGetApplicationContext()`
@*/
PetscErrorCode TSSetPostEvaluate(TS ts, PetscErrorCode (*func)(TS ts))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->postevaluate = func;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSPreStage - Runs the user-defined pre-stage function set using `TSSetPreStage()`

  Collective

  Input Parameters:
+ ts        - The `TS` context obtained from `TSCreate()`
- stagetime - The absolute time of the current stage

  Level: developer

  Note:
  `TSPreStage()` is typically used within time stepping implementations,
  most users would not generally call this routine themselves.

.seealso: [](ch_ts), `TS`, `TSPostStage()`, `TSSetPreStep()`, `TSPreStep()`, `TSPostStep()`
@*/
PetscErrorCode TSPreStage(TS ts, PetscReal stagetime)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->prestage) PetscCallBack("TS callback prestage", (*ts->prestage)(ts, stagetime));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSPostStage - Runs the user-defined post-stage function set using `TSSetPostStage()`

  Collective

  Input Parameters:
+ ts         - The `TS` context obtained from `TSCreate()`
. stagetime  - The absolute time of the current stage
. stageindex - Stage number
- Y          - Array of vectors (of size = total number of stages) with the stage solutions

  Level: developer

  Note:
  `TSPostStage()` is typically used within time stepping implementations,
  most users would not generally call this routine themselves.

.seealso: [](ch_ts), `TS`, `TSPreStage()`, `TSSetPreStep()`, `TSPreStep()`, `TSPostStep()`
@*/
PetscErrorCode TSPostStage(TS ts, PetscReal stagetime, PetscInt stageindex, Vec *Y)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->poststage) PetscCallBack("TS callback poststage", (*ts->poststage)(ts, stagetime, stageindex, Y));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSPostEvaluate - Runs the user-defined post-evaluate function set using `TSSetPostEvaluate()`

  Collective

  Input Parameter:
. ts - The `TS` context obtained from `TSCreate()`

  Level: developer

  Note:
  `TSPostEvaluate()` is typically used within time stepping implementations,
  most users would not generally call this routine themselves.

.seealso: [](ch_ts), `TS`, `TSSetPostEvaluate()`, `TSSetPreStep()`, `TSPreStep()`, `TSPostStep()`
@*/
PetscErrorCode TSPostEvaluate(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->postevaluate) {
    Vec              U;
    PetscObjectState sprev, spost;

    PetscCall(TSGetSolution(ts, &U));
    PetscCall(PetscObjectStateGet((PetscObject)U, &sprev));
    PetscCallBack("TS callback postevaluate", (*ts->postevaluate)(ts));
    PetscCall(PetscObjectStateGet((PetscObject)U, &spost));
    if (sprev != spost) PetscCall(TSRestartStep(ts));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetPostStep - Sets the general-purpose function
  called once at the end of each successful time step.

  Logically Collective

  Input Parameters:
+ ts   - The `TS` context obtained from `TSCreate()`
- func - The function

  Calling sequence of `func`:
. ts - the `TS` context

  Level: intermediate

  Note:
  The function set by `TSSetPostStep()` is called after each successful step. If the event handler locates an event at the
  given step, and `postevent()` modifies the solution vector, the solution vector obtained by `TSGetSolution()` inside `func` will
  contain the changes. To get the solution without these changes, use `TSSetPostEvaluate()` to set the appropriate callback.
  The scheme of the relevant function calls in `TSSolve()` is shown below
.vb
  ...
  Step()
  PostEvaluate()
  EventHandling()
  step_rollback ? PostEvaluate() : PostStep()
  ...
.ve
  where EventHandling() may result in one of the following three outcomes
.vb
  (1) | successful step | solution intact
  (2) | successful step | solution modified by `postevent()`
  (3) | step_rollback   | solution rolled back
.ve

.seealso: [](ch_ts), `TS`, `TSSetPreStep()`, `TSSetPreStage()`, `TSSetPostEvaluate()`, `TSGetTimeStep()`, `TSGetStepNumber()`, `TSGetTime()`, `TSRestartStep()`
@*/
PetscErrorCode TSSetPostStep(TS ts, PetscErrorCode (*func)(TS ts))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->poststep = func;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSPostStep - Runs the user-defined post-step function that was set with `TSSetPostStep()`

  Collective

  Input Parameter:
. ts - The `TS` context obtained from `TSCreate()`

  Note:
  `TSPostStep()` is typically used within time stepping implementations,
  so most users would not generally call this routine themselves.

  Level: developer

.seealso: [](ch_ts), `TS`, `TSSetPreStep()`, `TSSetPreStage()`, `TSSetPostEvaluate()`, `TSGetTimeStep()`, `TSGetStepNumber()`, `TSGetTime()`, `TSSetPostStep()`
@*/
PetscErrorCode TSPostStep(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->poststep) {
    Vec              U;
    PetscObjectId    idprev;
    PetscBool        sameObject;
    PetscObjectState sprev, spost;

    PetscCall(TSGetSolution(ts, &U));
    PetscCall(PetscObjectGetId((PetscObject)U, &idprev));
    PetscCall(PetscObjectStateGet((PetscObject)U, &sprev));
    PetscCallBack("TS callback poststep", (*ts->poststep)(ts));
    PetscCall(TSGetSolution(ts, &U));
    PetscCall(PetscObjectCompareId((PetscObject)U, idprev, &sameObject));
    PetscCall(PetscObjectStateGet((PetscObject)U, &spost));
    if (!sameObject || sprev != spost) PetscCall(TSRestartStep(ts));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSInterpolate - Interpolate the solution computed during the previous step to an arbitrary location in the interval

  Collective

  Input Parameters:
+ ts - time stepping context
- t  - time to interpolate to

  Output Parameter:
. U - state at given time

  Level: intermediate

  Developer Notes:
  `TSInterpolate()` and the storing of previous steps/stages should be generalized to support delay differential equations and continuous adjoints.

.seealso: [](ch_ts), `TS`, `TSSetExactFinalTime()`, `TSSolve()`
@*/
PetscErrorCode TSInterpolate(TS ts, PetscReal t, Vec U)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscCheck(t >= ts->ptime_prev && t <= ts->ptime, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_OUTOFRANGE, "Requested time %g not in last time steps [%g,%g]", (double)t, (double)ts->ptime_prev, (double)ts->ptime);
  PetscUseTypeMethod(ts, interpolate, t, U);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSStep - Steps one time step

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Level: developer

  Notes:
  The public interface for the ODE/DAE solvers is `TSSolve()`, you should almost for sure be using that routine and not this routine.

  The hook set using `TSSetPreStep()` is called before each attempt to take the step. In general, the time step size may
  be changed due to adaptive error controller or solve failures. Note that steps may contain multiple stages.

  This may over-step the final time provided in `TSSetMaxTime()` depending on the time-step used. `TSSolve()` interpolates to exactly the
  time provided in `TSSetMaxTime()`. One can use `TSInterpolate()` to determine an interpolated solution within the final timestep.

.seealso: [](ch_ts), `TS`, `TSCreate()`, `TSSetUp()`, `TSDestroy()`, `TSSolve()`, `TSSetPreStep()`, `TSSetPreStage()`, `TSSetPostStage()`, `TSInterpolate()`
@*/
PetscErrorCode TSStep(TS ts)
{
  static PetscBool cite = PETSC_FALSE;
  PetscReal        ptime;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(PetscCitationsRegister("@article{tspaper,\n"
                                   "  title         = {{PETSc/TS}: A Modern Scalable {DAE/ODE} Solver Library},\n"
                                   "  author        = {Abhyankar, Shrirang and Brown, Jed and Constantinescu, Emil and Ghosh, Debojyoti and Smith, Barry F. and Zhang, Hong},\n"
                                   "  journal       = {arXiv e-preprints},\n"
                                   "  eprint        = {1806.01437},\n"
                                   "  archivePrefix = {arXiv},\n"
                                   "  year          = {2018}\n}\n",
                                   &cite));
  PetscCall(TSSetUp(ts));
  PetscCall(TSTrajectorySetUp(ts->trajectory, ts));
  if (ts->eval_times)
    ts->eval_times->worktol = 0; /* In each step of TSSolve() 'eval_times->worktol' will be meaningfully defined (later) only once:
                                                   in TSAdaptChoose() or TSEvent_dt_cap(), and then reused till the end of the step */

  PetscCheck(ts->max_time < PETSC_MAX_REAL || ts->run_steps != PETSC_INT_MAX || ts->max_steps != PETSC_INT_MAX, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONGSTATE, "You must call TSSetMaxTime(), TSSetMaxSteps(), or TSSetRunSteps() or use -ts_max_time <time>, -ts_max_steps <steps>, -ts_run_steps <steps>");
  PetscCheck(ts->exact_final_time != TS_EXACTFINALTIME_UNSPECIFIED, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONGSTATE, "You must call TSSetExactFinalTime() or use -ts_exact_final_time <stepover,interpolate,matchstep> before calling TSStep()");
  PetscCheck(ts->exact_final_time != TS_EXACTFINALTIME_MATCHSTEP || ts->adapt, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "Since TS is not adaptive you cannot use TS_EXACTFINALTIME_MATCHSTEP, suggest TS_EXACTFINALTIME_INTERPOLATE");

  if (!ts->vec_sol0) PetscCall(VecDuplicate(ts->vec_sol, &ts->vec_sol0));
  PetscCall(VecCopy(ts->vec_sol, ts->vec_sol0));
  ts->time_step0 = ts->time_step;

  if (!ts->steps) ts->ptime_prev = ts->ptime;
  ptime = ts->ptime;

  ts->ptime_prev_rollback = ts->ptime_prev;
  ts->reason              = TS_CONVERGED_ITERATING;

  PetscCall(PetscLogEventBegin(TS_Step, ts, 0, 0, 0));
  PetscUseTypeMethod(ts, step);
  PetscCall(PetscLogEventEnd(TS_Step, ts, 0, 0, 0));

  if (ts->reason >= 0) {
    ts->ptime_prev = ptime;
    ts->steps++;
    ts->steprollback = PETSC_FALSE;
    ts->steprestart  = PETSC_FALSE;
    ts->stepresize   = PETSC_FALSE;
  }

  if (ts->reason < 0 && ts->errorifstepfailed) {
    PetscCall(TSMonitorCancel(ts));
    PetscCheck(ts->reason != TS_DIVERGED_NONLINEAR_SOLVE, PetscObjectComm((PetscObject)ts), PETSC_ERR_NOT_CONVERGED, "TSStep has failed due to %s, increase -ts_max_snes_failures or use unlimited to attempt recovery", TSConvergedReasons[ts->reason]);
    SETERRQ(PetscObjectComm((PetscObject)ts), PETSC_ERR_NOT_CONVERGED, "TSStep has failed due to %s", TSConvergedReasons[ts->reason]);
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSEvaluateWLTE - Evaluate the weighted local truncation error norm
  at the end of a time step with a given order of accuracy.

  Collective

  Input Parameters:
+ ts        - time stepping context
- wnormtype - norm type, either `NORM_2` or `NORM_INFINITY`

  Input/Output Parameter:
. order - optional, desired order for the error evaluation or `PETSC_DECIDE`;
           on output, the actual order of the error evaluation

  Output Parameter:
. wlte - the weighted local truncation error norm

  Level: advanced

  Note:
  If the timestepper cannot evaluate the error in a particular step
  (eg. in the first step or restart steps after event handling),
  this routine returns wlte=-1.0 .

.seealso: [](ch_ts), `TS`, `TSStep()`, `TSAdapt`, `TSErrorWeightedNorm()`
@*/
PetscErrorCode TSEvaluateWLTE(TS ts, NormType wnormtype, PetscInt *order, PetscReal *wlte)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidType(ts, 1);
  PetscValidLogicalCollectiveEnum(ts, wnormtype, 2);
  if (order) PetscAssertPointer(order, 3);
  if (order) PetscValidLogicalCollectiveInt(ts, *order, 3);
  PetscAssertPointer(wlte, 4);
  PetscCheck(wnormtype == NORM_2 || wnormtype == NORM_INFINITY, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "No support for norm type %s", NormTypes[wnormtype]);
  PetscUseTypeMethod(ts, evaluatewlte, wnormtype, order, wlte);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSEvaluateStep - Evaluate the solution at the end of a time step with a given order of accuracy.

  Collective

  Input Parameters:
+ ts    - time stepping context
. order - desired order of accuracy
- done  - whether the step was evaluated at this order (pass `NULL` to generate an error if not available)

  Output Parameter:
. U - state at the end of the current step

  Level: advanced

  Notes:
  This function cannot be called until all stages have been evaluated.

  It is normally called by adaptive controllers before a step has been accepted and may also be called by the user after `TSStep()` has returned.

.seealso: [](ch_ts), `TS`, `TSStep()`, `TSAdapt`
@*/
PetscErrorCode TSEvaluateStep(TS ts, PetscInt order, Vec U, PetscBool *done)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidType(ts, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 3);
  PetscUseTypeMethod(ts, evaluatestep, order, U, done);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetComputeInitialCondition - Get the function used to automatically compute an initial condition for the timestepping.

  Not collective

  Input Parameter:
. ts - time stepping context

  Output Parameter:
. initCondition - The function which computes an initial condition

  Calling sequence of `initCondition`:
+ ts - The timestepping context
- u  - The input vector in which the initial condition is stored

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSSetComputeInitialCondition()`, `TSComputeInitialCondition()`
@*/
PetscErrorCode TSGetComputeInitialCondition(TS ts, PetscErrorCode (**initCondition)(TS ts, Vec u))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(initCondition, 2);
  *initCondition = ts->ops->initcondition;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetComputeInitialCondition - Set the function used to automatically compute an initial condition for the timestepping.

  Logically collective

  Input Parameters:
+ ts            - time stepping context
- initCondition - The function which computes an initial condition

  Calling sequence of `initCondition`:
+ ts - The timestepping context
- e  - The input vector in which the initial condition is to be stored

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSGetComputeInitialCondition()`, `TSComputeInitialCondition()`
@*/
PetscErrorCode TSSetComputeInitialCondition(TS ts, PetscErrorCode (*initCondition)(TS ts, Vec e))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidFunction(initCondition, 2);
  ts->ops->initcondition = initCondition;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeInitialCondition - Compute an initial condition for the timestepping using the function previously set with `TSSetComputeInitialCondition()`

  Collective

  Input Parameters:
+ ts - time stepping context
- u  - The `Vec` to store the condition in which will be used in `TSSolve()`

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSGetComputeInitialCondition()`, `TSSetComputeInitialCondition()`, `TSSolve()`
@*/
PetscErrorCode TSComputeInitialCondition(TS ts, Vec u)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(u, VEC_CLASSID, 2);
  PetscTryTypeMethod(ts, initcondition, u);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetComputeExactError - Get the function used to automatically compute the exact error for the timestepping.

  Not collective

  Input Parameter:
. ts - time stepping context

  Output Parameter:
. exactError - The function which computes the solution error

  Calling sequence of `exactError`:
+ ts - The timestepping context
. u  - The approximate solution vector
- e  - The vector in which the error is stored

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSComputeExactError()`
@*/
PetscErrorCode TSGetComputeExactError(TS ts, PetscErrorCode (**exactError)(TS ts, Vec u, Vec e))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(exactError, 2);
  *exactError = ts->ops->exacterror;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetComputeExactError - Set the function used to automatically compute the exact error for the timestepping.

  Logically collective

  Input Parameters:
+ ts         - time stepping context
- exactError - The function which computes the solution error

  Calling sequence of `exactError`:
+ ts - The timestepping context
. u  - The approximate solution vector
- e  - The  vector in which the error is stored

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSGetComputeExactError()`, `TSComputeExactError()`
@*/
PetscErrorCode TSSetComputeExactError(TS ts, PetscErrorCode (*exactError)(TS ts, Vec u, Vec e))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidFunction(exactError, 2);
  ts->ops->exacterror = exactError;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeExactError - Compute the solution error for the timestepping using the function previously set with `TSSetComputeExactError()`

  Collective

  Input Parameters:
+ ts - time stepping context
. u  - The approximate solution
- e  - The `Vec` used to store the error

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSGetComputeInitialCondition()`, `TSSetComputeInitialCondition()`, `TSSolve()`
@*/
PetscErrorCode TSComputeExactError(TS ts, Vec u, Vec e)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(u, VEC_CLASSID, 2);
  PetscValidHeaderSpecific(e, VEC_CLASSID, 3);
  PetscTryTypeMethod(ts, exacterror, u, e);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetResize - Sets the resize callbacks.

  Logically Collective

  Input Parameters:
+ ts       - The `TS` context obtained from `TSCreate()`
. rollback - Whether a resize will restart the step
. setup    - The setup function
. transfer - The transfer function
- ctx      - [optional] The user-defined context

  Calling sequence of `setup`:
+ ts     - the `TS` context
. step   - the current step
. time   - the current time
. state  - the current vector of state
. resize - (output parameter) `PETSC_TRUE` if need resizing, `PETSC_FALSE` otherwise
- ctx    - user defined context

  Calling sequence of `transfer`:
+ ts      - the `TS` context
. nv      - the number of vectors to be transferred
. vecsin  - array of vectors to be transferred
. vecsout - array of transferred vectors
- ctx     - user defined context

  Notes:
  The `setup` function is called inside `TSSolve()` after `TSEventHandler()` or after `TSPostStep()`
  depending on the `rollback` value: if `rollback` is true, then these callbacks behave as error indicators
  and will flag the need to remesh and restart the current step. Otherwise, they will simply flag the solver
  that the size of the discrete problem has changed.
  In both cases, the solver will collect the needed vectors that will be
  transferred from the old to the new sizes using the `transfer` callback. These vectors will include the
  current solution vector, and other vectors needed by the specific solver used.
  For example, `TSBDF` uses previous solutions vectors to solve for the next time step.
  Other application specific objects associated with the solver, i.e. Jacobian matrices and `DM`,
  will be automatically reset if the sizes are changed and they must be specified again by the user
  inside the `transfer` function.
  The input and output arrays passed to `transfer` are allocated by PETSc.
  Vectors in `vecsout` must be created by the user.
  Ownership of vectors in `vecsout` is transferred to PETSc.

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSSetDM()`, `TSSetIJacobian()`, `TSSetRHSJacobian()`
@*/
PetscErrorCode TSSetResize(TS ts, PetscBool rollback, PetscErrorCode (*setup)(TS ts, PetscInt step, PetscReal time, Vec state, PetscBool *resize, void *ctx), PetscErrorCode (*transfer)(TS ts, PetscInt nv, Vec vecsin[], Vec vecsout[], void *ctx), void *ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->resizerollback = rollback;
  ts->resizesetup    = setup;
  ts->resizetransfer = transfer;
  ts->resizectx      = ctx;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*
  TSResizeRegisterOrRetrieve - Register or import vectors transferred with `TSResize()`.

  Collective

  Input Parameters:
+ ts   - The `TS` context obtained from `TSCreate()`
- flg - If `PETSC_TRUE` each TS implementation (e.g. `TSBDF`) will register vectors to be transferred, if `PETSC_FALSE` vectors will be imported from transferred vectors.

  Level: developer

  Note:
  `TSResizeRegisterOrRetrieve()` is declared PETSC_INTERN since it is
   used within time stepping implementations,
   so most users would not generally call this routine themselves.

.seealso: [](ch_ts), `TS`, `TSSetResize()`
@*/
static PetscErrorCode TSResizeRegisterOrRetrieve(TS ts, PetscBool flg)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscTryTypeMethod(ts, resizeregister, flg);
  /* PetscTryTypeMethod(adapt, resizeregister, flg); */
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode TSResizeReset(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(PetscObjectListDestroy(&ts->resizetransferobjs));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode TSResizeTransferVecs(TS ts, PetscInt cnt, Vec vecsin[], Vec vecsout[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidLogicalCollectiveInt(ts, cnt, 2);
  for (PetscInt i = 0; i < cnt; i++) PetscCall(VecLockReadPush(vecsin[i]));
  if (ts->resizetransfer) {
    PetscCall(PetscInfo(ts, "Transferring %" PetscInt_FMT " vectors\n", cnt));
    PetscCallBack("TS callback resize transfer", (*ts->resizetransfer)(ts, cnt, vecsin, vecsout, ts->resizectx));
  }
  for (PetscInt i = 0; i < cnt; i++) PetscCall(VecLockReadPop(vecsin[i]));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSResizeRegisterVec - Register a vector to be transferred with `TSResize()`.

  Collective

  Input Parameters:
+ ts   - The `TS` context obtained from `TSCreate()`
. name - A string identifying the vector
- vec  - The vector

  Level: developer

  Note:
  `TSResizeRegisterVec()` is typically used within time stepping implementations,
  so most users would not generally call this routine themselves.

.seealso: [](ch_ts), `TS`, `TSSetResize()`, `TSResize()`, `TSResizeRetrieveVec()`
@*/
PetscErrorCode TSResizeRegisterVec(TS ts, const char name[], Vec vec)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(name, 2);
  if (vec) PetscValidHeaderSpecific(vec, VEC_CLASSID, 3);
  PetscCall(PetscObjectListAdd(&ts->resizetransferobjs, name, (PetscObject)vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSResizeRetrieveVec - Retrieve a vector registered with `TSResizeRegisterVec()`.

  Collective

  Input Parameters:
+ ts   - The `TS` context obtained from `TSCreate()`
. name - A string identifying the vector
- vec  - The vector

  Level: developer

  Note:
  `TSResizeRetrieveVec()` is typically used within time stepping implementations,
  so most users would not generally call this routine themselves.

.seealso: [](ch_ts), `TS`, `TSSetResize()`, `TSResize()`, `TSResizeRegisterVec()`
@*/
PetscErrorCode TSResizeRetrieveVec(TS ts, const char name[], Vec *vec)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(name, 2);
  PetscAssertPointer(vec, 3);
  PetscCall(PetscObjectListFind(ts->resizetransferobjs, name, (PetscObject *)vec));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode TSResizeGetVecArray(TS ts, PetscInt *nv, const char **names[], Vec *vecs[])
{
  PetscInt        cnt;
  PetscObjectList tmp;
  Vec            *vecsin  = NULL;
  const char    **namesin = NULL;

  PetscFunctionBegin;
  for (tmp = ts->resizetransferobjs, cnt = 0; tmp; tmp = tmp->next)
    if (tmp->obj && tmp->obj->classid == VEC_CLASSID) cnt++;
  if (names) PetscCall(PetscMalloc1(cnt, &namesin));
  if (vecs) PetscCall(PetscMalloc1(cnt, &vecsin));
  for (tmp = ts->resizetransferobjs, cnt = 0; tmp; tmp = tmp->next) {
    if (tmp->obj && tmp->obj->classid == VEC_CLASSID) {
      if (vecs) vecsin[cnt] = (Vec)tmp->obj;
      if (names) namesin[cnt] = tmp->name;
      cnt++;
    }
  }
  if (nv) *nv = cnt;
  if (names) *names = namesin;
  if (vecs) *vecs = vecsin;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSResize - Runs the user-defined transfer functions provided with `TSSetResize()`

  Collective

  Input Parameter:
. ts - The `TS` context obtained from `TSCreate()`

  Level: developer

  Note:
  `TSResize()` is typically used within time stepping implementations,
  so most users would not generally call this routine themselves.

.seealso: [](ch_ts), `TS`, `TSSetResize()`
@*/
PetscErrorCode TSResize(TS ts)
{
  PetscInt     nv      = 0;
  const char **names   = NULL;
  Vec         *vecsin  = NULL;
  const char  *solname = "ts:vec_sol";

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (!ts->resizesetup) PetscFunctionReturn(PETSC_SUCCESS);
  if (ts->resizesetup) {
    PetscCall(VecLockReadPush(ts->vec_sol));
    PetscCallBack("TS callback resize setup", (*ts->resizesetup)(ts, ts->steps, ts->ptime, ts->vec_sol, &ts->stepresize, ts->resizectx));
    PetscCall(VecLockReadPop(ts->vec_sol));
    if (ts->stepresize) {
      if (ts->resizerollback) {
        PetscCall(TSRollBack(ts));
        ts->time_step = ts->time_step0;
      }
      PetscCall(TSResizeRegisterVec(ts, solname, ts->vec_sol));
      PetscCall(TSResizeRegisterOrRetrieve(ts, PETSC_TRUE)); /* specific impls register their own objects */
    }
  }

  PetscCall(TSResizeGetVecArray(ts, &nv, &names, &vecsin));
  if (nv) {
    Vec *vecsout, vecsol;

    /* Reset internal objects */
    PetscCall(TSReset(ts));

    /* Transfer needed vectors (users can call SetJacobian, SetDM, etc. here) */
    PetscCall(PetscCalloc1(nv, &vecsout));
    PetscCall(TSResizeTransferVecs(ts, nv, vecsin, vecsout));
    for (PetscInt i = 0; i < nv; i++) {
      const char *name;
      char       *oname;

      PetscCall(PetscObjectGetName((PetscObject)vecsin[i], &name));
      PetscCall(PetscStrallocpy(name, &oname));
      PetscCall(TSResizeRegisterVec(ts, names[i], vecsout[i]));
      if (vecsout[i]) PetscCall(PetscObjectSetName((PetscObject)vecsout[i], oname));
      PetscCall(PetscFree(oname));
      PetscCall(VecDestroy(&vecsout[i]));
    }
    PetscCall(PetscFree(vecsout));
    PetscCall(TSResizeRegisterOrRetrieve(ts, PETSC_FALSE)); /* specific impls import the transferred objects */

    PetscCall(TSResizeRetrieveVec(ts, solname, &vecsol));
    if (vecsol) PetscCall(TSSetSolution(ts, vecsol));
    PetscAssert(ts->vec_sol, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_NULL, "Missing TS solution");
  }

  PetscCall(PetscFree(names));
  PetscCall(PetscFree(vecsin));
  PetscCall(TSResizeReset(ts));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSolve - Steps the requested number of timesteps.

  Collective

  Input Parameters:
+ ts - the `TS` context obtained from `TSCreate()`
- u  - the solution vector  (can be null if `TSSetSolution()` was used and `TSSetExactFinalTime`(ts,`TS_EXACTFINALTIME_MATCHSTEP`) was not used,
       otherwise it must contain the initial conditions and will contain the solution at the final requested time

  Level: beginner

  Notes:
  The final time returned by this function may be different from the time of the internally
  held state accessible by `TSGetSolution()` and `TSGetTime()` because the method may have
  stepped over the final time.

.seealso: [](ch_ts), `TS`, `TSCreate()`, `TSSetSolution()`, `TSStep()`, `TSGetTime()`, `TSGetSolveTime()`
@*/
PetscErrorCode TSSolve(TS ts, Vec u)
{
  Vec solution;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (u) PetscValidHeaderSpecific(u, VEC_CLASSID, 2);

  PetscCall(TSSetExactFinalTimeDefault(ts));
  if (ts->exact_final_time == TS_EXACTFINALTIME_INTERPOLATE && u) { /* Need ts->vec_sol to be distinct so it is not overwritten when we interpolate at the end */
    if (!ts->vec_sol || u == ts->vec_sol) {
      PetscCall(VecDuplicate(u, &solution));
      PetscCall(TSSetSolution(ts, solution));
      PetscCall(VecDestroy(&solution)); /* grant ownership */
    }
    PetscCall(VecCopy(u, ts->vec_sol));
    PetscCheck(!ts->forward_solve, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "Sensitivity analysis does not support the mode TS_EXACTFINALTIME_INTERPOLATE");
  } else if (u) PetscCall(TSSetSolution(ts, u));
  PetscCall(TSSetUp(ts));
  PetscCall(TSTrajectorySetUp(ts->trajectory, ts));

  PetscCheck(ts->max_time < PETSC_MAX_REAL || ts->run_steps != PETSC_INT_MAX || ts->max_steps != PETSC_INT_MAX, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONGSTATE, "You must call TSSetMaxTime(), TSSetMaxSteps(), or TSSetRunSteps() or use -ts_max_time <time>, -ts_max_steps <steps>, -ts_run_steps <steps>");
  PetscCheck(ts->exact_final_time != TS_EXACTFINALTIME_UNSPECIFIED, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONGSTATE, "You must call TSSetExactFinalTime() or use -ts_exact_final_time <stepover,interpolate,matchstep> before calling TSSolve()");
  PetscCheck(ts->exact_final_time != TS_EXACTFINALTIME_MATCHSTEP || ts->adapt, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "Since TS is not adaptive you cannot use TS_EXACTFINALTIME_MATCHSTEP, suggest TS_EXACTFINALTIME_INTERPOLATE");
  PetscCheck(!(ts->eval_times && ts->exact_final_time != TS_EXACTFINALTIME_MATCHSTEP), PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "You must use TS_EXACTFINALTIME_MATCHSTEP when using time span or evaluation times");

  if (ts->eval_times) {
    for (PetscInt i = 0; i < ts->eval_times->num_time_points; i++) {
      PetscBool is_close = PetscIsCloseAtTol(ts->ptime, ts->eval_times->time_points[i], ts->eval_times->reltol * ts->time_step + ts->eval_times->abstol, 0);
      if (ts->ptime <= ts->eval_times->time_points[i] || is_close) {
        ts->eval_times->time_point_idx = i;
        if (is_close) { /* starting point in evaluation times */
          PetscCall(VecCopy(ts->vec_sol, ts->eval_times->sol_vecs[ts->eval_times->sol_ctr]));
          ts->eval_times->sol_times[ts->eval_times->sol_ctr] = ts->ptime;
          ts->eval_times->sol_ctr++;
          ts->eval_times->time_point_idx++;
        }
        break;
      }
    }
  }

  if (ts->forward_solve) PetscCall(TSForwardSetUp(ts));

  /* reset number of steps only when the step is not restarted. ARKIMEX
     restarts the step after an event. Resetting these counters in such case causes
     TSTrajectory to incorrectly save the output files
  */
  /* reset time step and iteration counters */
  if (!ts->steps) {
    ts->ksp_its           = 0;
    ts->snes_its          = 0;
    ts->num_snes_failures = 0;
    ts->reject            = 0;
    ts->steprestart       = PETSC_TRUE;
    ts->steprollback      = PETSC_FALSE;
    ts->stepresize        = PETSC_FALSE;
    ts->rhsjacobian.time  = PETSC_MIN_REAL;
  }

  /* make sure initial time step does not overshoot final time or the next point in evaluation times */
  if (ts->exact_final_time == TS_EXACTFINALTIME_MATCHSTEP) {
    PetscReal maxdt;
    PetscReal dt = ts->time_step;

    if (ts->eval_times) maxdt = ts->eval_times->time_points[ts->eval_times->time_point_idx] - ts->ptime;
    else maxdt = ts->max_time - ts->ptime;
    ts->time_step = dt >= maxdt ? maxdt : (PetscIsCloseAtTol(dt, maxdt, 10 * PETSC_MACHINE_EPSILON, 0) ? maxdt : dt);
  }
  ts->reason = TS_CONVERGED_ITERATING;

  {
    PetscViewer       viewer;
    PetscViewerFormat format;
    PetscBool         flg;
    static PetscBool  incall = PETSC_FALSE;

    if (!incall) {
      /* Estimate the convergence rate of the time discretization */
      PetscCall(PetscOptionsCreateViewer(PetscObjectComm((PetscObject)ts), ((PetscObject)ts)->options, ((PetscObject)ts)->prefix, "-ts_convergence_estimate", &viewer, &format, &flg));
      if (flg) {
        PetscConvEst conv;
        DM           dm;
        PetscReal   *alpha; /* Convergence rate of the solution error for each field in the L_2 norm */
        PetscInt     Nf;
        PetscBool    checkTemporal = PETSC_TRUE;

        incall = PETSC_TRUE;
        PetscCall(PetscOptionsGetBool(((PetscObject)ts)->options, ((PetscObject)ts)->prefix, "-ts_convergence_temporal", &checkTemporal, &flg));
        PetscCall(TSGetDM(ts, &dm));
        PetscCall(DMGetNumFields(dm, &Nf));
        PetscCall(PetscCalloc1(PetscMax(Nf, 1), &alpha));
        PetscCall(PetscConvEstCreate(PetscObjectComm((PetscObject)ts), &conv));
        PetscCall(PetscConvEstUseTS(conv, checkTemporal));
        PetscCall(PetscConvEstSetSolver(conv, (PetscObject)ts));
        PetscCall(PetscConvEstSetFromOptions(conv));
        PetscCall(PetscConvEstSetUp(conv));
        PetscCall(PetscConvEstGetConvRate(conv, alpha));
        PetscCall(PetscViewerPushFormat(viewer, format));
        PetscCall(PetscConvEstRateView(conv, alpha, viewer));
        PetscCall(PetscViewerPopFormat(viewer));
        PetscCall(PetscViewerDestroy(&viewer));
        PetscCall(PetscConvEstDestroy(&conv));
        PetscCall(PetscFree(alpha));
        incall = PETSC_FALSE;
      }
    }
  }

  PetscCall(TSViewFromOptions(ts, NULL, "-ts_view_pre"));

  if (ts->ops->solve) { /* This private interface is transitional and should be removed when all implementations are updated. */
    PetscUseTypeMethod(ts, solve);
    if (u) PetscCall(VecCopy(ts->vec_sol, u));
    ts->solvetime = ts->ptime;
    solution      = ts->vec_sol;
  } else { /* Step the requested number of timesteps. */
    if (ts->steps >= ts->max_steps) ts->reason = TS_CONVERGED_ITS;
    else if (ts->ptime >= ts->max_time) ts->reason = TS_CONVERGED_TIME;

    if (!ts->steps) {
      PetscCall(TSTrajectorySet(ts->trajectory, ts, ts->steps, ts->ptime, ts->vec_sol));
      PetscCall(TSEventInitialize(ts->event, ts, ts->ptime, ts->vec_sol));
    }

    ts->start_step = ts->steps; // records starting step
    while (!ts->reason) {
      PetscCall(TSMonitor(ts, ts->steps, ts->ptime, ts->vec_sol));
      if (!ts->steprollback || (ts->stepresize && ts->resizerollback)) PetscCall(TSPreStep(ts));
      PetscCall(TSStep(ts));
      if (ts->testjacobian) PetscCall(TSRHSJacobianTest(ts, NULL));
      if (ts->testjacobiantranspose) PetscCall(TSRHSJacobianTestTranspose(ts, NULL));
      if (ts->quadraturets && ts->costintegralfwd) { /* Must evaluate the cost integral before event is handled. The cost integral value can also be rolled back. */
        if (ts->reason >= 0) ts->steps--;            /* Revert the step number changed by TSStep() */
        PetscCall(TSForwardCostIntegral(ts));
        if (ts->reason >= 0) ts->steps++;
      }
      if (ts->forward_solve) {            /* compute forward sensitivities before event handling because postevent() may change RHS and jump conditions may have to be applied */
        if (ts->reason >= 0) ts->steps--; /* Revert the step number changed by TSStep() */
        PetscCall(TSForwardStep(ts));
        if (ts->reason >= 0) ts->steps++;
      }
      PetscCall(TSPostEvaluate(ts));
      PetscCall(TSEventHandler(ts)); /* The right-hand side may be changed due to event. Be careful with Any computation using the RHS information after this point. */
      if (ts->steprollback) PetscCall(TSPostEvaluate(ts));
      if (!ts->steprollback && ts->resizerollback) PetscCall(TSResize(ts));
      /* check convergence */
      if (!ts->reason) {
        if ((ts->steps - ts->start_step) >= ts->run_steps) ts->reason = TS_CONVERGED_ITS;
        else if (ts->steps >= ts->max_steps) ts->reason = TS_CONVERGED_ITS;
        else if (ts->ptime >= ts->max_time) ts->reason = TS_CONVERGED_TIME;
      }
      if (!ts->steprollback) {
        PetscCall(TSTrajectorySet(ts->trajectory, ts, ts->steps, ts->ptime, ts->vec_sol));
        PetscCall(TSPostStep(ts));
        if (!ts->resizerollback) PetscCall(TSResize(ts));

        if (ts->eval_times && ts->eval_times->time_point_idx < ts->eval_times->num_time_points && ts->reason >= 0) {
          PetscCheck(ts->eval_times->worktol > 0, PetscObjectComm((PetscObject)ts), PETSC_ERR_PLIB, "Unexpected state !(eval_times->worktol > 0) in TSSolve()");
          if (PetscIsCloseAtTol(ts->ptime, ts->eval_times->time_points[ts->eval_times->time_point_idx], ts->eval_times->worktol, 0)) {
            ts->eval_times->sol_times[ts->eval_times->sol_ctr] = ts->ptime;
            PetscCall(VecCopy(ts->vec_sol, ts->eval_times->sol_vecs[ts->eval_times->sol_ctr]));
            ts->eval_times->sol_ctr++;
            ts->eval_times->time_point_idx++;
          }
        }
      }
    }
    PetscCall(TSMonitor(ts, ts->steps, ts->ptime, ts->vec_sol));

    if (ts->exact_final_time == TS_EXACTFINALTIME_INTERPOLATE && ts->ptime > ts->max_time) {
      if (!u) u = ts->vec_sol;
      PetscCall(TSInterpolate(ts, ts->max_time, u));
      ts->solvetime = ts->max_time;
      solution      = u;
      PetscCall(TSMonitor(ts, -1, ts->solvetime, solution));
    } else {
      if (u) PetscCall(VecCopy(ts->vec_sol, u));
      ts->solvetime = ts->ptime;
      solution      = ts->vec_sol;
    }
  }

  PetscCall(TSViewFromOptions(ts, NULL, "-ts_view"));
  PetscCall(VecViewFromOptions(solution, (PetscObject)ts, "-ts_view_solution"));
  PetscCall(PetscObjectSAWsBlock((PetscObject)ts));
  if (ts->adjoint_solve) PetscCall(TSAdjointSolve(ts));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetTime - Gets the time of the most recently completed step.

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. t - the current time. This time may not corresponds to the final time set with `TSSetMaxTime()`, use `TSGetSolveTime()`.

  Level: beginner

  Note:
  When called during time step evaluation (e.g. during residual evaluation or via hooks set using `TSSetPreStep()`,
  `TSSetPreStage()`, `TSSetPostStage()`, or `TSSetPostStep()`), the time is the time at the start of the step being evaluated.

.seealso: [](ch_ts), `TS`, `TSGetSolveTime()`, `TSSetTime()`, `TSGetTimeStep()`, `TSGetStepNumber()`
@*/
PetscErrorCode TSGetTime(TS ts, PetscReal *t)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(t, 2);
  *t = ts->ptime;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetPrevTime - Gets the starting time of the previously completed step.

  Not Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. t - the previous time

  Level: beginner

.seealso: [](ch_ts), `TS`, `TSGetTime()`, `TSGetSolveTime()`, `TSGetTimeStep()`
@*/
PetscErrorCode TSGetPrevTime(TS ts, PetscReal *t)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(t, 2);
  *t = ts->ptime_prev;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetTime - Allows one to reset the time.

  Logically Collective

  Input Parameters:
+ ts - the `TS` context obtained from `TSCreate()`
- t  - the time

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSGetTime()`, `TSSetMaxSteps()`
@*/
PetscErrorCode TSSetTime(TS ts, PetscReal t)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidLogicalCollectiveReal(ts, t, 2);
  ts->ptime = t;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetOptionsPrefix - Sets the prefix used for searching for all
  TS options in the database.

  Logically Collective

  Input Parameters:
+ ts     - The `TS` context
- prefix - The prefix to prepend to all option names

  Level: advanced

  Note:
  A hyphen (-) must NOT be given at the beginning of the prefix name.
  The first character of all runtime options is AUTOMATICALLY the
  hyphen.

.seealso: [](ch_ts), `TS`, `TSSetFromOptions()`, `TSAppendOptionsPrefix()`
@*/
PetscErrorCode TSSetOptionsPrefix(TS ts, const char prefix[])
{
  SNES snes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(PetscObjectSetOptionsPrefix((PetscObject)ts, prefix));
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESSetOptionsPrefix(snes, prefix));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSAppendOptionsPrefix - Appends to the prefix used for searching for all
  TS options in the database.

  Logically Collective

  Input Parameters:
+ ts     - The `TS` context
- prefix - The prefix to prepend to all option names

  Level: advanced

  Note:
  A hyphen (-) must NOT be given at the beginning of the prefix name.
  The first character of all runtime options is AUTOMATICALLY the
  hyphen.

.seealso: [](ch_ts), `TS`, `TSGetOptionsPrefix()`, `TSSetOptionsPrefix()`, `TSSetFromOptions()`
@*/
PetscErrorCode TSAppendOptionsPrefix(TS ts, const char prefix[])
{
  SNES snes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(PetscObjectAppendOptionsPrefix((PetscObject)ts, prefix));
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESAppendOptionsPrefix(snes, prefix));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetOptionsPrefix - Sets the prefix used for searching for all
  `TS` options in the database.

  Not Collective

  Input Parameter:
. ts - The `TS` context

  Output Parameter:
. prefix - A pointer to the prefix string used

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSAppendOptionsPrefix()`, `TSSetFromOptions()`
@*/
PetscErrorCode TSGetOptionsPrefix(TS ts, const char *prefix[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(prefix, 2);
  PetscCall(PetscObjectGetOptionsPrefix((PetscObject)ts, prefix));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetRHSJacobian - Returns the Jacobian J at the present timestep.

  Not Collective, but parallel objects are returned if ts is parallel

  Input Parameter:
. ts - The `TS` context obtained from `TSCreate()`

  Output Parameters:
+ Amat - The (approximate) Jacobian J of G, where U_t = G(U,t)  (or `NULL`)
. Pmat - The matrix from which the preconditioner is constructed, usually the same as `Amat`  (or `NULL`)
. func - Function to compute the Jacobian of the RHS  (or `NULL`)
- ctx  - User-defined context for Jacobian evaluation routine  (or `NULL`)

  Level: intermediate

  Note:
  You can pass in `NULL` for any return argument you do not need.

.seealso: [](ch_ts), `TS`, `TSGetTimeStep()`, `TSGetMatrices()`, `TSGetTime()`, `TSGetStepNumber()`

@*/
PetscErrorCode TSGetRHSJacobian(TS ts, Mat *Amat, Mat *Pmat, TSRHSJacobianFn **func, void **ctx)
{
  DM dm;

  PetscFunctionBegin;
  if (Amat || Pmat) {
    SNES snes;
    PetscCall(TSGetSNES(ts, &snes));
    PetscCall(SNESSetUpMatrices(snes));
    PetscCall(SNESGetJacobian(snes, Amat, Pmat, NULL, NULL));
  }
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetRHSJacobian(dm, func, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetIJacobian - Returns the implicit Jacobian at the present timestep.

  Not Collective, but parallel objects are returned if ts is parallel

  Input Parameter:
. ts - The `TS` context obtained from `TSCreate()`

  Output Parameters:
+ Amat - The (approximate) Jacobian of F(t,U,U_t)
. Pmat - The matrix from which the preconditioner is constructed, often the same as `Amat`
. f    - The function to compute the matrices
- ctx  - User-defined context for Jacobian evaluation routine

  Level: advanced

  Note:
  You can pass in `NULL` for any return argument you do not need.

.seealso: [](ch_ts), `TS`, `TSGetTimeStep()`, `TSGetRHSJacobian()`, `TSGetMatrices()`, `TSGetTime()`, `TSGetStepNumber()`
@*/
PetscErrorCode TSGetIJacobian(TS ts, Mat *Amat, Mat *Pmat, TSIJacobianFn **f, void **ctx)
{
  DM dm;

  PetscFunctionBegin;
  if (Amat || Pmat) {
    SNES snes;
    PetscCall(TSGetSNES(ts, &snes));
    PetscCall(SNESSetUpMatrices(snes));
    PetscCall(SNESGetJacobian(snes, Amat, Pmat, NULL, NULL));
  }
  PetscCall(TSGetDM(ts, &dm));
  PetscCall(DMTSGetIJacobian(dm, f, ctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#include <petsc/private/dmimpl.h>
/*@
  TSSetDM - Sets the `DM` that may be used by some nonlinear solvers or preconditioners under the `TS`

  Logically Collective

  Input Parameters:
+ ts - the `TS` integrator object
- dm - the dm, cannot be `NULL`

  Level: intermediate

  Notes:
  A `DM` can only be used for solving one problem at a time because information about the problem is stored on the `DM`,
  even when not using interfaces like `DMTSSetIFunction()`.  Use `DMClone()` to get a distinct `DM` when solving
  different problems using the same function space.

.seealso: [](ch_ts), `TS`, `DM`, `TSGetDM()`, `SNESSetDM()`, `SNESGetDM()`
@*/
PetscErrorCode TSSetDM(TS ts, DM dm)
{
  SNES snes;
  DMTS tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscValidHeaderSpecific(dm, DM_CLASSID, 2);
  PetscCall(PetscObjectReference((PetscObject)dm));
  if (ts->dm) { /* Move the DMTS context over to the new DM unless the new DM already has one */
    if (ts->dm->dmts && !dm->dmts) {
      PetscCall(DMCopyDMTS(ts->dm, dm));
      PetscCall(DMGetDMTS(ts->dm, &tsdm));
      /* Grant write privileges to the replacement DM */
      if (tsdm->originaldm == ts->dm) tsdm->originaldm = dm;
    }
    PetscCall(DMDestroy(&ts->dm));
  }
  ts->dm = dm;

  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESSetDM(snes, dm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetDM - Gets the `DM` that may be used by some preconditioners

  Not Collective

  Input Parameter:
. ts - the `TS`

  Output Parameter:
. dm - the `DM`

  Level: intermediate

.seealso: [](ch_ts), `TS`, `DM`, `TSSetDM()`, `SNESSetDM()`, `SNESGetDM()`
@*/
PetscErrorCode TSGetDM(TS ts, DM *dm)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (!ts->dm) {
    PetscCall(DMShellCreate(PetscObjectComm((PetscObject)ts), &ts->dm));
    if (ts->snes) PetscCall(SNESSetDM(ts->snes, ts->dm));
  }
  *dm = ts->dm;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  SNESTSFormFunction - Function to evaluate nonlinear residual defined by an ODE solver algorithm implemented within `TS`

  Logically Collective

  Input Parameters:
+ snes - nonlinear solver
. U    - the current state at which to evaluate the residual
- ctx  - user context, must be a `TS`

  Output Parameter:
. F - the nonlinear residual

  Level: developer

  Note:
  This function is not normally called by users and is automatically registered with the `SNES` used by `TS`.
  It is most frequently passed to `MatFDColoringSetFunction()`.

.seealso: [](ch_ts), `SNESSetFunction()`, `MatFDColoringSetFunction()`
@*/
PetscErrorCode SNESTSFormFunction(SNES snes, Vec U, Vec F, void *ctx)
{
  TS ts = (TS)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(snes, SNES_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 2);
  PetscValidHeaderSpecific(F, VEC_CLASSID, 3);
  PetscValidHeaderSpecific(ts, TS_CLASSID, 4);
  PetscCheck(ts->ops->snesfunction, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "No method snesfunction for TS of type %s", ((PetscObject)ts)->type_name);
  PetscCall((*ts->ops->snesfunction)(snes, U, F, ts));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  SNESTSFormJacobian - Function to evaluate the Jacobian defined by an ODE solver algorithm implemented within `TS`

  Collective

  Input Parameters:
+ snes - nonlinear solver
. U    - the current state at which to evaluate the residual
- ctx  - user context, must be a `TS`

  Output Parameters:
+ A - the Jacobian
- B - the matrix used to construct the preconditioner (often the same as `A`)

  Level: developer

  Note:
  This function is not normally called by users and is automatically registered with the `SNES` used by `TS`.

.seealso: [](ch_ts), `SNESSetJacobian()`
@*/
PetscErrorCode SNESTSFormJacobian(SNES snes, Vec U, Mat A, Mat B, void *ctx)
{
  TS ts = (TS)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(snes, SNES_CLASSID, 1);
  PetscValidHeaderSpecific(U, VEC_CLASSID, 2);
  PetscValidHeaderSpecific(A, MAT_CLASSID, 3);
  PetscValidHeaderSpecific(B, MAT_CLASSID, 4);
  PetscValidHeaderSpecific(ts, TS_CLASSID, 5);
  PetscCheck(ts->ops->snesjacobian, PetscObjectComm((PetscObject)ts), PETSC_ERR_SUP, "No method snesjacobian for TS of type %s", ((PetscObject)ts)->type_name);
  PetscCall((*ts->ops->snesjacobian)(snes, U, A, B, ts));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSComputeRHSFunctionLinear - Evaluate the right-hand side via the user-provided Jacobian, for linear problems Udot = A U only

  Collective

  Input Parameters:
+ ts  - time stepping context
. t   - time at which to evaluate
. U   - state at which to evaluate
- ctx - context

  Output Parameter:
. F - right-hand side

  Level: intermediate

  Note:
  This function is intended to be passed to `TSSetRHSFunction()` to evaluate the right-hand side for linear problems.
  The matrix (and optionally the evaluation context) should be passed to `TSSetRHSJacobian()`.

.seealso: [](ch_ts), `TS`, `TSSetRHSFunction()`, `TSSetRHSJacobian()`, `TSComputeRHSJacobianConstant()`
@*/
PetscErrorCode TSComputeRHSFunctionLinear(TS ts, PetscReal t, Vec U, Vec F, void *ctx)
{
  Mat Arhs, Brhs;

  PetscFunctionBegin;
  PetscCall(TSGetRHSMats_Private(ts, &Arhs, &Brhs));
  /* undo the damage caused by shifting */
  PetscCall(TSRecoverRHSJacobian(ts, Arhs, Brhs));
  PetscCall(TSComputeRHSJacobian(ts, t, U, Arhs, Brhs));
  PetscCall(MatMult(Arhs, U, F));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSComputeRHSJacobianConstant - Reuses a Jacobian that is time-independent.

  Collective

  Input Parameters:
+ ts  - time stepping context
. t   - time at which to evaluate
. U   - state at which to evaluate
- ctx - context

  Output Parameters:
+ A - Jacobian
- B - matrix used to construct the preconditioner, often the same as `A`

  Level: intermediate

  Note:
  This function is intended to be passed to `TSSetRHSJacobian()` to evaluate the Jacobian for linear time-independent problems.

.seealso: [](ch_ts), `TS`, `TSSetRHSFunction()`, `TSSetRHSJacobian()`, `TSComputeRHSFunctionLinear()`
@*/
PetscErrorCode TSComputeRHSJacobianConstant(TS ts, PetscReal t, Vec U, Mat A, Mat B, void *ctx)
{
  PetscFunctionBegin;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSComputeIFunctionLinear - Evaluate the left hand side via the user-provided Jacobian, for linear problems only

  Collective

  Input Parameters:
+ ts   - time stepping context
. t    - time at which to evaluate
. U    - state at which to evaluate
. Udot - time derivative of state vector
- ctx  - context

  Output Parameter:
. F - left hand side

  Level: intermediate

  Notes:
  The assumption here is that the left hand side is of the form A*Udot (and not A*Udot + B*U). For other cases, the
  user is required to write their own `TSComputeIFunction()`.
  This function is intended to be passed to `TSSetIFunction()` to evaluate the left hand side for linear problems.
  The matrix (and optionally the evaluation context) should be passed to `TSSetIJacobian()`.

  Note that using this function is NOT equivalent to using `TSComputeRHSFunctionLinear()` since that solves Udot = A U

.seealso: [](ch_ts), `TS`, `TSSetIFunction()`, `TSSetIJacobian()`, `TSComputeIJacobianConstant()`, `TSComputeRHSFunctionLinear()`
@*/
PetscErrorCode TSComputeIFunctionLinear(TS ts, PetscReal t, Vec U, Vec Udot, Vec F, void *ctx)
{
  Mat A, B;

  PetscFunctionBegin;
  PetscCall(TSGetIJacobian(ts, &A, &B, NULL, NULL));
  PetscCall(TSComputeIJacobian(ts, t, U, Udot, 1.0, A, B, PETSC_TRUE));
  PetscCall(MatMult(A, Udot, F));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSComputeIJacobianConstant - Reuses the matrix previously computed with the provided `TSIJacobianFn` for a semi-implicit DAE or ODE

  Collective

  Input Parameters:
+ ts    - time stepping context
. t     - time at which to evaluate
. U     - state at which to evaluate
. Udot  - time derivative of state vector
. shift - shift to apply
- ctx   - context

  Output Parameters:
+ A - pointer to operator
- B - pointer to matrix from which the preconditioner is built (often `A`)

  Level: advanced

  Notes:
  This function is intended to be passed to `TSSetIJacobian()` to evaluate the Jacobian for linear time-independent problems.

  It is only appropriate for problems of the form

  $$
  M \dot{U} = F(U,t)
  $$

  where M is constant and F is non-stiff.  The user must pass M to `TSSetIJacobian()`.  The current implementation only
  works with IMEX time integration methods such as `TSROSW` and `TSARKIMEX`, since there is no support for de-constructing
  an implicit operator of the form

  $$
  shift*M + J
  $$

  where J is the Jacobian of -F(U).  Support may be added in a future version of PETSc, but for now, the user must store
  a copy of M or reassemble it when requested.

.seealso: [](ch_ts), `TS`, `TSROSW`, `TSARKIMEX`, `TSSetIFunction()`, `TSSetIJacobian()`, `TSComputeIFunctionLinear()`
@*/
PetscErrorCode TSComputeIJacobianConstant(TS ts, PetscReal t, Vec U, Vec Udot, PetscReal shift, Mat A, Mat B, void *ctx)
{
  PetscFunctionBegin;
  PetscCall(MatScale(A, shift / ts->ijacobian.shift));
  ts->ijacobian.shift = shift;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetEquationType - Gets the type of the equation that `TS` is solving.

  Not Collective

  Input Parameter:
. ts - the `TS` context

  Output Parameter:
. equation_type - see `TSEquationType`

  Level: beginner

.seealso: [](ch_ts), `TS`, `TSSetEquationType()`, `TSEquationType`
@*/
PetscErrorCode TSGetEquationType(TS ts, TSEquationType *equation_type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(equation_type, 2);
  *equation_type = ts->equation_type;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetEquationType - Sets the type of the equation that `TS` is solving.

  Not Collective

  Input Parameters:
+ ts            - the `TS` context
- equation_type - see `TSEquationType`

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSGetEquationType()`, `TSEquationType`
@*/
PetscErrorCode TSSetEquationType(TS ts, TSEquationType equation_type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->equation_type = equation_type;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetConvergedReason - Gets the reason the `TS` iteration was stopped.

  Not Collective

  Input Parameter:
. ts - the `TS` context

  Output Parameter:
. reason - negative value indicates diverged, positive value converged, see `TSConvergedReason` or the
            manual pages for the individual convergence tests for complete lists

  Level: beginner

  Note:
  Can only be called after the call to `TSSolve()` is complete.

.seealso: [](ch_ts), `TS`, `TSSolve()`, `TSConvergedReason`
@*/
PetscErrorCode TSGetConvergedReason(TS ts, TSConvergedReason *reason)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(reason, 2);
  *reason = ts->reason;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetConvergedReason - Sets the reason for handling the convergence of `TSSolve()`.

  Logically Collective; reason must contain common value

  Input Parameters:
+ ts     - the `TS` context
- reason - negative value indicates diverged, positive value converged, see `TSConvergedReason` or the
            manual pages for the individual convergence tests for complete lists

  Level: advanced

  Note:
  Can only be called while `TSSolve()` is active.

.seealso: [](ch_ts), `TS`, `TSSolve()`, `TSConvergedReason`
@*/
PetscErrorCode TSSetConvergedReason(TS ts, TSConvergedReason reason)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->reason = reason;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetSolveTime - Gets the time after a call to `TSSolve()`

  Not Collective

  Input Parameter:
. ts - the `TS` context

  Output Parameter:
. ftime - the final time. This time corresponds to the final time set with `TSSetMaxTime()`

  Level: beginner

  Note:
  Can only be called after the call to `TSSolve()` is complete.

.seealso: [](ch_ts), `TS`, `TSSolve()`, `TSConvergedReason`
@*/
PetscErrorCode TSGetSolveTime(TS ts, PetscReal *ftime)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(ftime, 2);
  *ftime = ts->solvetime;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetSNESIterations - Gets the total number of nonlinear iterations
  used by the time integrator.

  Not Collective

  Input Parameter:
. ts - `TS` context

  Output Parameter:
. nits - number of nonlinear iterations

  Level: intermediate

  Note:
  This counter is reset to zero for each successive call to `TSSolve()`.

.seealso: [](ch_ts), `TS`, `TSSolve()`, `TSGetKSPIterations()`
@*/
PetscErrorCode TSGetSNESIterations(TS ts, PetscInt *nits)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(nits, 2);
  *nits = ts->snes_its;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetKSPIterations - Gets the total number of linear iterations
  used by the time integrator.

  Not Collective

  Input Parameter:
. ts - `TS` context

  Output Parameter:
. lits - number of linear iterations

  Level: intermediate

  Note:
  This counter is reset to zero for each successive call to `TSSolve()`.

.seealso: [](ch_ts), `TS`, `TSSolve()`, `TSGetSNESIterations()`
@*/
PetscErrorCode TSGetKSPIterations(TS ts, PetscInt *lits)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(lits, 2);
  *lits = ts->ksp_its;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetStepRejections - Gets the total number of rejected steps.

  Not Collective

  Input Parameter:
. ts - `TS` context

  Output Parameter:
. rejects - number of steps rejected

  Level: intermediate

  Note:
  This counter is reset to zero for each successive call to `TSSolve()`.

.seealso: [](ch_ts), `TS`, `TSSolve()`, `TSGetSNESIterations()`, `TSGetKSPIterations()`, `TSSetMaxStepRejections()`, `TSGetSNESFailures()`, `TSSetMaxSNESFailures()`, `TSSetErrorIfStepFails()`
@*/
PetscErrorCode TSGetStepRejections(TS ts, PetscInt *rejects)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(rejects, 2);
  *rejects = ts->reject;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetSNESFailures - Gets the total number of failed `SNES` solves in a `TS`

  Not Collective

  Input Parameter:
. ts - `TS` context

  Output Parameter:
. fails - number of failed nonlinear solves

  Level: intermediate

  Note:
  This counter is reset to zero for each successive call to `TSSolve()`.

.seealso: [](ch_ts), `TS`, `TSSolve()`, `TSGetSNESIterations()`, `TSGetKSPIterations()`, `TSSetMaxStepRejections()`, `TSGetStepRejections()`, `TSSetMaxSNESFailures()`
@*/
PetscErrorCode TSGetSNESFailures(TS ts, PetscInt *fails)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(fails, 2);
  *fails = ts->num_snes_failures;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetMaxStepRejections - Sets the maximum number of step rejections before a time step fails

  Not Collective

  Input Parameters:
+ ts      - `TS` context
- rejects - maximum number of rejected steps, pass `PETSC_UNLIMITED` for unlimited

  Options Database Key:
. -ts_max_reject - Maximum number of step rejections before a step fails

  Level: intermediate

  Developer Note:
  The options database name is incorrect.

.seealso: [](ch_ts), `TS`, `SNES`, `TSGetSNESIterations()`, `TSGetKSPIterations()`, `TSSetMaxSNESFailures()`, `TSGetStepRejections()`, `TSGetSNESFailures()`, `TSSetErrorIfStepFails()`, `TSGetConvergedReason()`
@*/
PetscErrorCode TSSetMaxStepRejections(TS ts, PetscInt rejects)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (rejects == PETSC_UNLIMITED || rejects == -1) {
    ts->max_reject = PETSC_UNLIMITED;
  } else {
    PetscCheck(rejects >= 0, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_OUTOFRANGE, "Cannot have a negative maximum number of rejections");
    ts->max_reject = rejects;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetMaxSNESFailures - Sets the maximum number of failed `SNES` solves

  Not Collective

  Input Parameters:
+ ts    - `TS` context
- fails - maximum number of failed nonlinear solves, pass `PETSC_UNLIMITED` to allow any number of failures.

  Options Database Key:
. -ts_max_snes_failures - Maximum number of nonlinear solve failures

  Level: intermediate

.seealso: [](ch_ts), `TS`, `SNES`, `TSGetSNESIterations()`, `TSGetKSPIterations()`, `TSSetMaxStepRejections()`, `TSGetStepRejections()`, `TSGetSNESFailures()`, `SNESGetConvergedReason()`, `TSGetConvergedReason()`
@*/
PetscErrorCode TSSetMaxSNESFailures(TS ts, PetscInt fails)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (fails == PETSC_UNLIMITED || fails == -1) {
    ts->max_snes_failures = PETSC_UNLIMITED;
  } else {
    PetscCheck(fails >= 0, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_OUTOFRANGE, "Cannot have a negative maximum number of failures");
    ts->max_snes_failures = fails;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetErrorIfStepFails - Immediately error if no step succeeds during `TSSolve()`

  Not Collective

  Input Parameters:
+ ts  - `TS` context
- err - `PETSC_TRUE` to error if no step succeeds, `PETSC_FALSE` to return without failure

  Options Database Key:
. -ts_error_if_step_fails - Error if no step succeeds

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSGetSNESIterations()`, `TSGetKSPIterations()`, `TSSetMaxStepRejections()`, `TSGetStepRejections()`, `TSGetSNESFailures()`, `TSGetConvergedReason()`
@*/
PetscErrorCode TSSetErrorIfStepFails(TS ts, PetscBool err)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->errorifstepfailed = err;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetAdapt - Get the adaptive controller context for the current method

  Collective if controller has not yet been created

  Input Parameter:
. ts - time stepping context

  Output Parameter:
. adapt - adaptive controller

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSAdapt`, `TSAdaptSetType()`, `TSAdaptChoose()`
@*/
PetscErrorCode TSGetAdapt(TS ts, TSAdapt *adapt)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(adapt, 2);
  if (!ts->adapt) {
    PetscCall(TSAdaptCreate(PetscObjectComm((PetscObject)ts), &ts->adapt));
    PetscCall(PetscObjectIncrementTabLevel((PetscObject)ts->adapt, (PetscObject)ts, 1));
  }
  *adapt = ts->adapt;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetTolerances - Set tolerances for local truncation error when using an adaptive controller

  Logically Collective

  Input Parameters:
+ ts    - time integration context
. atol  - scalar absolute tolerances
. vatol - vector of absolute tolerances or `NULL`, used in preference to `atol` if present
. rtol  - scalar relative tolerances
- vrtol - vector of relative tolerances or `NULL`, used in preference to `rtol` if present

  Options Database Keys:
+ -ts_rtol <rtol> - relative tolerance for local truncation error
- -ts_atol <atol> - Absolute tolerance for local truncation error

  Level: beginner

  Notes:
  `PETSC_CURRENT` or `PETSC_DETERMINE` may be used for `atol` or `rtol` to indicate the current value
  or the default value from when the object's type was set.

  With PETSc's implicit schemes for DAE problems, the calculation of the local truncation error
  (LTE) includes both the differential and the algebraic variables. If one wants the LTE to be
  computed only for the differential or the algebraic part then this can be done using the vector of
  tolerances vatol. For example, by setting the tolerance vector with the desired tolerance for the
  differential part and infinity for the algebraic part, the LTE calculation will include only the
  differential variables.

  Fortran Note:
  Use `PETSC_CURRENT_INTEGER` or `PETSC_DETERMINE_INTEGER`.

.seealso: [](ch_ts), `TS`, `TSAdapt`, `TSErrorWeightedNorm()`, `TSGetTolerances()`
@*/
PetscErrorCode TSSetTolerances(TS ts, PetscReal atol, Vec vatol, PetscReal rtol, Vec vrtol)
{
  PetscFunctionBegin;
  if (atol == (PetscReal)PETSC_DETERMINE) {
    ts->atol = ts->default_atol;
  } else if (atol != (PetscReal)PETSC_CURRENT) {
    PetscCheck(atol >= 0.0, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_OUTOFRANGE, "Absolute tolerance %g must be non-negative", (double)atol);
    ts->atol = atol;
  }

  if (vatol) {
    PetscCall(PetscObjectReference((PetscObject)vatol));
    PetscCall(VecDestroy(&ts->vatol));
    ts->vatol = vatol;
  }

  if (rtol == (PetscReal)PETSC_DETERMINE) {
    ts->rtol = ts->default_rtol;
  } else if (rtol != (PetscReal)PETSC_CURRENT) {
    PetscCheck(rtol >= 0.0, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_OUTOFRANGE, "Relative tolerance %g must be non-negative", (double)rtol);
    ts->rtol = rtol;
  }

  if (vrtol) {
    PetscCall(PetscObjectReference((PetscObject)vrtol));
    PetscCall(VecDestroy(&ts->vrtol));
    ts->vrtol = vrtol;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetTolerances - Get tolerances for local truncation error when using adaptive controller

  Logically Collective

  Input Parameter:
. ts - time integration context

  Output Parameters:
+ atol  - scalar absolute tolerances, `NULL` to ignore
. vatol - vector of absolute tolerances, `NULL` to ignore
. rtol  - scalar relative tolerances, `NULL` to ignore
- vrtol - vector of relative tolerances, `NULL` to ignore

  Level: beginner

.seealso: [](ch_ts), `TS`, `TSAdapt`, `TSErrorWeightedNorm()`, `TSSetTolerances()`
@*/
PetscErrorCode TSGetTolerances(TS ts, PetscReal *atol, Vec *vatol, PetscReal *rtol, Vec *vrtol)
{
  PetscFunctionBegin;
  if (atol) *atol = ts->atol;
  if (vatol) *vatol = ts->vatol;
  if (rtol) *rtol = ts->rtol;
  if (vrtol) *vrtol = ts->vrtol;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSErrorWeightedNorm - compute a weighted norm of the difference between two state vectors based on supplied absolute and relative tolerances

  Collective

  Input Parameters:
+ ts        - time stepping context
. U         - state vector, usually ts->vec_sol
. Y         - state vector to be compared to U
- wnormtype - norm type, either `NORM_2` or `NORM_INFINITY`

  Output Parameters:
+ norm  - weighted norm, a value of 1.0 achieves a balance between absolute and relative tolerances
. norma - weighted norm, a value of 1.0 means that the error meets the absolute tolerance set by the user
- normr - weighted norm, a value of 1.0 means that the error meets the relative tolerance set by the user

  Options Database Key:
. -ts_adapt_wnormtype <wnormtype> - 2, INFINITY

  Level: developer

.seealso: [](ch_ts), `TS`, `VecErrorWeightedNorms()`, `TSErrorWeightedENorm()`
@*/
PetscErrorCode TSErrorWeightedNorm(TS ts, Vec U, Vec Y, NormType wnormtype, PetscReal *norm, PetscReal *norma, PetscReal *normr)
{
  PetscInt norma_loc, norm_loc, normr_loc;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(VecErrorWeightedNorms(U, Y, NULL, wnormtype, ts->atol, ts->vatol, ts->rtol, ts->vrtol, ts->adapt->ignore_max, norm, &norm_loc, norma, &norma_loc, normr, &normr_loc));
  if (wnormtype == NORM_2) {
    if (norm_loc) *norm = PetscSqrtReal(PetscSqr(*norm) / norm_loc);
    if (norma_loc) *norma = PetscSqrtReal(PetscSqr(*norma) / norma_loc);
    if (normr_loc) *normr = PetscSqrtReal(PetscSqr(*normr) / normr_loc);
  }
  PetscCheck(!PetscIsInfOrNanScalar(*norm), PetscObjectComm((PetscObject)ts), PETSC_ERR_FP, "Infinite or not-a-number generated in norm");
  PetscCheck(!PetscIsInfOrNanScalar(*norma), PetscObjectComm((PetscObject)ts), PETSC_ERR_FP, "Infinite or not-a-number generated in norma");
  PetscCheck(!PetscIsInfOrNanScalar(*normr), PetscObjectComm((PetscObject)ts), PETSC_ERR_FP, "Infinite or not-a-number generated in normr");
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSErrorWeightedENorm - compute a weighted error norm based on supplied absolute and relative tolerances

  Collective

  Input Parameters:
+ ts        - time stepping context
. E         - error vector
. U         - state vector, usually ts->vec_sol
. Y         - state vector, previous time step
- wnormtype - norm type, either `NORM_2` or `NORM_INFINITY`

  Output Parameters:
+ norm  - weighted norm, a value of 1.0 achieves a balance between absolute and relative tolerances
. norma - weighted norm, a value of 1.0 means that the error meets the absolute tolerance set by the user
- normr - weighted norm, a value of 1.0 means that the error meets the relative tolerance set by the user

  Options Database Key:
. -ts_adapt_wnormtype <wnormtype> - 2, INFINITY

  Level: developer

.seealso: [](ch_ts), `TS`, `VecErrorWeightedNorms()`, `TSErrorWeightedNorm()`
@*/
PetscErrorCode TSErrorWeightedENorm(TS ts, Vec E, Vec U, Vec Y, NormType wnormtype, PetscReal *norm, PetscReal *norma, PetscReal *normr)
{
  PetscInt norma_loc, norm_loc, normr_loc;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCall(VecErrorWeightedNorms(U, Y, E, wnormtype, ts->atol, ts->vatol, ts->rtol, ts->vrtol, ts->adapt->ignore_max, norm, &norm_loc, norma, &norma_loc, normr, &normr_loc));
  if (wnormtype == NORM_2) {
    if (norm_loc) *norm = PetscSqrtReal(PetscSqr(*norm) / norm_loc);
    if (norma_loc) *norma = PetscSqrtReal(PetscSqr(*norma) / norma_loc);
    if (normr_loc) *normr = PetscSqrtReal(PetscSqr(*normr) / normr_loc);
  }
  PetscCheck(!PetscIsInfOrNanScalar(*norm), PetscObjectComm((PetscObject)ts), PETSC_ERR_FP, "Infinite or not-a-number generated in norm");
  PetscCheck(!PetscIsInfOrNanScalar(*norma), PetscObjectComm((PetscObject)ts), PETSC_ERR_FP, "Infinite or not-a-number generated in norma");
  PetscCheck(!PetscIsInfOrNanScalar(*normr), PetscObjectComm((PetscObject)ts), PETSC_ERR_FP, "Infinite or not-a-number generated in normr");
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetCFLTimeLocal - Set the local CFL constraint relative to forward Euler

  Logically Collective

  Input Parameters:
+ ts      - time stepping context
- cfltime - maximum stable time step if using forward Euler (value can be different on each process)

  Note:
  After calling this function, the global CFL time can be obtained by calling TSGetCFLTime()

  Level: intermediate

.seealso: [](ch_ts), `TSGetCFLTime()`, `TSADAPTCFL`
@*/
PetscErrorCode TSSetCFLTimeLocal(TS ts, PetscReal cfltime)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->cfltime_local = cfltime;
  ts->cfltime       = -1.;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetCFLTime - Get the maximum stable time step according to CFL criteria applied to forward Euler

  Collective

  Input Parameter:
. ts - time stepping context

  Output Parameter:
. cfltime - maximum stable time step for forward Euler

  Level: advanced

.seealso: [](ch_ts), `TSSetCFLTimeLocal()`
@*/
PetscErrorCode TSGetCFLTime(TS ts, PetscReal *cfltime)
{
  PetscFunctionBegin;
  if (ts->cfltime < 0) PetscCallMPI(MPIU_Allreduce(&ts->cfltime_local, &ts->cfltime, 1, MPIU_REAL, MPIU_MIN, PetscObjectComm((PetscObject)ts)));
  *cfltime = ts->cfltime;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSVISetVariableBounds - Sets the lower and upper bounds for the solution vector. xl <= x <= xu

  Input Parameters:
+ ts - the `TS` context.
. xl - lower bound.
- xu - upper bound.

  Level: advanced

  Note:
  If this routine is not called then the lower and upper bounds are set to
  `PETSC_NINFINITY` and `PETSC_INFINITY` respectively during `SNESSetUp()`.

.seealso: [](ch_ts), `TS`
@*/
PetscErrorCode TSVISetVariableBounds(TS ts, Vec xl, Vec xu)
{
  SNES snes;

  PetscFunctionBegin;
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(SNESVISetVariableBounds(snes, xl, xu));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSComputeLinearStability - computes the linear stability function at a point

  Collective

  Input Parameters:
+ ts - the `TS` context
. xr - real part of input argument
- xi - imaginary part of input argument

  Output Parameters:
+ yr - real part of function value
- yi - imaginary part of function value

  Level: developer

.seealso: [](ch_ts), `TS`, `TSSetRHSFunction()`, `TSComputeIFunction()`
@*/
PetscErrorCode TSComputeLinearStability(TS ts, PetscReal xr, PetscReal xi, PetscReal *yr, PetscReal *yi)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscUseTypeMethod(ts, linearstability, xr, xi, yr, yi);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSRestartStep - Flags the solver to restart the next step

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Level: advanced

  Notes:
  Multistep methods like `TSBDF` or Runge-Kutta methods with FSAL property require restarting the solver in the event of
  discontinuities. These discontinuities may be introduced as a consequence of explicitly modifications to the solution
  vector (which PETSc attempts to detect and handle) or problem coefficients (which PETSc is not able to detect). For
  the sake of correctness and maximum safety, users are expected to call `TSRestart()` whenever they introduce
  discontinuities in callback routines (e.g. prestep and poststep routines, or implicit/rhs function routines with
  discontinuous source terms).

.seealso: [](ch_ts), `TS`, `TSBDF`, `TSSolve()`, `TSSetPreStep()`, `TSSetPostStep()`
@*/
PetscErrorCode TSRestartStep(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->steprestart = PETSC_TRUE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSRollBack - Rolls back one time step

  Collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSGetStepRollBack()`, `TSCreate()`, `TSSetUp()`, `TSDestroy()`, `TSSolve()`, `TSSetPreStep()`, `TSSetPreStage()`, `TSInterpolate()`
@*/
PetscErrorCode TSRollBack(TS ts)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCheck(!ts->steprollback, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONGSTATE, "TSRollBack already called");
  PetscTryTypeMethod(ts, rollback);
  PetscCall(VecCopy(ts->vec_sol0, ts->vec_sol));
  ts->time_step  = ts->ptime - ts->ptime_prev;
  ts->ptime      = ts->ptime_prev;
  ts->ptime_prev = ts->ptime_prev_rollback;
  ts->steps--;
  ts->steprollback = PETSC_TRUE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetStepRollBack - Get the internal flag indicating if you are rolling back a step

  Not collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. flg - the rollback flag

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSCreate()`, `TSRollBack()`
@*/
PetscErrorCode TSGetStepRollBack(TS ts, PetscBool *flg)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(flg, 2);
  *flg = ts->steprollback;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetStepResize - Get the internal flag indicating if the current step is after a resize.

  Not collective

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameter:
. flg - the resize flag

  Level: advanced

.seealso: [](ch_ts), `TS`, `TSCreate()`, `TSSetResize()`
@*/
PetscErrorCode TSGetStepResize(TS ts, PetscBool *flg)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(flg, 2);
  *flg = ts->stepresize;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetStages - Get the number of stages and stage values

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameters:
+ ns - the number of stages
- Y  - the current stage vectors

  Level: advanced

  Note:
  Both `ns` and `Y` can be `NULL`.

.seealso: [](ch_ts), `TS`, `TSCreate()`
@*/
PetscErrorCode TSGetStages(TS ts, PetscInt *ns, Vec **Y)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ns) PetscAssertPointer(ns, 2);
  if (Y) PetscAssertPointer(Y, 3);
  if (!ts->ops->getstages) {
    if (ns) *ns = 0;
    if (Y) *Y = NULL;
  } else PetscUseTypeMethod(ts, getstages, ns, Y);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSComputeIJacobianDefaultColor - Computes the Jacobian using finite differences and coloring to exploit matrix sparsity.

  Collective

  Input Parameters:
+ ts    - the `TS` context
. t     - current timestep
. U     - state vector
. Udot  - time derivative of state vector
. shift - shift to apply, see note below
- ctx   - an optional user context

  Output Parameters:
+ J - Jacobian matrix (not altered in this routine)
- B - newly computed Jacobian matrix to use with preconditioner (generally the same as `J`)

  Level: intermediate

  Notes:
  If F(t,U,Udot)=0 is the DAE, the required Jacobian is

  dF/dU + shift*dF/dUdot

  Most users should not need to explicitly call this routine, as it
  is used internally within the nonlinear solvers.

  This will first try to get the coloring from the `DM`.  If the `DM` type has no coloring
  routine, then it will try to get the coloring from the matrix.  This requires that the
  matrix have nonzero entries precomputed.

.seealso: [](ch_ts), `TS`, `TSSetIJacobian()`, `MatFDColoringCreate()`, `MatFDColoringSetFunction()`
@*/
PetscErrorCode TSComputeIJacobianDefaultColor(TS ts, PetscReal t, Vec U, Vec Udot, PetscReal shift, Mat J, Mat B, void *ctx)
{
  SNES          snes;
  MatFDColoring color;
  PetscBool     hascolor, matcolor = PETSC_FALSE;

  PetscFunctionBegin;
  PetscCall(PetscOptionsGetBool(((PetscObject)ts)->options, ((PetscObject)ts)->prefix, "-ts_fd_color_use_mat", &matcolor, NULL));
  PetscCall(PetscObjectQuery((PetscObject)B, "TSMatFDColoring", (PetscObject *)&color));
  if (!color) {
    DM         dm;
    ISColoring iscoloring;

    PetscCall(TSGetDM(ts, &dm));
    PetscCall(DMHasColoring(dm, &hascolor));
    if (hascolor && !matcolor) {
      PetscCall(DMCreateColoring(dm, IS_COLORING_GLOBAL, &iscoloring));
      PetscCall(MatFDColoringCreate(B, iscoloring, &color));
      PetscCall(MatFDColoringSetFunction(color, (MatFDColoringFn *)SNESTSFormFunction, (void *)ts));
      PetscCall(MatFDColoringSetFromOptions(color));
      PetscCall(MatFDColoringSetUp(B, iscoloring, color));
      PetscCall(ISColoringDestroy(&iscoloring));
    } else {
      MatColoring mc;

      PetscCall(MatColoringCreate(B, &mc));
      PetscCall(MatColoringSetDistance(mc, 2));
      PetscCall(MatColoringSetType(mc, MATCOLORINGSL));
      PetscCall(MatColoringSetFromOptions(mc));
      PetscCall(MatColoringApply(mc, &iscoloring));
      PetscCall(MatColoringDestroy(&mc));
      PetscCall(MatFDColoringCreate(B, iscoloring, &color));
      PetscCall(MatFDColoringSetFunction(color, (MatFDColoringFn *)SNESTSFormFunction, (void *)ts));
      PetscCall(MatFDColoringSetFromOptions(color));
      PetscCall(MatFDColoringSetUp(B, iscoloring, color));
      PetscCall(ISColoringDestroy(&iscoloring));
    }
    PetscCall(PetscObjectCompose((PetscObject)B, "TSMatFDColoring", (PetscObject)color));
    PetscCall(PetscObjectDereference((PetscObject)color));
  }
  PetscCall(TSGetSNES(ts, &snes));
  PetscCall(MatFDColoringApply(B, color, U, snes));
  if (J != B) {
    PetscCall(MatAssemblyBegin(J, MAT_FINAL_ASSEMBLY));
    PetscCall(MatAssemblyEnd(J, MAT_FINAL_ASSEMBLY));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSSetFunctionDomainError - Set a function that tests if the current state vector is valid

  Input Parameters:
+ ts   - the `TS` context
- func - function called within `TSFunctionDomainError()`

  Calling sequence of `func`:
+ ts     - the `TS` context
. time   - the current time (of the stage)
. state  - the state to check if it is valid
- accept - (output parameter) `PETSC_FALSE` if the state is not acceptable, `PETSC_TRUE` if acceptable

  Level: intermediate

  Notes:
  If an implicit ODE solver is being used then, in addition to providing this routine, the
  user's code should call `SNESSetFunctionDomainError()` when domain errors occur during
  function evaluations where the functions are provided by `TSSetIFunction()` or `TSSetRHSFunction()`.
  Use `TSGetSNES()` to obtain the `SNES` object

  Developer Notes:
  The naming of this function is inconsistent with the `SNESSetFunctionDomainError()`
  since one takes a function pointer and the other does not.

.seealso: [](ch_ts), `TSAdaptCheckStage()`, `TSFunctionDomainError()`, `SNESSetFunctionDomainError()`, `TSGetSNES()`
@*/
PetscErrorCode TSSetFunctionDomainError(TS ts, PetscErrorCode (*func)(TS ts, PetscReal time, Vec state, PetscBool *accept))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->functiondomainerror = func;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSFunctionDomainError - Checks if the current state is valid

  Input Parameters:
+ ts        - the `TS` context
. stagetime - time of the simulation
- Y         - state vector to check.

  Output Parameter:
. accept - Set to `PETSC_FALSE` if the current state vector is valid.

  Level: developer

  Note:
  This function is called by the `TS` integration routines and calls the user provided function (set with `TSSetFunctionDomainError()`)
  to check if the current state is valid.

.seealso: [](ch_ts), `TS`, `TSSetFunctionDomainError()`
@*/
PetscErrorCode TSFunctionDomainError(TS ts, PetscReal stagetime, Vec Y, PetscBool *accept)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  *accept = PETSC_TRUE;
  if (ts->functiondomainerror) PetscCall((*ts->functiondomainerror)(ts, stagetime, Y, accept));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSClone - This function clones a time step `TS` object.

  Collective

  Input Parameter:
. tsin - The input `TS`

  Output Parameter:
. tsout - The output `TS` (cloned)

  Level: developer

  Notes:
  This function is used to create a clone of a `TS` object. It is used in `TSARKIMEX` for initializing the slope for first stage explicit methods.
  It will likely be replaced in the future with a mechanism of switching methods on the fly.

  When using `TSDestroy()` on a clone the user has to first reset the correct `TS` reference in the embedded `SNES` object: e.g., by running
.vb
 SNES snes_dup = NULL;
 TSGetSNES(ts,&snes_dup);
 TSSetSNES(ts,snes_dup);
.ve

.seealso: [](ch_ts), `TS`, `SNES`, `TSCreate()`, `TSSetType()`, `TSSetUp()`, `TSDestroy()`, `TSSetProblemType()`
@*/
PetscErrorCode TSClone(TS tsin, TS *tsout)
{
  TS     t;
  SNES   snes_start;
  DM     dm;
  TSType type;

  PetscFunctionBegin;
  PetscAssertPointer(tsin, 1);
  *tsout = NULL;

  PetscCall(PetscHeaderCreate(t, TS_CLASSID, "TS", "Time stepping", "TS", PetscObjectComm((PetscObject)tsin), TSDestroy, TSView));

  /* General TS description */
  t->numbermonitors    = 0;
  t->setupcalled       = PETSC_FALSE;
  t->ksp_its           = 0;
  t->snes_its          = 0;
  t->nwork             = 0;
  t->rhsjacobian.time  = PETSC_MIN_REAL;
  t->rhsjacobian.scale = 1.;
  t->ijacobian.shift   = 1.;

  PetscCall(TSGetSNES(tsin, &snes_start));
  PetscCall(TSSetSNES(t, snes_start));

  PetscCall(TSGetDM(tsin, &dm));
  PetscCall(TSSetDM(t, dm));

  t->adapt = tsin->adapt;
  PetscCall(PetscObjectReference((PetscObject)t->adapt));

  t->trajectory = tsin->trajectory;
  PetscCall(PetscObjectReference((PetscObject)t->trajectory));

  t->event = tsin->event;
  if (t->event) t->event->refct++;

  t->problem_type      = tsin->problem_type;
  t->ptime             = tsin->ptime;
  t->ptime_prev        = tsin->ptime_prev;
  t->time_step         = tsin->time_step;
  t->max_time          = tsin->max_time;
  t->steps             = tsin->steps;
  t->max_steps         = tsin->max_steps;
  t->equation_type     = tsin->equation_type;
  t->atol              = tsin->atol;
  t->rtol              = tsin->rtol;
  t->max_snes_failures = tsin->max_snes_failures;
  t->max_reject        = tsin->max_reject;
  t->errorifstepfailed = tsin->errorifstepfailed;

  PetscCall(TSGetType(tsin, &type));
  PetscCall(TSSetType(t, type));

  t->vec_sol = NULL;

  t->cfltime          = tsin->cfltime;
  t->cfltime_local    = tsin->cfltime_local;
  t->exact_final_time = tsin->exact_final_time;

  t->ops[0] = tsin->ops[0];

  if (((PetscObject)tsin)->fortran_func_pointers) {
    PetscInt i;
    PetscCall(PetscMalloc((10) * sizeof(void (*)(void)), &((PetscObject)t)->fortran_func_pointers));
    for (i = 0; i < 10; i++) ((PetscObject)t)->fortran_func_pointers[i] = ((PetscObject)tsin)->fortran_func_pointers[i];
  }
  *tsout = t;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode RHSWrapperFunction_TSRHSJacobianTest(void *ctx, Vec x, Vec y)
{
  TS ts = (TS)ctx;

  PetscFunctionBegin;
  PetscCall(TSComputeRHSFunction(ts, 0, x, y));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSRHSJacobianTest - Compares the multiply routine provided to the `MATSHELL` with differencing on the `TS` given RHS function.

  Logically Collective

  Input Parameter:
. ts - the time stepping routine

  Output Parameter:
. flg - `PETSC_TRUE` if the multiply is likely correct

  Options Database Key:
. -ts_rhs_jacobian_test_mult -mat_shell_test_mult_view - run the test at each timestep of the integrator

  Level: advanced

  Note:
  This only works for problems defined using `TSSetRHSFunction()` and Jacobian NOT `TSSetIFunction()` and Jacobian

.seealso: [](ch_ts), `TS`, `Mat`, `MATSHELL`, `MatCreateShell()`, `MatShellGetContext()`, `MatShellGetOperation()`, `MatShellTestMultTranspose()`, `TSRHSJacobianTestTranspose()`
@*/
PetscErrorCode TSRHSJacobianTest(TS ts, PetscBool *flg)
{
  Mat              J, B;
  TSRHSJacobianFn *func;
  void            *ctx;

  PetscFunctionBegin;
  PetscCall(TSGetRHSJacobian(ts, &J, &B, &func, &ctx));
  PetscCall((*func)(ts, 0.0, ts->vec_sol, J, B, ctx));
  PetscCall(MatShellTestMult(J, RHSWrapperFunction_TSRHSJacobianTest, ts->vec_sol, ts, flg));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSRHSJacobianTestTranspose - Compares the multiply transpose routine provided to the `MATSHELL` with differencing on the `TS` given RHS function.

  Logically Collective

  Input Parameter:
. ts - the time stepping routine

  Output Parameter:
. flg - `PETSC_TRUE` if the multiply is likely correct

  Options Database Key:
. -ts_rhs_jacobian_test_mult_transpose -mat_shell_test_mult_transpose_view - run the test at each timestep of the integrator

  Level: advanced

  Notes:
  This only works for problems defined using `TSSetRHSFunction()` and Jacobian NOT `TSSetIFunction()` and Jacobian

.seealso: [](ch_ts), `TS`, `Mat`, `MatCreateShell()`, `MatShellGetContext()`, `MatShellGetOperation()`, `MatShellTestMultTranspose()`, `TSRHSJacobianTest()`
@*/
PetscErrorCode TSRHSJacobianTestTranspose(TS ts, PetscBool *flg)
{
  Mat              J, B;
  void            *ctx;
  TSRHSJacobianFn *func;

  PetscFunctionBegin;
  PetscCall(TSGetRHSJacobian(ts, &J, &B, &func, &ctx));
  PetscCall((*func)(ts, 0.0, ts->vec_sol, J, B, ctx));
  PetscCall(MatShellTestMultTranspose(J, RHSWrapperFunction_TSRHSJacobianTest, ts->vec_sol, ts, flg));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetUseSplitRHSFunction - Use the split RHSFunction when a multirate method is used.

  Logically Collective

  Input Parameters:
+ ts                   - timestepping context
- use_splitrhsfunction - `PETSC_TRUE` indicates that the split RHSFunction will be used

  Options Database Key:
. -ts_use_splitrhsfunction - <true,false>

  Level: intermediate

  Note:
  This is only for multirate methods

.seealso: [](ch_ts), `TS`, `TSGetUseSplitRHSFunction()`
@*/
PetscErrorCode TSSetUseSplitRHSFunction(TS ts, PetscBool use_splitrhsfunction)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->use_splitrhsfunction = use_splitrhsfunction;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetUseSplitRHSFunction - Gets whether to use the split RHSFunction when a multirate method is used.

  Not Collective

  Input Parameter:
. ts - timestepping context

  Output Parameter:
. use_splitrhsfunction - `PETSC_TRUE` indicates that the split RHSFunction will be used

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSSetUseSplitRHSFunction()`
@*/
PetscErrorCode TSGetUseSplitRHSFunction(TS ts, PetscBool *use_splitrhsfunction)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  *use_splitrhsfunction = ts->use_splitrhsfunction;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetMatStructure - sets the relationship between the nonzero structure of the RHS Jacobian matrix to the IJacobian matrix.

  Logically  Collective

  Input Parameters:
+ ts  - the time-stepper
- str - the structure (the default is `UNKNOWN_NONZERO_PATTERN`)

  Level: intermediate

  Note:
  When the relationship between the nonzero structures is known and supplied the solution process can be much faster

.seealso: [](ch_ts), `TS`, `MatAXPY()`, `MatStructure`
 @*/
PetscErrorCode TSSetMatStructure(TS ts, MatStructure str)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  ts->axpy_pattern = str;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetEvaluationTimes - sets the evaluation points. The solution will be computed and stored for each time requested

  Collective

  Input Parameters:
+ ts          - the time-stepper
. n           - number of the time points
- time_points - array of the time points

  Options Database Key:
. -ts_eval_times <t0,...tn> - Sets the evaluation times

  Level: intermediate

  Notes:
  The elements in `time_points` must be all increasing. They correspond to the intermediate points for time integration.

  `TS_EXACTFINALTIME_MATCHSTEP` must be used to make the last time step in each sub-interval match the intermediate points specified.

  The intermediate solutions are saved in a vector array that can be accessed with `TSGetEvaluationSolutions()`. Thus using evaluation times may
  pressure the memory system when using a large number of time points.

.seealso: [](ch_ts), `TS`, `TSGetEvaluationTimes()`, `TSGetEvaluationSolutions()`, `TSSetTimeSpan()`
 @*/
PetscErrorCode TSSetEvaluationTimes(TS ts, PetscInt n, PetscReal *time_points)
{
  PetscBool is_sorted;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (ts->eval_times && n != ts->eval_times->num_time_points) {
    PetscCall(PetscFree(ts->eval_times->time_points));
    PetscCall(VecDestroyVecs(ts->eval_times->num_time_points, &ts->eval_times->sol_vecs));
    PetscCall(PetscMalloc1(n, &ts->eval_times->time_points));
  }
  if (!ts->eval_times) {
    TSEvaluationTimes eval_times;
    PetscCall(PetscNew(&eval_times));
    PetscCall(PetscMalloc1(n, &eval_times->time_points));
    eval_times->reltol  = 1e-6;
    eval_times->abstol  = 10 * PETSC_MACHINE_EPSILON;
    eval_times->worktol = 0;
    ts->eval_times      = eval_times;
  }
  ts->eval_times->num_time_points = n;
  PetscCall(PetscSortedReal(n, time_points, &is_sorted));
  PetscCheck(is_sorted, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "time_points array must be sorted");
  PetscCall(PetscArraycpy(ts->eval_times->time_points, time_points, n));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetEvaluationTimes - gets the evaluation times set with `TSSetEvaluationTimes()`

  Not Collective

  Input Parameter:
. ts - the time-stepper

  Output Parameters:
+ n           - number of the time points
- time_points - array of the time points

  Level: beginner

  Note:
  The values obtained are valid until the `TS` object is destroyed.

  Both `n` and `time_points` can be `NULL`.

  Also used to see time points set by `TSSetTimeSpan()`.

.seealso: [](ch_ts), `TS`, `TSSetEvaluationTimes()`, `TSGetEvaluationSolutions()`
 @*/
PetscErrorCode TSGetEvaluationTimes(TS ts, PetscInt *n, const PetscReal *time_points[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (n) PetscAssertPointer(n, 2);
  if (time_points) PetscAssertPointer(time_points, 3);
  if (!ts->eval_times) {
    if (n) *n = 0;
    if (time_points) *time_points = NULL;
  } else {
    if (n) *n = ts->eval_times->num_time_points;
    if (time_points) *time_points = ts->eval_times->time_points;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSGetEvaluationSolutions - Get the number of solutions and the solutions at the evaluation time points specified

  Input Parameter:
. ts - the `TS` context obtained from `TSCreate()`

  Output Parameters:
+ nsol      - the number of solutions
. sol_times - array of solution times corresponding to the solution vectors. See note below
- Sols      - the solution vectors

  Level: intermediate

  Notes:
  Both `nsol` and `Sols` can be `NULL`.

  Some time points in the evaluation points may be skipped by `TS` so that `nsol` is less than the number of points specified by `TSSetEvaluationTimes()`.
  For example, manipulating the step size, especially with a reduced precision, may cause `TS` to step over certain evaluation times.

  Also used to see view solutions requested by `TSSetTimeSpan()`.

.seealso: [](ch_ts), `TS`, `TSSetEvaluationTimes()`, `TSGetEvaluationTimes()`
@*/
PetscErrorCode TSGetEvaluationSolutions(TS ts, PetscInt *nsol, const PetscReal *sol_times[], Vec **Sols)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  if (nsol) PetscAssertPointer(nsol, 2);
  if (sol_times) PetscAssertPointer(sol_times, 3);
  if (Sols) PetscAssertPointer(Sols, 4);
  if (!ts->eval_times) {
    if (nsol) *nsol = 0;
    if (sol_times) *sol_times = NULL;
    if (Sols) *Sols = NULL;
  } else {
    if (nsol) *nsol = ts->eval_times->sol_ctr;
    if (sol_times) *sol_times = ts->eval_times->sol_times;
    if (Sols) *Sols = ts->eval_times->sol_vecs;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSSetTimeSpan - sets the time span. The solution will be computed and stored for each time requested in the span

  Collective

  Input Parameters:
+ ts         - the time-stepper
. n          - number of the time points (>=2)
- span_times - array of the time points. The first element and the last element are the initial time and the final time respectively.

  Options Database Key:
. -ts_time_span <t0,...tf> - Sets the time span

  Level: intermediate

  Notes:
  This function is identical to `TSSetEvaluationTimes()`, except that it also sets the initial time and final time for the `ts` to the first and last `span_times` entries.

  The elements in tspan must be all increasing. They correspond to the intermediate points for time integration.

  `TS_EXACTFINALTIME_MATCHSTEP` must be used to make the last time step in each sub-interval match the intermediate points specified.

  The intermediate solutions are saved in a vector array that can be accessed with `TSGetEvaluationSolutions()`. Thus using time span may
  pressure the memory system when using a large number of span points.

.seealso: [](ch_ts), `TS`, `TSSetEvaluationTimes()`, `TSGetEvaluationTimes()`, `TSGetEvaluationSolutions()`
 @*/
PetscErrorCode TSSetTimeSpan(TS ts, PetscInt n, PetscReal *span_times)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscCheck(n >= 2, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_WRONG, "Minimum time span size is 2 but %" PetscInt_FMT " is provided", n);
  PetscCall(TSSetEvaluationTimes(ts, n, span_times));
  PetscCall(TSSetTime(ts, span_times[0]));
  PetscCall(TSSetMaxTime(ts, span_times[n - 1]));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSPruneIJacobianColor - Remove nondiagonal zeros in the Jacobian matrix and update the `MatMFFD` coloring information.

  Collective

  Input Parameters:
+ ts - the `TS` context
. J  - Jacobian matrix (not altered in this routine)
- B  - newly computed Jacobian matrix to use with preconditioner

  Level: intermediate

  Notes:
  This function improves the `MatFDColoring` performance when the Jacobian matrix was over-allocated or contains
  many constant zeros entries, which is typically the case when the matrix is generated by a `DM`
  and multiple fields are involved.

  Users need to make sure that the Jacobian matrix is properly filled to reflect the sparsity
  structure. For `MatFDColoring`, the values of nonzero entries are not important. So one can
  usually call `TSComputeIJacobian()` with randomized input vectors to generate a dummy Jacobian.
  `TSComputeIJacobian()` should be called before `TSSolve()` but after `TSSetUp()`.

.seealso: [](ch_ts), `TS`, `MatFDColoring`, `TSComputeIJacobianDefaultColor()`, `MatEliminateZeros()`, `MatFDColoringCreate()`, `MatFDColoringSetFunction()`
@*/
PetscErrorCode TSPruneIJacobianColor(TS ts, Mat J, Mat B)
{
  MatColoring   mc            = NULL;
  ISColoring    iscoloring    = NULL;
  MatFDColoring matfdcoloring = NULL;

  PetscFunctionBegin;
  /* Generate new coloring after eliminating zeros in the matrix */
  PetscCall(MatEliminateZeros(B, PETSC_TRUE));
  PetscCall(MatColoringCreate(B, &mc));
  PetscCall(MatColoringSetDistance(mc, 2));
  PetscCall(MatColoringSetType(mc, MATCOLORINGSL));
  PetscCall(MatColoringSetFromOptions(mc));
  PetscCall(MatColoringApply(mc, &iscoloring));
  PetscCall(MatColoringDestroy(&mc));
  /* Replace the old coloring with the new one */
  PetscCall(MatFDColoringCreate(B, iscoloring, &matfdcoloring));
  PetscCall(MatFDColoringSetFunction(matfdcoloring, (MatFDColoringFn *)SNESTSFormFunction, (void *)ts));
  PetscCall(MatFDColoringSetFromOptions(matfdcoloring));
  PetscCall(MatFDColoringSetUp(B, iscoloring, matfdcoloring));
  PetscCall(PetscObjectCompose((PetscObject)B, "TSMatFDColoring", (PetscObject)matfdcoloring));
  PetscCall(PetscObjectDereference((PetscObject)matfdcoloring));
  PetscCall(ISColoringDestroy(&iscoloring));
  PetscFunctionReturn(PETSC_SUCCESS);
}
