#include <petsc/private/tsimpl.h> /*I "petscts.h"  I*/

PetscFunctionList TSList              = NULL;
PetscBool         TSRegisterAllCalled = PETSC_FALSE;

/*@
  TSSetType - Sets the algorithm/method to be used for integrating the ODE with the given `TS`.

  Collective

  Input Parameters:
+ ts   - The `TS` context
- type - A known method

  Options Database Key:
. -ts_type <type> - Sets the method; use -help for a list of available methods (for instance, euler)

  Level: intermediate

  Notes:
  See `TSType` for available methods (for instance)
+  TSEULER - Euler
.  TSBEULER - Backward Euler
-  TSPSEUDO - Pseudo-timestepping

  Normally, it is best to use the `TSSetFromOptions()` command and
  then set the `TS` type from the options database rather than by using
  this routine.  Using the options database provides the user with
  maximum flexibility in evaluating the many different solvers.
  The TSSetType() routine is provided for those situations where it
  is necessary to set the timestepping solver independently of the
  command line or options database.  This might be the case, for example,
  when the choice of solver changes during the execution of the
  program, and the user's application is taking responsibility for
  choosing the appropriate method.  In other words, this routine is
  not for beginners.

.seealso: [](ch_ts), `TS`, `TSSolve()`, `TSCreate()`, `TSSetFromOptions()`, `TSDestroy()`, `TSType`
@*/
PetscErrorCode TSSetType(TS ts, TSType type)
{
  PetscErrorCode (*r)(TS);
  PetscBool match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(type, 2);
  PetscCall(PetscObjectTypeCompare((PetscObject)ts, type, &match));
  if (match) PetscFunctionReturn(PETSC_SUCCESS);

  PetscCall(PetscFunctionListFind(TSList, type, &r));
  PetscCheck(r, PetscObjectComm((PetscObject)ts), PETSC_ERR_ARG_UNKNOWN_TYPE, "Unknown TS type: %s", type);
  PetscTryTypeMethod(ts, destroy);
  PetscCall(PetscMemzero(ts->ops, sizeof(*ts->ops)));
  ts->usessnes           = PETSC_FALSE;
  ts->default_adapt_type = TSADAPTNONE;

  ts->setupcalled = PETSC_FALSE;

  PetscCall(PetscObjectChangeTypeName((PetscObject)ts, type));
  PetscCall((*r)(ts));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  TSGetType - Gets the `TS` method type (as a string) that is being used to solve the ODE with the given `TS`

  Not Collective

  Input Parameter:
. ts - The `TS`

  Output Parameter:
. type - The `TSType`

  Level: intermediate

.seealso: [](ch_ts), `TS`, `TSType`, `TSSetType()`
@*/
PetscErrorCode TSGetType(TS ts, TSType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts, TS_CLASSID, 1);
  PetscAssertPointer(type, 2);
  *type = ((PetscObject)ts)->type_name;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  TSRegister - Adds a creation method to the `TS` package.

  Not Collective, No Fortran Support

  Input Parameters:
+ sname    - The name of a new user-defined creation routine
- function - The creation routine itself

  Level: advanced

  Notes:
  `TSRegister()` may be called multiple times to add several user-defined tses.

  Example Usage:
.vb
  TSRegister("my_ts",  MyTSCreate);
.ve

  Then, your ts type can be chosen with the procedural interface via
.vb
    TS ts;
    TSCreate(MPI_Comm, &ts);
    TSSetType(ts, "my_ts")
.ve
  or at runtime via the option
.vb
    -ts_type my_ts
.ve

.seealso: [](ch_ts), `TSSetType()`, `TSType`, `TSRegisterAll()`, `TSRegisterDestroy()`
@*/
PetscErrorCode TSRegister(const char sname[], PetscErrorCode (*function)(TS))
{
  PetscFunctionBegin;
  PetscCall(TSInitializePackage());
  PetscCall(PetscFunctionListAdd(&TSList, sname, function));
  PetscFunctionReturn(PETSC_SUCCESS);
}
