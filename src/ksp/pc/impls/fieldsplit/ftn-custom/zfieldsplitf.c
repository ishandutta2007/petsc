#include <petsc/private/ftnimpl.h>
#include <petscksp.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
  #define pcfieldsplitgetsubksp_      PCFIELDSPLITGETSUBKSP
  #define pcfieldsplitschurgetsubksp_ PCFIELDSPLITSCHURGETSUBKSP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
  #define pcfieldsplitgetsubksp_      pcfieldsplitgetsubksp
  #define pcfieldsplitschurgetsubksp_ pcfieldsplitschurgetsubksp
#endif

PETSC_EXTERN void pcfieldsplitschurgetsubksp_(PC *pc, PetscInt *n_local, KSP *ksp, PetscErrorCode *ierr)
{
  KSP     *tksp;
  PetscInt i, nloc;
  CHKFORTRANNULLINTEGER(n_local);
  *ierr = PCFieldSplitSchurGetSubKSP(*pc, &nloc, &tksp);
  if (*ierr) return;
  if (n_local) *n_local = nloc;
  CHKFORTRANNULLOBJECT(ksp);
  if (ksp) {
    for (i = 0; i < nloc; i++) ksp[i] = tksp[i];
  }
  *ierr = PetscFree(tksp);
}

PETSC_EXTERN void pcfieldsplitgetsubksp_(PC *pc, PetscInt *n_local, KSP *ksp, PetscErrorCode *ierr)
{
  KSP     *tksp;
  PetscInt i, nloc;
  CHKFORTRANNULLINTEGER(n_local);
  *ierr = PCFieldSplitGetSubKSP(*pc, &nloc, &tksp);
  if (*ierr) return;
  if (n_local) *n_local = nloc;
  CHKFORTRANNULLOBJECT(ksp);
  if (ksp) {
    for (i = 0; i < nloc; i++) ksp[i] = tksp[i];
  }
  *ierr = PetscFree(tksp);
}
