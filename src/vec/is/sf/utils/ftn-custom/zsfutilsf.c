#include <petsc/private/fortranimpl.h>
#include <petscsf.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
  #define petscsfdistributesection_ PETSCSFDISTRIBUTESECTION
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
  #define petscsfdistributesection_ petscsfdistributesection
#endif

PETSC_EXTERN void petscsfdistributesection_(PetscSF *sf, PetscSection *rootSection, PetscInt **remoteOffsets, PetscSection *leafSection, PetscErrorCode *ierr)
{
  if (remoteOffsets != PETSC_NULL_INTEGER_Fortran) {
    (void)PetscError(PETSC_COMM_SELF, __LINE__, "PetscSFDistributeSection_Fortran", __FILE__, PETSC_ERR_SUP, PETSC_ERROR_INITIAL, "The remoteOffsets argument must be PETSC_NULL_INTEGER in Fortran");
    *ierr = PETSC_ERR_SUP;
    return;
  }
  *ierr = PetscSFDistributeSection(*sf, *rootSection, NULL, *leafSection);
}
