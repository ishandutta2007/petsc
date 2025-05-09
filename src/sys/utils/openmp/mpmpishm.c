#include <petscsys.h> /*I  "petscsys.h"  I*/
#include <petsc/private/petscimpl.h>
#include <pthread.h>
#include <hwloc.h>
#include <omp.h>

/* Use mmap() to allocate shared mmeory (for the pthread_barrier_t object) if it is available,
   otherwise use MPI_Win_allocate_shared. They should have the same effect except MPI-3 is much
   simpler to use. However, on a Cori Haswell node with Cray MPI, MPI-3 worsened a test's performance
   by 50%. Until the reason is found out, we use mmap() instead.
*/
#define USE_MMAP_ALLOCATE_SHARED_MEMORY

#if defined(USE_MMAP_ALLOCATE_SHARED_MEMORY) && defined(PETSC_HAVE_MMAP)
  #include <sys/mman.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
#endif

struct _n_PetscOmpCtrl {
  MPI_Comm           omp_comm;        /* a shared memory communicator to spawn omp threads */
  MPI_Comm           omp_master_comm; /* a communicator to give to third party libraries */
  PetscMPIInt        omp_comm_size;   /* size of omp_comm, a kind of OMP_NUM_THREADS */
  PetscBool          is_omp_master;   /* rank 0's in omp_comm */
  MPI_Win            omp_win;         /* a shared memory window containing a barrier */
  pthread_barrier_t *barrier;         /* pointer to the barrier */
  hwloc_topology_t   topology;
  hwloc_cpuset_t     cpuset;     /* cpu bindings of omp master */
  hwloc_cpuset_t     omp_cpuset; /* union of cpu bindings of ranks in omp_comm */
};

/* Allocate and initialize a pthread_barrier_t object in memory shared by processes in omp_comm
   contained by the controller.

   PETSc OpenMP controller users do not call this function directly. This function exists
   only because we want to separate shared memory allocation methods from other code.
 */
static inline PetscErrorCode PetscOmpCtrlCreateBarrier(PetscOmpCtrl ctrl)
{
  MPI_Aint              size;
  void                 *baseptr;
  pthread_barrierattr_t attr;

#if defined(USE_MMAP_ALLOCATE_SHARED_MEMORY) && defined(PETSC_HAVE_MMAP)
  int  fd;
  char pathname[PETSC_MAX_PATH_LEN];
#else
  PetscMPIInt disp_unit;
#endif

  PetscFunctionBegin;
#if defined(USE_MMAP_ALLOCATE_SHARED_MEMORY) && defined(PETSC_HAVE_MMAP)
  size = sizeof(pthread_barrier_t);
  if (ctrl->is_omp_master) {
    /* use PETSC_COMM_SELF in PetscGetTmp, since it is a collective call. Using omp_comm would otherwise bcast the partially populated pathname to slaves */
    PetscCall(PetscGetTmp(PETSC_COMM_SELF, pathname, PETSC_MAX_PATH_LEN));
    PetscCall(PetscStrlcat(pathname, "/petsc-shm-XXXXXX", PETSC_MAX_PATH_LEN));
    /* mkstemp replaces XXXXXX with a unique file name and opens the file for us */
    fd = mkstemp(pathname);
    PetscCheck(fd != -1, PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not create tmp file %s with mkstemp", pathname);
    PetscCallExternal(ftruncate, fd, size);
    baseptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    PetscCheck(baseptr != MAP_FAILED, PETSC_COMM_SELF, PETSC_ERR_LIB, "mmap() failed");
    PetscCallExternal(close, fd);
    PetscCallMPI(MPI_Bcast(pathname, PETSC_MAX_PATH_LEN, MPI_CHAR, 0, ctrl->omp_comm));
    /* this MPI_Barrier is to wait slaves to open the file before master unlinks it */
    PetscCallMPI(MPI_Barrier(ctrl->omp_comm));
    PetscCallExternal(unlink, pathname);
  } else {
    PetscCallMPI(MPI_Bcast(pathname, PETSC_MAX_PATH_LEN, MPI_CHAR, 0, ctrl->omp_comm));
    fd = open(pathname, O_RDWR);
    PetscCheck(fd != -1, PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not open tmp file %s", pathname);
    baseptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    PetscCheck(baseptr != MAP_FAILED, PETSC_COMM_SELF, PETSC_ERR_LIB, "mmap() failed");
    PetscCallExternal(close, fd);
    PetscCallMPI(MPI_Barrier(ctrl->omp_comm));
  }
#else
  size = ctrl->is_omp_master ? sizeof(pthread_barrier_t) : 0;
  PetscCallMPI(MPI_Win_allocate_shared(size, 1, MPI_INFO_NULL, ctrl->omp_comm, &baseptr, &ctrl->omp_win));
  PetscCallMPI(MPI_Win_shared_query(ctrl->omp_win, 0, &size, &disp_unit, &baseptr));
#endif
  ctrl->barrier = (pthread_barrier_t *)baseptr;

  /* omp master initializes the barrier */
  if (ctrl->is_omp_master) {
    PetscCallMPI(MPI_Comm_size(ctrl->omp_comm, &ctrl->omp_comm_size));
    PetscCallExternal(pthread_barrierattr_init, &attr);
    PetscCallExternal(pthread_barrierattr_setpshared, &attr, PTHREAD_PROCESS_SHARED); /* make the barrier also work for processes */
    PetscCallExternal(pthread_barrier_init, ctrl->barrier, &attr, (unsigned int)ctrl->omp_comm_size);
    PetscCallExternal(pthread_barrierattr_destroy, &attr);
  }

  /* this MPI_Barrier is to make sure the omp barrier is initialized before slaves use it */
  PetscCallMPI(MPI_Barrier(ctrl->omp_comm));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/* Destroy the pthread barrier in the PETSc OpenMP controller */
static inline PetscErrorCode PetscOmpCtrlDestroyBarrier(PetscOmpCtrl ctrl)
{
  PetscFunctionBegin;
  /* this MPI_Barrier is to make sure slaves have finished using the omp barrier before master destroys it */
  PetscCallMPI(MPI_Barrier(ctrl->omp_comm));
  if (ctrl->is_omp_master) PetscCallExternal(pthread_barrier_destroy, ctrl->barrier);

#if defined(USE_MMAP_ALLOCATE_SHARED_MEMORY) && defined(PETSC_HAVE_MMAP)
  PetscCallExternal(munmap, ctrl->barrier, sizeof(pthread_barrier_t));
#else
  PetscCallMPI(MPI_Win_free(&ctrl->omp_win));
#endif
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscOmpCtrlCreate - create a PETSc OpenMP controller, which manages PETSc's interaction with third party libraries that use OpenMP

  Input Parameters:
+ petsc_comm - a communicator some PETSc object (for example, a matrix) lives in
- nthreads   - number of threads per MPI rank to spawn in a library using OpenMP. If nthreads = -1, let PETSc decide a suitable value

  Output Parameter:
. pctrl - a PETSc OpenMP controller

  Level: developer

  Developer Note:
  Possibly use the variable `PetscNumOMPThreads` to determine the number for threads to use

.seealso: `PetscOmpCtrlDestroy()`, `PetscOmpCtrlGetOmpComms()`, `PetscOmpCtrlBarrier()`, `PetscOmpCtrlOmpRegionOnMasterBegin()`, `PetscOmpCtrlOmpRegionOnMasterEnd()`,
@*/
PetscErrorCode PetscOmpCtrlCreate(MPI_Comm petsc_comm, PetscInt nthreads, PetscOmpCtrl *pctrl)
{
  PetscOmpCtrl   ctrl;
  unsigned long *cpu_ulongs = NULL;
  PetscShmComm   pshmcomm;
  MPI_Comm       shm_comm;
  PetscMPIInt    shm_rank, shm_comm_size, omp_rank, color, nr_cpu_ulongs;
  PetscInt       num_packages, num_cores;

  PetscFunctionBegin;
  PetscCall(PetscNew(&ctrl));

  /*=================================================================================
    Init hwloc
   ==================================================================================*/
  PetscCallExternal(hwloc_topology_init, &ctrl->topology);
#if HWLOC_API_VERSION >= 0x00020000
  /* to filter out unneeded info and have faster hwloc_topology_load */
  PetscCallExternal(hwloc_topology_set_all_types_filter, ctrl->topology, HWLOC_TYPE_FILTER_KEEP_NONE);
  PetscCallExternal(hwloc_topology_set_type_filter, ctrl->topology, HWLOC_OBJ_CORE, HWLOC_TYPE_FILTER_KEEP_ALL);
#endif
  PetscCallExternal(hwloc_topology_load, ctrl->topology);

  /*=================================================================================
    Split petsc_comm into multiple omp_comms. Ranks in an omp_comm have access to
    physically shared memory. Rank 0 of each omp_comm is called an OMP master, and
    others are called slaves. OMP Masters make up a new comm called omp_master_comm,
    which is usually passed to third party libraries.
   ==================================================================================*/

  /* fetch the stored shared memory communicator */
  PetscCall(PetscShmCommGet(petsc_comm, &pshmcomm));
  PetscCall(PetscShmCommGetMpiShmComm(pshmcomm, &shm_comm));

  PetscCallMPI(MPI_Comm_rank(shm_comm, &shm_rank));
  PetscCallMPI(MPI_Comm_size(shm_comm, &shm_comm_size));

  /* PETSc decides nthreads, which is the smaller of shm_comm_size or cores per package(socket) */
  if (nthreads == -1) {
    num_packages = hwloc_get_nbobjs_by_type(ctrl->topology, HWLOC_OBJ_PACKAGE) <= 0 ? 1 : hwloc_get_nbobjs_by_type(ctrl->topology, HWLOC_OBJ_PACKAGE);
    num_cores    = hwloc_get_nbobjs_by_type(ctrl->topology, HWLOC_OBJ_CORE) <= 0 ? 1 : hwloc_get_nbobjs_by_type(ctrl->topology, HWLOC_OBJ_CORE);
    nthreads     = num_cores / num_packages;
    if (nthreads > shm_comm_size) nthreads = shm_comm_size;
  }

  PetscCheck(nthreads >= 1 && nthreads <= shm_comm_size, petsc_comm, PETSC_ERR_ARG_OUTOFRANGE, "number of OpenMP threads %" PetscInt_FMT " can not be < 1 or > the MPI shared memory communicator size %d", nthreads, shm_comm_size);
  if (shm_comm_size % nthreads) PetscCall(PetscPrintf(petsc_comm, "Warning: number of OpenMP threads %" PetscInt_FMT " is not a factor of the MPI shared memory communicator size %d, which may cause load-imbalance!\n", nthreads, shm_comm_size));

  /* split shm_comm into a set of omp_comms with each of size nthreads. Ex., if
     shm_comm_size=16, nthreads=8, then ranks 0~7 get color 0 and ranks 8~15 get
     color 1. They are put in two omp_comms. Note that petsc_ranks may or may not
     be consecutive in a shm_comm, but shm_ranks always run from 0 to shm_comm_size-1.
     Use 0 as key so that rank ordering wont change in new comm.
   */
  color = shm_rank / nthreads;
  PetscCallMPI(MPI_Comm_split(shm_comm, color, 0 /*key*/, &ctrl->omp_comm));

  /* put rank 0's in omp_comms (i.e., master ranks) into a new comm - omp_master_comm */
  PetscCallMPI(MPI_Comm_rank(ctrl->omp_comm, &omp_rank));
  if (!omp_rank) {
    ctrl->is_omp_master = PETSC_TRUE; /* master */
    color               = 0;
  } else {
    ctrl->is_omp_master = PETSC_FALSE;   /* slave */
    color               = MPI_UNDEFINED; /* to make slaves get omp_master_comm = MPI_COMM_NULL in MPI_Comm_split */
  }
  PetscCallMPI(MPI_Comm_split(petsc_comm, color, 0 /*key*/, &ctrl->omp_master_comm));

  /*=================================================================================
    Each omp_comm has a pthread_barrier_t in its shared memory, which is used to put
    slave ranks in sleep and idle their CPU, so that the master can fork OMP threads
    and run them on the idle CPUs.
   ==================================================================================*/
  PetscCall(PetscOmpCtrlCreateBarrier(ctrl));

  /*=================================================================================
    omp master logs its cpu binding (i.e., cpu set) and computes a new binding that
    is the union of the bindings of all ranks in the omp_comm
    =================================================================================*/

  ctrl->cpuset = hwloc_bitmap_alloc();
  PetscCheck(ctrl->cpuset, PETSC_COMM_SELF, PETSC_ERR_LIB, "hwloc_bitmap_alloc() failed");
  PetscCallExternal(hwloc_get_cpubind, ctrl->topology, ctrl->cpuset, HWLOC_CPUBIND_PROCESS);

  /* hwloc main developer said they will add new APIs hwloc_bitmap_{nr,to,from}_ulongs in 2.1 to help us simplify the following bitmap pack/unpack code */
  nr_cpu_ulongs = (hwloc_bitmap_last(hwloc_topology_get_topology_cpuset(ctrl->topology)) + sizeof(unsigned long) * 8) / sizeof(unsigned long) / 8;
  PetscCall(PetscMalloc1(nr_cpu_ulongs, &cpu_ulongs));
  if (nr_cpu_ulongs == 1) {
    cpu_ulongs[0] = hwloc_bitmap_to_ulong(ctrl->cpuset);
  } else {
    for (PetscMPIInt i = 0; i < nr_cpu_ulongs; i++) cpu_ulongs[i] = hwloc_bitmap_to_ith_ulong(ctrl->cpuset, (unsigned)i);
  }

  PetscCallMPI(MPI_Reduce(ctrl->is_omp_master ? MPI_IN_PLACE : cpu_ulongs, cpu_ulongs, nr_cpu_ulongs, MPI_UNSIGNED_LONG, MPI_BOR, 0, ctrl->omp_comm));

  if (ctrl->is_omp_master) {
    ctrl->omp_cpuset = hwloc_bitmap_alloc();
    PetscCheck(ctrl->omp_cpuset, PETSC_COMM_SELF, PETSC_ERR_LIB, "hwloc_bitmap_alloc() failed");
    if (nr_cpu_ulongs == 1) {
#if HWLOC_API_VERSION >= 0x00020000
      PetscCallExternal(hwloc_bitmap_from_ulong, ctrl->omp_cpuset, cpu_ulongs[0]);
#else
      hwloc_bitmap_from_ulong(ctrl->omp_cpuset, cpu_ulongs[0]);
#endif
    } else {
      for (PetscMPIInt i = 0; i < nr_cpu_ulongs; i++) {
#if HWLOC_API_VERSION >= 0x00020000
        PetscCallExternal(hwloc_bitmap_set_ith_ulong, ctrl->omp_cpuset, (unsigned)i, cpu_ulongs[i]);
#else
        hwloc_bitmap_set_ith_ulong(ctrl->omp_cpuset, (unsigned)i, cpu_ulongs[i]);
#endif
      }
    }
  }
  PetscCall(PetscFree(cpu_ulongs));
  *pctrl = ctrl;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscOmpCtrlDestroy - destroy the PETSc OpenMP controller

  Input Parameter:
. pctrl - a PETSc OpenMP controller

  Level: developer

.seealso: `PetscOmpCtrlCreate()`, `PetscOmpCtrlGetOmpComms()`, `PetscOmpCtrlBarrier()`, `PetscOmpCtrlOmpRegionOnMasterBegin()`, `PetscOmpCtrlOmpRegionOnMasterEnd()`,
@*/
PetscErrorCode PetscOmpCtrlDestroy(PetscOmpCtrl *pctrl)
{
  PetscOmpCtrl ctrl = *pctrl;

  PetscFunctionBegin;
  hwloc_bitmap_free(ctrl->cpuset);
  hwloc_topology_destroy(ctrl->topology);
  PetscCall(PetscOmpCtrlDestroyBarrier(ctrl));
  PetscCallMPI(MPI_Comm_free(&ctrl->omp_comm));
  if (ctrl->is_omp_master) {
    hwloc_bitmap_free(ctrl->omp_cpuset);
    PetscCallMPI(MPI_Comm_free(&ctrl->omp_master_comm));
  }
  PetscCall(PetscFree(ctrl));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscOmpCtrlGetOmpComms - Get MPI communicators from a PETSc OMP controller

  Input Parameter:
. ctrl - a PETSc OMP controller

  Output Parameters:
+ omp_comm        - a communicator that includes a master rank and slave ranks where master spawns threads
. omp_master_comm - on master ranks, return a communicator that include master ranks of each omp_comm;
                    on slave ranks, `MPI_COMM_NULL` will be return in reality.
- is_omp_master   - true if the calling process is an OMP master rank.

  Note:
  Any output parameter can be `NULL`. The parameter is just ignored.

  Level: developer

.seealso: `PetscOmpCtrlCreate()`, `PetscOmpCtrlDestroy()`, `PetscOmpCtrlBarrier()`, `PetscOmpCtrlOmpRegionOnMasterBegin()`, `PetscOmpCtrlOmpRegionOnMasterEnd()`,
@*/
PetscErrorCode PetscOmpCtrlGetOmpComms(PetscOmpCtrl ctrl, MPI_Comm *omp_comm, MPI_Comm *omp_master_comm, PetscBool *is_omp_master)
{
  PetscFunctionBegin;
  if (omp_comm) *omp_comm = ctrl->omp_comm;
  if (omp_master_comm) *omp_master_comm = ctrl->omp_master_comm;
  if (is_omp_master) *is_omp_master = ctrl->is_omp_master;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscOmpCtrlBarrier - Do barrier on MPI ranks in omp_comm contained by the PETSc OMP controller (to let slave ranks free their CPU)

  Input Parameter:
. ctrl - a PETSc OMP controller

  Notes:
  This is a pthread barrier on MPI ranks. Using `MPI_Barrier()` instead is conceptually correct. But MPI standard does not
  require processes blocked by `MPI_Barrier()` free their CPUs to let other processes progress. In practice, to minilize latency,
  MPI ranks stuck in `MPI_Barrier()` keep polling and do not free CPUs. In contrast, pthread_barrier has this requirement.

  A code using `PetscOmpCtrlBarrier()` would be like this,
.vb
  if (is_omp_master) {
    PetscOmpCtrlOmpRegionOnMasterBegin(ctrl);
    Call the library using OpenMP
    PetscOmpCtrlOmpRegionOnMasterEnd(ctrl);
  }
  PetscOmpCtrlBarrier(ctrl);
.ve

  Level: developer

.seealso: `PetscOmpCtrlOmpRegionOnMasterBegin()`, `PetscOmpCtrlOmpRegionOnMasterEnd()`, `PetscOmpCtrlCreate()`, `PetscOmpCtrlDestroy()`,
@*/
PetscErrorCode PetscOmpCtrlBarrier(PetscOmpCtrl ctrl)
{
  int err;

  PetscFunctionBegin;
  err = pthread_barrier_wait(ctrl->barrier);
  PetscCheck(!err || err == PTHREAD_BARRIER_SERIAL_THREAD, PETSC_COMM_SELF, PETSC_ERR_LIB, "pthread_barrier_wait failed within PetscOmpCtrlBarrier with return code %d", err);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscOmpCtrlOmpRegionOnMasterBegin - Mark the beginning of an OpenMP library call on master ranks

  Input Parameter:
. ctrl - a PETSc OMP controller

  Note:
  Only master ranks can call this function. Call `PetscOmpCtrlGetOmpComms()` to know if this is a master rank.
  This function changes CPU binding of master ranks and nthreads-var of OpenMP runtime

  Level: developer

.seealso: `PetscOmpCtrlOmpRegionOnMasterEnd()`, `PetscOmpCtrlCreate()`, `PetscOmpCtrlDestroy()`, `PetscOmpCtrlBarrier()`
@*/
PetscErrorCode PetscOmpCtrlOmpRegionOnMasterBegin(PetscOmpCtrl ctrl)
{
  PetscFunctionBegin;
  PetscCallExternal(hwloc_set_cpubind, ctrl->topology, ctrl->omp_cpuset, HWLOC_CPUBIND_PROCESS);
  omp_set_num_threads(ctrl->omp_comm_size); /* may override the OMP_NUM_THREAD env var */
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscOmpCtrlOmpRegionOnMasterEnd - Mark the end of an OpenMP library call on master ranks

  Input Parameter:
. ctrl - a PETSc OMP controller

  Note:
  Only master ranks can call this function. Call `PetscOmpCtrlGetOmpComms()` to know if this is a master rank.
  This function restores the CPU binding of master ranks and set and nthreads-var of OpenMP runtime to 1.

  Level: developer

.seealso: `PetscOmpCtrlOmpRegionOnMasterBegin()`, `PetscOmpCtrlCreate()`, `PetscOmpCtrlDestroy()`, `PetscOmpCtrlBarrier()`
@*/
PetscErrorCode PetscOmpCtrlOmpRegionOnMasterEnd(PetscOmpCtrl ctrl)
{
  PetscFunctionBegin;
  PetscCallExternal(hwloc_set_cpubind, ctrl->topology, ctrl->cpuset, HWLOC_CPUBIND_PROCESS);
  omp_set_num_threads(1);
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef USE_MMAP_ALLOCATE_SHARED_MEMORY
