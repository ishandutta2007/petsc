#pragma once

#include <petscsys.h>

/* MANSEC = Sys */
/* SUBMANSEC = Bag */

/*S
   PetscBag - PETSc object that manages a collection of user data including parameters.
              A bag is essentially a C struct with serialization (you can save it and load it from files).

   Level: beginner

   Sample Usage:
.vb
      typedef struct {
         PetscInt     height;
         PetscScalar  root;
         PetscReal    byebye;
      } MyParameters;

      PetscBag     bag;
      MyParameters *params;

      PetscCall(PetscBagCreate(PETSC_COMM_WORLD,sizeof(MyParameters),&bag));
      PetscCall(PetscBagGetData(bag,(void **)&params));
      PetscCall(PetscBagSetName(bag,"MyParameters"));
      PetscCall(PetscBagRegisterInt(bag,&params.height,22,"height","Height of the water tower"));
.ve
   Note:
  `PetscBag` is a C struct, it is not a `PetscObject`

.seealso: `PetscBagSetName()`, `PetscBagGetName()`, `PetscBagView()`, `PetscBagLoad()`, `PetscBagGetData()`
          `PetscBagRegisterReal()`, `PetscBagRegisterInt()`, `PetscBagRegisterBool()`, `PetscBagRegisterScalar()`
          `PetscBagSetFromOptions()`, `PetscBagRegisterVec()`, `PetscBagCreate()`, `PetscBagDestroy()`, `PetscBagRegisterEnum()`
S*/
typedef struct _n_PetscBag     *PetscBag;
typedef struct _n_PetscBagItem *PetscBagItem;

PETSC_EXTERN PetscErrorCode PetscBagCreate(MPI_Comm, size_t, PetscBag *);
PETSC_EXTERN PetscErrorCode PetscBagDestroy(PetscBag *);
PETSC_EXTERN PetscErrorCode PetscBagGetData(PetscBag, void *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterReal(PetscBag, void *, PetscReal, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterRealArray(PetscBag, void *, PetscInt, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterString(PetscBag, void *, PetscInt, const char *, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterScalar(PetscBag, void *, PetscScalar, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterInt(PetscBag, void *, PetscInt, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterInt64(PetscBag, void *, PetscInt64, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterIntArray(PetscBag, void *, PetscInt, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterEnum(PetscBag, void *, const char *const *, PetscEnum, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterBool(PetscBag, void *, PetscBool, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagRegisterBoolArray(PetscBag, void *, PetscInt, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagGetNames(PetscBag, const char *[]);

PETSC_EXTERN PetscErrorCode PetscBagSetFromOptions(PetscBag);
PETSC_EXTERN PetscErrorCode PetscBagGetName(PetscBag, const char **);
PETSC_EXTERN PetscErrorCode PetscBagSetName(PetscBag, const char *, const char *);
PETSC_EXTERN PetscErrorCode PetscBagSetOptionsPrefix(PetscBag, const char *);

PETSC_EXTERN PetscErrorCode PetscBagView(PetscBag, PetscViewer);
PETSC_EXTERN PetscErrorCode PetscBagLoad(PetscViewer, PetscBag);
PETSC_EXTERN PetscErrorCode PetscBagViewFromOptions(PetscBag, PetscObject, const char[]);

#define PETSC_BAG_FILE_CLASSID 1211219
