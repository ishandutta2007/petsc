/*
      Code for manipulating files.
*/
#include <petscsys.h>
#if defined(PETSC_HAVE_WINDOWS_H)
  #include <windows.h>
#endif

#if defined(PETSC_HAVE_GET_USER_NAME)
PetscErrorCode PetscGetUserName(char name[], size_t nlen)
{
  PetscFunctionBegin;
  GetUserName((LPTSTR)name, (LPDWORD)(&nlen));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#else
/*@C
  PetscGetUserName - Returns the name of the user.

  Not Collective

  Input Parameter:
. nlen - length of name

  Output Parameter:
. name - contains user name. Must be long enough to hold the name

  Level: developer

  Fortran Note:
.vb
  character*(32) str
  call PetscGetUserName(str,ierr)
.ve

.seealso: `PetscGetHostName()`
@*/
PetscErrorCode PetscGetUserName(char name[], size_t nlen)
{
  const char *user;

  PetscFunctionBegin;
  user = getenv("USER");
  if (!user) user = "Unknown";
  PetscCall(PetscStrncpy(name, user, nlen));
  PetscFunctionReturn(PETSC_SUCCESS);
}
#endif
