static char help[] = "Tests PetscRandom functions.\n\n";

#include <petscsys.h>

#define PETSC_MAXBSIZE 40
#define DATAFILENAME   "ex2_stock.txt"

struct himaInfoTag {
  PetscInt   n;
  PetscReal  r;
  PetscReal  dt;
  PetscInt   totalNumSim;
  PetscReal *St0;
  PetscReal *vol;
};
typedef struct himaInfoTag himaInfo;

PetscErrorCode readData(MPI_Comm, himaInfo *);
PetscReal      mcVal(PetscReal, PetscReal, PetscReal, PetscReal, PetscReal);
void           exchangeVal(PetscReal *, PetscReal *);
PetscReal      basketPayoff(PetscReal[], PetscReal[], PetscInt, PetscReal, PetscReal, PetscReal[]);
PetscErrorCode stdNormalArray(PetscReal *, PetscInt, PetscRandom);
PetscInt       divWork(PetscMPIInt, PetscInt, PetscMPIInt);

/*
   Contributed by Xiaoyan Zeng <zengxia@iit.edu> and Liu, Kwong Ip" <kiliu@math.hkbu.edu.hk>

   Example of usage:
     mpiexec -n 4 ./ex2 -num_of_stocks 30 -interest_rate 0.4 -time_interval 0.01 -num_of_simulations 10000
*/

int main(int argc, char *argv[])
{
  PetscReal   r, dt;
  PetscInt    n;
  PetscInt    i, myNumSim, totalNumSim, numdim;
  PetscReal  *vol, *St0, x, totalx;
  PetscMPIInt size, rank;
  PetscReal  *eps;
  himaInfo    hinfo;
  PetscRandom ran;

  PetscFunctionBeginUser;
  PetscCall(PetscInitialize(&argc, &argv, NULL, help));
  PetscCall(PetscRandomCreate(PETSC_COMM_WORLD, &ran));
  PetscCall(PetscRandomSetFromOptions(ran));

  PetscCallMPI(MPI_Comm_size(PETSC_COMM_WORLD, &size));
  PetscCallMPI(MPI_Comm_rank(PETSC_COMM_WORLD, &rank));

  hinfo.n           = 31;
  hinfo.r           = 0.04;
  hinfo.dt          = 1.0 / 12; /* a month as a period */
  hinfo.totalNumSim = 1000;

  PetscCall(PetscOptionsGetInt(NULL, NULL, "-num_of_stocks", &hinfo.n, NULL));
  PetscCheck(hinfo.n >= 1 && hinfo.n <= 31, PETSC_COMM_SELF, PETSC_ERR_SUP, "Only 31 stocks listed in stock.txt. num_of_stocks %" PetscInt_FMT " must between 1 and 31", hinfo.n);
  PetscCall(PetscOptionsGetReal(NULL, NULL, "-interest_rate", &hinfo.r, NULL));
  PetscCall(PetscOptionsGetReal(NULL, NULL, "-time_interval", &hinfo.dt, NULL));
  PetscCall(PetscOptionsGetInt(NULL, NULL, "-num_of_simulations", &hinfo.totalNumSim, NULL));

  n           = hinfo.n;
  r           = hinfo.r;
  dt          = hinfo.dt;
  totalNumSim = hinfo.totalNumSim;
  PetscCall(PetscMalloc1(2 * n + 1, &hinfo.vol));
  vol = hinfo.vol;
  St0 = hinfo.St0 = hinfo.vol + n;
  PetscCall(readData(PETSC_COMM_WORLD, &hinfo));

  numdim = n * (n + 1) / 2;
  if (numdim % 2 == 1) numdim++;
  PetscCall(PetscMalloc1(numdim, &eps));

  myNumSim = divWork(rank, totalNumSim, size);

  x = 0;
  for (i = 0; i < myNumSim; i++) {
    PetscCall(stdNormalArray(eps, numdim, ran));
    x += basketPayoff(vol, St0, n, r, dt, eps);
  }

  PetscCallMPI(MPI_Reduce(&x, &totalx, 1, MPIU_REAL, MPIU_SUM, 0, PETSC_COMM_WORLD));
  /* payoff = exp(-r*dt*n)*(totalx/totalNumSim);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Option price = $%.3f using %ds of %s computation with %d %s for %d stocks, %d trading period per year, %.2f%% interest rate\n",
   payoff,(int)(stop - start),"parallel",size,"processors",n,(int)(1/dt),r); */

  PetscCall(PetscFree(vol));
  PetscCall(PetscFree(eps));
  PetscCall(PetscRandomDestroy(&ran));
  PetscCall(PetscFinalize());
  return 0;
}

PetscErrorCode stdNormalArray(PetscReal *eps, PetscInt numdim, PetscRandom ran)
{
  PetscInt    i;
  PetscScalar u1, u2;
  PetscReal   t;

  PetscFunctionBegin;
  for (i = 0; i < numdim; i += 2) {
    PetscCall(PetscRandomGetValue(ran, &u1));
    PetscCall(PetscRandomGetValue(ran, &u2));

    t          = PetscSqrtReal(-2 * PetscLogReal(PetscRealPart(u1)));
    eps[i]     = t * PetscCosReal(2 * PETSC_PI * PetscRealPart(u2));
    eps[i + 1] = t * PetscSinReal(2 * PETSC_PI * PetscRealPart(u2));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscReal basketPayoff(PetscReal vol[], PetscReal St0[], PetscInt n, PetscReal r, PetscReal dt, PetscReal eps[])
{
  PetscReal Stk[PETSC_MAXBSIZE], temp;
  PetscReal payoff;
  PetscInt  maxk, i, j;
  PetscInt  pointcount = 0;

  for (i = 0; i < n; i++) Stk[i] = St0[i];

  for (i = 0; i < n; i++) {
    maxk = 0;
    for (j = 0; j < (n - i); j++) {
      Stk[j] = mcVal(Stk[j], r, vol[j], dt, eps[pointcount++]);
      if ((Stk[j] / St0[j]) > (Stk[maxk] / St0[maxk])) maxk = j;
    }
    exchangeVal(Stk + j - 1, Stk + maxk);
    exchangeVal(St0 + j - 1, St0 + maxk);
    exchangeVal(vol + j - 1, vol + maxk);
  }

  payoff = 0;
  for (i = 0; i < n; i++) {
    temp = (Stk[i] / St0[i]) - 1;
    if (temp > 0) payoff += temp;
  }
  return payoff;
}

PetscErrorCode readData(MPI_Comm comm, himaInfo *hinfo)
{
  PetscInt    i;
  FILE       *fd;
  char        temp[50];
  PetscMPIInt rank;
  PetscReal  *v = hinfo->vol, *t = hinfo->St0;
  PetscInt    num = hinfo->n;

  PetscFunctionBeginUser;
  PetscCallMPI(MPI_Comm_rank(comm, &rank));
  if (rank == 0) {
    PetscCall(PetscFOpen(PETSC_COMM_SELF, DATAFILENAME, "r", &fd));
    for (i = 0; i < num; i++) {
      double vv, tt;
      PetscCheck(fscanf(fd, "%s%lf%lf", temp, &vv, &tt) == 3, PETSC_COMM_SELF, PETSC_ERR_FILE_UNEXPECTED, "Badly formatted input file");
      v[i] = vv;
      t[i] = tt;
    }
    fclose(fd);
  }
  PetscCallMPI(MPI_Bcast(v, (PetscMPIInt)(2 * num), MPIU_REAL, 0, PETSC_COMM_WORLD));
  /* ierr = PetscPrintf(PETSC_COMM_SELF,"[%d] vol %g, ... %g; St0 %g, ... %g\n",rank,hinfo->vol[0],hinfo->vol[num-1],hinfo->St0 [0],hinfo->St0[num-1]); */
  PetscFunctionReturn(PETSC_SUCCESS);
}

void exchangeVal(PetscReal *a, PetscReal *b)
{
  PetscReal t;

  t  = *a;
  *a = *b;
  *b = t;
}

PetscReal mcVal(PetscReal St, PetscReal r, PetscReal vol, PetscReal dt, PetscReal eps)
{
  return St * PetscExpReal((r - 0.5 * vol * vol) * dt + vol * PetscSqrtReal(dt) * eps);
}

PetscInt divWork(PetscMPIInt id, PetscInt num, PetscMPIInt size)
{
  PetscInt numit;

  numit = (PetscInt)(((PetscReal)num) / size);
  numit++;
  return numit;
}

/*TEST

   test:
      nsize: 2
      output_file: output/empty.out
      localrunfiles: ex2_stock.txt

TEST*/
