static char help[] = "Show MatShift BUG happening after copying a matrix with no rows on a process";
/*
   Contributed by: Eric Chamberland
*/
#include <petscmat.h>

/* DEFINE this to turn on/off the bug: */
#define SET_2nd_PROC_TO_HAVE_NO_LOCAL_LINES

int main(int argc, char **args)
{
  Mat         C;
  PetscInt    i, m = 3;
  PetscMPIInt rank, size;
  PetscScalar v;
  Mat         lMatA;
  PetscInt    locallines;
  PetscInt    d_nnz[3] = {0, 0, 0};
  PetscInt    o_nnz[3] = {0, 0, 0};

  PetscFunctionBeginUser;
  PetscCall(PetscInitialize(&argc, &args, NULL, help));
  PetscCallMPI(MPI_Comm_rank(PETSC_COMM_WORLD, &rank));
  PetscCallMPI(MPI_Comm_size(PETSC_COMM_WORLD, &size));

  PetscCheck(2 == size, PETSC_COMM_WORLD, PETSC_ERR_ARG_INCOMP, "Relevant with 2 processes only");
  PetscCall(MatCreate(PETSC_COMM_WORLD, &C));

#ifdef SET_2nd_PROC_TO_HAVE_NO_LOCAL_LINES
  if (0 == rank) {
    locallines = m;
    d_nnz[0]   = 1;
    d_nnz[1]   = 1;
    d_nnz[2]   = 1;
  } else {
    locallines = 0;
  }
#else
  if (0 == rank) {
    locallines = m - 1;
    d_nnz[0]   = 1;
    d_nnz[1]   = 1;
  } else {
    locallines = 1;
    d_nnz[0]   = 1;
  }
#endif

  PetscCall(MatSetSizes(C, locallines, locallines, m, m));
  PetscCall(MatSetFromOptions(C));
  PetscCall(MatXAIJSetPreallocation(C, 1, d_nnz, o_nnz, NULL, NULL));

  v = 2;
  /* Assembly on the diagonal: */
  for (i = 0; i < m; i++) PetscCall(MatSetValues(C, 1, &i, 1, &i, &v, ADD_VALUES));
  PetscCall(MatAssemblyBegin(C, MAT_FINAL_ASSEMBLY));
  PetscCall(MatAssemblyEnd(C, MAT_FINAL_ASSEMBLY));
  PetscCall(MatSetOption(C, MAT_NEW_NONZERO_LOCATION_ERR, PETSC_TRUE));
  PetscCall(MatSetOption(C, MAT_KEEP_NONZERO_PATTERN, PETSC_TRUE));
  PetscCall(MatView(C, PETSC_VIEWER_STDOUT_WORLD));
  PetscCall(MatConvert(C, MATSAME, MAT_INITIAL_MATRIX, &lMatA));
  PetscCall(MatView(lMatA, PETSC_VIEWER_STDOUT_WORLD));

  PetscCall(MatShift(lMatA, -1.0));

  PetscCall(MatDestroy(&lMatA));
  PetscCall(MatDestroy(&C));
  PetscCall(PetscFinalize());
  return 0;
}

/*TEST

   test:
      nsize: 2

TEST*/
