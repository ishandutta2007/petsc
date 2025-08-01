(ch_profiling)=

# Profiling

PETSc includes a consistent, lightweight scheme for profiling
application programs. The PETSc routines automatically log
performance data if certain options are specified at runtime. The user
can also log information about application codes for a complete picture
of performance.

In addition, as described in
{any}`sec_ploginfo`, PETSc provides a mechanism for
printing informative messages about computations.
{any}`sec_profbasic` introduces the various profiling
options in PETSc, while the remainder of the chapter focuses on details
such as monitoring application codes and tips for accurate profiling.

(sec_profbasic)=

## Basic Profiling Information

The profiling options include the following:

- `-log_view  [:filename]` - Prints an ASCII version of performance data at the
  program’s conclusion. These statistics are comprehensive and concise
  and require little overhead; thus, `-log_view` is intended as the
  primary means of monitoring the performance of PETSc codes. See {any}`sec_ploginfo`
- `-info [infofile]` - Prints verbose information about code to
  stdout or an optional file. This option provides details about
  algorithms, data structures, etc. Since the overhead of printing such
  output slows a code, this option should not be used when evaluating a
  program’s performance. See {any}`sec_PetscInfo`
- `-log_trace [logfile]` - Traces the beginning and ending of all
  PETSc events. This option, which can be used in conjunction with
  `-info`, is useful to see where a program is hanging without
  running in the debugger. See `PetscLogTraceBegin()`.

As discussed in {any}`sec_mpelogs`, additional profiling
can be done with MPE.

(sec_ploginfo)=

### Interpreting `-log_view` Output: The Basics

As shown in {any}`listing <listing_exprof>` in {any}`sec_profiling_programs`, the
option `-log_view` `[:filename]` activates printing of profile data to standard
output or an ASCII file at the conclusion of a program. See `PetscLogView()` for all the possible
output options.

We print performance data for each routine, organized by PETSc
libraries, followed by any user-defined events (discussed in
{any}`sec_profileuser`). For each routine, the output data
include the maximum time and floating point operation (flop) rate over
all processes. Information about parallel performance is also included,
as discussed in the following section.

For the purpose of PETSc floating point operation counting, we define
one *flop* as one operation of any of the following types:
multiplication, division, addition, or subtraction. For example, one
`VecAXPY()` operation, which computes $y = \alpha x + y$ for
vectors of length $N$, requires $2N$ flop (consisting of
$N$ additions and $N$ multiplications). Bear in mind that
flop rates present only a limited view of performance, since memory
loads and stores are the real performance barrier.

For simplicity, the remainder of this discussion focuses on interpreting
profile data for the `KSP` library, which provides the linear solvers
at the heart of the PETSc package. Recall the hierarchical organization
of the PETSc library, as shown in
{any}`fig_library`. Each `KSP` solver is composed
of a `PC` (preconditioner) and a `KSP` (Krylov subspace) part, which
are in turn built on top of the `Mat` (matrix) and `Vec` (vector)
modules. Thus, operations in the `KSP` module are composed of
lower-level operations in these packages. Note also that the nonlinear
solvers library, `SNES`, is built on top of the `KSP` module, and
the timestepping library, `TS`, is in turn built on top of `SNES`.

We briefly discuss interpretation of the sample output in
{any}`listing <listing_exprof>`, which was generated by solving a
linear system on one process using restarted GMRES and ILU
preconditioning. The linear solvers in `KSP` consist of two basic
phases, `KSPSetUp()` and `KSPSolve()`, each of which consists of a
variety of actions, depending on the particular solution technique. For
the case of using the `PCILU` preconditioner and `KSPGMRES` Krylov
subspace method, the breakdown of PETSc routines is listed below. As
indicated by the levels of indentation, the operations in `KSPSetUp()`
include all of the operations within `PCSetUp()`, which in turn
include `MatILUFactor()`, and so on.

- `KSPSetUp` - Set up linear solver

  - `PCSetUp` - Set up preconditioner

    - `MatILUFactor` - Factor matrix

      - `MatILUFactorSymbolic` - Symbolic factorization phase
      - `MatLUFactorNumeric` - Numeric factorization phase

- `KSPSolve` - Solve linear system

  - `PCApply` - Apply preconditioner

    - `MatSolve` - Forward/backward triangular solves

  - `KSPGMRESOrthog` - Orthogonalization in GMRES

    - `VecDot` or `VecMDot` - Inner products
    - `VecAXPY` or `VecMAXPY` - vector updates

  - `MatMult` - Matrix-vector product

  - `MatMultAdd` - Matrix-vector product + vector addition

    - `VecScale`, `VecNorm`, `VecAXPY`, `VecCopy`, ...

The summaries printed via `-log_view` reflect this routine hierarchy.
For example, the performance summaries for a particular high-level
routine such as `KSPSolve()` include all of the operations accumulated
in the lower-level components that make up the routine.

The output produced with `-log_view` is flat, meaning that the hierarchy
of PETSc operations is not completely clear. For a
particular problem, the user should generally have an idea of the basic
operations that are required for its implementation (e.g., which
operations are performed when using GMRES and ILU, as described above),
so that interpreting the `-log_view` data should be relatively
straightforward.
If this is problematic then it is also possible to examine
the profiling information in a nested format. For more information see
{any}`sec_nestedevents`.

(sec_parperformance)=

### Interpreting `-log_view` Output: Parallel Performance

We next discuss performance summaries for parallel programs, as shown
within {any}`listing <listing_exparprof>` and {any}`listing <listing_exparprof2>`,
which presents the
output generated by the `-log_view` option. The program that generated
this data is
<a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ksp/ksp/tutorials/ex10.c.html">KSP Tutorial ex10</a>.
The code loads a matrix and right-hand-side vector from a binary file
and then solves the resulting linear system; the program then repeats
this process for a second linear system. This particular case was run on
four processors of an Intel x86_64 Linux cluster, using restarted GMRES
and the block Jacobi preconditioner, where each block was solved with
ILU. The two input files `medium` and `arco6` can be obtained from
[datafiles](https://gitlab.com/petsc/datafiles), see {any}`petsc_repositories`.

The first {any}`listing <listing_exparprof>` presents an overall
performance summary, including times, floating-point operations,
computational rates, and message-passing activity (such as the number
and size of messages sent and collective operations). Summaries for
various user-defined stages of monitoring (as discussed in
{any}`sec_profstages`) are also given. Information about the
various phases of computation then follow (as shown separately here in
the second {any}`listing <listing_exparprof2>`). Finally, a summary of
object creation and destruction is presented.

(listing_exparprof)=

```none
mpiexec -n 4 ./ex10 -f0 medium -f1 arco6 -ksp_gmres_classicalgramschmidt -log_view -mat_type baij \
            -matload_block_size 3 -pc_type bjacobi -options_left

Number of iterations = 19
Residual norm 1.088292e-05
Number of iterations = 59
Residual norm 3.871022e-02
---------------------------------------------- PETSc Performance Summary: ----------------------------------------------

./ex10 on a intel-bdw-opt named beboplogin4 with 4 processors, by jczhang Mon Apr 23 13:36:54 2018
Using PETSc Development Git Revision: v3.9-163-gbe3efd42 Git Date: 2018-04-16 10:45:40 -0500

                         Max       Max/Min        Avg      Total
Time (sec):           1.849e-01      1.00002   1.849e-01
Objects:              1.060e+02      1.00000   1.060e+02
Flops:                2.361e+08      1.00684   2.353e+08  9.413e+08
Flops/sec:            1.277e+09      1.00685   1.273e+09  5.091e+09
MPI Msg Count:        2.360e+02      1.34857   2.061e+02  8.245e+02
MPI Msg Len (bytes):  1.256e+07      2.24620   4.071e+04  3.357e+07
MPI Reductions:       2.160e+02      1.00000

Summary of Stages:   ----- Time ------  ----- Flop -----  --- Messages ---  -- Message Lengths --  -- Reductions --
                        Avg     %Total     Avg     %Total   counts   %Total     Avg         %Total   counts   %Total
 0:      Main Stage: 5.9897e-04   0.3%  0.0000e+00   0.0%  0.000e+00   0.0%  0.000e+00        0.0%  2.000e+00   0.9%
 1:   Load System 0: 2.9113e-03   1.6%  0.0000e+00   0.0%  3.550e+01   4.3%  5.984e+02        0.1%  2.200e+01  10.2%
 2:      KSPSetUp 0: 7.7349e-04   0.4%  9.9360e+03   0.0%  0.000e+00   0.0%  0.000e+00        0.0%  2.000e+00   0.9%
 3:      KSPSolve 0: 1.7690e-03   1.0%  2.9673e+05   0.0%  1.520e+02  18.4%  1.800e+02        0.1%  3.900e+01  18.1%
 4:   Load System 1: 1.0056e-01  54.4%  0.0000e+00   0.0%  3.700e+01   4.5%  5.657e+05       62.4%  2.200e+01  10.2%
 5:      KSPSetUp 1: 5.6883e-03   3.1%  2.1205e+07   2.3%  0.000e+00   0.0%  0.000e+00        0.0%  2.000e+00   0.9%
 6:      KSPSolve 1: 7.2578e-02  39.3%  9.1979e+08  97.7%  6.000e+02  72.8%  2.098e+04       37.5%  1.200e+02  55.6%

------------------------------------------------------------------------------------------------------------------------

.... [Summary of various phases, see part II below] ...

------------------------------------------------------------------------------------------------------------------------

Object Type          Creations   Destructions    (Reports information only for process 0.)
...
--- Event Stage 3: KSPSolve 0

              Matrix     0              4
              Vector    20             30
           Index Set     0              3
         Vec Scatter     0              1
       Krylov Solver     0              2
      Preconditioner     0              2
```

We next focus on the summaries for the various phases of the
computation, as given in the table within
the following {any}`listing <listing_exparprof2>`. The summary for each
phase presents the maximum times and flop rates over all processes, as
well as the ratio of maximum to minimum times and flop rates for all
processes. A ratio of approximately 1 indicates that computations within
a given phase are well balanced among the processes; as the ratio
increases, the balance becomes increasingly poor. Also, the total
computational rate (in units of MFlop/sec) is given for each phase in
the final column of the phase summary table.

$$
{\rm Total\: Mflop/sec} \:=\: 10^{-6} * ({\rm sum\; of\; flop\; over\; all\; processors})/({\rm max\; time\; over\; all\; processors})
$$

*Note*: Total computational rates $<$ 1 MFlop are listed as 0 in
this column of the phase summary table. Additional statistics for each
phase include the total number of messages sent, the average message
length, and the number of global reductions.

(listing_exparprof2)=

```none
mpiexec -n 4 ./ex10 -f0 medium -f1 arco6 -ksp_gmres_classicalgramschmidt -log_view -mat_type baij \
            -matload_block_size 3 -pc_type bjacobi -options_left

---------------------------------------------- PETSc Performance Summary: ----------------------------------------------
.... [Overall summary, see part I] ...

Phase summary info:
   Count: number of times phase was executed
   Time and Flop/sec: Max - maximum over all processors
                       Ratio - ratio of maximum to minimum over all processors
   Mess: number of messages sent
   AvgLen: average message length
   Reduct: number of global reductions
   Global: entire computation
   Stage: optional user-defined stages of a computation. Set stages with PetscLogStagePush() and PetscLogStagePop().
      %T - percent time in this phase         %F - percent flop in this phase
      %M - percent messages in this phase     %L - percent message lengths in this phase
      %R - percent reductions in this phase
   Total Mflop/s: 10^6 * (sum of flop over all processors)/(max time over all processors)
------------------------------------------------------------------------------------------------------------------------
Phase              Count      Time (sec)       Flop/sec                          --- Global ---  --- Stage ----  Total
                            Max    Ratio      Max    Ratio  Mess AvgLen  Reduct  %T %F %M %L %R  %T %F %M %L %R Mflop/s
------------------------------------------------------------------------------------------------------------------------
...

--- Event Stage 5: KSPSetUp 1

MatLUFactorNum         1 1.0 3.6440e-03 1.1 5.30e+06 1.0 0.0e+00 0.0e+00 0.0e+00  2  2  0  0  0  62100  0  0  0  5819
MatILUFactorSym        1 1.0 1.7111e-03 1.4 0.00e+00 0.0 0.0e+00 0.0e+00 0.0e+00  1  0  0  0  0  26  0  0  0  0     0
MatGetRowIJ            1 1.0 1.1921e-06 1.2 0.00e+00 0.0 0.0e+00 0.0e+00 0.0e+00  0  0  0  0  0   0  0  0  0  0     0
MatGetOrdering         1 1.0 3.0041e-05 1.1 0.00e+00 0.0 0.0e+00 0.0e+00 0.0e+00  0  0  0  0  0   1  0  0  0  0     0
KSPSetUp               2 1.0 6.6495e-04 1.5 0.00e+00 0.0 0.0e+00 0.0e+00 2.0e+00  0  0  0  0  1   9  0  0  0100     0
PCSetUp                2 1.0 5.4271e-03 1.2 5.30e+06 1.0 0.0e+00 0.0e+00 0.0e+00  3  2  0  0  0  90100  0  0  0  3907
PCSetUpOnBlocks        1 1.0 5.3999e-03 1.2 5.30e+06 1.0 0.0e+00 0.0e+00 0.0e+00  3  2  0  0  0  90100  0  0  0  3927

--- Event Stage 6: KSPSolve 1

MatMult               60 1.0 2.4068e-02 1.1 6.54e+07 1.0 6.0e+02 2.1e+04 0.0e+00 12 27 73 37  0  32 28100100  0 10731
MatSolve              61 1.0 1.9177e-02 1.0 5.99e+07 1.0 0.0e+00 0.0e+00 0.0e+00 10 25  0  0  0  26 26  0  0  0 12491
VecMDot               59 1.0 1.4741e-02 1.3 4.86e+07 1.0 0.0e+00 0.0e+00 5.9e+01  7 21  0  0 27  18 21  0  0 49 13189
VecNorm               61 1.0 3.0417e-03 1.4 3.29e+06 1.0 0.0e+00 0.0e+00 6.1e+01  1  1  0  0 28   4  1  0  0 51  4332
VecScale              61 1.0 9.9802e-04 1.0 1.65e+06 1.0 0.0e+00 0.0e+00 0.0e+00  1  1  0  0  0   1  1  0  0  0  6602
VecCopy                2 1.0 5.9128e-05 1.4 0.00e+00 0.0 0.0e+00 0.0e+00 0.0e+00  0  0  0  0  0   0  0  0  0  0     0
VecSet                64 1.0 8.0323e-04 1.0 0.00e+00 0.0 0.0e+00 0.0e+00 0.0e+00  0  0  0  0  0   1  0  0  0  0     0
VecAXPY                3 1.0 7.4387e-05 1.1 1.62e+05 1.0 0.0e+00 0.0e+00 0.0e+00  0  0  0  0  0   0  0  0  0  0  8712
VecMAXPY              61 1.0 8.8558e-03 1.1 5.18e+07 1.0 0.0e+00 0.0e+00 0.0e+00  5 22  0  0  0  12 23  0  0  0 23393
VecScatterBegin       60 1.0 9.6416e-04 1.8 0.00e+00 0.0 6.0e+02 2.1e+04 0.0e+00  0  0 73 37  0   1  0100100  0     0
VecScatterEnd         60 1.0 6.1543e-03 1.2 0.00e+00 0.0 0.0e+00 0.0e+00 0.0e+00  3  0  0  0  0   8  0  0  0  0     0
VecNormalize          61 1.0 4.2675e-03 1.3 4.94e+06 1.0 0.0e+00 0.0e+00 6.1e+01  2  2  0  0 28   5  2  0  0 51  4632
KSPGMRESOrthog        59 1.0 2.2627e-02 1.1 9.72e+07 1.0 0.0e+00 0.0e+00 5.9e+01 11 41  0  0 27  29 42  0  0 49 17185
KSPSolve               1 1.0 7.2577e-02 1.0 2.31e+08 1.0 6.0e+02 2.1e+04 1.2e+02 39 98 73 37 56  99100100100100 12673
PCSetUpOnBlocks        1 1.0 9.5367e-07 0.0 0.00e+00 0.0 0.0e+00 0.0e+00 0.0e+00  0  0  0  0  0   0  0  0  0  0     0
PCApply               61 1.0 2.0427e-02 1.0 5.99e+07 1.0 0.0e+00 0.0e+00 0.0e+00 11 25  0  0  0  28 26  0  0  0 11726
------------------------------------------------------------------------------------------------------------------------
.... [Conclusion of overall summary, see part I] ...
```

As discussed in the preceding section, the performance summaries for
higher-level PETSc routines include the statistics for the lower levels
of which they are made up. For example, the communication within
matrix-vector products `MatMult()` consists of vector scatter
operations, as given by the routines `VecScatterBegin()` and
`VecScatterEnd()`.

The final data presented are the percentages of the various statistics
(time (`%T`), flop/sec (`%F`), messages(`%M`), average message
length (`%L`), and reductions (`%R`)) for each event relative to the
total computation and to any user-defined stages (discussed in
{any}`sec_profstages`). These statistics can aid in
optimizing performance, since they indicate the sections of code that
could benefit from various kinds of tuning.
{any}`ch_performance` gives suggestions about achieving good
performance with PETSc codes.

The additional option `-log_view_memory` causes the display of additional columns of information about how much
memory was allocated and freed during each logged event. This is useful
to understand what phases of a computation require the most memory.

(sec_mpelogs)=

### Using `-log_mpe` with Jumpshot

It is also possible to use the *Jumpshot* package
{cite}`upshot` to visualize PETSc events. This package comes
with the MPE software, which is part of the MPICH
{cite}`mpich-web-page` implementation of MPI. The option

```none
-log_mpe [logfile]
```

creates a logfile of events appropriate for viewing with *Jumpshot*. The
user can either use the default logging file or specify a name via
`logfile`. Events can be deactivated as described in
{any}`sec_deactivate`.

The user can also log MPI events. To do this, simply consider the PETSc
application as any MPI application, and follow the MPI implementation’s
instructions for logging MPI calls. For example, when using MPICH, this
merely required adding `-llmpich` to the library list *before*
`-lmpich`.

(sec_nestedevents)=

### Profiling Nested Events

It is possible to output the PETSc logging information in a nested format
where the hierarchy of events is explicit. This output can be generated
either as an XML file or as a text file in a format suitable for viewing as
a flame graph.

One can generate the XML output by passing the option `-log_view :[logfilename]:ascii_xml`.
It can be viewed by copying `${PETSC_DIR}/share/petsc/xml/performance_xml2html.xsl`
into the current directory, then opening the logfile in your browser.

The flame graph output can be generated with the option `-log_view :[logfile]:ascii_flamegraph`.
It can then be visualised with either [FlameGraph](https://github.com/brendangregg/FlameGraph)
or [speedscope](https://www.speedscope.app). A flamegraph can be visualized directly from
stdout using, for example,
`ImageMagick's display utility <https://imagemagick.org/script/display.php>`:

```
cd $PETSC_DIR/src/sys/tests
make ex30
mpiexec -n 2 ./ex30 -log_view ::ascii_flamegraph | flamegraph | display
```

Note that user-defined stages (see {any}`sec_profstages`) will be ignored when
using this nested format.

(sec_profileuser)=

## Profiling Application Codes

PETSc automatically logs object creation, times, and floating-point
counts for the library routines. Users can easily supplement this
information by profiling their application codes as well. The basic
steps involved in logging a user-defined portion of code, called an
*event*, are shown in the code fragment below:

```
PetscLogEvent  USER_EVENT;
PetscClassId   classid;
PetscLogDouble user_event_flops;

PetscClassIdRegister("class name",&classid);
PetscLogEventRegister("User event name",classid,&USER_EVENT);
PetscLogEventBegin(USER_EVENT,0,0,0,0);
/* code segment to monitor */
PetscLogFlops(user_event_flops);
PetscLogEventEnd(USER_EVENT,0,0,0,0);
```

One must register the event by calling `PetscLogEventRegister()`,
which assigns a unique integer to identify the event for profiling
purposes:

```
PetscLogEventRegister(const char string[],PetscClassId classid,PetscLogEvent *e);
```

Here `string` is a user-defined event name, and `color` is an
optional user-defined event color (for use with *Jumpshot* logging; see
{any}`sec_mpelogs`); one should see the manual page for
details. The argument returned in `e` should then be passed to the
`PetscLogEventBegin()` and `PetscLogEventEnd()` routines.

Events are logged by using the pair

```
PetscLogEventBegin(int event,PetscObject o1,PetscObject o2,PetscObject o3,PetscObject o4);
PetscLogEventEnd(int event,PetscObject o1,PetscObject o2,PetscObject o3,PetscObject o4);
```

The four objects are the PETSc objects that are most closely associated
with the event. For instance, in a matrix-vector product they would be
the matrix and the two vectors. These objects can be omitted by
specifying 0 for `o1` - `o4`. The code between these two routine
calls will be automatically timed and logged as part of the specified
event.

Events are collective by default on the communicator of `o1` (if present).
They can be made not collective by using `PetscLogEventSetCollective()`.
No synchronization is performed on collective events in optimized builds unless
the command line option `-log_sync` is used; however, we do check for collective
semantics in debug mode.

The user can log the number of floating-point operations for this
segment of code by calling

```
PetscLogFlops(number of flop for this code segment);
```

between the calls to `PetscLogEventBegin()` and
`PetscLogEventEnd()`. This value will automatically be added to the
global flop counter for the entire program.

(sec_profstages)=

## Profiling Multiple Sections of Code

By default, the profiling produces a single set of statistics for all
code between the `PetscInitialize()` and `PetscFinalize()` calls
within a program. One can independently monitor several "stages" of code
by switching among the various stages with the commands

```
PetscLogStagePush(PetscLogStage stage);
PetscLogStagePop();
```

see the manual pages for details.
The command

```
PetscLogStageRegister(const char *name,PetscLogStage *stage)
```

allows one to associate a name with a stage; these names are printed
whenever summaries are generated with `-log_view`. The following code fragment uses three profiling
stages within an program.

```
PetscInitialize(int *argc,char ***args,0,0);
/* stage 0 of code here */
PetscLogStageRegister("Stage 0 of Code", &stagenum0);
for (i=0; i<ntimes; i++) {
    PetscLogStageRegister("Stage 1 of Code", &stagenum1);
    PetscLogStagePush(stagenum1);
    /* stage 1 of code here */
    PetscLogStagePop();
    PetscLogStageRegister("Stage 2 of Code", &stagenum2);
    PetscLogStagePush(stagenum2);
    /* stage 2 of code here */
    PetscLogStagePop();
}
PetscFinalize();
```

The listings above
show output generated by
`-log_view` for a program that employs several profiling stages. In
particular, this program is subdivided into six stages: loading a matrix and right-hand-side vector from a binary file,
setting up the preconditioner, and solving the linear system; this
sequence is then repeated for a second linear system. For simplicity,
the second listing contains output only for
stages 5 and 6 (linear solve of the second system), which comprise the
part of this computation of most interest to us in terms of performance
monitoring. This code organization (solving a small linear system
followed by a larger system) enables generation of more accurate
profiling statistics for the second system by overcoming the often
considerable overhead of paging, as discussed in
{any}`sec_profaccuracy`.

(sec_deactivate)=

## Restricting Event Logging

By default, all PETSc operations are logged. To enable or disable the
PETSc logging of individual events, one uses the commands

```
PetscLogEventActivate(PetscLogEvent event);
PetscLogEventDeactivate(PetscLogEvent event);
```

The `event` may be either a predefined PETSc event (as listed in the
file `$PETSC_DIR/include/petsclog.h`) or one obtained with
`PetscLogEventRegister()` (as described in
{any}`sec_profileuser`).

PETSc also provides routines that deactivate (or activate) logging for
entire components of the library. Currently, the components that support
such logging (de)activation are `Mat` (matrices), `Vec` (vectors),
`KSP` (linear solvers, including `KSP` and `PC`), and `SNES`
(nonlinear solvers):

```
PetscLogEventDeactivateClass(MAT_CLASSID);
PetscLogEventDeactivateClass(KSP_CLASSID); /* includes PC and KSP */
PetscLogEventDeactivateClass(VEC_CLASSID);
PetscLogEventDeactivateClass(SNES_CLASSID);
```

and

```
PetscLogEventActivateClass(MAT_CLASSID);
PetscLogEventActivateClass(KSP_CLASSID);   /* includes PC and KSP */
PetscLogEventActivateClass(VEC_CLASSID);
PetscLogEventActivateClass(SNES_CLASSID);
```

(sec_petscinfo)=

## Interpreting `-info` Output: Informative Messages

Users can activate the printing of verbose information about algorithms,
data structures, etc. to the screen by using the option `-info` or by
calling `PetscInfoAllow(PETSC_TRUE)`. Such logging, which is used
throughout the PETSc libraries, can aid the user in understanding
algorithms and tuning program performance. For example, as discussed in
{any}`sec_matsparse`, `-info` activates the printing of
information about memory allocation during matrix assembly.

One can selectively turn off informative messages about any of the basic
PETSc objects (e.g., `Mat`, `SNES`) with the command

```
PetscInfoDeactivateClass(int object_classid)
```

where `object_classid` is one of `MAT_CLASSID`, `SNES_CLASSID`,
etc. Messages can be reactivated with the command

```
PetscInfoActivateClass(int object_classid)
```

Such deactivation can be useful when one wishes to view information
about higher-level PETSc libraries (e.g., `TS` and `SNES`) without
seeing all lower level data as well (e.g., `Mat`).

One can turn on or off logging for particular classes at runtime

```console
-info [filename][:[~]<list,of,classnames>[:[~]self]]
```

The `list,of,classnames` is a list, separated by commas with no spaces, of classes one wishes to view the information on. For
example `vec,ksp`. Information on all other classes will not be displayed. The ~ indicates to not display the list of classes but rather to display all other classes.

`self` indicates to display information on objects that are associated with `PETSC_COMM_SELF` while `~self` indicates to display information only for parallel objects.

See `PetscInfo()` for links to all the info operations that are available.

Application programmers can log their own messages, as well, by using the
routine

```
PetscInfo(void* obj,char *message,...)
```

where `obj` is the PETSc object associated most closely with the
logging statement, `message`. For example, in the line search Newton
methods, we use a statement such as

```
PetscInfo(snes,"Cubic step, lambda %g\n",lambda);
```

## Time

PETSc application programmers can access the wall clock time directly
with the command

```
PetscLogDouble time;
PetscCall(PetscTime(&time));
```

which returns the current time in seconds since the epoch, and is
commonly implemented with `MPI_Wtime`. A floating point number is
returned in order to express fractions of a second. In addition, as
discussed in {any}`sec_profileuser`, PETSc can automatically
profile user-defined segments of code.

## Saving Output to a File

All output from PETSc programs (including informative messages,
profiling information, and convergence data) can be saved to a file by
using the command line option `-history [filename]`. If no file name
is specified, the output is stored in the file
`${HOME}/.petschistory`. Note that this option only saves output
printed with the `PetscPrintf()` and `PetscFPrintf()` commands, not
the standard `printf()` and `fprintf()` statements.

(sec_profaccuracy)=

## Accurate Profiling and Paging Overheads

One factor that often plays a significant role in profiling a code is
paging by the operating system. Generally, when running a program, only
a few pages required to start it are loaded into memory rather than the
entire executable. When the execution proceeds to code segments that are
not in memory, a pagefault occurs, prompting the required pages to be
loaded from the disk (a very slow process). This activity distorts the
results significantly. (The paging effects are noticeable in the log
files generated by `-log_mpe`, which is described in
{any}`sec_mpelogs`.)

To eliminate the effects of paging when profiling the performance of a
program, we have found an effective procedure is to run the *exact same
code* on a small dummy problem before running it on the actual problem
of interest. We thus ensure that all code required by a solver is loaded
into memory during solution of the small problem. When the code proceeds
to the actual (larger) problem of interest, all required pages have
already been loaded into main memory, so that the performance numbers
are not distorted.

When this procedure is used in conjunction with the user-defined stages
of profiling described in {any}`sec_profstages`, we can
focus easily on the problem of interest. For example, we used this
technique in the program
<a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ksp/ksp/tutorials/ex10.c.html">KSP Tutorial ex10</a>
to generate the timings within
{any}`listing <listing_exparprof>` and {any}`listing <listing_exparprof2>`.
In this case, the profiled code
of interest (solving the linear system for the larger problem) occurs
within event stages 5 and 6. {any}`sec_parperformance`
provides details about interpreting such profiling data.

In particular, the macros

```
PetscPreLoadBegin(PetscBool flag,char* stagename)
PetscPreLoadStage(char *stagename)
```

and

```
PetscPreLoadEnd()
```

can be used to easily convert a regular PETSc program to one that uses
preloading. The command line options `-preload` `true` and
`-preload` `false` may be used to turn on and off preloading at run
time for PETSc programs that use these macros.

## NVIDIA Nsight Systems profiling

Nsight Systems will generate profiling data with a CUDA executable
with the command `nsys`.
For example, in serial

```bash
nsys profile -t nvtx,cuda -o file --stats=true --force-overwrite true ./a.out
```

will generate a file `file.qdstrm` with performance data that is
annotated with PETSc events (methods) and Kokkos device kernel names.
The Nsight Systems GUI, `nsys-ui`, can be used to navigate this file
(<https://developer.nvidia.com/nsight-systems>). The Nsight Systems GUI
lets you see a timeline of code performance information like kernels,
memory mallocs and frees, CPU-GPU communication, and high-level data like time, sizes
of memory copies, and more, in a popup window when the mouse
hovers over the section.
To view the data, start `nsys-ui` without any arguments and then `Import` the
`.qdstrm` file in the GUI.
A side effect of this viewing process is the generation of a file `file.nsys-rep`, which can be viewed directly
with `nsys-ui` in the future.

For an MPI parallel job, only one process can call `nsys`,
say have rank zero output `nsys` data and have all other
ranks call the executable directly. For example with MPICH
or Open MPI - we can run a parallel job on 4 MPI tasks as:

```console
mpiexec -n 1 nsys profile -t nvtx,cuda -o file_name --stats=true --force-overwrite true ./a.out : -n 3 ./a.out
```

(sec_using_tau)=

Note: The Nsight GUI can open profiling reports from elsewhere. For
example, a report from a compute node can be analyzed on your local
machine, but care should be taken to use the exact same versions of
Nsight Systems that generated the report.
To check the version of Nsight on the compute node run `nsys-ui` and
note the version number at the top of the window.

## ROCProfiler profiling

For AMD GPUs, log events registered to PETSc can be displayed as ranges in trace files generated by `rocprof` by running with the `-log_roctx` flag.
See the `rocprof` [documentation](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/latest) for details of how to run the profiler.
At the least, you will need:

```console
mpiexec -n 1 rocprofv3 --marker-trace -o file_name -- ./path/to/application -log_roctx
```


## Using TAU

TAU profiles can be generated without the need for instrumentation through the
use of the perfstubs package. PETSc by default is configured with `--with-tau-perfstubs`.
To generate profiles with TAU, first setup TAU:

```bash
wget http://tau.uoregon.edu/tau.tgz
./configure -cc=mpicc -c++=mpicxx -mpi -bfd=download -unwind=download && make install
export PATH=<tau dir>/x86_64/bin:$PATH
```

For more information on configuring TAU, see [http://tau.uoregon.edu](http://tau.uoregon.edu).
Next, run your program with TAU. For instance, to profile `ex56`,

```bash
cd $PETSC_DIR/src/snes/tutorials
make ex56
mpirun -n 4 tau_exec -T mpi ./ex56 -log_perfstubs <args>
```

This should produce four `profile.*` files with profile data that can be
viewed with `paraprof/pprof`:

```shell
Reading Profile files in profile.*

NODE 0;CONTEXT 0;THREAD 0:
---------------------------------------------------------------------------------------
%Time    Exclusive    Inclusive       #Call      #Subrs  Inclusive Name
              msec   total msec                          usec/call
---------------------------------------------------------------------------------------
100.0           26        1,838           1       41322    1838424 .TAU application
 73.2            1        1,345           2         168     672950 SNESSolve
 62.2            3        1,142           2        1282     571442 SNESJacobianEval
 62.0        1,136        1,138           2          76     569494 DMPlexJacobianFE
 60.1        0.046        1,105           1          32    1105001 Solve 1
 15.2           87          279           5       11102      55943 Mesh Setup
 13.2        0.315          241           1          32     241765 Solve 0
  7.8           80          144       38785       38785          4 MPI_Allreduce()
  7.0           69          128           6       43386      21491 DualSpaceSetUp
  6.2            1          114           4          54      28536 PCSetUp
  6.0           12          110           2         892      55407 PCSetUp_GAMG+
  3.9           70           70           1           0      70888 MPI_Init_thread()
  3.7           68           68       41747           0          2 MPI Collective Sync
  3.6            8           66           4        3536      16548 SNESFunctionEval
  2.6           45           48         171         171        281 MPI_Bcast()
  1.9           34           34        7836           0          4 MPI_Barrier()
  1.8        0.567           33           2          68      16912  GAMG Coarsen
```

```{raw} html
<hr>
```

```{eval-rst}
.. bibliography:: /petsc.bib
   :filter: docname in docnames
```
