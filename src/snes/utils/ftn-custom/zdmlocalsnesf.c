#include <petsc/private/ftnimpl.h>
#include <petsc/private/snesimpl.h>
#if defined(PETSC_HAVE_FORTRAN_CAPS)
  #define dmsnessetjacobianlocal_ DMSNESSETJACOBIANLOCAL
  #define dmsnessetfunctionlocal_ DMSNESSETFUNCTIONLOCAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
  #define dmsnessetjacobianlocal_ dmsnessetjacobianlocal
  #define dmsnessetfunctionlocal_ dmsnessetfunctionlocal
#endif

static struct {
  PetscFortranCallbackId lf;
  PetscFortranCallbackId lj;
} _cb;

static PetscErrorCode sourlj(DM dm, Vec X, Mat J, Mat P, void *ptr)
{
  void (*func)(DM *, Vec *, Mat *, Mat *, void *, PetscErrorCode *), *ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  PetscCall(DMGetDMSNES(dm, &sdm));
  PetscCall(PetscObjectGetFortranCallback((PetscObject)sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, _cb.lj, (PetscVoidFn **)&func, &ctx));
  PetscCallFortranVoidFunction((*func)(&dm, &X, &J, &P, ctx, &ierr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

PETSC_EXTERN void dmsnessetjacobianlocal_(DM *dm, void (*jac)(DM *, Vec *, Mat *, Mat *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
  DMSNES sdm;

  *ierr = DMGetDMSNESWrite(*dm, &sdm);
  if (*ierr) return;
  *ierr = PetscObjectSetFortranCallback((PetscObject)sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, &_cb.lj, (PetscVoidFn *)jac, ctx);
  if (*ierr) return;
  *ierr = DMSNESSetJacobianLocal(*dm, sourlj, NULL);
}

static PetscErrorCode sourlf(DM dm, Vec X, Vec F, void *ptr)
{
  void (*func)(DM *, Vec *, Vec *, void *, PetscErrorCode *), *ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  PetscCall(DMGetDMSNES(dm, &sdm));
  PetscCall(PetscObjectGetFortranCallback((PetscObject)sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, _cb.lf, (PetscVoidFn **)&func, &ctx));
  PetscCallFortranVoidFunction((*func)(&dm, &X, &F, ctx, &ierr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

PETSC_EXTERN void dmsnessetfunctionlocal_(DM *dm, void (*func)(DM *, Vec *, Vec *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
  DMSNES sdm;

  *ierr = DMGetDMSNESWrite(*dm, &sdm);
  if (*ierr) return;
  *ierr = PetscObjectSetFortranCallback((PetscObject)sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, &_cb.lf, (PetscVoidFn *)func, ctx);
  if (*ierr) return;
  *ierr = DMSNESSetFunctionLocal(*dm, sourlf, NULL);
}
