#pragma once

/*
  This private file should not be included in users' code.  Defines the fields shared by all
  vector implementations.
*/

#include <petscvec.h>
#include <petsc/private/petscimpl.h>

PETSC_INTERN PetscBool VecRegisterAllCalled;
PETSC_EXTERN MPI_Op    MPIU_MAXLOC;
PETSC_EXTERN MPI_Op    MPIU_MINLOC;

/* ----------------------------------------------------------------------------*/

typedef struct _VecOps *VecOps;
struct _VecOps {
  PetscErrorCode (*duplicate)(Vec, Vec *);                                          /* get single vector */
  PetscErrorCode (*duplicatevecs)(Vec, PetscInt, Vec **);                           /* get array of vectors */
  PetscErrorCode (*destroyvecs)(PetscInt, Vec[]);                                   /* free array of vectors */
  PetscErrorCode (*dot)(Vec, Vec, PetscScalar *);                                   /* z = x^H * y */
  PetscErrorCode (*mdot)(Vec, PetscInt, const Vec[], PetscScalar *);                /* z[j] = x dot y[j] */
  PetscErrorCode (*norm)(Vec, NormType, PetscReal *);                               /* z = sqrt(x^H * x) */
  PetscErrorCode (*tdot)(Vec, Vec, PetscScalar *);                                  /* x'*y */
  PetscErrorCode (*mtdot)(Vec, PetscInt, const Vec[], PetscScalar *);               /* z[j] = x dot y[j] */
  PetscErrorCode (*scale)(Vec, PetscScalar);                                        /* x = alpha * x   */
  PetscErrorCode (*copy)(Vec, Vec);                                                 /* y = x */
  PetscErrorCode (*set)(Vec, PetscScalar);                                          /* y = alpha  */
  PetscErrorCode (*swap)(Vec, Vec);                                                 /* exchange x and y */
  PetscErrorCode (*axpy)(Vec, PetscScalar, Vec);                                    /* y = y + alpha * x */
  PetscErrorCode (*axpby)(Vec, PetscScalar, PetscScalar, Vec);                      /* y = alpha * x + beta * y*/
  PetscErrorCode (*maxpy)(Vec, PetscInt, const PetscScalar *, Vec *);               /* y = y + alpha[j] x[j] */
  PetscErrorCode (*aypx)(Vec, PetscScalar, Vec);                                    /* y = x + alpha * y */
  PetscErrorCode (*waxpy)(Vec, PetscScalar, Vec, Vec);                              /* w = y + alpha * x */
  PetscErrorCode (*axpbypcz)(Vec, PetscScalar, PetscScalar, PetscScalar, Vec, Vec); /* z = alpha * x + beta *y + gamma *z*/
  PetscErrorCode (*pointwisemult)(Vec, Vec, Vec);                                   /* w = x .* y */
  PetscErrorCode (*pointwisedivide)(Vec, Vec, Vec);                                 /* w = x ./ y */
  PetscErrorCode (*setvalues)(Vec, PetscInt, const PetscInt[], const PetscScalar[], InsertMode);
  PetscErrorCode (*assemblybegin)(Vec);            /* start global assembly */
  PetscErrorCode (*assemblyend)(Vec);              /* end global assembly */
  PetscErrorCode (*getarray)(Vec, PetscScalar **); /* get data array */
  PetscErrorCode (*getsize)(Vec, PetscInt *);
  PetscErrorCode (*getlocalsize)(Vec, PetscInt *);
  PetscErrorCode (*restorearray)(Vec, PetscScalar **); /* restore data array */
  PetscErrorCode (*max)(Vec, PetscInt *, PetscReal *); /* z = max(x); idx=index of max(x) */
  PetscErrorCode (*min)(Vec, PetscInt *, PetscReal *); /* z = min(x); idx=index of min(x) */
  PetscErrorCode (*setrandom)(Vec, PetscRandom);       /* set y[j] = random numbers */
  PetscErrorCode (*setoption)(Vec, VecOption, PetscBool);
  PetscErrorCode (*setvaluesblocked)(Vec, PetscInt, const PetscInt[], const PetscScalar[], InsertMode);
  PetscErrorCode (*destroy)(Vec);
  PetscErrorCode (*view)(Vec, PetscViewer);
  PetscErrorCode (*placearray)(Vec, const PetscScalar *);   /* place data array */
  PetscErrorCode (*replacearray)(Vec, const PetscScalar *); /* replace data array */
  PetscErrorCode (*dot_local)(Vec, Vec, PetscScalar *);
  PetscErrorCode (*tdot_local)(Vec, Vec, PetscScalar *);
  PetscErrorCode (*norm_local)(Vec, NormType, PetscReal *);
  PetscErrorCode (*mdot_local)(Vec, PetscInt, const Vec[], PetscScalar *);
  PetscErrorCode (*mtdot_local)(Vec, PetscInt, const Vec[], PetscScalar *);
  PetscErrorCode (*load)(Vec, PetscViewer);
  PetscErrorCode (*reciprocal)(Vec);
  PetscErrorCode (*conjugate)(Vec);
  PetscErrorCode (*setlocaltoglobalmapping)(Vec, ISLocalToGlobalMapping);
  PetscErrorCode (*getlocaltoglobalmapping)(Vec, ISLocalToGlobalMapping *);
  PetscErrorCode (*resetarray)(Vec); /* vector points to its original array, i.e. undoes any VecPlaceArray() */
  PetscErrorCode (*setfromoptions)(Vec, PetscOptionItems);
  PetscErrorCode (*maxpointwisedivide)(Vec, Vec, PetscReal *); /* m = max abs(x ./ y) */
  PetscErrorCode (*pointwisemax)(Vec, Vec, Vec);
  PetscErrorCode (*pointwisemaxabs)(Vec, Vec, Vec);
  PetscErrorCode (*pointwisemin)(Vec, Vec, Vec);
  PetscErrorCode (*getvalues)(Vec, PetscInt, const PetscInt[], PetscScalar[]);
  PetscErrorCode (*sqrt)(Vec);
  PetscErrorCode (*abs)(Vec);
  PetscErrorCode (*exp)(Vec);
  PetscErrorCode (*log)(Vec);
  PetscErrorCode (*shift)(Vec, PetscScalar);
  PetscErrorCode (*create)(Vec);
  PetscErrorCode (*stridegather)(Vec, PetscInt, Vec, InsertMode);
  PetscErrorCode (*stridescatter)(Vec, PetscInt, Vec, InsertMode);
  PetscErrorCode (*dotnorm2)(Vec, Vec, PetscScalar *, PetscScalar *);
  PetscErrorCode (*getsubvector)(Vec, IS, Vec *);
  PetscErrorCode (*restoresubvector)(Vec, IS, Vec *);
  PetscErrorCode (*getarrayread)(Vec, const PetscScalar **);
  PetscErrorCode (*restorearrayread)(Vec, const PetscScalar **);
  PetscErrorCode (*stridesubsetgather)(Vec, PetscInt, const PetscInt[], const PetscInt[], Vec, InsertMode);
  PetscErrorCode (*stridesubsetscatter)(Vec, PetscInt, const PetscInt[], const PetscInt[], Vec, InsertMode);
  PetscErrorCode (*viewnative)(Vec, PetscViewer);
  PetscErrorCode (*loadnative)(Vec, PetscViewer);
  PetscErrorCode (*createlocalvector)(Vec, Vec *);
  PetscErrorCode (*getlocalvector)(Vec, Vec);
  PetscErrorCode (*restorelocalvector)(Vec, Vec);
  PetscErrorCode (*getlocalvectorread)(Vec, Vec);
  PetscErrorCode (*restorelocalvectorread)(Vec, Vec);
  PetscErrorCode (*bindtocpu)(Vec, PetscBool);
  PetscErrorCode (*getarraywrite)(Vec, PetscScalar **);
  PetscErrorCode (*restorearraywrite)(Vec, PetscScalar **);
  PetscErrorCode (*getarrayandmemtype)(Vec, PetscScalar **, PetscMemType *);
  PetscErrorCode (*restorearrayandmemtype)(Vec, PetscScalar **);
  PetscErrorCode (*getarrayreadandmemtype)(Vec, const PetscScalar **, PetscMemType *);
  PetscErrorCode (*restorearrayreadandmemtype)(Vec, const PetscScalar **);
  PetscErrorCode (*getarraywriteandmemtype)(Vec, PetscScalar **, PetscMemType *);
  PetscErrorCode (*restorearraywriteandmemtype)(Vec, PetscScalar **, PetscMemType *);
  PetscErrorCode (*concatenate)(PetscInt, const Vec[], Vec *, IS *[]);
  PetscErrorCode (*sum)(Vec, PetscScalar *);
  PetscErrorCode (*setpreallocationcoo)(Vec, PetscCount, const PetscInt[]);
  PetscErrorCode (*setvaluescoo)(Vec, const PetscScalar[], InsertMode);
  PetscErrorCode (*errorwnorm)(Vec, Vec, Vec, NormType, PetscReal, Vec, PetscReal, Vec, PetscReal, PetscReal *, PetscInt *, PetscReal *, PetscInt *, PetscReal *, PetscInt *);
  PetscErrorCode (*maxpby)(Vec, PetscInt, const PetscScalar *, PetscScalar, Vec *); /* y = beta y + alpha[j] x[j] */
};

#if defined(offsetof) && (defined(__cplusplus) || (PETSC_C_VERSION >= 23))
static_assert(offsetof(struct _VecOps, duplicate) == sizeof(void (*)(void)) * VECOP_DUPLICATE, "");
static_assert(offsetof(struct _VecOps, set) == sizeof(void (*)(void)) * VECOP_SET, "");
static_assert(offsetof(struct _VecOps, view) == sizeof(void (*)(void)) * VECOP_VIEW, "");
static_assert(offsetof(struct _VecOps, load) == sizeof(void (*)(void)) * VECOP_LOAD, "");
static_assert(offsetof(struct _VecOps, viewnative) == sizeof(void (*)(void)) * VECOP_VIEWNATIVE, "");
static_assert(offsetof(struct _VecOps, loadnative) == sizeof(void (*)(void)) * VECOP_LOADNATIVE, "");
#endif

/*
    The stash is used to temporarily store inserted vec values that
  belong to another processor. During the assembly phase the stashed
  values are moved to the correct processor and
*/

typedef struct {
  PetscInt     nmax;     /* maximum stash size */
  PetscInt     umax;     /* max stash size user wants */
  PetscInt     oldnmax;  /* the nmax value used previously */
  PetscInt     n;        /* stash size */
  PetscInt     bs;       /* block size of the stash */
  PetscInt     reallocs; /* preserve the no of mallocs invoked */
  PetscInt    *idx;      /* global row numbers in stash */
  PetscScalar *array;    /* array to hold stashed values */
  /* The following variables are used for communication */
  MPI_Comm     comm;
  PetscMPIInt  size, rank;
  PetscMPIInt  tag1, tag2;
  MPI_Request *send_waits;        /* array of send requests */
  MPI_Request *recv_waits;        /* array of receive requests */
  MPI_Status  *send_status;       /* array of send status */
  PetscMPIInt  nsends, nrecvs;    /* numbers of sends and receives */
  PetscScalar *svalues, *rvalues; /* sending and receiving data */
  PetscInt    *sindices, *rindices;
  PetscInt     rmax;       /* maximum message length */
  PetscInt    *nprocs;     /* tmp data used both during scatterbegin and end */
  PetscMPIInt  nprocessed; /* number of messages already processed */
  PetscBool    donotstash;
  PetscBool    ignorenegidx; /* ignore negative indices passed into VecSetValues/VetGetValues */
  InsertMode   insertmode;
  PetscInt    *bowners;
} VecStash;

struct _p_Vec {
  PETSCHEADER(struct _VecOps);
  PetscLayout map;
  void       *data; /* implementation-specific data */
  PetscBool   array_gotten;
  VecStash    stash, bstash; /* used for storing off-proc values during assembly */
  PetscBool   petscnative;   /* means the ->data starts with VECHEADER and can use VecGetArrayFast()*/
  PetscInt    lock;          /* lock state. vector can be free (=0), locked for read (>0) or locked for write(<0) */
#if PetscDefined(USE_DEBUG)
  PetscStack lockstack; /* the file,func,line of where locks are added */
#endif
  PetscOffloadMask offloadmask; /* a mask which indicates where the valid vector data is (GPU, CPU or both) */
#if defined(PETSC_HAVE_DEVICE)
  void     *spptr; /* this is the special pointer to the array on the GPU */
  PetscBool boundtocpu;
  PetscBool bindingpropagates;
  size_t    minimum_bytes_pinned_memory; /* minimum data size in bytes for which pinned memory will be allocated */
  PetscBool pinned_memory;               /* PETSC_TRUE if the current host allocation has been made from pinned memory. */
#endif
  char *defaultrandtype;
};

PETSC_EXTERN PetscLogEvent VEC_SetRandom;
PETSC_EXTERN PetscLogEvent VEC_View;
PETSC_EXTERN PetscLogEvent VEC_Max;
PETSC_EXTERN PetscLogEvent VEC_Min;
PETSC_EXTERN PetscLogEvent VEC_Dot;
PETSC_EXTERN PetscLogEvent VEC_MDot;
PETSC_EXTERN PetscLogEvent VEC_TDot;
PETSC_EXTERN PetscLogEvent VEC_MTDot;
PETSC_EXTERN PetscLogEvent VEC_Norm;
PETSC_EXTERN PetscLogEvent VEC_Normalize;
PETSC_EXTERN PetscLogEvent VEC_Scale;
PETSC_EXTERN PetscLogEvent VEC_Shift;
PETSC_EXTERN PetscLogEvent VEC_Copy;
PETSC_EXTERN PetscLogEvent VEC_Set;
PETSC_EXTERN PetscLogEvent VEC_AXPY;
PETSC_EXTERN PetscLogEvent VEC_AYPX;
PETSC_EXTERN PetscLogEvent VEC_WAXPY;
PETSC_EXTERN PetscLogEvent VEC_MAXPY;
PETSC_EXTERN PetscLogEvent VEC_AssemblyEnd;
PETSC_EXTERN PetscLogEvent VEC_PointwiseMult;
PETSC_EXTERN PetscLogEvent VEC_PointwiseDivide;
PETSC_EXTERN PetscLogEvent VEC_Reciprocal;
PETSC_EXTERN PetscLogEvent VEC_SetValues;
PETSC_EXTERN PetscLogEvent VEC_SetPreallocateCOO;
PETSC_EXTERN PetscLogEvent VEC_SetValuesCOO;
PETSC_EXTERN PetscLogEvent VEC_Load;
PETSC_EXTERN PetscLogEvent VEC_ScatterBegin;
PETSC_EXTERN PetscLogEvent VEC_ScatterEnd;
PETSC_EXTERN PetscLogEvent VEC_ReduceArithmetic;
PETSC_EXTERN PetscLogEvent VEC_ReduceCommunication;
PETSC_EXTERN PetscLogEvent VEC_ReduceBegin;
PETSC_EXTERN PetscLogEvent VEC_ReduceEnd;
PETSC_EXTERN PetscLogEvent VEC_Swap;
PETSC_EXTERN PetscLogEvent VEC_AssemblyBegin;
PETSC_EXTERN PetscLogEvent VEC_DotNorm2;
PETSC_EXTERN PetscLogEvent VEC_AXPBYPCZ;
PETSC_EXTERN PetscLogEvent VEC_Ops;
PETSC_EXTERN PetscLogEvent VEC_ViennaCLCopyToGPU;
PETSC_EXTERN PetscLogEvent VEC_ViennaCLCopyFromGPU;
PETSC_EXTERN PetscLogEvent VEC_CUDACopyToGPU;
PETSC_EXTERN PetscLogEvent VEC_CUDACopyFromGPU;
PETSC_EXTERN PetscLogEvent VEC_HIPCopyToGPU;
PETSC_EXTERN PetscLogEvent VEC_HIPCopyFromGPU;

PETSC_SINGLE_LIBRARY_INTERN PetscErrorCode VecView_Seq(Vec, PetscViewer);
#if defined(PETSC_HAVE_VIENNACL)
PETSC_EXTERN PetscErrorCode VecViennaCLAllocateCheckHost(Vec v);
PETSC_EXTERN PetscErrorCode VecViennaCLCopyFromGPU(Vec v);
#endif

/*
     Common header shared by array based vectors,
   currently Vec_Seq and Vec_MPI
*/
#define VECHEADER \
  PetscScalar *array; \
  PetscScalar *array_allocated; /* if the array was allocated by PETSc this is its pointer */ \
  PetscScalar *unplacedarray;   /* if one called VecPlaceArray(), this is where it stashed the original */

/* Get Root type of vector. e.g. VECSEQ -> VECSTANDARD, VECMPICUDA -> VECCUDA */
PETSC_EXTERN PetscErrorCode VecGetRootType_Private(Vec, VecType *);

/* Default obtain and release vectors; can be used by any implementation */
PETSC_INTERN PetscErrorCode VecDuplicateVecs_Default(Vec, PetscInt, Vec *[]);
PETSC_INTERN PetscErrorCode VecDestroyVecs_Default(PetscInt, Vec[]);
PETSC_INTERN PetscErrorCode VecView_Binary(Vec, PetscViewer);

PETSC_SINGLE_LIBRARY_INTERN PetscErrorCode VecLoad_Default(Vec, PetscViewer);

PETSC_INTERN PetscInt NormIds[4]; /* map from NormType to IDs used to cache/retrieve values of norms, 1_AND_2 is excluded */

PETSC_INTERN PetscErrorCode VecStashCreate_Private(MPI_Comm, PetscInt, VecStash *);
PETSC_INTERN PetscErrorCode VecStashDestroy_Private(VecStash *);
PETSC_INTERN PetscErrorCode VecStashScatterEnd_Private(VecStash *);
PETSC_INTERN PetscErrorCode VecStashSetInitialSize_Private(VecStash *, PetscInt);
PETSC_INTERN PetscErrorCode VecStashGetInfo_Private(VecStash *, PetscInt *, PetscInt *);
PETSC_INTERN PetscErrorCode VecStashScatterBegin_Private(VecStash *, const PetscInt *);
PETSC_INTERN PetscErrorCode VecStashScatterGetMesg_Private(VecStash *, PetscMPIInt *, PetscInt **, PetscScalar **, PetscInt *);
PETSC_INTERN PetscErrorCode VecStashSortCompress_Private(VecStash *);
PETSC_INTERN PetscErrorCode VecStashGetOwnerList_Private(VecStash *, PetscLayout, PetscMPIInt *, PetscMPIInt **);

PETSC_INTERN PetscErrorCode VecStashExpand_Private(VecStash *, PetscInt);

/*
  VecStashValue_Private - inserts a single value into the stash.

  Input Parameters:
  stash  - the stash
  idx    - the global of the inserted value
  values - the value inserted
*/
static inline PetscErrorCode VecStashValue_Private(VecStash *stash, PetscInt row, PetscScalar value)
{
  /* Check and see if we have sufficient memory */
  PetscFunctionBegin;
  if (((stash)->n + 1) > (stash)->nmax) PetscCall(VecStashExpand_Private(stash, 1));
  (stash)->idx[(stash)->n]   = row;
  (stash)->array[(stash)->n] = value;
  (stash)->n++;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*
  VecStashValuesBlocked_Private - inserts 1 block of values into the stash.

  Input Parameters:
  stash  - the stash
  idx    - the global block index
  values - the values inserted
*/
static inline PetscErrorCode VecStashValuesBlocked_Private(VecStash *stash, PetscInt row, PetscScalar *values)
{
  PetscInt     stash_bs = (stash)->bs;
  PetscScalar *array;

  PetscFunctionBegin;
  if (((stash)->n + 1) > (stash)->nmax) PetscCall(VecStashExpand_Private(stash, 1));
  array                    = (stash)->array + stash_bs * (stash)->n;
  (stash)->idx[(stash)->n] = row;
  PetscCall(PetscArraycpy(array, values, stash_bs));
  (stash)->n++;
  PetscFunctionReturn(PETSC_SUCCESS);
}

PETSC_INTERN PetscErrorCode VecStrideGather_Default(Vec, PetscInt, Vec, InsertMode);
PETSC_INTERN PetscErrorCode VecStrideScatter_Default(Vec, PetscInt, Vec, InsertMode);
PETSC_INTERN PetscErrorCode VecStrideSubSetGather_Default(Vec, PetscInt, const PetscInt[], const PetscInt[], Vec, InsertMode);
PETSC_INTERN PetscErrorCode VecStrideSubSetScatter_Default(Vec, PetscInt, const PetscInt[], const PetscInt[], Vec, InsertMode);

PETSC_INTERN PetscErrorCode VecReciprocal_Default(Vec);
#if defined(PETSC_HAVE_MATLAB)
PETSC_EXTERN PetscErrorCode VecMatlabEnginePut_Default(PetscObject, void *);
PETSC_EXTERN PetscErrorCode VecMatlabEngineGet_Default(PetscObject, void *);
#endif

PETSC_SINGLE_LIBRARY_INTERN PetscErrorCode PetscSectionGetField_Internal(PetscSection, PetscSection, Vec, PetscInt, PetscInt, PetscInt, IS *, Vec *);
PETSC_SINGLE_LIBRARY_INTERN PetscErrorCode PetscSectionRestoreField_Internal(PetscSection, PetscSection, Vec, PetscInt, PetscInt, PetscInt, IS *, Vec *);

#define VecCheckSameLocalSize(x, ar1, y, ar2) \
  do { \
    PetscCheck((x)->map->n == (y)->map->n, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Incompatible vector local lengths parameter # %d local size %" PetscInt_FMT " != parameter # %d local size %" PetscInt_FMT, ar1, (x)->map->n, ar2, (y)->map->n); \
  } while (0)

#define VecCheckSameSize(x, ar1, y, ar2) \
  do { \
    PetscCheck((x)->map->N == (y)->map->N, PetscObjectComm((PetscObject)x), PETSC_ERR_ARG_INCOMP, "Incompatible vector global lengths parameter # %d global size %" PetscInt_FMT " != parameter # %d global size %" PetscInt_FMT, ar1, (x)->map->N, ar2, \
               (y)->map->N); \
    VecCheckSameLocalSize(x, ar1, y, ar2); \
  } while (0)

#define VecCheckLocalSize(x, ar1, n) \
  do { \
    PetscCheck((x)->map->n == (n), PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Incorrect vector local size: parameter # %d local size %" PetscInt_FMT " != %" PetscInt_FMT, ar1, (x)->map->n, n); \
  } while (0)

#define VecCheckSize(x, ar1, n, N) \
  do { \
    PetscCheck((x)->map->N == (N), PetscObjectComm((PetscObject)x), PETSC_ERR_ARG_INCOMP, "Incorrect vector global size: parameter # %d global size %" PetscInt_FMT " != %" PetscInt_FMT, ar1, (x)->map->N, N); \
    VecCheckLocalSize(x, ar1, n); \
  } while (0)

typedef struct _VecTaggerOps *VecTaggerOps;
struct _VecTaggerOps {
  PetscErrorCode (*create)(VecTagger);
  PetscErrorCode (*destroy)(VecTagger);
  PetscErrorCode (*setfromoptions)(VecTagger, PetscOptionItems);
  PetscErrorCode (*setup)(VecTagger);
  PetscErrorCode (*view)(VecTagger, PetscViewer);
  PetscErrorCode (*computeboxes)(VecTagger, Vec, PetscInt *, VecTaggerBox **, PetscBool *);
  PetscErrorCode (*computeis)(VecTagger, Vec, IS *, PetscBool *);
};
struct _p_VecTagger {
  PETSCHEADER(struct _VecTaggerOps);
  void     *data;
  PetscInt  blocksize;
  PetscBool invert;
  PetscBool setupcalled;
};

PETSC_INTERN PetscBool      VecTaggerRegisterAllCalled;
PETSC_INTERN PetscErrorCode VecTaggerComputeIS_FromBoxes(VecTagger, Vec, IS *, PetscBool *);
PETSC_INTERN PetscMPIInt    Petsc_Reduction_keyval;

PETSC_INTERN PetscInt       VecGetSubVectorSavedStateId;
PETSC_INTERN PetscErrorCode VecGetSubVectorContiguityAndBS_Private(Vec, IS, PetscBool *, PetscInt *, PetscInt *);
PETSC_INTERN PetscErrorCode VecGetSubVectorThroughVecScatter_Private(Vec, IS, PetscInt, Vec *);

#if PetscDefined(HAVE_CUDA)
PETSC_INTERN PetscErrorCode VecCreate_CUDA(Vec);
PETSC_INTERN PetscErrorCode VecCreate_SeqCUDA(Vec);
PETSC_INTERN PetscErrorCode VecCreate_MPICUDA(Vec);
PETSC_INTERN PetscErrorCode VecCUDAGetArrays_Private(Vec, const PetscScalar **, const PetscScalar **, PetscOffloadMask *);
PETSC_INTERN PetscErrorCode VecConvert_Seq_SeqCUDA_inplace(Vec);
PETSC_INTERN PetscErrorCode VecConvert_MPI_MPICUDA_inplace(Vec);
#endif

#if PetscDefined(HAVE_HIP)
PETSC_INTERN PetscErrorCode VecCreate_HIP(Vec);
PETSC_INTERN PetscErrorCode VecCreate_SeqHIP(Vec);
PETSC_INTERN PetscErrorCode VecCreate_MPIHIP(Vec);
PETSC_INTERN PetscErrorCode VecHIPGetArrays_Private(Vec, const PetscScalar **, const PetscScalar **, PetscOffloadMask *);
PETSC_INTERN PetscErrorCode VecConvert_Seq_SeqHIP_inplace(Vec);
PETSC_INTERN PetscErrorCode VecConvert_MPI_MPIHIP_inplace(Vec);
#endif

#if defined(PETSC_HAVE_KOKKOS)
PETSC_INTERN PetscErrorCode VecCreateMPIKokkosWithArrays_Private(MPI_Comm, PetscInt, PetscInt, PetscInt, const PetscScalar *, const PetscScalar *, Vec *);
PETSC_INTERN PetscErrorCode VecConvert_Seq_SeqKokkos_inplace(Vec);
PETSC_INTERN PetscErrorCode VecConvert_MPI_MPIKokkos_inplace(Vec);
#endif

PETSC_SINGLE_LIBRARY_INTERN PetscErrorCode VecCreateWithLayout_Private(PetscLayout, Vec *);
PETSC_SINGLE_LIBRARY_INTERN PetscErrorCode VecCreateSeqWithLayoutAndArray_Private(PetscLayout, const PetscScalar *, Vec *);
PETSC_SINGLE_LIBRARY_INTERN PetscErrorCode VecCreateMPIWithLayoutAndArray_Private(PetscLayout, const PetscScalar *, Vec *);

/* std::upper_bound(): Given a sorted array, return index of the first element in range [first,last) whose value
   is greater than value, or last if there is no such element.
*/
static inline PetscErrorCode PetscSortedIntUpperBound(const PetscInt *array, PetscCount first, PetscCount last, PetscInt value, PetscCount *upper)
{
  PetscCount count = last - first;

  PetscFunctionBegin;
  while (count > 0) {
    const PetscCount step = count / 2;
    PetscCount       it   = first + step;

    if (value < array[it]) {
      count = step;
    } else {
      first = it + 1;
      count -= step + 1;
    }
  }
  *upper = first;
  PetscFunctionReturn(PETSC_SUCCESS);
}

#define VEC_ASYNC_FN_NAME(Base) "Vec" Base "Async_Private_C"
#define VecAsyncFnName(Base)    VEC_##Base##_ASYNC_FN_NAME

#define VEC_Abs_ASYNC_FN_NAME             VEC_ASYNC_FN_NAME("Abs")
#define VEC_AXPBY_ASYNC_FN_NAME           VEC_ASYNC_FN_NAME("AXPBY")
#define VEC_AXPBYPCZ_ASYNC_FN_NAME        VEC_ASYNC_FN_NAME("AXPBYPCZ")
#define VEC_AXPY_ASYNC_FN_NAME            VEC_ASYNC_FN_NAME("AXPY")
#define VEC_AYPX_ASYNC_FN_NAME            VEC_ASYNC_FN_NAME("AYPX")
#define VEC_Conjugate_ASYNC_FN_NAME       VEC_ASYNC_FN_NAME("Conjugate")
#define VEC_Copy_ASYNC_FN_NAME            VEC_ASYNC_FN_NAME("Copy")
#define VEC_Exp_ASYNC_FN_NAME             VEC_ASYNC_FN_NAME("Exp")
#define VEC_Log_ASYNC_FN_NAME             VEC_ASYNC_FN_NAME("Log")
#define VEC_MAXPY_ASYNC_FN_NAME           VEC_ASYNC_FN_NAME("MAXPY")
#define VEC_PointwiseDivide_ASYNC_FN_NAME VEC_ASYNC_FN_NAME("PointwiseDivide")
#define VEC_PointwiseMax_ASYNC_FN_NAME    VEC_ASYNC_FN_NAME("PointwiseMax")
#define VEC_PointwiseMaxAbs_ASYNC_FN_NAME VEC_ASYNC_FN_NAME("PointwiseMaxAbs")
#define VEC_PointwiseMin_ASYNC_FN_NAME    VEC_ASYNC_FN_NAME("PointwiseMin")
#define VEC_PointwiseMult_ASYNC_FN_NAME   VEC_ASYNC_FN_NAME("PointwiseMult")
#define VEC_Reciprocal_ASYNC_FN_NAME      VEC_ASYNC_FN_NAME("Reciprocal")
#define VEC_Scale_ASYNC_FN_NAME           VEC_ASYNC_FN_NAME("Scale")
#define VEC_Set_ASYNC_FN_NAME             VEC_ASYNC_FN_NAME("Set")
#define VEC_Shift_ASYNC_FN_NAME           VEC_ASYNC_FN_NAME("Shift")
#define VEC_SqrtAbs_ASYNC_FN_NAME         VEC_ASYNC_FN_NAME("SqrtAbs")
#define VEC_Swap_ASYNC_FN_NAME            VEC_ASYNC_FN_NAME("Swap")
#define VEC_WAXPY_ASYNC_FN_NAME           VEC_ASYNC_FN_NAME("WAXPY")

PETSC_INTERN PetscErrorCode VecAbsAsync_Private(Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecAXPBYAsync_Private(Vec, PetscScalar, PetscScalar, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecAXPBYPCZAsync_Private(Vec, PetscScalar, PetscScalar, PetscScalar, Vec, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecAXPYAsync_Private(Vec, PetscScalar, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecAYPXAsync_Private(Vec, PetscScalar, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecConjugateAsync_Private(Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecCopyAsync_Private(Vec, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecExpAsync_Private(Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecLogAsync_Private(Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecMAXPYAsync_Private(Vec, PetscInt, const PetscScalar *, Vec[], PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecPointwiseDivideAsync_Private(Vec, Vec, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecPointwiseMaxAsync_Private(Vec, Vec, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecPointwiseMaxAbsAsync_Private(Vec, Vec, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecPointwiseMinAsync_Private(Vec, Vec, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecPointwiseMultAsync_Private(Vec, Vec, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecReciprocalAsync_Private(Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecScaleAsync_Private(Vec, PetscScalar, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecSetAsync_Private(Vec, PetscScalar, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecShiftAsync_Private(Vec, PetscScalar, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecSqrtAbsAsync_Private(Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecSwapAsync_Private(Vec, Vec, PetscDeviceContext);
PETSC_INTERN PetscErrorCode VecWAXPYAsync_Private(Vec, PetscScalar, Vec, Vec, PetscDeviceContext);

#define VecMethodDispatch(v, dctx, async_name, name, async_arg_types, ...) \
  do { \
    PetscErrorCode(*_8_f) async_arg_types = NULL; \
    if (dctx) PetscCall(PetscObjectQueryFunction((PetscObject)(v), async_name, &_8_f)); \
    if (_8_f) { \
      PetscCall((*_8_f)(v, __VA_ARGS__, dctx)); \
    } else { \
      PetscUseTypeMethod(v, name, __VA_ARGS__); \
    } \
  } while (0)

// return in lda the vector's local size aligned to <alignment> bytes, where lda is an integer pointer
#define VecGetLocalSizeAligned(v, alignment, lda) \
  do { \
    PetscInt     n = (v)->map->n; \
    const size_t s = (alignment) / sizeof(PetscScalar); \
    *(lda)         = ((n + s - 1) / s) * s; \
  } while (0)
