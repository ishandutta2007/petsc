static char help[] = "Nonlinear, time-dependent. Developed from radiative_surface_balance.c \n";
/*
  Contributed by Steve Froehlich, Illinois Institute of Technology

   Usage:
    mpiexec -n <np> ./ex5 [options]
    ./ex5 -help  [view PETSc options]
    ./ex5 -ts_type sundials -ts_view
    ./ex5 -da_grid_x 20 -da_grid_y 20 -log_view
    ./ex5 -da_grid_x 20 -da_grid_y 20 -ts_type rosw -ts_atol 1.e-6 -ts_rtol 1.e-6
    ./ex5 -drawcontours -draw_pause 0.1 -draw_fields 0,1,2,3,4
*/

/*
   -----------------------------------------------------------------------

   Governing equations:

        R      = s*(Ea*Ta^4 - Es*Ts^4)
        SH     = p*Cp*Ch*wind*(Ta - Ts)
        LH     = p*L*Ch*wind*B(q(Ta) - q(Ts))
        G      = k*(Tgnd - Ts)/dz

        Fnet   = R + SH + LH + G

        du/dt  = -u*(du/dx) - v*(du/dy) - 2*omeg*sin(lat)*v - (1/p)*(dP/dx)
        dv/dt  = -u*(dv/dx) - v*(dv/dy) + 2*omeg*sin(lat)*u - (1/p)*(dP/dy)
        dTs/dt = Fnet/(Cp*dz) - Div([u*Ts, v*Ts]) + D*Lap(Ts)
               = Fnet/(Cs*dz) - u*(dTs/dx) - v*(dTs/dy) + D*(Ts_xx + Ts_yy)
        dp/dt  = -Div([u*p,v*p])
               = - u*dp/dx - v*dp/dy
        dTa/dt = Fnet/Cp

   Equation of State:

        P = p*R*Ts

   -----------------------------------------------------------------------

   Program considers the evolution of a two dimensional atmosphere from
   sunset to sunrise. There are two components:
                1. Surface energy balance model to compute diabatic dT (Fnet)
                2. Dynamical model using simplified primitive equations

   Program is to be initiated at sunset and run to sunrise.

   Inputs are:
                Surface temperature
                Dew point temperature
                Air temperature
                Temperature at cloud base (if clouds are present)
                Fraction of sky covered by clouds
                Wind speed
                Precipitable water in centimeters
                Wind direction

   Inputs are read in from the text file ex5_control.txt. To change an
   input value use ex5_control.txt.

   Solvers:
            Backward Euler = default solver
            Sundials = fastest and most accurate, requires Sundials libraries

   This model is under development and should be used only as an example
   and not as a predictive weather model.
*/

#include <petscts.h>
#include <petscdm.h>
#include <petscdmda.h>

/* stefan-boltzmann constant */
#define SIG 0.000000056703
/* absorption-emission constant for surface */
#define EMMSFC 1
/* amount of time (seconds) that passes before new flux is calculated */
#define TIMESTEP 1

/* variables of interest to be solved at each grid point */
typedef struct {
  PetscScalar Ts, Ta; /* surface and air temperature */
  PetscScalar u, v;   /* wind speed */
  PetscScalar p;      /* density */
} Field;

/* User defined variables. Used in solving for variables of interest */
typedef struct {
  DM          da;             /* grid */
  PetscScalar csoil;          /* heat constant for layer */
  PetscScalar dzlay;          /* thickness of top soil layer */
  PetscScalar emma;           /* emission parameter */
  PetscScalar wind;           /* wind speed */
  PetscScalar dewtemp;        /* dew point temperature (moisture in air) */
  PetscScalar pressure1;      /* sea level pressure */
  PetscScalar airtemp;        /* temperature of air near boundary layer inversion */
  PetscScalar Ts;             /* temperature at the surface */
  PetscScalar fract;          /* fraction of sky covered by clouds */
  PetscScalar Tc;             /* temperature at base of lowest cloud layer */
  PetscScalar lat;            /* Latitude in degrees */
  PetscScalar init;           /* initialization scenario */
  PetscScalar deep_grnd_temp; /* temperature of ground under top soil surface layer */
} AppCtx;

/* Struct for visualization */
typedef struct {
  PetscBool   drawcontours; /* flag - 1 indicates drawing contours */
  PetscViewer drawviewer;
  PetscInt    interval;
} MonitorCtx;

/* Inputs read in from text file */
struct in {
  PetscScalar Ts;     /* surface temperature  */
  PetscScalar Td;     /* dewpoint temperature */
  PetscScalar Tc;     /* temperature of cloud base */
  PetscScalar fr;     /* fraction of sky covered by clouds */
  PetscScalar wnd;    /* wind speed */
  PetscScalar Ta;     /* air temperature */
  PetscScalar pwt;    /* precipitable water */
  PetscScalar wndDir; /* wind direction */
  PetscScalar lat;    /* latitude */
  PetscReal   time;   /* time in hours */
  PetscScalar init;
};

/* functions */
extern PetscScalar    emission(PetscScalar);                         /* sets emission/absorption constant depending on water vapor content */
extern PetscScalar    calc_q(PetscScalar);                           /* calculates specific humidity */
extern PetscScalar    mph2mpers(PetscScalar);                        /* converts miles per hour to meters per second */
extern PetscScalar    Lconst(PetscScalar);                           /* calculates latent heat constant taken from Satellite estimates of wind speed and latent heat flux over the global oceans., Bentamy et al. */
extern PetscScalar    fahr_to_cel(PetscScalar);                      /* converts Fahrenheit to Celsius */
extern PetscScalar    cel_to_fahr(PetscScalar);                      /* converts Celsius to Fahrenheit */
extern PetscScalar    calcmixingr(PetscScalar, PetscScalar);         /* calculates mixing ratio */
extern PetscScalar    cloud(PetscScalar);                            /* cloud radiative parameterization */
extern PetscErrorCode FormInitialSolution(DM, Vec, void *);          /* Specifies initial conditions for the system of equations (PETSc defined function) */
extern PetscErrorCode RhsFunc(TS, PetscReal, Vec, Vec, void *);      /* Specifies the user defined functions                     (PETSc defined function) */
extern PetscErrorCode Monitor(TS, PetscInt, PetscReal, Vec, void *); /* Specifies output and visualization tools                 (PETSc defined function) */
extern PetscErrorCode readinput(struct in *put);                     /* reads input from text file */
extern PetscErrorCode calcfluxs(PetscScalar, PetscScalar, PetscScalar, PetscScalar, PetscScalar, PetscScalar *); /* calculates upward IR from surface */
extern PetscErrorCode calcfluxa(PetscScalar, PetscScalar, PetscScalar, PetscScalar *);                           /* calculates downward IR from atmosphere */
extern PetscErrorCode sensibleflux(PetscScalar, PetscScalar, PetscScalar, PetscScalar *);                        /* calculates sensible heat flux */
extern PetscErrorCode potential_temperature(PetscScalar, PetscScalar, PetscScalar, PetscScalar, PetscScalar *);  /* calculates potential temperature */
extern PetscErrorCode latentflux(PetscScalar, PetscScalar, PetscScalar, PetscScalar, PetscScalar *);             /* calculates latent heat flux */
extern PetscErrorCode calc_gflux(PetscScalar, PetscScalar, PetscScalar *);                                       /* calculates flux between top soil layer and underlying earth */

int main(int argc, char **argv)
{
  PetscInt      time; /* amount of loops */
  struct in     put;
  PetscScalar   rh;                 /* relative humidity */
  PetscScalar   x;                  /* memory variable for relative humidity calculation */
  PetscScalar   deep_grnd_temp;     /* temperature of ground under top soil surface layer */
  PetscScalar   emma;               /* absorption-emission constant for air */
  PetscScalar   pressure1 = 101300; /* surface pressure */
  PetscScalar   mixratio;           /* mixing ratio */
  PetscScalar   airtemp;            /* temperature of air near boundary layer inversion */
  PetscScalar   dewtemp;            /* dew point temperature */
  PetscScalar   sfctemp;            /* temperature at surface */
  PetscScalar   pwat;               /* total column precipitable water */
  PetscScalar   cloudTemp;          /* temperature at base of cloud */
  AppCtx        user;               /*  user-defined work context */
  MonitorCtx    usermonitor;        /* user-defined monitor context */
  TS            ts;
  SNES          snes;
  DM            da;
  Vec           T, rhs; /* solution vector */
  Mat           J;      /* Jacobian matrix */
  PetscReal     ftime, dt;
  PetscInt      steps, dof = 5;
  PetscBool     use_coloring  = PETSC_TRUE;
  MatFDColoring matfdcoloring = 0;
  PetscBool     monitor_off   = PETSC_FALSE;
  PetscBool     prunejacobian = PETSC_FALSE;

  PetscFunctionBeginUser;
  PetscCall(PetscInitialize(&argc, &argv, NULL, help));

  /* Inputs */
  PetscCall(readinput(&put));

  sfctemp   = put.Ts;
  dewtemp   = put.Td;
  cloudTemp = put.Tc;
  airtemp   = put.Ta;
  pwat      = put.pwt;

  PetscCall(PetscPrintf(PETSC_COMM_WORLD, "Initial Temperature = %g\n", (double)sfctemp)); /* input surface temperature */

  deep_grnd_temp = sfctemp - 10;   /* set underlying ground layer temperature */
  emma           = emission(pwat); /* accounts for radiative effects of water vapor */

  /* Converts from Fahrenheit to Celsius */
  sfctemp        = fahr_to_cel(sfctemp);
  airtemp        = fahr_to_cel(airtemp);
  dewtemp        = fahr_to_cel(dewtemp);
  cloudTemp      = fahr_to_cel(cloudTemp);
  deep_grnd_temp = fahr_to_cel(deep_grnd_temp);

  /* Converts from Celsius to Kelvin */
  sfctemp += 273;
  airtemp += 273;
  dewtemp += 273;
  cloudTemp += 273;
  deep_grnd_temp += 273;

  /* Calculates initial relative humidity */
  x        = calcmixingr(dewtemp, pressure1);
  mixratio = calcmixingr(sfctemp, pressure1);
  rh       = (x / mixratio) * 100;

  PetscCall(PetscPrintf(PETSC_COMM_WORLD, "Initial RH = %.1f percent\n\n", (double)rh)); /* prints initial relative humidity */

  time = 3600 * put.time; /* sets amount of timesteps to run model */

  /* Configure PETSc TS solver */
  /*------------------------------------------*/

  /* Create grid */
  PetscCall(DMDACreate2d(PETSC_COMM_WORLD, DM_BOUNDARY_PERIODIC, DM_BOUNDARY_PERIODIC, DMDA_STENCIL_STAR, 20, 20, PETSC_DECIDE, PETSC_DECIDE, dof, 1, NULL, NULL, &da));
  PetscCall(DMSetFromOptions(da));
  PetscCall(DMSetUp(da));
  PetscCall(DMDASetUniformCoordinates(da, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0));

  /* Define output window for each variable of interest */
  PetscCall(DMDASetFieldName(da, 0, "Ts"));
  PetscCall(DMDASetFieldName(da, 1, "Ta"));
  PetscCall(DMDASetFieldName(da, 2, "u"));
  PetscCall(DMDASetFieldName(da, 3, "v"));
  PetscCall(DMDASetFieldName(da, 4, "p"));

  /* set values for appctx */
  user.da             = da;
  user.Ts             = sfctemp;
  user.fract          = put.fr;         /* fraction of sky covered by clouds */
  user.dewtemp        = dewtemp;        /* dew point temperature (mositure in air) */
  user.csoil          = 2000000;        /* heat constant for layer */
  user.dzlay          = 0.08;           /* thickness of top soil layer */
  user.emma           = emma;           /* emission parameter */
  user.wind           = put.wnd;        /* wind speed */
  user.pressure1      = pressure1;      /* sea level pressure */
  user.airtemp        = airtemp;        /* temperature of air near boundar layer inversion */
  user.Tc             = cloudTemp;      /* temperature at base of lowest cloud layer */
  user.init           = put.init;       /* user chosen initiation scenario */
  user.lat            = 70 * 0.0174532; /* converts latitude degrees to latitude in radians */
  user.deep_grnd_temp = deep_grnd_temp; /* temp in lowest ground layer */

  /* set values for MonitorCtx */
  usermonitor.drawcontours = PETSC_FALSE;
  PetscCall(PetscOptionsHasName(NULL, NULL, "-drawcontours", &usermonitor.drawcontours));
  if (usermonitor.drawcontours) {
    PetscReal bounds[] = {1000.0, -1000., -1000., -1000., 1000., -1000., 1000., -1000., 1000, -1000, 100700, 100800};
    PetscCall(PetscViewerDrawOpen(PETSC_COMM_WORLD, 0, 0, 0, 0, 300, 300, &usermonitor.drawviewer));
    PetscCall(PetscViewerDrawSetBounds(usermonitor.drawviewer, dof, bounds));
  }
  usermonitor.interval = 1;
  PetscCall(PetscOptionsGetInt(NULL, NULL, "-monitor_interval", &usermonitor.interval, NULL));

  /*  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Extract global vectors from DA;
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  PetscCall(DMCreateGlobalVector(da, &T));
  PetscCall(VecDuplicate(T, &rhs)); /* r: vector to put the computed right-hand side */

  PetscCall(TSCreate(PETSC_COMM_WORLD, &ts));
  PetscCall(TSSetProblemType(ts, TS_NONLINEAR));
  PetscCall(TSSetType(ts, TSBEULER));
  PetscCall(TSSetRHSFunction(ts, rhs, RhsFunc, &user));

  /* Set Jacobian evaluation routine - use coloring to compute finite difference Jacobian efficiently */
  PetscCall(DMSetMatType(da, MATAIJ));
  PetscCall(DMCreateMatrix(da, &J));
  if (use_coloring) {
    ISColoring iscoloring;
    PetscInt   ncolors;

    PetscCall(DMCreateColoring(da, IS_COLORING_GLOBAL, &iscoloring));
    PetscCall(MatFDColoringCreate(J, iscoloring, &matfdcoloring));
    PetscCall(MatFDColoringSetFromOptions(matfdcoloring));
    PetscCall(MatFDColoringSetUp(J, iscoloring, matfdcoloring));
    PetscCall(ISColoringGetColors(iscoloring, NULL, &ncolors, NULL));
    PetscCall(PetscPrintf(PETSC_COMM_WORLD, "DMColoring generates %" PetscInt_FMT " colors\n", ncolors));
    PetscCall(ISColoringDestroy(&iscoloring));
    PetscCall(TSSetIJacobian(ts, J, J, TSComputeIJacobianDefaultColor, NULL));
  } else {
    PetscCall(TSGetSNES(ts, &snes));
    PetscCall(SNESSetJacobian(snes, J, J, SNESComputeJacobianDefault, NULL));
  }

  /* Define what to print for ts_monitor option */
  PetscCall(PetscOptionsHasName(NULL, NULL, "-monitor_off", &monitor_off));
  if (!monitor_off) PetscCall(TSMonitorSet(ts, Monitor, &usermonitor, NULL));
  dt    = TIMESTEP; /* initial time step */
  ftime = TIMESTEP * time;
  PetscCall(PetscPrintf(PETSC_COMM_WORLD, "time %" PetscInt_FMT ", ftime %g hour, TIMESTEP %g\n", time, (double)(ftime / 3600), (double)dt));

  PetscCall(TSSetTimeStep(ts, dt));
  PetscCall(TSSetMaxSteps(ts, time));
  PetscCall(TSSetMaxTime(ts, ftime));
  PetscCall(TSSetExactFinalTime(ts, TS_EXACTFINALTIME_STEPOVER));
  PetscCall(TSSetSolution(ts, T));
  PetscCall(TSSetDM(ts, da));

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set runtime options
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  PetscCall(PetscOptionsGetBool(NULL, NULL, "-prune_jacobian", &prunejacobian, NULL));
  PetscCall(TSSetFromOptions(ts));
  if (prunejacobian && matfdcoloring) {
    PetscRandom rctx;
    Vec         Tdot;
    /* Compute the Jacobian with randomized vector values to capture the sparsity pattern for coloring */
    PetscCall(VecDuplicate(T, &Tdot));
    PetscCall(PetscRandomCreate(PETSC_COMM_WORLD, &rctx));
    PetscCall(PetscRandomSetInterval(rctx, 1.0, 2.0));
    PetscCall(VecSetRandom(T, rctx));
    PetscCall(VecSetRandom(Tdot, rctx));
    PetscCall(PetscRandomDestroy(&rctx));
    PetscCall(TSSetUp(ts));
    PetscCall(TSComputeIJacobian(ts, 0.0, T, Tdot, 0.12345, J, J, PETSC_FALSE));
    PetscCall(VecDestroy(&Tdot));
    PetscCall(MatFDColoringDestroy(&matfdcoloring));
    PetscCall(TSPruneIJacobianColor(ts, J, J));
  }
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Solve nonlinear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  PetscCall(FormInitialSolution(da, T, &user));
  PetscCall(TSSolve(ts, T));
  PetscCall(TSGetSolveTime(ts, &ftime));
  PetscCall(TSGetStepNumber(ts, &steps));
  PetscCall(PetscPrintf(PETSC_COMM_WORLD, "Solution T after %g hours %" PetscInt_FMT " steps\n", (double)(ftime / 3600), steps));

  if (matfdcoloring) PetscCall(MatFDColoringDestroy(&matfdcoloring));
  if (usermonitor.drawcontours) PetscCall(PetscViewerDestroy(&usermonitor.drawviewer));
  PetscCall(MatDestroy(&J));
  PetscCall(VecDestroy(&T));
  PetscCall(VecDestroy(&rhs));
  PetscCall(TSDestroy(&ts));
  PetscCall(DMDestroy(&da));

  PetscCall(PetscFinalize());
  return 0;
}
/*****************************end main program********************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
PetscErrorCode calcfluxs(PetscScalar sfctemp, PetscScalar airtemp, PetscScalar emma, PetscScalar fract, PetscScalar cloudTemp, PetscScalar *flux)
{
  PetscFunctionBeginUser;
  *flux = SIG * ((EMMSFC * emma * PetscPowScalarInt(airtemp, 4)) + (EMMSFC * fract * (1 - emma) * PetscPowScalarInt(cloudTemp, 4)) - (EMMSFC * PetscPowScalarInt(sfctemp, 4))); /* calculates flux using Stefan-Boltzmann relation */
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode calcfluxa(PetscScalar sfctemp, PetscScalar airtemp, PetscScalar emma, PetscScalar *flux) /* this function is not currently called upon */
{
  PetscScalar emm = 0.001;

  PetscFunctionBeginUser;
  *flux = SIG * (-emm * (PetscPowScalarInt(airtemp, 4))); /* calculates flux usinge Stefan-Boltzmann relation */
  PetscFunctionReturn(PETSC_SUCCESS);
}
PetscErrorCode sensibleflux(PetscScalar sfctemp, PetscScalar airtemp, PetscScalar wind, PetscScalar *sheat)
{
  PetscScalar density = 1;    /* air density */
  PetscScalar Cp      = 1005; /* heat capacity for dry air */
  PetscScalar wndmix;         /* temperature change from wind mixing: wind*Ch */

  PetscFunctionBeginUser;
  wndmix = 0.0025 + 0.0042 * wind;                      /* regression equation valid for neutral and stable BL */
  *sheat = density * Cp * wndmix * (airtemp - sfctemp); /* calculates sensible heat flux */
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode latentflux(PetscScalar sfctemp, PetscScalar dewtemp, PetscScalar wind, PetscScalar pressure1, PetscScalar *latentheat)
{
  PetscScalar density = 1; /* density of dry air */
  PetscScalar q;           /* actual specific humitity */
  PetscScalar qs;          /* saturation specific humidity */
  PetscScalar wndmix;      /* temperature change from wind mixing: wind*Ch */
  PetscScalar beta = .4;   /* moisture availability */
  PetscScalar mr;          /* mixing ratio */
  PetscScalar lhcnst;      /* latent heat of vaporization constant = 2501000 J/kg at 0c */
                           /* latent heat of saturation const = 2834000 J/kg */
                           /* latent heat of fusion const = 333700 J/kg */

  PetscFunctionBeginUser;
  wind   = mph2mpers(wind);                 /* converts wind from mph to meters per second */
  wndmix = 0.0025 + 0.0042 * wind;          /* regression equation valid for neutral BL */
  lhcnst = Lconst(sfctemp);                 /* calculates latent heat of evaporation */
  mr     = calcmixingr(sfctemp, pressure1); /* calculates saturation mixing ratio */
  qs     = calc_q(mr);                      /* calculates saturation specific humidty */
  mr     = calcmixingr(dewtemp, pressure1); /* calculates mixing ratio */
  q      = calc_q(mr);                      /* calculates specific humidty */

  *latentheat = density * wndmix * beta * lhcnst * (q - qs); /* calculates latent heat flux */
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode potential_temperature(PetscScalar temp, PetscScalar pressure1, PetscScalar pressure2, PetscScalar sfctemp, PetscScalar *pottemp)
{
  PetscScalar kdry; /* poisson constant for dry atmosphere */
  PetscScalar pavg; /* average atmospheric pressure */
  /* PetscScalar mixratio; mixing ratio */
  /* PetscScalar kmoist;   poisson constant for moist atmosphere */

  PetscFunctionBeginUser;
  /* mixratio = calcmixingr(sfctemp,pressure1); */

  /* initialize poisson constant */
  kdry = 0.2854;
  /* kmoist = 0.2854*(1 - 0.24*mixratio); */

  pavg     = ((0.7 * pressure1) + pressure2) / 2;             /* calculates simple average press */
  *pottemp = temp * (PetscPowScalar(pressure1 / pavg, kdry)); /* calculates potential temperature */
  PetscFunctionReturn(PETSC_SUCCESS);
}
extern PetscScalar calcmixingr(PetscScalar dtemp, PetscScalar pressure1)
{
  PetscScalar e;        /* vapor pressure */
  PetscScalar mixratio; /* mixing ratio */

  dtemp    = dtemp - 273;                                                  /* converts from Kelvin to Celsius */
  e        = 6.11 * (PetscPowScalar(10, (7.5 * dtemp) / (237.7 + dtemp))); /* converts from dew point temp to vapor pressure */
  e        = e * 100;                                                      /* converts from hPa to Pa */
  mixratio = (0.622 * e) / (pressure1 - e);                                /* computes mixing ratio */
  mixratio = mixratio * 1;                                                 /* convert to g/Kg */

  return mixratio;
}
extern PetscScalar calc_q(PetscScalar rv)
{
  PetscScalar specific_humidity;     /* define specific humidity variable */
  specific_humidity = rv / (1 + rv); /* calculates specific humidity */
  return specific_humidity;
}

PetscErrorCode calc_gflux(PetscScalar sfctemp, PetscScalar deep_grnd_temp, PetscScalar *Gflux)
{
  PetscScalar k;                       /* thermal conductivity parameter */
  PetscScalar n                = 0.38; /* value of soil porosity */
  PetscScalar dz               = 1;    /* depth of layer between soil surface and deep soil layer */
  PetscScalar unit_soil_weight = 2700; /* unit soil weight in kg/m^3 */

  PetscFunctionBeginUser;
  k      = ((0.135 * (1 - n) * unit_soil_weight) + 64.7) / (unit_soil_weight - (0.947 * (1 - n) * unit_soil_weight)); /* dry soil conductivity */
  *Gflux = (k * (deep_grnd_temp - sfctemp) / dz);                                                                     /* calculates flux from deep ground layer */
  PetscFunctionReturn(PETSC_SUCCESS);
}
extern PetscScalar emission(PetscScalar pwat)
{
  PetscScalar emma;

  emma = 0.725 + 0.17 * PetscLog10Real(PetscRealPart(pwat));

  return emma;
}
extern PetscScalar cloud(PetscScalar fract)
{
  PetscScalar emma = 0;

  /* modifies radiative balance depending on cloud cover */
  if (fract >= 0.9) emma = 1;
  else if (0.9 > fract && fract >= 0.8) emma = 0.9;
  else if (0.8 > fract && fract >= 0.7) emma = 0.85;
  else if (0.7 > fract && fract >= 0.6) emma = 0.75;
  else if (0.6 > fract && fract >= 0.5) emma = 0.65;
  else if (0.4 > fract && fract >= 0.3) emma = emma * 1.086956;
  return emma;
}
extern PetscScalar Lconst(PetscScalar sfctemp)
{
  PetscScalar Lheat;
  sfctemp -= 273;                               /* converts from kelvin to celsius */
  Lheat = 4186.8 * (597.31 - 0.5625 * sfctemp); /* calculates latent heat constant */
  return Lheat;
}
extern PetscScalar mph2mpers(PetscScalar wind)
{
  wind = ((wind * 1.6 * 1000) / 3600); /* converts wind from mph to meters per second */
  return wind;
}
extern PetscScalar fahr_to_cel(PetscScalar temp)
{
  temp = (5 * (temp - 32)) / 9; /* converts from farhrenheit to celsius */
  return temp;
}
extern PetscScalar cel_to_fahr(PetscScalar temp)
{
  temp = ((temp * 9) / 5) + 32; /* converts from celsius to farhrenheit */
  return temp;
}
PetscErrorCode readinput(struct in *put)
{
  int    i;
  char   x;
  FILE  *ifp;
  double tmp;

  PetscFunctionBeginUser;
  ifp = fopen("ex5_control.txt", "r");
  PetscCheck(ifp, PETSC_COMM_SELF, PETSC_ERR_FILE_OPEN, "Unable to open input file");
  for (i = 0; i < 110; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->Ts = tmp;

  for (i = 0; i < 43; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->Td = tmp;

  for (i = 0; i < 43; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->Ta = tmp;

  for (i = 0; i < 43; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->Tc = tmp;

  for (i = 0; i < 43; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->fr = tmp;

  for (i = 0; i < 43; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->wnd = tmp;

  for (i = 0; i < 43; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->pwt = tmp;

  for (i = 0; i < 43; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->wndDir = tmp;

  for (i = 0; i < 43; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->time = tmp;

  for (i = 0; i < 63; i++) PetscCheck(fscanf(ifp, "%c", &x) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  PetscCheck(fscanf(ifp, "%lf", &tmp) == 1, PETSC_COMM_SELF, PETSC_ERR_FILE_READ, "Unable to read file");
  put->init = tmp;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* ------------------------------------------------------------------- */
PetscErrorCode FormInitialSolution(DM da, Vec Xglobal, void *ctx)
{
  AppCtx  *user = (AppCtx *)ctx; /* user-defined application context */
  PetscInt i, j, xs, ys, xm, ym, Mx, My;
  Field  **X;

  PetscFunctionBeginUser;
  PetscCall(DMDAGetInfo(da, PETSC_IGNORE, &Mx, &My, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE));

  /* Get pointers to vector data */
  PetscCall(DMDAVecGetArray(da, Xglobal, &X));

  /* Get local grid boundaries */
  PetscCall(DMDAGetCorners(da, &xs, &ys, NULL, &xm, &ym, NULL));

  /* Compute function over the locally owned part of the grid */

  if (user->init == 1) {
    for (j = ys; j < ys + ym; j++) {
      for (i = xs; i < xs + xm; i++) {
        X[j][i].Ts = user->Ts - i * 0.0001;
        X[j][i].Ta = X[j][i].Ts - 5;
        X[j][i].u  = 0;
        X[j][i].v  = 0;
        X[j][i].p  = 1.25;
        if ((j == 5 || j == 6) && (i == 4 || i == 5)) X[j][i].p += 0.00001;
        if ((j == 5 || j == 6) && (i == 12 || i == 13)) X[j][i].p += 0.00001;
      }
    }
  } else {
    for (j = ys; j < ys + ym; j++) {
      for (i = xs; i < xs + xm; i++) {
        X[j][i].Ts = user->Ts;
        X[j][i].Ta = X[j][i].Ts - 5;
        X[j][i].u  = 0;
        X[j][i].v  = 0;
        X[j][i].p  = 1.25;
      }
    }
  }

  /* Restore vectors */
  PetscCall(DMDAVecRestoreArray(da, Xglobal, &X));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*
   RhsFunc - Evaluates nonlinear function F(u).

   Input Parameters:
.  ts - the TS context
.  t - current time
.  Xglobal - input vector
.  F - output vector
.  ptr - optional user-defined context, as set by SNESSetFunction()

   Output Parameter:
.  F - rhs function vector
 */
PetscErrorCode RhsFunc(TS ts, PetscReal t, Vec Xglobal, Vec F, void *ctx)
{
  AppCtx     *user = (AppCtx *)ctx; /* user-defined application context */
  DM          da   = user->da;
  PetscInt    i, j, Mx, My, xs, ys, xm, ym;
  PetscReal   dhx, dhy;
  Vec         localT;
  Field     **X, **Frhs;                                            /* structures that contain variables of interest and left hand side of governing equations respectively */
  PetscScalar csoil          = user->csoil;                         /* heat constant for layer */
  PetscScalar dzlay          = user->dzlay;                         /* thickness of top soil layer */
  PetscScalar emma           = user->emma;                          /* emission parameter */
  PetscScalar wind           = user->wind;                          /* wind speed */
  PetscScalar dewtemp        = user->dewtemp;                       /* dew point temperature (moisture in air) */
  PetscScalar pressure1      = user->pressure1;                     /* sea level pressure */
  PetscScalar airtemp        = user->airtemp;                       /* temperature of air near boundary layer inversion */
  PetscScalar fract          = user->fract;                         /* fraction of the sky covered by clouds */
  PetscScalar Tc             = user->Tc;                            /* temperature at base of lowest cloud layer */
  PetscScalar lat            = user->lat;                           /* latitude */
  PetscScalar Cp             = 1005.7;                              /* specific heat of air at constant pressure */
  PetscScalar Rd             = 287.058;                             /* gas constant for dry air */
  PetscScalar diffconst      = 1000;                                /* diffusion coefficient */
  PetscScalar f              = 2 * 0.0000727 * PetscSinScalar(lat); /* coriolis force */
  PetscScalar deep_grnd_temp = user->deep_grnd_temp;                /* temp in lowest ground layer */
  PetscScalar Ts, u, v, p;
  PetscScalar u_abs, u_plus, u_minus, v_abs, v_plus, v_minus;

  PetscScalar sfctemp1, fsfc1, Ra;
  PetscScalar sheat;      /* sensible heat flux */
  PetscScalar latentheat; /* latent heat flux */
  PetscScalar groundflux; /* flux from conduction of deep ground layer in contact with top soil */
  PetscInt    xend, yend;

  PetscFunctionBeginUser;
  PetscCall(DMGetLocalVector(da, &localT));
  PetscCall(DMDAGetInfo(da, PETSC_IGNORE, &Mx, &My, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE, PETSC_IGNORE));

  dhx = (PetscReal)(Mx - 1) / (5000 * (Mx - 1)); /* dhx = 1/dx; assume 2D space domain: [0.0, 1.e5] x [0.0, 1.e5] */
  dhy = (PetscReal)(My - 1) / (5000 * (Mx - 1)); /* dhy = 1/dy; */

  /*
     Scatter ghost points to local vector,using the 2-step process
        DAGlobalToLocalBegin(),DAGlobalToLocalEnd().
     By placing code between these two statements, computations can be
     done while messages are in transition.
  */
  PetscCall(DMGlobalToLocalBegin(da, Xglobal, INSERT_VALUES, localT));
  PetscCall(DMGlobalToLocalEnd(da, Xglobal, INSERT_VALUES, localT));

  /* Get pointers to vector data */
  PetscCall(DMDAVecGetArrayRead(da, localT, &X));
  PetscCall(DMDAVecGetArray(da, F, &Frhs));

  /* Get local grid boundaries */
  PetscCall(DMDAGetCorners(da, &xs, &ys, NULL, &xm, &ym, NULL));

  /* Compute function over the locally owned part of the grid */
  /* the interior points */
  xend = xs + xm;
  yend = ys + ym;
  for (j = ys; j < yend; j++) {
    for (i = xs; i < xend; i++) {
      Ts = X[j][i].Ts;
      u  = X[j][i].u;
      v  = X[j][i].v;
      p  = X[j][i].p; /*P = X[j][i].P; */

      sfctemp1 = (double)Ts;
      PetscCall(calcfluxs(sfctemp1, airtemp, emma, fract, Tc, &fsfc1));       /* calculates surface net radiative flux */
      PetscCall(sensibleflux(sfctemp1, airtemp, wind, &sheat));               /* calculate sensible heat flux */
      PetscCall(latentflux(sfctemp1, dewtemp, wind, pressure1, &latentheat)); /* calculates latent heat flux */
      PetscCall(calc_gflux(sfctemp1, deep_grnd_temp, &groundflux));           /* calculates flux from earth below surface soil layer by conduction */
      PetscCall(calcfluxa(sfctemp1, airtemp, emma, &Ra));                     /* Calculates the change in downward radiative flux */
      fsfc1 = fsfc1 + latentheat + sheat + groundflux;                        /* adds radiative, sensible heat, latent heat, and ground heat flux yielding net flux */

      /* convective coefficients for upwinding */
      u_abs   = PetscAbsScalar(u);
      u_plus  = .5 * (u + u_abs); /* u if u>0; 0 if u<0 */
      u_minus = .5 * (u - u_abs); /* u if u <0; 0 if u>0 */

      v_abs   = PetscAbsScalar(v);
      v_plus  = .5 * (v + v_abs); /* v if v>0; 0 if v<0 */
      v_minus = .5 * (v - v_abs); /* v if v <0; 0 if v>0 */

      /* Solve governing equations */
      /* P = p*Rd*Ts; */

      /* du/dt -> time change of east-west component of the wind */
      Frhs[j][i].u = -u_plus * (u - X[j][i - 1].u) * dhx - u_minus * (X[j][i + 1].u - u) * dhx                                             /* - u(du/dx) */
                   - v_plus * (u - X[j - 1][i].u) * dhy - v_minus * (X[j + 1][i].u - u) * dhy                                              /* - v(du/dy) */
                   - (Rd / p) * (Ts * (X[j][i + 1].p - X[j][i - 1].p) * 0.5 * dhx + p * 0 * (X[j][i + 1].Ts - X[j][i - 1].Ts) * 0.5 * dhx) /* -(R/p)[Ts(dp/dx)+ p(dTs/dx)] */
                                                                                                                                           /*                     -(1/p)*(X[j][i+1].P - X[j][i-1].P)*dhx */
                   + f * v;

      /* dv/dt -> time change of north-south component of the wind */
      Frhs[j][i].v = -u_plus * (v - X[j][i - 1].v) * dhx - u_minus * (X[j][i + 1].v - v) * dhx                                             /* - u(dv/dx) */
                   - v_plus * (v - X[j - 1][i].v) * dhy - v_minus * (X[j + 1][i].v - v) * dhy                                              /* - v(dv/dy) */
                   - (Rd / p) * (Ts * (X[j + 1][i].p - X[j - 1][i].p) * 0.5 * dhy + p * 0 * (X[j + 1][i].Ts - X[j - 1][i].Ts) * 0.5 * dhy) /* -(R/p)[Ts(dp/dy)+ p(dTs/dy)] */
                                                                                                                                           /*                     -(1/p)*(X[j+1][i].P - X[j-1][i].P)*dhy */
                   - f * u;

      /* dT/dt -> time change of temperature */
      Frhs[j][i].Ts = (fsfc1 / (csoil * dzlay))                                                    /* Fnet/(Cp*dz)  diabatic change in T */
                    - u_plus * (Ts - X[j][i - 1].Ts) * dhx - u_minus * (X[j][i + 1].Ts - Ts) * dhx /* - u*(dTs/dx)  advection x */
                    - v_plus * (Ts - X[j - 1][i].Ts) * dhy - v_minus * (X[j + 1][i].Ts - Ts) * dhy /* - v*(dTs/dy)  advection y */
                    + diffconst * ((X[j][i + 1].Ts - 2 * Ts + X[j][i - 1].Ts) * dhx * dhx          /* + D(Ts_xx + Ts_yy)  diffusion */
                                   + (X[j + 1][i].Ts - 2 * Ts + X[j - 1][i].Ts) * dhy * dhy);

      /* dp/dt -> time change of */
      Frhs[j][i].p = -u_plus * (p - X[j][i - 1].p) * dhx - u_minus * (X[j][i + 1].p - p) * dhx /* - u*(dp/dx) */
                   - v_plus * (p - X[j - 1][i].p) * dhy - v_minus * (X[j + 1][i].p - p) * dhy; /* - v*(dp/dy) */

      Frhs[j][i].Ta = Ra / Cp; /* dTa/dt time change of air temperature */
    }
  }

  /* Restore vectors */
  PetscCall(DMDAVecRestoreArrayRead(da, localT, &X));
  PetscCall(DMDAVecRestoreArray(da, F, &Frhs));
  PetscCall(DMRestoreLocalVector(da, &localT));
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode Monitor(TS ts, PetscInt step, PetscReal time, Vec T, void *ctx)
{
  const PetscScalar *array;
  MonitorCtx        *user   = (MonitorCtx *)ctx;
  PetscViewer        viewer = user->drawviewer;
  PetscReal          norm;

  PetscFunctionBeginUser;
  PetscCall(VecNorm(T, NORM_INFINITY, &norm));

  if (step % user->interval == 0) {
    PetscCall(VecGetArrayRead(T, &array));
    PetscCall(PetscPrintf(PETSC_COMM_WORLD, "step %" PetscInt_FMT ", time %8.1f,  %6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f\n", step, (double)time, (double)(((array[0] - 273) * 9) / 5 + 32), (double)(((array[1] - 273) * 9) / 5 + 32), (double)array[2], (double)array[3], (double)array[4], (double)array[5]));
    PetscCall(VecRestoreArrayRead(T, &array));
  }

  if (user->drawcontours) PetscCall(VecView(T, viewer));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*TEST

   build:
      requires: !complex !single

   test:
      args: -ts_max_steps 130 -monitor_interval 60
      output_file: output/ex5.out
      requires: !complex !single
      localrunfiles: ex5_control.txt

   test:
      suffix: 2
      nsize: 4
      args: -ts_max_steps 130 -monitor_interval 60
      output_file: output/ex5.out
      localrunfiles: ex5_control.txt
      requires: !complex !single

   # Test TSPruneIJacobianColor() for improved FD coloring
   test:
      suffix: 3
      nsize: 4
      args: -ts_max_steps 130 -monitor_interval 60 -prune_jacobian -mat_coloring_view
      requires: !complex !single
      localrunfiles: ex5_control.txt

TEST*/
