static char help[] = "Test MatMultHermitianTranspose() and MatMultHermitianTransposeAdd().\n\n";

#include <petscmat.h>

int main(int argc, char **args)
{
  Mat         A, B, C;
  Vec         x, y, ys;
  PetscInt    i, j;
  PetscScalar v;
  PetscBool   flg;

  PetscFunctionBeginUser;
  PetscCall(PetscInitialize(&argc, &args, NULL, help));
  PetscCall(MatCreate(PETSC_COMM_WORLD, &A));
  PetscCall(MatSetSizes(A, PETSC_DECIDE, PETSC_DECIDE, 2, 2));
  PetscCall(MatSetType(A, MATAIJ));
  PetscCall(MatSetFromOptions(A));
  PetscCall(MatSetUp(A));

  i = 0;
  j = 0;
  v = 2.0;
  PetscCall(MatSetValues(A, 1, &i, 1, &j, &v, INSERT_VALUES));
  i = 0;
  j = 1;
  v = 3.0 + 4.0 * PETSC_i;
  PetscCall(MatSetValues(A, 1, &i, 1, &j, &v, INSERT_VALUES));
  i = 1;
  j = 0;
  v = 5.0 + 6.0 * PETSC_i;
  PetscCall(MatSetValues(A, 1, &i, 1, &j, &v, INSERT_VALUES));
  i = 1;
  j = 1;
  v = 7.0 + 8.0 * PETSC_i;
  PetscCall(MatSetValues(A, 1, &i, 1, &j, &v, INSERT_VALUES));
  PetscCall(MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY));
  PetscCall(MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY));

  /* Create vectors */
  PetscCall(MatCreateVecs(A, &x, &y));
  PetscCall(VecDuplicate(y, &ys));

  i = 0;
  v = 10.0 + 11.0 * PETSC_i;
  PetscCall(VecSetValues(x, 1, &i, &v, INSERT_VALUES));
  i = 1;
  v = 100.0 + 120.0 * PETSC_i;
  PetscCall(VecSetValues(x, 1, &i, &v, INSERT_VALUES));
  PetscCall(VecAssemblyBegin(x));
  PetscCall(VecAssemblyEnd(x));

  PetscCall(MatMultHermitianTranspose(A, x, y));
  PetscCall(VecView(y, PETSC_VIEWER_STDOUT_WORLD));
  PetscCall(MatMultHermitianTransposeAdd(A, x, y, ys));
  PetscCall(VecView(ys, PETSC_VIEWER_STDOUT_WORLD));

  PetscCall(MatHermitianTranspose(A, MAT_INITIAL_MATRIX, &B));
  PetscCall(MatCreateHermitianTranspose(A, &C));
  PetscCall(MatMultHermitianTransposeEqual(B, C, 4, &flg));
  PetscCheck(flg, PETSC_COMM_WORLD, PETSC_ERR_PLIB, "B^Hx != C^Hx");
  PetscCall(MatMultHermitianTransposeAddEqual(B, C, 4, &flg));
  PetscCheck(flg, PETSC_COMM_WORLD, PETSC_ERR_PLIB, "y+B^Hx != y+C^Hx");
  PetscCall(MatDestroy(&C));
  PetscCall(MatDestroy(&B));

  PetscCall(MatDestroy(&A));

  PetscCall(VecDestroy(&x));
  PetscCall(VecDestroy(&y));
  PetscCall(VecDestroy(&ys));
  PetscCall(PetscFinalize());
  return 0;
}

/*TEST

   build:
      requires: complex

   testset:
      output_file: output/ex197_1.out
      test:
         suffix: 1
         args: -mat_type {{aij dense}}
      test:
         suffix: 1_cuda
         requires: cuda
         args: -mat_type densecuda
         filter: sed -e 's/seqcuda/seq/'

   testset:
      output_file: output/ex197_2.out
      nsize: 2
      test:
         suffix: 2
         args: -mat_type {{aij dense}}
      test:
         suffix: 2_scalapack
         requires: scalapack
         args: -mat_type scalapack

TEST*/
