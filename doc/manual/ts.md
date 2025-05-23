(ch_ts)=

# TS: Scalable ODE and DAE Solvers

The `TS` library provides a framework for the scalable solution of
ODEs and DAEs arising from the discretization of time-dependent PDEs.

**Simple Example:** Consider the PDE

$$
u_t = u_{xx}
$$

discretized with centered finite differences in space yielding the
semi-discrete equation

$$
\begin{aligned}
          (u_i)_t & =  & \frac{u_{i+1} - 2 u_{i} + u_{i-1}}{h^2}, \\
           u_t      &  = & \tilde{A} u;\end{aligned}
$$

or with piecewise linear finite elements approximation in space
$u(x,t) \doteq \sum_i \xi_i(t) \phi_i(x)$ yielding the
semi-discrete equation

$$
B {\xi}'(t) = A \xi(t)
$$

Now applying the backward Euler method results in

$$
( B - dt^n A  ) u^{n+1} = B u^n,
$$

in which

$$
{u^n}_i = \xi_i(t_n) \doteq u(x_i,t_n),
$$

$$
{\xi}'(t_{n+1}) \doteq \frac{{u^{n+1}}_i - {u^{n}}_i }{dt^{n}},
$$

$A$ is the stiffness matrix, and $B$ is the identity for
finite differences or the mass matrix for the finite element method.

The PETSc interface for solving time dependent problems assumes the
problem is written in the form

$$
F(t,u,\dot{u}) = G(t,u), \quad u(t_0) = u_0.
$$

In general, this is a differential algebraic equation (DAE) [^id5]. For
ODE with nontrivial mass matrices such as arise in FEM, the implicit/DAE
interface significantly reduces overhead to prepare the system for
algebraic solvers (`SNES`/`KSP`) by having the user assemble the
correctly shifted matrix. Therefore this interface is also useful for
ODE systems.

To solve an ODE or DAE one uses:

- Function $F(t,u,\dot{u})$

  ```
  TSSetIFunction(TS ts,Vec R,PetscErrorCode (*f)(TS,PetscReal,Vec,Vec,Vec,void*),void *funP);
  ```

  The vector `R` is an optional location to store the residual. The
  arguments to the function `f()` are the timestep context, current
  time, input state $u$, input time derivative $\dot{u}$,
  and the (optional) user-provided context `funP`. If
  $F(t,u,\dot{u}) = \dot{u}$ then one need not call this
  function.

- Function $G(t,u)$, if it is nonzero, is provided with the
  function

  ```
  TSSetRHSFunction(TS ts,Vec R,PetscErrorCode (*f)(TS,PetscReal,Vec,Vec,void*),void *funP);
  ```

- Jacobian


  $\sigma F_{\dot{u}}(t^n,u^n,\dot{u}^n) + F_u(t^n,u^n,\dot{u}^n)$

  If using a fully implicit or semi-implicit (IMEX) method one also
  can provide an appropriate (approximate) Jacobian matrix of


  $F()$

  .

  ```
  TSSetIJacobian(TS ts,Mat A,Mat B,PetscErrorCode (*fjac)(TS,PetscReal,Vec,Vec,PetscReal,Mat,Mat,void*),void *jacP);
  ```

  The arguments for the function `fjac()` are the timestep context,
  current time, input state $u$, input derivative
  $\dot{u}$, input shift $\sigma$, matrix $A$,
  matrix used to construct the preconditioner $B$, and the (optional) user-provided
  context `jacP`.

  The Jacobian needed for the nonlinear system is, by the chain rule,

  $$
  \begin{aligned}
      \frac{d F}{d u^n} &  = &  \frac{\partial F}{\partial \dot{u}}|_{u^n} \frac{\partial \dot{u}}{\partial u}|_{u^n} + \frac{\partial F}{\partial u}|_{u^n}.\end{aligned}
  $$

  For any ODE integration method the approximation of $\dot{u}$
  is linear in $u^n$ hence
  $\frac{\partial \dot{u}}{\partial u}|_{u^n} = \sigma$, where
  the shift $\sigma$ depends on the ODE integrator and time step
  but not on the function being integrated. Thus

  $$
  \begin{aligned}
      \frac{d F}{d u^n} &  = &    \sigma F_{\dot{u}}(t^n,u^n,\dot{u}^n) + F_u(t^n,u^n,\dot{u}^n).\end{aligned}
  $$

  This explains why the user provide Jacobian is in the given form for
  all integration methods. An equivalent way to derive the formula is
  to note that

  $$
  F(t^n,u^n,\dot{u}^n) = F(t^n,u^n,w+\sigma*u^n)
  $$

  where $w$ is some linear combination of previous time solutions
  of $u$ so that

  $$
  \frac{d F}{d u^n} = \sigma F_{\dot{u}}(t^n,u^n,\dot{u}^n) + F_u(t^n,u^n,\dot{u}^n)
  $$

  again by the chain rule.

  For example, consider backward Euler’s method applied to the ODE
  $F(t, u, \dot{u}) = \dot{u} - f(t, u)$ with
  $\dot{u} = (u^n - u^{n-1})/\delta t$ and
  $\frac{\partial \dot{u}}{\partial u}|_{u^n} = 1/\delta t$
  resulting in

  $$
  \begin{aligned}
      \frac{d F}{d u^n} & = &   (1/\delta t)F_{\dot{u}} + F_u(t^n,u^n,\dot{u}^n).\end{aligned}
  $$

  But $F_{\dot{u}} = 1$, in this special case, resulting in the
  expected Jacobian $I/\delta t - f_u(t,u^n)$.

- Jacobian 

  $G_u$

  If using a fully implicit method and the function 

  $G()$

   is
  provided, one also can provide an appropriate (approximate)
  Jacobian matrix of 

  $G()$

  .

  ```
  TSSetRHSJacobian(TS ts,Mat A,Mat B,
  PetscErrorCode (*fjac)(TS,PetscReal,Vec,Mat,Mat,void*),void *jacP);
  ```

  The arguments for the function `fjac()` are the timestep context,
  current time, input state $u$, matrix $A$,
  matrix used to construct the preconditioner $B$, and the (optional) user-provided
  context `jacP`.

Providing appropriate $F()$ and $G()$ for your problem
allows for the easy runtime switching between explicit, semi-implicit
(IMEX), and fully implicit methods.

(sec_ts_basic)=

## Basic TS Options

The user first creates a `TS` object with the command

```
int TSCreate(MPI_Comm comm,TS *ts);
```

```
int TSSetProblemType(TS ts,TSProblemType problemtype);
```

The `TSProblemType` is one of `TS_LINEAR` or `TS_NONLINEAR`.

To set up `TS` for solving an ODE, one must set the “initial
conditions” for the ODE with

```
TSSetSolution(TS ts, Vec initialsolution);
```

One can set the solution method with the routine

```
TSSetType(TS ts,TSType type);
```

Some of the currently supported types are `TSEULER`, `TSRK` (Runge-Kutta), `TSBEULER`, `TSCN` (Crank-Nicolson), `TSTHETA`, `TSGLLE` (generalized linear), and `TSPSEUDO`.
They can also be set with the options database option `-ts_type euler, rk, beuler, cn, theta, gl, pseudo, sundials, eimex, arkimex, rosw`.
A list of available methods is given in {any}`integrator_table`.

Set the initial time with the command

```
TSSetTime(TS ts,PetscReal time);
```

One can change the timestep with the command

```
TSSetTimeStep(TS ts,PetscReal dt);
```

can determine the current timestep with the routine

```
TSGetTimeStep(TS ts,PetscReal* dt);
```

Here, “current” refers to the timestep being used to attempt to promote
the solution form $u^n$ to $u^{n+1}.$

One sets the total number of timesteps to run or the total time to run
(whatever is first) with the commands

```
TSSetMaxSteps(TS ts,PetscInt maxsteps);
TSSetMaxTime(TS ts,PetscReal maxtime);
```

and determines the behavior near the final time with

```
TSSetExactFinalTime(TS ts,TSExactFinalTimeOption eftopt);
```

where `eftopt` is one of
`TS_EXACTFINALTIME_STEPOVER`,`TS_EXACTFINALTIME_INTERPOLATE`, or
`TS_EXACTFINALTIME_MATCHSTEP`. One performs the requested number of
time steps with

```
TSSolve(TS ts,Vec U);
```

The solve call implicitly sets up the timestep context; this can be done
explicitly with

```
TSSetUp(TS ts);
```

One destroys the context with

```
TSDestroy(TS *ts);
```

and views it with

```
TSView(TS ts,PetscViewer viewer);
```

In place of `TSSolve()`, a single step can be taken using

```
TSStep(TS ts);
```

(sec_imex)=

## DAE Formulations

You can find a discussion of DAEs in {cite}`ascherpetzold1998` or [Scholarpedia](http://www.scholarpedia.org/article/Differential-algebraic_equations). In PETSc, TS deals with the semi-discrete form of the equations, so that space has already been discretized. If the DAE depends explicitly on the coordinate $x$, then this will just appear as any other data for the equation, not as an explicit argument. Thus we have

$$
F(t, u, \dot{u}) = 0
$$

In this form, only fully implicit solvers are appropriate. However, specialized solvers for restricted forms of DAE are supported by PETSc. Below we consider an ODE which is augmented with algebraic constraints on the variables.

### Hessenberg Index-1 DAE

> This is a Semi-Explicit Index-1 DAE which has the form

$$
\begin{aligned}
  \dot{u} &= f(t, u, z) \\
        0 &= h(t, u, z)
\end{aligned}
$$

where $z$ is a new constraint variable, and the Jacobian $\frac{dh}{dz}$ is non-singular everywhere. We have suppressed the $x$ dependence since it plays no role here. Using the non-singularity of the Jacobian and the Implicit Function Theorem, we can solve for $z$ in terms of $u$. This means we could, in principle, plug $z(u)$ into the first equation to obtain a simple ODE, even if this is not the numerical process we use. Below we show that this type of DAE can be used with IMEX schemes.

### Hessenberg Index-2 DAE

> This DAE has the form

$$
\begin{aligned}
  \dot{u} &= f(t, u, z) \\
        0 &= h(t, u)
\end{aligned}
$$

Notice that the constraint equation $h$ is not a function of the constraint variable $z$. This means that we cannot naively invert as we did in the index-1 case. Our strategy will be to convert this into an index-1 DAE using a time derivative, which loosely corresponds to the idea of an index being the number of derivatives necessary to get back to an ODE. If we differentiate the constraint equation with respect to time, we can use the ODE to simplify it,

$$
\begin{aligned}
        0 &= \dot{h}(t, u) \\
          &= \frac{dh}{du} \dot{u} + \frac{\partial h}{\partial t} \\
          &= \frac{dh}{du} f(t, u, z) + \frac{\partial h}{\partial t}
\end{aligned}
$$

If the Jacobian $\frac{dh}{du} \frac{df}{dz}$ is non-singular, then we have precisely a semi-explicit index-1 DAE, and we can once again use the PETSc IMEX tools to solve it. A common example of an index-2 DAE is the incompressible Navier-Stokes equations, since the continuity equation $\nabla\cdot u = 0$ does not involve the pressure. Using PETSc IMEX with the above conversion then corresponds to the Segregated Runge-Kutta method applied to this equation {cite}`colomesbadia2016`.

## Using Implicit-Explicit (IMEX) Methods

For “stiff” problems or those with multiple time scales $F()$ will
be treated implicitly using a method suitable for stiff problems and
$G()$ will be treated explicitly when using an IMEX method like
TSARKIMEX. $F()$ is typically linear or weakly nonlinear while
$G()$ may have very strong nonlinearities such as arise in
non-oscillatory methods for hyperbolic PDE. The user provides three
pieces of information, the APIs for which have been described above.

- “Slow” part $G(t,u)$ using `TSSetRHSFunction()`.
- “Stiff” part $F(t,u,\dot u)$ using `TSSetIFunction()`.
- Jacobian $F_u + \sigma F_{\dot u}$ using `TSSetIJacobian()`.

The user needs to set `TSSetEquationType()` to `TS_EQ_IMPLICIT` or
higher if the problem is implicit; e.g.,
$F(t,u,\dot u) = M \dot u - f(t,u)$, where $M$ is not the
identity matrix:

- the problem is an implicit ODE (defined implicitly through
  `TSSetIFunction()`) or
- a DAE is being solved.

An IMEX problem representation can be made implicit by setting `TSARKIMEXSetFullyImplicit()`.
Note that multilevel preconditioners (e.g. `PCMG`), won't work in the fully implicit case; the
same holds true for any other `TS` type requiring a fully implicit formulation in case both
Jacobians are specified.

In PETSc, DAEs and ODEs are formulated as $F(t,u,\dot{u})=G(t,u)$, where $F()$ is meant to be integrated implicitly and $G()$ explicitly. An IMEX formulation such as $M\dot{u}=f(t,u)+g(t,u)$ requires the user to provide $M^{-1} g(t,u)$ or solve $g(t,u) - M x=0$ in place of $G(t,u)$. General cases such as $F(t,u,\dot{u})=G(t,u)$ are not amenable to IMEX Runge-Kutta, but can be solved by using fully implicit methods. Some use-case examples for `TSARKIMEX` are listed in {numref}`tab_DE_forms` and a list of methods with a summary of their properties is given in {any}`tab_IMEX_RK_PETSc`.

```{eval-rst}
.. list-table:: Use case examples for ``TSARKIMEX``
   :name: tab_DE_forms
   :widths: 40 40 80

   * - :math:`\dot{u} = g(t,u)`
     - nonstiff ODE
     - :math:`\begin{aligned}F(t,u,\dot{u}) &= \dot{u} \\ G(t,u) &= g(t,u)\end{aligned}`
   * - :math:`M \dot{u} = g(t,u)`
     - nonstiff ODE with mass matrix
     - :math:`\begin{aligned}F(t,u,\dot{u}) &= \dot{u} \\ G(t,u) &= M^{-1} g(t,u)\end{aligned}`
   * - :math:`\dot{u} = f(t,u)`
     - stiff ODE
     - :math:`\begin{aligned}F(t,u,\dot{u}) &= \dot{u} - f(t,u) \\ G(t,u) &= 0\end{aligned}`
   * - :math:`M \dot{u} = f(t,u)`
     - stiff ODE with mass matrix
     - :math:`\begin{aligned}F(t,u,\dot{u}) &= M \dot{u} - f(t,u) \\ G(t,u) &= 0\end{aligned}`
   * - :math:`\dot{u} = f(t,u) + g(t,u)`
     - stiff-nonstiff ODE
     - :math:`\begin{aligned}F(t,u,\dot{u}) &= \dot{u} - f(t,u) \\ G(t,u) &= g(t,u)\end{aligned}`
   * - :math:`M \dot{u} = f(t,u) + g(t,u)`
     - stiff-nonstiff ODE with mass matrix
     - :math:`\begin{aligned}F(t,u,\dot{u}) &= M\dot{u} - f(t,u) \\ G(t,u) &= M^{-1} g(t,u)\end{aligned}`
   * - :math:`\begin{aligned}\dot{u} &= f(t,u,z) + g(t,u,z)\\0 &= h(t,y,z)\end{aligned}`
     - semi-explicit index-1 DAE
     - :math:`\begin{aligned}F(t,u,\dot{u}) &= \begin{pmatrix}\dot{u} - f(t,u,z)\\h(t, u, z)\end{pmatrix}\\G(t,u) &= g(t,u)\end{aligned}`
   * - :math:`f(t,u,\dot{u})=0`
     - fully implicit ODE/DAE
     - :math:`\begin{aligned}F(t,u,\dot{u}) &= f(t,u,\dot{u})\\G(t,u) &= 0\end{aligned}`; the user needs to set ``TSSetEquationType()`` to ``TS_EQ_IMPLICIT`` or higher
```

{numref}`tab_IMEX_RK_PETSc` lists of the currently available IMEX Runge-Kutta schemes. For each method, it gives the `-ts_arkimex_type` name, the reference, the total number of stages/implicit stages, the order/stage-order, the implicit stability properties (IM), stiff accuracy (SA), the existence of an embedded scheme, and dense output (DO).

```{eval-rst}
.. list-table:: IMEX Runge-Kutta schemes
  :name: tab_IMEX_RK_PETSc
  :header-rows: 1

  * - Name
    - Reference
    - Stages (IM)
    - Order (Stage)
    - IM
    - SA
    - Embed
    - DO
    - Remarks
  * - a2
    - based on CN
    - 2 (1)
    - 2 (2)
    - A-Stable
    - yes
    - yes (1)
    - yes (2)
    -
  * - l2
    - SSP2(2,2,2) :cite:`pareschi_2005`
    - 2 (2)
    - 2 (1)
    - L-Stable
    - yes
    - yes (1)
    - yes (2)
    - SSP SDIRK
  * - ars122
    - ARS122 :cite:`ascher_1997`
    - 2 (1)
    - 3 (1)
    - A-Stable
    - yes
    - yes (1)
    - yes (2)
    -
  * - 2c
    - :cite:`giraldo_2013`
    - 3 (2)
    - 2 (2)
    - L-Stable
    - yes
    - yes (1)
    - yes (2)
    - SDIRK
  * - 2d
    - :cite:`giraldo_2013`
    - 3 (2)
    - 2 (2)
    - L-Stable
    - yes
    - yes (1)
    - yes (2)
    - SDIRK
  * -  2e
    - :cite:`giraldo_2013`
    - 3 (2)
    - 2 (2)
    - L-Stable
    - yes
    - yes (1)
    - yes (2)
    - SDIRK
  * - prssp2
    - PRS(3,3,2) :cite:`pareschi_2005`
    - 3 (3)
    - 3 (1)
    - L-Stable
    - yes
    - no
    - no
    - SSP
  * - 3
    - :cite:`kennedy_2003`
    - 4 (3)
    - 3 (2)
    - L-Stable
    - yes
    - yes (2)
    - yes (2)
    - SDIRK
  * - bpr3
    - :cite:`boscarino_tr2011`
    - 5 (4)
    - 3 (2)
    - L-Stable
    - yes
    - no
    - no
    - SDIRK
  * - ars443
    - :cite:`ascher_1997`
    - 5 (4)
    - 3 (1)
    - L-Stable
    - yes
    - no
    - no
    - SDIRK
  * - 4
    - :cite:`kennedy_2003`
    - 6 (5)
    - 4 (2)
    - L-Stable
    - yes
    - yes (3)
    - yes
    - SDIRK
  * - 5
    - :cite:`kennedy_2003`
    - 8 (7)
    - 5 (2)
    - L-Stable
    - yes
    - yes (4)
    - yes (3)
    - SDIRK
```

ROSW are linearized implicit Runge-Kutta methods known as Rosenbrock
W-methods. They can accommodate inexact Jacobian matrices in their
formulation. A series of methods are available in PETSc are listed in
{numref}`tab_IMEX_RosW_PETSc` below. For each method, it gives the reference, the total number of stages and implicit stages, the scheme order and stage order, the implicit stability properties (IM), stiff accuracy (SA), the existence of an embedded scheme, dense output (DO), the capacity to use inexact Jacobian matrices (-W), and high order integration of differential algebraic equations (PDAE).

```{eval-rst}
.. list-table:: Rosenbrock W-schemes
   :name: tab_IMEX_RosW_PETSc
   :header-rows: 1

   * - TS
     - Reference
     - Stages (IM)
     - Order (Stage)
     - IM
     - SA
     - Embed
     - DO
     - -W
     - PDAE
     - Remarks
   * - theta1
     - classical
     - 1(1)
     - 1(1)
     - L-Stable
     - -
     - -
     - -
     - -
     - -
     - -
   * - theta2
     - classical
     - 1(1)
     - 2(2)
     - A-Stable
     - -
     - -
     - -
     - -
     - -
     - -
   * - 2m
     - Zoltan
     - 2(2)
     - 2(1)
     - L-Stable
     - No
     - Yes(1)
     - Yes(2)
     - Yes
     - No
     - SSP
   * - 2p
     - Zoltan
     - 2(2)
     - 2(1)
     - L-Stable
     - No
     - Yes(1)
     - Yes(2)
     - Yes
     - No
     - SSP
   * - ra3pw
     - :cite:`rang_2005`
     - 3(3)
     - 3(1)
     - A-Stable
     - No
     - Yes
     - Yes(2)
     - No
     - Yes(3)
     - -
   * - ra34pw2
     - :cite:`rang_2005`
     - 4(4)
     - 3(1)
     - L-Stable
     - Yes
     - Yes
     - Yes(3)
     - Yes
     - Yes(3)
     - -
   * - rodas3
     - :cite:`sandu_1997`
     - 4(4)
     - 3(1)
     - L-Stable
     - Yes
     - Yes
     - No
     - No
     - Yes
     - -
   * - sandu3
     - :cite:`sandu_1997`
     - 3(3)
     - 3(1)
     - L-Stable
     - Yes
     - Yes
     - Yes(2)
     - No
     - No
     - -
   * - assp3p3s1c
     - unpub.
     - 3(2)
     - 3(1)
     - A-Stable
     - No
     - Yes
     - Yes(2)
     - Yes
     - No
     - SSP
   * - lassp3p4s2c
     - unpub.
     - 4(3)
     - 3(1)
     - L-Stable
     - No
     - Yes
     - Yes(3)
     - Yes
     - No
     - SSP
   * - lassp3p4s2c
     - unpub.
     - 4(3)
     - 3(1)
     - L-Stable
     - No
     - Yes
     - Yes(3)
     - Yes
     - No
     - SSP
   * - ark3
     - unpub.
     - 4(3)
     - 3(1)
     - L-Stable
     - No
     - Yes
     - Yes(3)
     - Yes
     - No
     - IMEX-RK
```

## IMEX Methods for fast-slow systems

Consider a fast-slow ODE system

$$
\begin{aligned}
\dot{u}^{slow} & = f^{slow}(t, u^{slow},u^{fast}) \\
M \dot{u}^{fast} & = g^{fast}(t, u^{slow},u^{fast}) + f^{fast}(t, u^{slow},u^{fast})
\end{aligned}
$$

where $u^{slow}$ is the slow component and $u^{fast}$ is the
fast component. The fast component can be partitioned additively as
described above. Thus we want to treat $f^{slow}()$ and
$f^{fast}()$ explicitly and the other terms implicitly when using
TSARKIMEX. This is achieved by using the following APIs:

- `TSARKIMEXSetFastSlowSplit()` informs PETSc to use ARKIMEX to solve a fast-slow system.
- `TSRHSSplitSetIS()` specifies the index set for the slow/fast components.
- `TSRHSSplitSetRHSFunction()` specifies the parts to be handled explicitly $f^{slow}()$ and $f^{fast}()$.
- `TSRHSSplitSetIFunction()` and `TSRHSSplitSetIJacobian()` specify the implicit part and its Jacobian.

Note that this ODE system can also be solved by padding zeros in the implicit part and using the standard IMEX methods. However, one needs to provide the full-dimensional Jacobian whereas only a partial Jacobian is needed for the fast-slow split which is more efficient in storage and speed.

## GLEE methods

In this section, we describe explicit and implicit time stepping methods
with global error estimation that are introduced in
{cite}`constantinescu_tr2016b`. The solution vector for a
GLEE method is either \[$y$, $\tilde{y}$\] or
\[$y$,$\varepsilon$\], where $y$ is the solution,
$\tilde{y}$ is the “auxiliary solution,” and $\varepsilon$
is the error. The working vector that `TSGLEE` uses is $Y$ =
\[$y$,$\tilde{y}$\], or \[$y$,$\varepsilon$\]. A
GLEE method is defined by

- $(p,r,s)$: (order, steps, and stages),
- $\gamma$: factor representing the global error ratio,
- $A, U, B, V$: method coefficients,
- $S$: starting method to compute the working vector from the
  solution (say at the beginning of time integration) so that
  $Y = Sy$,
- $F$: finalizing method to compute the solution from the working
  vector,$y = FY$.
- $F_\text{embed}$: coefficients for computing the auxiliary
  solution $\tilde{y}$ from the working vector
  ($\tilde{y} = F_\text{embed} Y$),
- $F_\text{error}$: coefficients to compute the estimated error
  vector from the working vector
  ($\varepsilon = F_\text{error} Y$).
- $S_\text{error}$: coefficients to initialize the auxiliary
  solution ($\tilde{y}$ or $\varepsilon$) from a specified
  error vector ($\varepsilon$). It is currently implemented only
  for $r = 2$. We have $y_\text{aux} =
  S_{error}[0]*\varepsilon + S_\text{error}[1]*y$, where
  $y_\text{aux}$ is the 2nd component of the working vector
  $Y$.

The methods can be described in two mathematically equivalent forms:
propagate two components (“$y\tilde{y}$ form”) and propagating the
solution and its estimated error (“$y\varepsilon$ form”). The two
forms are not explicitly specified in `TSGLEE`; rather, the specific
values of $B, U, S, F, F_{embed}$, and $F_{error}$
characterize whether the method is in $y\tilde{y}$ or
$y\varepsilon$ form.

The API used by this `TS` method includes:

- `TSGetSolutionComponents`: Get all the solution components of the
  working vector

  ```
  ierr = TSGetSolutionComponents(TS,int*,Vec*)
  ```

  Call with `NULL` as the last argument to get the total number of
  components in the working vector $Y$ (this is $r$ (not
  $r-1$)), then call to get the $i$-th solution component.

- `TSGetAuxSolution`: Returns the auxiliary solution
  $\tilde{y}$ (computed as $F_\text{embed} Y$)

  ```
  ierr = TSGetAuxSolution(TS,Vec*)
  ```

- `TSGetTimeError`: Returns the estimated error vector
  $\varepsilon$ (computed as $F_\text{error} Y$ if
  $n=0$ or restores the error estimate at the end of the previous
  step if $n=-1$)

  ```
  ierr = TSGetTimeError(TS,PetscInt n,Vec*)
  ```

- `TSSetTimeError`: Initializes the auxiliary solution
  ($\tilde{y}$ or $\varepsilon$) for a specified initial
  error.

  ```
  ierr = TSSetTimeError(TS,Vec)
  ```

The local error is estimated as $\varepsilon(n+1)-\varepsilon(n)$.
This is to be used in the error control. The error in $y\tilde{y}$
GLEE is
$\varepsilon(n) = \frac{1}{1-\gamma} * (\tilde{y}(n) - y(n))$.

Note that $y$ and $\tilde{y}$ are reported to `TSAdapt`
`basic` (`TSADAPTBASIC`), and thus it computes the local error as
$\varepsilon_{loc} = (\tilde{y} -
y)$. However, the actual local error is $\varepsilon_{loc}
= \varepsilon_{n+1} - \varepsilon_n = \frac{1}{1-\gamma} * [(\tilde{y} -
y)_{n+1} - (\tilde{y} - y)_n]$.

{numref}`tab_IMEX_GLEE_PETSc` lists currently available GL schemes with global error estimation {cite}`constantinescu_tr2016b`.

```{eval-rst}
.. list-table:: GL schemes with global error estimation
   :name: tab_IMEX_GLEE_PETSc
   :header-rows: 1

   * - TS
     - Reference
     - IM/EX
     - :math:`(p,r,s)`
     - :math:`\gamma`
     - Form
     - Notes
   * - ``TSGLEEi1``
     - ``BE1``
     - IM
     - :math:`(1,3,2)`
     - :math:`0.5`
     - :math:`y\varepsilon`
     - Based on backward Euler
   * - ``TSGLEE23``
     - ``23``
     - EX
     - :math:`(2,3,2)`
     - :math:`0`
     - :math:`y\varepsilon`
     -
   * - ``TSGLEE24``
     - ``24``
     - EX
     - :math:`(2,4,2)`
     - :math:`0`
     - :math:`y\tilde{y}`
     -
   * - ``TSGLEE25I``
     - ``25i``
     - EX
     - :math:`(2,5,2)`
     - :math:`0`
     - :math:`y\tilde{y}`
     -
   * - ``TSGLEE35``
     - ``35``
     - EX
     - :math:`(3,5,2)`
     - :math:`0`
     - :math:`y\tilde{y}`
     -
   * - ``TSGLEEEXRK2A``
     - ``exrk2a``
     - EX
     - :math:`(2,6,2)`
     - :math:`0.25`
     - :math:`y\varepsilon`
     -
   * - ``TSGLEERK32G1``
     - ``rk32g1``
     - EX
     - :math:`(3,8,2)`
     - :math:`0`
     - :math:`y\varepsilon`
     -
   * - ``TSGLEERK285EX``
     - ``rk285ex``
     - EX
     - :math:`(2,9,2)`
     - :math:`0.25`
     - :math:`y\varepsilon`
     -
```

## Using fully implicit methods

To use a fully implicit method like `TSTHETA`, `TSBDF` or `TSDIRK`, either
provide the Jacobian of $F()$ (and $G()$ if $G()$ is
provided) or use a `DM` that provides a coloring so the Jacobian can
be computed efficiently via finite differences.

## Using the Explicit Runge-Kutta timestepper with variable timesteps

The explicit Euler and Runge-Kutta methods require the ODE be in the
form

$$
\dot{u} = G(u,t).
$$

The user can either call `TSSetRHSFunction()` and/or they can call
`TSSetIFunction()` (so long as the function provided to
`TSSetIFunction()` is equivalent to $\dot{u} + \tilde{F}(t,u)$)
but the Jacobians need not be provided. [^id6]

The Explicit Runge-Kutta timestepper with variable timesteps is an
implementation of the standard Runge-Kutta with an embedded method. The
error in each timestep is calculated using the solutions from the
Runge-Kutta method and its embedded method (the 2-norm of the difference
is used). The default method is the $3$rd-order Bogacki-Shampine
method with a $2$nd-order embedded method (`TSRK3BS`). Other
available methods are the $5$th-order Fehlberg RK scheme with a
$4$th-order embedded method (`TSRK5F`), the
$5$th-order Dormand-Prince RK scheme with a $4$th-order
embedded method (`TSRK5DP`), the $5$th-order Bogacki-Shampine
RK scheme with a $4$th-order embedded method (`TSRK5BS`, and
the $6$th-, $7$th, and $8$th-order robust Verner
RK schemes with a $5$th-, $6$th, and $7$th-order
embedded method, respectively (`TSRK6VR`, `TSRK7VR`, `TSRK8VR`).
Variable timesteps cannot be used with RK schemes that do not have an
embedded method (`TSRK1FE` - $1$st-order, $1$-stage
forward Euler, `TSRK2A` - $2$nd-order, $2$-stage RK
scheme, `TSRK3` - $3$rd-order, $3$-stage RK scheme,
`TSRK4` - $4$-th order, $4$-stage RK scheme).

## Special Cases

- $\dot{u} = A u.$ First compute the matrix $A$ then call

  ```
  TSSetProblemType(ts,TS_LINEAR);
  TSSetRHSFunction(ts,NULL,TSComputeRHSFunctionLinear,NULL);
  TSSetRHSJacobian(ts,A,A,TSComputeRHSJacobianConstant,NULL);
  ```

  or

  ```
  TSSetProblemType(ts,TS_LINEAR);
  TSSetIFunction(ts,NULL,TSComputeIFunctionLinear,NULL);
  TSSetIJacobian(ts,A,A,TSComputeIJacobianConstant,NULL);
  ```

- $\dot{u} = A(t) u.$ Use

  ```
  TSSetProblemType(ts,TS_LINEAR);
  TSSetRHSFunction(ts,NULL,TSComputeRHSFunctionLinear,NULL);
  TSSetRHSJacobian(ts,A,A,YourComputeRHSJacobian, &appctx);
  ```

  where `YourComputeRHSJacobian()` is a function you provide that
  computes $A$ as a function of time. Or use

  ```
  TSSetProblemType(ts,TS_LINEAR);
  TSSetIFunction(ts,NULL,TSComputeIFunctionLinear,NULL);
  TSSetIJacobian(ts,A,A,YourComputeIJacobian, &appctx);
  ```

## Monitoring and visualizing solutions

- `-ts_monitor` - prints the time and timestep at each iteration.
- `-ts_adapt_monitor` - prints information about the timestep
  adaption calculation at each iteration.
- `-ts_monitor_lg_timestep` - plots the size of each timestep,
  `TSMonitorLGTimeStep()`.
- `-ts_monitor_lg_solution` - for ODEs with only a few components
  (not arising from the discretization of a PDE) plots the solution as
  a function of time, `TSMonitorLGSolution()`.
- `-ts_monitor_lg_error` - for ODEs with only a few components plots
  the error as a function of time, only if `TSSetSolutionFunction()`
  is provided, `TSMonitorLGError()`.
- `-ts_monitor_draw_solution` - plots the solution at each iteration,
  `TSMonitorDrawSolution()`.
- `-ts_monitor_draw_error` - plots the error at each iteration only
  if `TSSetSolutionFunction()` is provided,
  `TSMonitorDrawSolution()`.
- `-ts_monitor_solution binary[:filename]` - saves the solution at each
  iteration to a binary file, `TSMonitorSolution()`. Solution viewers work
  with other time-aware formats, e.g., `-ts_monitor_solution cgns:sol.cgns`,
  and can output one solution every 10 time steps by adding
  `-ts_monitor_solution_interval 10`. Use `-ts_monitor_solution_interval -1`
  to output data only at then end of a time loop.
- `-ts_monitor_solution_vtk <filename-%03D.vts>` - saves the solution
  at each iteration to a file in vtk format,
  `TSMonitorSolutionVTK()`.

## Error control via variable time-stepping

Most of the time stepping methods available in PETSc have an error
estimation and error control mechanism. This mechanism is implemented by
changing the step size in order to maintain user specified absolute and
relative tolerances. The PETSc object responsible with error control is
`TSAdapt`. The available `TSAdapt` types are listed in the following table.

```{eval-rst}
.. list-table:: ``TSAdapt``: available adaptors
   :name: tab_adaptors
   :header-rows: 1

   * - ID
     - Name
     - Notes
   * - ``TSADAPTNONE``
     - ``none``
     - no adaptivity
   * - ``TSADAPTBASIC``
     - ``basic``
     - the default adaptor
   * - ``TSADAPTGLEE``
     - ``glee``
     - extension of the basic adaptor to treat :math:`{\rm Tol}_{\rm A}` and :math:`{\rm Tol}_{\rm R}` as separate criteria. It can also control global errors if the integrator (e.g., ``TSGLEE``) provides this information
   * - ``TSADAPTDSP``
     - ``dsp``
     - adaptive controller for time-stepping based on digital signal processing
```

When using `TSADAPTBASIC` (the default), the user typically provides a
desired absolute ${\rm Tol}_{\rm A}$ or a relative
${\rm Tol}_{\rm R}$ error tolerance by invoking
`TSSetTolerances()` or at the command line with options `-ts_atol`
and `-ts_rtol`. The error estimate is based on the local truncation
error, so for every step the algorithm verifies that the estimated local
truncation error satisfies the tolerances provided by the user and
computes a new step size to be taken. For multistage methods, the local
truncation is obtained by comparing the solution $y$ to a lower
order $\widehat{p}=p-1$ approximation, $\widehat{y}$, where
$p$ is the order of the method and $\widehat{p}$ the order
of $\widehat{y}$.

The adaptive controller at step $n$ computes a tolerance level

$$
\begin{aligned}
Tol_n(i)&=&{\rm Tol}_{\rm A}(i) +  \max(y_n(i),\widehat{y}_n(i)) {\rm Tol}_{\rm R}(i)\,,\end{aligned}
$$

and forms the acceptable error level

$$
\begin{aligned}
\rm wlte_n&=& \frac{1}{m} \sum_{i=1}^{m}\sqrt{\frac{\left\|y_n(i)
  -\widehat{y}_n(i)\right\|}{Tol(i)}}\,,\end{aligned}
$$

where the errors are computed componentwise, $m$ is the dimension
of $y$ and `-ts_adapt_wnormtype` is `2` (default). If
`-ts_adapt_wnormtype` is `infinity` (max norm), then

$$
\begin{aligned}
\rm wlte_n&=& \max_{1\dots m}\frac{\left\|y_n(i)
  -\widehat{y}_n(i)\right\|}{Tol(i)}\,.\end{aligned}
$$

The error tolerances are satisfied when $\rm wlte\le 1.0$.

The next step size is based on this error estimate, and determined by

$$
\begin{aligned}
 \Delta t_{\rm new}(t)&=&\Delta t_{\rm{old}} \min(\alpha_{\max},
 \max(\alpha_{\min}, \beta (1/\rm wlte)^\frac{1}{\widehat{p}+1}))\,,\end{aligned}
$$ (hnew)

where $\alpha_{\min}=$`-ts_adapt_clip`[0] and
$\alpha_{\max}$=`-ts_adapt_clip`[1] keep the change in
$\Delta t$ to within a certain factor, and $\beta<1$ is
chosen through `-ts_adapt_safety` so that there is some margin to
which the tolerances are satisfied and so that the probability of
rejection is decreased.

This adaptive controller works in the following way. After completing
step $k$, if $\rm wlte_{k+1} \le 1.0$, then the step is
accepted and the next step is modified according to
{eq}`hnew`; otherwise, the step is rejected and retaken
with the step length computed in {eq}`hnew`.

`TSADAPTGLEE` is an extension of the basic
adaptor to treat ${\rm Tol}_{\rm A}$ and ${\rm Tol}_{\rm R}$
as separate criteria. it can also control global errors if the
integrator (e.g., `TSGLEE`) provides this information.

## Handling of discontinuities

For problems that involve discontinuous right-hand sides, one can set an
“event” function $g(t,u)$ for PETSc to detect and locate the times
of discontinuities (zeros of $g(t,u)$). Events can be defined
through the event monitoring routine

```
TSSetEventHandler(TS ts,PetscInt nevents,PetscInt *direction,PetscBool *terminate,PetscErrorCode (*indicator)(TS,PetscReal,Vec,PetscScalar*,void* eventP),PetscErrorCode (*postevent)(TS,PetscInt,PetscInt[],PetscReal,Vec,PetscBool,void* eventP),void *eventP);
```

Here, `nevents` denotes the number of events, `direction` sets the
type of zero crossing to be detected for an event (+1 for positive
zero-crossing, -1 for negative zero-crossing, and 0 for both),
`terminate` conveys whether the time-stepping should continue or halt
when an event is located, `eventmonitor` is a user- defined routine
that specifies the event description, `postevent` is an optional
user-defined routine to take specific actions following an event.

The arguments to `indicator()` are the timestep context, current
time, input state $u$, array of event function value, and the
(optional) user-provided context `eventP`.

The arguments to `postevent()` routine are the timestep context,
number of events occurred, indices of events occurred, current time, input
state $u$, a boolean flag indicating forward solve (1) or adjoint
solve (0), and the (optional) user-provided context `eventP`.

(sec_tchem)=

## Explicit integrators with finite element mass matrices

Discretized finite element problems often have the form $M \dot u = G(t, u)$ where $M$ is the mass matrix.
Such problems can be solved using `DMTSSetIFunction()` with implicit integrators.
When $M$ is nonsingular (i.e., the problem is an ODE, not a DAE), explicit integrators can be applied to $\dot u = M^{-1} G(t, u)$ or $\dot u = \hat M^{-1} G(t, u)$, where $\hat M$ is the lumped mass matrix.
While the true mass matrix generally has a dense inverse and thus must be solved iteratively, the lumped mass matrix is diagonal (e.g., computed via collocated quadrature or row sums of $M$).
To have PETSc create and apply a (lumped) mass matrix automatically, first use `DMTSSetRHSFunction()` to specify $G$ and set a `PetscFE` using `DMAddField()` and `DMCreateDS()`, then call either `DMTSCreateRHSMassMatrix()` or `DMTSCreateRHSMassMatrixLumped()` to automatically create the mass matrix and a `KSP` that will be used to apply $M^{-1}$.
This `KSP` can be customized using the `"mass_"` prefix.

(section_sa)=

## Performing sensitivity analysis with the TS ODE Solvers

The `TS` library provides a framework based on discrete adjoint models
for sensitivity analysis for ODEs and DAEs. The ODE/DAE solution process
(henceforth called the forward run) can be obtained by using either
explicit or implicit solvers in `TS`, depending on the problem
properties. Currently supported method types are `TSRK` (Runge-Kutta)
explicit methods and `TSTHETA` implicit methods, which include
`TSBEULER` and `TSCN`.

### Using the discrete adjoint methods

Consider the ODE/DAE

$$
F(t,y,\dot{y},p) = 0, \quad y(t_0)=y_0(p) \quad t_0 \le t \le t_F
$$

and the cost function(s)

$$
\Psi_i(y_0,p) = \Phi_i(y_F,p) + \int_{t_0}^{t_F} r_i(y(t),p,t)dt \quad i=1,...,n_\text{cost}.
$$

The `TSAdjoint` routines of PETSc provide

$$
\frac{\partial \Psi_i}{\partial y_0} = \lambda_i
$$

and

$$
\frac{\partial \Psi_i}{\partial p} = \mu_i + \lambda_i (\frac{\partial y_0}{\partial p}).
$$

To perform the discrete adjoint sensitivity analysis one first sets up
the `TS` object for a regular forward run but with one extra function
call

```
TSSetSaveTrajectory(TS ts),
```

then calls `TSSolve()` in the usual manner.

One must create two arrays of $n_\text{cost}$ vectors
$\lambda$ and $\mu$ (if there are no parameters $p$
then one can use `NULL` for the $\mu$ array.) The
$\lambda$ vectors are the same dimension and parallel layout as
the solution vector for the ODE, the $\mu$ vectors are of dimension
$p$; when $p$ is small usually all its elements are on the
first MPI process, while the vectors have no entries on the other
processes. $\lambda_i$ and $\mu_i$ should be initialized with
the values $d\Phi_i/dy|_{t=t_F}$ and $d\Phi_i/dp|_{t=t_F}$
respectively. Then one calls

```
TSSetCostGradients(TS ts,PetscInt numcost, Vec *lambda,Vec *mu);
```

where `numcost` denotes $n_\text{cost}$.
If $F()$ is a function of $p$ one needs to also provide the
Jacobian $-F_p$ with

```
TSSetRHSJacobianP(TS ts,Mat Amat,PetscErrorCode (*fp)(TS,PetscReal,Vec,Mat,void*),void *ctx)
```

or

```
TSSetIJacobianP(TS ts,Mat Amat,PetscErrorCode (*fp)(TS,PetscReal,Vec,Vec,PetscReal,Mat,void*),void *ctx)
```

or both, depending on which form is used to define the ODE.

The arguments for the function `fp()` are the timestep context,
current time, $y$, and the (optional) user-provided context.

If there is an integral term in the cost function, i.e. $r$ is
nonzero, it can be transformed into another ODE that is augmented to the
original ODE. To evaluate the integral, one needs to create a child
`TS` objective by calling

```
TSCreateQuadratureTS(TS ts,PetscBool fwd,TS *quadts);
```

and provide the ODE RHS function (which evaluates the integrand
$r$) with

```
TSSetRHSFunction(TS quadts,Vec R,PetscErrorCode (*rf)(TS,PetscReal,Vec,Vec,void*),void *ctx)
```

Similar to the settings for the original ODE, Jacobians of the integrand
can be provided with

```
TSSetRHSJacobian(TS quadts,Vec DRDU,Vec DRDU,PetscErrorCode (*drdyf)(TS,PetscReal,Vec,Vec*,void*),void *ctx)
TSSetRHSJacobianP(TS quadts,Vec DRDU,Vec DRDU,PetscErrorCode (*drdyp)(TS,PetscReal,Vec,Vec*,void*),void *ctx)
```

where $\mathrm{drdyf}= dr /dy$, $\mathrm{drdpf} = dr /dp$.
Since the integral term is additive to the cost function, its gradient
information will be included in $\lambda$ and $\mu$.

Lastly, one starts the backward run by calling

```
TSAdjointSolve(TS ts).
```

One can obtain the value of the integral term by calling

```
TSGetCostIntegral(TS ts,Vec *q).
```

or accessing directly the solution vector used by `quadts`.

The second argument of `TSCreateQuadratureTS()` allows one to choose
if the integral term is evaluated in the forward run (inside
`TSSolve()`) or in the backward run (inside `TSAdjointSolve()`) when
`TSSetCostGradients()` and `TSSetCostIntegrand()` are called before
`TSSolve()`. Note that this also allows for evaluating the integral
without having to use the adjoint solvers.

To provide a better understanding of the use of the adjoint solvers, we
introduce a simple example, corresponding to
<a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/power_grid/ex3sa.c.html">TS Power Grid Tutorial ex3sa</a>.
The problem is to study dynamic security of power system when there are
credible contingencies such as short-circuits or loss of generators,
transmission lines, or loads. The dynamic security constraints are
incorporated as equality constraints in the form of discretized
differential equations and inequality constraints for bounds on the
trajectory. The governing ODE system is

$$
\begin{aligned}
    \phi' &= &\omega_B (\omega - \omega_S)  \\
    2H/\omega_S \, \omega' & =& p_m - p_{max} sin(\phi) -D (\omega - \omega_S), \quad t_0 \leq t \leq t_F,\end{aligned}
$$

where $\phi$ is the phase angle and $\omega$ is the
frequency.

The initial conditions at time $t_0$ are

$$
\begin{aligned}
\phi(t_0) &=& \arcsin \left( p_m / p_{max} \right), \\
w(t_0) & =& 1.\end{aligned}
$$

$p_{max}$ is a positive number when the system operates normally.
At an event such as fault incidence/removal, $p_{max}$ will change
to $0$ temporarily and back to the original value after the fault
is fixed. The objective is to maximize $p_m$ subject to the above
ODE constraints and $\phi<\phi_S$ during all times. To accommodate
the inequality constraint, we want to compute the sensitivity of the
cost function

$$
\Psi(p_m,\phi) = -p_m + c \int_{t_0}^{t_F} \left( \max(0, \phi - \phi_S ) \right)^2 dt
$$

with respect to the parameter $p_m$. $numcost$ is $1$
since it is a scalar function.

For ODE solution, PETSc requires user-provided functions to evaluate the
system $F(t,y,\dot{y},p)$ (set by `TSSetIFunction()` ) and its
corresponding Jacobian $F_y + \sigma F_{\dot y}$ (set by
`TSSetIJacobian()`). Note that the solution state $y$ is
$[ \phi \;  \omega ]^T$ here. For sensitivity analysis, we need to
provide a routine to compute $\mathrm{f}_p=[0 \; 1]^T$ using
`TSASetRHSJacobianP()`, and three routines corresponding to the
integrand $r=c \left( \max(0, \phi - \phi_S ) \right)^2$,
$r_p = [0 \; 0]^T$ and
$r_y= [ 2 c \left( \max(0, \phi - \phi_S ) \right) \; 0]^T$ using
`TSSetCostIntegrand()`.

In the adjoint run, $\lambda$ and $\mu$ are initialized as
$[ 0 \;  0 ]^T$ and $[-1]$ at the final time $t_F$.
After `TSAdjointSolve()`, the sensitivity of the cost function w.r.t.
initial conditions is given by the sensitivity variable $\lambda$
(at time $t_0$) directly. And the sensitivity of the cost function
w.r.t. the parameter $p_m$ can be computed (by users) as

$$
\frac{\mathrm{d} \Psi}{\mathrm{d} p_m} = \mu(t_0) + \lambda(t_0)  \frac{\mathrm{d} \left[ \phi(t_0) \; \omega(t_0) \right]^T}{\mathrm{d} p_m}  .
$$

For explicit methods where one does not need to provide the Jacobian
$F_u$ for the forward solve one still does need it for the
backward solve and thus must call

```
TSSetRHSJacobian(TS ts,Mat Amat, Mat Pmat,PetscErrorCode (*f)(TS,PetscReal,Vec,Mat,Mat,void*),void *fP);
```

Examples include:

- discrete adjoint sensitivity using explicit and implicit time stepping methods for an ODE problem
  <a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/ex20adj.c.html">TS Tutorial ex20adj</a>,
- an optimization problem using the discrete adjoint models of the ERK (for nonstiff ODEs)
  and the Theta methods (for stiff DAEs)
  <a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/ex20opt_ic.c.html">TS Tutorial ex20opt_ic</a>
  and
  <a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/ex20opt_p.c.html">TS Tutorial ex20opt_p</a>,
- an ODE-constrained optimization using the discrete adjoint models of the
  Theta methods for cost function with an integral term
  <a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/power_grid/ex3opt.c.html">TS Power Grid Tutorial ex3opt</a>,
- discrete adjoint sensitivity using the Crank-Nicolson methods for DAEs with discontinuities
  <a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/power_grid/stability_9bus/ex9busadj.c.html">TS Power Grid Stability Tutorial ex9busadj</a>,
- a DAE-constrained optimization problem using the discrete adjoint models of the Crank-Nicolson
  methods for cost function with an integral term
  <a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/power_grid/stability_9bus/ex9busopt.c.html">TS Power Grid Tutorial ex9busopt</a>,
- discrete adjoint sensitivity using the Crank-Nicolson methods for a PDE problem
  <a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/advection-diffusion-reaction/ex5adj.c.html">TS Advection-Diffusion-Reaction Tutorial ex5adj</a>.

### Checkpointing

The discrete adjoint model requires the states (and stage values in the
context of multistage timestepping methods) to evaluate the Jacobian
matrices during the adjoint (backward) run. By default, PETSc stores the
whole trajectory to disk as binary files, each of which contains the
information for a single time step including state, time, and stage
values (optional). One can also make PETSc store the trajectory to
memory with the option `-ts_trajectory_type memory`. However, there
might not be sufficient memory capacity especially for large-scale
problems and long-time integration.

A so-called checkpointing scheme is needed to solve this problem. The
scheme stores checkpoints at selective time steps and recomputes the
missing information. The `revolve` library is used by PETSc
`TSTrajectory` to generate an optimal checkpointing schedule that
minimizes the recomputations given a limited number of available
checkpoints. One can specify the number of available checkpoints with
the option
`-ts_trajectory_max_cps_ram [maximum number of checkpoints in RAM]`.
Note that one checkpoint corresponds to one time step.

The `revolve` library also provides an optimal multistage
checkpointing scheme that uses both RAM and disk for storage. This
scheme is automatically chosen if one uses both the option
`-ts_trajectory_max_cps_ram [maximum number of checkpoints in RAM]`
and the option
`-ts_trajectory_max_cps_disk [maximum number of checkpoints on disk]`.

Some other useful options are listed below.

- `-ts_trajectory_view` prints the total number of recomputations,
- `-ts_monitor` and `-ts_adjoint_monitor` allow users to monitor
  the progress of the adjoint work flow,
- `-ts_trajectory_type visualization` may be used to save the whole
  trajectory for visualization. It stores the solution and the time,
  but no stage values. The binary files generated can be read into
  MATLAB via the script
  `$PETSC_DIR/share/petsc/matlab/PetscReadBinaryTrajectory.m`.

(sec_sundials)=

## Using Sundials from PETSc

Sundials is a parallel ODE solver developed by Hindmarsh et al. at LLNL.
The `TS` library provides an interface to use the CVODE component of
Sundials directly from PETSc. (To configure PETSc to use Sundials, see
the installation guide, `installation/index.htm`.)

To use the Sundials integrators, call

```
TSSetType(TS ts,TSType TSSUNDIALS);
```

or use the command line option `-ts_type` `sundials`.

Sundials’ CVODE solver comes with two main integrator families, Adams
and BDF (backward differentiation formula). One can select these with

```
TSSundialsSetType(TS ts,TSSundialsLmmType [SUNDIALS_ADAMS,SUNDIALS_BDF]);
```

or the command line option `-ts_sundials_type <adams,bdf>`. BDF is the
default.

Sundials does not use the `SNES` library within PETSc for its
nonlinear solvers, so one cannot change the nonlinear solver options via
`SNES`. Rather, Sundials uses the preconditioners within the `PC`
package of PETSc, which can be accessed via

```
TSSundialsGetPC(TS ts,PC *pc);
```

The user can then directly set preconditioner options; alternatively,
the usual runtime options can be employed via `-pc_xxx`.

Finally, one can set the Sundials tolerances via

```
TSSundialsSetTolerance(TS ts,double abs,double rel);
```

where `abs` denotes the absolute tolerance and `rel` the relative
tolerance.

Other PETSc-Sundials options include

```
TSSundialsSetGramSchmidtType(TS ts,TSSundialsGramSchmidtType type);
```

where `type` is either `SUNDIALS_MODIFIED_GS` or
`SUNDIALS_UNMODIFIED_GS`. This may be set via the options data base
with `-ts_sundials_gramschmidt_type <modifed,unmodified>`.

The routine

```
TSSundialsSetMaxl(TS ts,PetscInt restart);
```

sets the number of vectors in the Krylov subpspace used by GMRES. This
may be set in the options database with `-ts_sundials_maxl` `maxl`.

## Using TChem from PETSc

TChem [^id7] is a package originally developed at Sandia National
Laboratory that can read in CHEMKIN [^id8] data files and compute the
right-hand side function and its Jacobian for a reaction ODE system. To
utilize PETSc’s ODE solvers for these systems, first install PETSc with
the additional `configure` option `--download-tchem`. We currently
provide two examples of its use; one for single cell reaction and one
for an “artificial” one dimensional problem with periodic boundary
conditions and diffusion of all species. The self-explanatory examples
are the
<a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/extchem.c.html">The TS tutorial extchem</a>
and
<a href="PETSC_DOC_OUT_ROOT_PLACEHOLDER/src/ts/tutorials/extchemfield.c.html">The TS tutorial extchemfield</a>.

[^id5]: If the matrix $F_{\dot{u}}(t) = \partial F
    / \partial \dot{u}$ is nonsingular then it is an ODE and can be
    transformed to the standard explicit form, although this
    transformation may not lead to efficient algorithms.

[^id6]: PETSc will automatically translate the function provided to the
    appropriate form.

[^id7]: [bitbucket.org/jedbrown/tchem](https://bitbucket.org/jedbrown/tchem)

[^id8]: [en.wikipedia.org/wiki/CHEMKIN](https://en.wikipedia.org/wiki/CHEMKIN)

```{raw} html
<hr>
```

# Solving Steady-State Problems with Pseudo-Timestepping

**Simple Example:** `TS` provides a general code for performing pseudo
timestepping with a variable timestep at each physical node point. For
example, instead of directly attacking the steady-state problem

$$
G(u) = 0,
$$

we can use pseudo-transient continuation by solving

$$
u_t = G(u).
$$

Using time differencing

$$
u_t \doteq \frac{{u^{n+1}} - {u^{n}} }{dt^{n}}
$$

with the backward Euler method, we obtain nonlinear equations at a
series of pseudo-timesteps

$$
\frac{1}{dt^n} B (u^{n+1} - u^{n} ) = G(u^{n+1}).
$$

For this problem the user must provide $G(u)$, the time steps
$dt^{n}$ and the left-hand-side matrix $B$ (or optionally,
if the timestep is position independent and $B$ is the identity
matrix, a scalar timestep), as well as optionally the Jacobian of
$G(u)$.

More generally, this can be applied to implicit ODE and DAE for which
the transient form is

$$
F(u,\dot{u}) = 0.
$$

For solving steady-state problems with pseudo-timestepping one proceeds
as follows.

- Provide the function `G(u)` with the routine

  ```
  TSSetRHSFunction(TS ts,Vec r,PetscErrorCode (*f)(TS,PetscReal,Vec,Vec,void*),void *fP);
  ```

  The arguments to the function `f()` are the timestep context, the
  current time, the input for the function, the output for the function
  and the (optional) user-provided context variable `fP`.

- Provide the (approximate) Jacobian matrix of `G(u)` and a function
  to compute it at each Newton iteration. This is done with the command

  ```
  TSSetRHSJacobian(TS ts,Mat Amat, Mat Pmat,PetscErrorCode (*f)(TS,PetscReal,Vec,Mat,Mat,void*),void *fP);
  ```

  The arguments for the function `f()` are the timestep context, the
  current time, the location where the Jacobian is to be computed, the
  (approximate) Jacobian matrix, an alternative approximate Jacobian
  matrix used to construct the preconditioner, and the optional
  user-provided context, passed in as `fP`. The user must provide the
  Jacobian as a matrix; thus, if using a matrix-free approach, one must
  create a `MATSHELL` matrix.

In addition, the user must provide a routine that computes the
pseudo-timestep. This is slightly different depending on if one is using
a constant timestep over the entire grid, or it varies with location.

- For location-independent pseudo-timestepping, one uses the routine

  ```
  TSPseudoSetTimeStep(TS ts,PetscInt(*dt)(TS,PetscReal*,void*),void* dtctx);
  ```

  The function `dt` is a user-provided function that computes the
  next pseudo-timestep. As a default one can use
  `TSPseudoTimeStepDefault(TS,PetscReal*,void*)` for `dt`. This
  routine updates the pseudo-timestep with one of two strategies: the
  default

  $$
  dt^{n} = dt_{\mathrm{increment}}*dt^{n-1}*\frac{|| F(u^{n-1}) ||}{|| F(u^{n})||}
  $$

  or, the alternative,

  $$
  dt^{n} = dt_{\mathrm{increment}}*dt^{0}*\frac{|| F(u^{0}) ||}{|| F(u^{n})||}
  $$

  which can be set with the call

  ```
  TSPseudoIncrementDtFromInitialDt(TS ts);
  ```

  or the option `-ts_pseudo_increment_dt_from_initial_dt`. The value
  $dt_{\mathrm{increment}}$ is by default $1.1$, but can be
  reset with the call

  ```
  TSPseudoSetTimeStepIncrement(TS ts,PetscReal inc);
  ```

  or the option `-ts_pseudo_increment <inc>`.

- For location-dependent pseudo-timestepping, the interface function
  has not yet been created.

```{eval-rst}
.. bibliography:: /petsc.bib
   :filter: docname in docnames

```
