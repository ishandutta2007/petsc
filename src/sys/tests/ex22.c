static char help[] = "Tests the PetscByteSwap()\n";

#include <petscsys.h>

int main(int argc, char **argv)
{
  PetscInt    oint[2], sint[2];
  PetscBool   obool[2], sbool[2];
  PetscScalar oscalar[2], sscalar[2];
  double      odouble[2], sdouble[2];
  float       ofloat[2], sfloat[2];
  short       oshort[2], sshort[2];

  PetscFunctionBeginUser;
  PetscCall(PetscInitialize(&argc, &argv, NULL, help));

  sint[0] = oint[0] = 5;
  sint[1] = oint[1] = 19;
  sbool[0] = obool[0] = PETSC_FALSE;
  sbool[1] = obool[1] = PETSC_TRUE;
  sscalar[0] = oscalar[0] = 3.14159265;
  sscalar[1] = oscalar[1] = 1.3806504e-23;
  sdouble[0] = odouble[0] = 3.14159265;
  sdouble[1] = odouble[1] = 1.3806504e-23;
  sfloat[0] = ofloat[0] = (float)3.14159265;
  sfloat[1] = ofloat[1] = (float)1.3806504e-23;
  sshort[0] = oshort[0] = 5;
  sshort[1] = oshort[1] = 19;

  PetscCall(PetscByteSwap(sint, PETSC_INT, 2));
  PetscCall(PetscByteSwap(sbool, PETSC_BOOL, 2));
  PetscCall(PetscByteSwap(sscalar, PETSC_SCALAR, 2));
  PetscCall(PetscByteSwap(sdouble, PETSC_DOUBLE, 2));
  PetscCall(PetscByteSwap(sfloat, PETSC_FLOAT, 2));
  PetscCall(PetscByteSwap(sshort, PETSC_SHORT, 2));

  PetscCall(PetscByteSwap(sint, PETSC_INT, 2));
  PetscCall(PetscByteSwap(sbool, PETSC_BOOL, 2));
  PetscCall(PetscByteSwap(sscalar, PETSC_SCALAR, 2));
  PetscCall(PetscByteSwap(sdouble, PETSC_DOUBLE, 2));
  PetscCall(PetscByteSwap(sfloat, PETSC_FLOAT, 2));
  PetscCall(PetscByteSwap(sshort, PETSC_SHORT, 2));

  if ((sint[0] != oint[0]) || (sint[1] != oint[1])) PetscCall(PetscPrintf(PETSC_COMM_SELF, "Byteswap mismatch for PETSC_INT\n"));
  if ((sbool[0] != obool[0]) || (sbool[1] != obool[1])) PetscCall(PetscPrintf(PETSC_COMM_SELF, "Byteswap mismatch for PETSC_BOOL\n"));
  if ((sscalar[0] != oscalar[0]) || (sscalar[1] != oscalar[1])) PetscCall(PetscPrintf(PETSC_COMM_SELF, "Byteswap mismatch for PETSC_SCALAR\n"));
  if ((sdouble[0] != odouble[0]) || (sdouble[1] != odouble[1])) PetscCall(PetscPrintf(PETSC_COMM_SELF, "Byteswap mismatch for PETSC_DOUBLE\n"));
  if ((sfloat[0] != ofloat[0]) || (sfloat[1] != ofloat[1])) PetscCall(PetscPrintf(PETSC_COMM_SELF, "Byteswap mismatch for PETSC_FLOAT\n"));
  if ((sshort[0] != oshort[0]) || (sshort[1] != oshort[1])) PetscCall(PetscPrintf(PETSC_COMM_SELF, "Byteswap mismatch for PETSC_SHORT\n"));

  PetscCall(PetscFinalize());
  return 0;
}

/*TEST

   test:
     output_file: output/empty.out

TEST*/
