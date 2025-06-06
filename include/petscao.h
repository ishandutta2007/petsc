#pragma once

#include <petscis.h>

/* MANSEC = Vec */
/* SUBMANSEC = AO */

/*S
   AO - Abstract PETSc object that manages mapping between different global numberings

   Level: intermediate

   Note:
   An application ordering is usually a mapping between an application-centric
   numbering (the ordering that is "natural" for the application) and
   the parallel numbering ($0$ to $n_0-1$ on the first MPI process, $n_0$ to $n_1 - 1$ on the second MPI process, etc)
   that PETSc uses.

.seealso: `AOCreateBasic()`, `AOCreateBasicIS()`, `AOPetscToApplication()`, `AOView()`, `AOApplicationToPetsc()`, `AOType`, `AOSetType()`
S*/
typedef struct _p_AO *AO;

/*J
    AOType - String with the name of a PETSc application ordering type

   Level: beginner

.seealso: `AOSetType()`, `AO`, `AOApplicationToPetsc()`, `AOCreateBasic()`, `AOCreate()`
J*/
typedef const char *AOType;
#define AOBASIC          "basic"
#define AOADVANCED       "advanced"
#define AOMAPPING        "mapping"
#define AOMEMORYSCALABLE "memoryscalable"

/* Logging support */
PETSC_EXTERN PetscClassId AO_CLASSID;

PETSC_EXTERN PetscErrorCode AOInitializePackage(void);
PETSC_EXTERN PetscErrorCode AOFinalizePackage(void);

PETSC_EXTERN PetscErrorCode AOCreate(MPI_Comm, AO *);
PETSC_EXTERN PetscErrorCode AOSetIS(AO, IS, IS);
PETSC_EXTERN PetscErrorCode AOSetFromOptions(AO);

PETSC_EXTERN PetscErrorCode AOCreateBasic(MPI_Comm, PetscInt, const PetscInt[], const PetscInt[], AO *);
PETSC_EXTERN PetscErrorCode AOCreateBasicIS(IS, IS, AO *);
PETSC_EXTERN PetscErrorCode AOCreateMemoryScalable(MPI_Comm, PetscInt, const PetscInt[], const PetscInt[], AO *);
PETSC_EXTERN PetscErrorCode AOCreateMemoryScalableIS(IS, IS, AO *);
PETSC_EXTERN PetscErrorCode AOCreateMapping(MPI_Comm, PetscInt, const PetscInt[], const PetscInt[], AO *);
PETSC_EXTERN PetscErrorCode AOCreateMappingIS(IS, IS, AO *);

PETSC_EXTERN PetscErrorCode AOView(AO, PetscViewer);
PETSC_EXTERN PetscErrorCode AOViewFromOptions(AO, PetscObject, const char[]);
PETSC_EXTERN PetscErrorCode AODestroy(AO *);

/* Dynamic creation and loading functions */
PETSC_EXTERN PetscErrorCode AOSetType(AO, AOType);
PETSC_EXTERN PetscErrorCode AOGetType(AO, AOType *);

PETSC_EXTERN PetscErrorCode AORegister(const char[], PetscErrorCode (*)(AO));
PETSC_EXTERN PetscErrorCode AORegisterAll(void);

PETSC_EXTERN PetscErrorCode AOPetscToApplication(AO, PetscInt, PetscInt[]);
PETSC_EXTERN PetscErrorCode AOApplicationToPetsc(AO, PetscInt, PetscInt[]);
PETSC_EXTERN PetscErrorCode AOPetscToApplicationIS(AO, IS);
PETSC_EXTERN PetscErrorCode AOApplicationToPetscIS(AO, IS);

PETSC_EXTERN PetscErrorCode AOPetscToApplicationPermuteInt(AO, PetscInt, PetscInt[]);
PETSC_EXTERN PetscErrorCode AOApplicationToPetscPermuteInt(AO, PetscInt, PetscInt[]);
PETSC_EXTERN PetscErrorCode AOPetscToApplicationPermuteReal(AO, PetscInt, PetscReal[]);
PETSC_EXTERN PetscErrorCode AOApplicationToPetscPermuteReal(AO, PetscInt, PetscReal[]);

PETSC_EXTERN PetscErrorCode AOMappingHasApplicationIndex(AO, PetscInt, PetscBool *);
PETSC_EXTERN PetscErrorCode AOMappingHasPetscIndex(AO, PetscInt, PetscBool *);
