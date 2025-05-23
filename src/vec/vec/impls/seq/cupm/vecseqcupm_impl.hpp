#pragma once

#include "vecseqcupm.hpp"

#include <petsc/private/randomimpl.h> // for _p_PetscRandom

#include "../src/sys/objects/device/impls/cupm/cupmthrustutility.hpp"
#include "../src/sys/objects/device/impls/cupm/kernels.hpp"

#if PetscDefined(USE_COMPLEX)
  #include <thrust/transform_reduce.h>
#endif
#include <thrust/transform.h>
#include <thrust/reduce.h>
#include <thrust/functional.h>
#include <thrust/tuple.h>
#include <thrust/device_ptr.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/inner_product.h>

namespace Petsc
{

namespace vec
{

namespace cupm
{

namespace impl
{

// ==========================================================================================
// VecSeq_CUPM - Private API
// ==========================================================================================

template <device::cupm::DeviceType T>
inline Vec_Seq *VecSeq_CUPM<T>::VecIMPLCast_(Vec v) noexcept
{
  return static_cast<Vec_Seq *>(v->data);
}

template <device::cupm::DeviceType T>
inline constexpr VecType VecSeq_CUPM<T>::VECIMPLCUPM_() noexcept
{
  return VECSEQCUPM();
}

template <device::cupm::DeviceType T>
inline constexpr VecType VecSeq_CUPM<T>::VECIMPL_() noexcept
{
  return VECSEQ;
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::ClearAsyncFunctions(Vec v) noexcept
{
  PetscFunctionBegin;
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Abs), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(AXPBY), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(AXPBYPCZ), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(AXPY), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(AYPX), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Conjugate), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Copy), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Exp), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Log), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(MAXPY), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseDivide), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseMax), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseMaxAbs), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseMin), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseMult), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Reciprocal), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Scale), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Set), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Shift), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(SqrtAbs), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Swap), nullptr));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(WAXPY), nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::InitializeAsyncFunctions(Vec v) noexcept
{
  PetscFunctionBegin;
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Abs), VecSeq_CUPM<T>::AbsAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(AXPBY), VecSeq_CUPM<T>::AXPBYAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(AXPBYPCZ), VecSeq_CUPM<T>::AXPBYPCZAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(AXPY), VecSeq_CUPM<T>::AXPYAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(AYPX), VecSeq_CUPM<T>::AYPXAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Conjugate), VecSeq_CUPM<T>::ConjugateAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Copy), VecSeq_CUPM<T>::CopyAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Exp), VecSeq_CUPM<T>::ExpAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Log), VecSeq_CUPM<T>::LogAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(MAXPY), VecSeq_CUPM<T>::MAXPYAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseDivide), VecSeq_CUPM<T>::PointwiseDivideAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseMax), VecSeq_CUPM<T>::PointwiseMaxAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseMaxAbs), VecSeq_CUPM<T>::PointwiseMaxAbsAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseMin), VecSeq_CUPM<T>::PointwiseMinAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(PointwiseMult), VecSeq_CUPM<T>::PointwiseMultAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Reciprocal), VecSeq_CUPM<T>::ReciprocalAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Scale), VecSeq_CUPM<T>::ScaleAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Set), VecSeq_CUPM<T>::SetAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Shift), VecSeq_CUPM<T>::ShiftAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(SqrtAbs), VecSeq_CUPM<T>::SqrtAbsAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(Swap), VecSeq_CUPM<T>::SwapAsync));
  PetscCall(PetscObjectComposeFunction(PetscObjectCast(v), VecAsyncFnName(WAXPY), VecSeq_CUPM<T>::WAXPYAsync));
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::VecDestroy_IMPL_(Vec v) noexcept
{
  PetscFunctionBegin;
  PetscCall(ClearAsyncFunctions(v));
  PetscCall(VecDestroy_Seq(v));
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::VecResetArray_IMPL_(Vec v) noexcept
{
  return VecResetArray_Seq(v);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::VecPlaceArray_IMPL_(Vec v, const PetscScalar *a) noexcept
{
  return VecPlaceArray_Seq(v, a);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::VecCreate_IMPL_Private_(Vec v, PetscBool *alloc_missing, PetscInt, PetscScalar *host_array) noexcept
{
  PetscMPIInt size;

  PetscFunctionBegin;
  if (alloc_missing) *alloc_missing = PETSC_FALSE;
  PetscCallMPI(MPI_Comm_size(PetscObjectComm(PetscObjectCast(v)), &size));
  PetscCheck(size <= 1, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Must create VecSeq on communicator of size 1, have size %d", size);
  PetscCall(VecCreate_Seq_Private(v, host_array));
  PetscCall(InitializeAsyncFunctions(v));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// for functions with an early return based one vec size we still need to artificially bump the
// object state. This is to prevent the following:
//
// 0. Suppose you have a Vec {
//   rank 0: [0],
//   rank 1: [<empty>]
// }
// 1. both ranks have Vec with PetscObjectState = 0, stashed norm of 0
// 2. Vec enters e.g. VecSet(10)
// 3. rank 1 has local size 0 and bails immediately
// 4. rank 0 has local size 1 and enters function, eventually calls DeviceArrayWrite()
// 5. DeviceArrayWrite() calls PetscObjectStateIncrease(), now state = 1
// 6. Vec enters VecNorm(), and calls VecNormAvailable()
// 7. rank 1 has object state = 0, equal to stash and returns early with norm = 0
// 8. rank 0 has object state = 1, not equal to stash, continues to impl function
// 9. rank 0 deadlocks on MPI_Allreduce() because rank 1 bailed early
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::MaybeIncrementEmptyLocalVec(Vec v) noexcept
{
  PetscFunctionBegin;
  if (PetscUnlikely((v->map->n == 0) && (v->map->N != 0))) PetscCall(PetscObjectStateIncrease(PetscObjectCast(v)));
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::CreateSeqCUPM_(Vec v, PetscDeviceContext dctx, PetscScalar *host_array, PetscScalar *device_array) noexcept
{
  PetscFunctionBegin;
  PetscCall(base_type::VecCreate_IMPL_Private(v, nullptr, 0, host_array));
  PetscCall(Initialize_CUPMBase(v, PETSC_FALSE, host_array, device_array, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
template <typename BinaryFuncT>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseBinary_(BinaryFuncT &&binary, Vec xin, Vec yin, Vec zout, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  if (const auto n = zout->map->n) {
    cupmStream_t stream;

    PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
    PetscCall(GetHandlesFrom_(dctx, &stream));
    // clang-format off
    PetscCallThrust(
      const auto dxptr = thrust::device_pointer_cast(DeviceArrayRead(dctx, xin).data());

      THRUST_CALL(
        thrust::transform,
        stream,
        dxptr, dxptr + n,
        thrust::device_pointer_cast(DeviceArrayRead(dctx, yin).data()),
        thrust::device_pointer_cast(DeviceArrayWrite(dctx, zout).data()),
        std::forward<BinaryFuncT>(binary)
      )
    );
    // clang-format on
    PetscCall(PetscLogGpuFlops(n));
    PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  } else {
    PetscCall(MaybeIncrementEmptyLocalVec(zout));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
template <typename BinaryFuncT>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseBinaryDispatch_(PetscErrorCode (*VecSeqFunction)(Vec, Vec, Vec), BinaryFuncT &&binary, Vec wout, Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  if (xin->boundtocpu || yin->boundtocpu) {
    PetscCall((*VecSeqFunction)(wout, xin, yin));
  } else {
    // note order of arguments! xin and yin are read, wout is written!
    PetscCall(PointwiseBinary_(std::forward<BinaryFuncT>(binary), xin, yin, wout, dctx));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
template <typename UnaryFuncT>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseUnary_(UnaryFuncT &&unary, Vec xinout, Vec yin, PetscDeviceContext dctx) noexcept
{
  const auto inplace = !yin || (xinout == yin);

  PetscFunctionBegin;
  if (const auto n = xinout->map->n) {
    cupmStream_t stream;
    const auto   apply = [&](PetscScalar *xinout, PetscScalar *yin = nullptr) {
      PetscFunctionBegin;
      // clang-format off
      PetscCallThrust(
        const auto xptr = thrust::device_pointer_cast(xinout);

        THRUST_CALL(
          thrust::transform,
          stream,
          xptr, xptr + n,
          (yin && (yin != xinout)) ? thrust::device_pointer_cast(yin) : xptr,
          std::forward<UnaryFuncT>(unary)
        )
      );
      // clang-format on
      PetscFunctionReturn(PETSC_SUCCESS);
    };

    PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
    PetscCall(GetHandlesFrom_(dctx, &stream));
    if (inplace) {
      PetscCall(apply(DeviceArrayReadWrite(dctx, xinout).data()));
    } else {
      PetscCall(apply(DeviceArrayRead(dctx, xinout).data(), DeviceArrayWrite(dctx, yin).data()));
    }
    PetscCall(PetscLogGpuFlops(n));
    PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  } else {
    if (inplace) {
      PetscCall(MaybeIncrementEmptyLocalVec(xinout));
    } else {
      PetscCall(MaybeIncrementEmptyLocalVec(yin));
    }
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// ==========================================================================================
// VecSeq_CUPM - Public API - Constructors
// ==========================================================================================

// VecCreateSeqCUPM()
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::CreateSeqCUPM(MPI_Comm comm, PetscInt bs, PetscInt n, Vec *v, PetscBool call_set_type) noexcept
{
  PetscFunctionBegin;
  PetscCall(Create_CUPMBase(comm, bs, n, n, v, call_set_type));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecCreateSeqCUPMWithArrays()
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::CreateSeqCUPMWithBothArrays(MPI_Comm comm, PetscInt bs, PetscInt n, const PetscScalar host_array[], const PetscScalar device_array[], Vec *v) noexcept
{
  PetscDeviceContext dctx;

  PetscFunctionBegin;
  PetscCall(GetHandles_(&dctx));
  // do NOT call VecSetType(), otherwise ops->create() -> create() ->
  // CreateSeqCUPM_() is called!
  PetscCall(CreateSeqCUPM(comm, bs, n, v, PETSC_FALSE));
  PetscCall(CreateSeqCUPM_(*v, dctx, PetscRemoveConstCast(host_array), PetscRemoveConstCast(device_array)));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->duplicate
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Duplicate(Vec v, Vec *y) noexcept
{
  PetscDeviceContext dctx;

  PetscFunctionBegin;
  PetscCall(GetHandles_(&dctx));
  PetscCall(Duplicate_CUPMBase(v, y, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// ==========================================================================================
// VecSeq_CUPM - Public API - Utility
// ==========================================================================================

// v->ops->bindtocpu
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::BindToCPU(Vec v, PetscBool usehost) noexcept
{
  PetscDeviceContext dctx;

  PetscFunctionBegin;
  PetscCall(GetHandles_(&dctx));
  PetscCall(BindToCPU_CUPMBase(v, usehost, dctx));

  // REVIEW ME: this absolutely should be some sort of bulk mempcy rather than this mess
  VecSetOp_CUPM(dot, VecDot_Seq, Dot);
  VecSetOp_CUPM(norm, VecNorm_Seq, Norm);
  VecSetOp_CUPM(tdot, VecTDot_Seq, TDot);
  VecSetOp_CUPM(mdot, VecMDot_Seq, MDot);
  VecSetOp_CUPM(resetarray, VecResetArray_Seq, base_type::template ResetArray<PETSC_MEMTYPE_HOST>);
  VecSetOp_CUPM(placearray, VecPlaceArray_Seq, base_type::template PlaceArray<PETSC_MEMTYPE_HOST>);
  v->ops->mtdot = v->ops->mtdot_local = VecMTDot_Seq;
  VecSetOp_CUPM(max, VecMax_Seq, Max);
  VecSetOp_CUPM(min, VecMin_Seq, Min);
  VecSetOp_CUPM(setpreallocationcoo, VecSetPreallocationCOO_Seq, SetPreallocationCOO);
  VecSetOp_CUPM(setvaluescoo, VecSetValuesCOO_Seq, SetValuesCOO);
  PetscFunctionReturn(PETSC_SUCCESS);
}

// ==========================================================================================
// VecSeq_CUPM - Public API - Mutators
// ==========================================================================================

// v->ops->getlocalvector or v->ops->getlocalvectorread
template <device::cupm::DeviceType T>
template <PetscMemoryAccessMode access>
inline PetscErrorCode VecSeq_CUPM<T>::GetLocalVector(Vec v, Vec w) noexcept
{
  PetscBool wisseqcupm;

  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQCUPM(), VECMPICUPM());
  PetscCall(PetscObjectTypeCompare(PetscObjectCast(w), VECSEQCUPM(), &wisseqcupm));
  if (wisseqcupm) {
    if (const auto wseq = VecIMPLCast(w)) {
      if (auto &alloced = wseq->array_allocated) {
        const auto useit = UseCUPMHostAlloc(util::exchange(w->pinned_memory, PETSC_FALSE));

        PetscCall(PetscFree(alloced));
      }
      wseq->array         = nullptr;
      wseq->unplacedarray = nullptr;
    }
    if (const auto wcu = VecCUPMCast(w)) {
      if (auto &device_array = wcu->array_d) {
        cupmStream_t stream;

        PetscCall(GetHandles_(&stream));
        PetscCallCUPM(cupmFreeAsync(device_array, stream));
      }
      PetscCall(PetscFree(w->spptr /* wcu */));
    }
  }
  if (v->petscnative && wisseqcupm) {
    PetscCall(PetscFree(w->data));
    w->data          = v->data;
    w->offloadmask   = v->offloadmask;
    w->pinned_memory = v->pinned_memory;
    w->spptr         = v->spptr;
    PetscCall(PetscObjectStateIncrease(PetscObjectCast(w)));
  } else {
    const auto array = &VecIMPLCast(w)->array;

    if (access == PETSC_MEMORY_ACCESS_READ) {
      PetscCall(VecGetArrayRead(v, const_cast<const PetscScalar **>(array)));
    } else {
      PetscCall(VecGetArray(v, array));
    }
    w->offloadmask = PETSC_OFFLOAD_CPU;
    if (wisseqcupm) {
      PetscDeviceContext dctx;

      PetscCall(GetHandles_(&dctx));
      PetscCall(DeviceAllocateCheck_(dctx, w));
    }
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->restorelocalvector or v->ops->restorelocalvectorread
template <device::cupm::DeviceType T>
template <PetscMemoryAccessMode access>
inline PetscErrorCode VecSeq_CUPM<T>::RestoreLocalVector(Vec v, Vec w) noexcept
{
  PetscBool wisseqcupm;

  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQCUPM(), VECMPICUPM());
  PetscCall(PetscObjectTypeCompare(PetscObjectCast(w), VECSEQCUPM(), &wisseqcupm));
  if (v->petscnative && wisseqcupm) {
    // the assignments to nullptr are __critical__, as w may persist after this call returns
    // and shouldn't share data with v!
    v->pinned_memory = w->pinned_memory;
    v->offloadmask   = util::exchange(w->offloadmask, PETSC_OFFLOAD_UNALLOCATED);
    v->data          = util::exchange(w->data, nullptr);
    v->spptr         = util::exchange(w->spptr, nullptr);
  } else {
    const auto array = &VecIMPLCast(w)->array;

    if (access == PETSC_MEMORY_ACCESS_READ) {
      PetscCall(VecRestoreArrayRead(v, const_cast<const PetscScalar **>(array)));
    } else {
      PetscCall(VecRestoreArray(v, array));
    }
    if (w->spptr && wisseqcupm) {
      cupmStream_t stream;

      PetscCall(GetHandles_(&stream));
      PetscCallCUPM(cupmFreeAsync(VecCUPMCast(w)->array_d, stream));
      PetscCall(PetscFree(w->spptr));
    }
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// ==========================================================================================
// VecSeq_CUPM - Public API - Compute Methods
// ==========================================================================================

// VecAYPXAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AYPXAsync(Vec yin, PetscScalar alpha, Vec xin, PetscDeviceContext dctx) noexcept
{
  const auto n = static_cast<cupmBlasInt_t>(yin->map->n);
  PetscBool  xiscupm;

  PetscFunctionBegin;
  PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(xin), &xiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  if (!xiscupm) {
    PetscCall(VecAYPX_Seq(yin, alpha, xin));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
  if (alpha == PetscScalar(0.0)) {
    cupmStream_t stream;

    PetscCall(GetHandlesFrom_(dctx, &stream));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCall(PetscCUPMMemcpyAsync(DeviceArrayWrite(dctx, yin).data(), DeviceArrayRead(dctx, xin).data(), n, cupmMemcpyDeviceToDevice, stream));
    PetscCall(PetscLogGpuTimeEnd());
  } else if (n) {
    const auto       alphaIsOne = alpha == PetscScalar(1.0);
    const auto       calpha     = cupmScalarPtrCast(&alpha);
    cupmBlasHandle_t cupmBlasHandle;

    PetscCall(GetHandlesFrom_(dctx, &cupmBlasHandle));
    {
      const auto yptr = DeviceArrayReadWrite(dctx, yin);
      const auto xptr = DeviceArrayRead(dctx, xin);

      PetscCall(PetscLogGpuTimeBegin());
      if (alphaIsOne) {
        PetscCallCUPMBLAS(cupmBlasXaxpy(cupmBlasHandle, n, calpha, xptr.cupmdata(), 1, yptr.cupmdata(), 1));
      } else {
        const auto one = cupmScalarCast(1.0);

        PetscCallCUPMBLAS(cupmBlasXscal(cupmBlasHandle, n, calpha, yptr.cupmdata(), 1));
        PetscCallCUPMBLAS(cupmBlasXaxpy(cupmBlasHandle, n, &one, xptr.cupmdata(), 1, yptr.cupmdata(), 1));
      }
      PetscCall(PetscLogGpuTimeEnd());
    }
    PetscCall(PetscLogGpuFlops((alphaIsOne ? 1 : 2) * n));
  }
  if (n > 0) PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->aypx
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AYPX(Vec yin, PetscScalar alpha, Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(AYPXAsync(yin, alpha, xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecAXPYAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AXPYAsync(Vec yin, PetscScalar alpha, Vec xin, PetscDeviceContext dctx) noexcept
{
  PetscBool xiscupm;

  PetscFunctionBegin;
  PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(xin), &xiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  if (xiscupm) {
    const auto       n = static_cast<cupmBlasInt_t>(yin->map->n);
    cupmBlasHandle_t cupmBlasHandle;

    PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
    PetscCall(GetHandlesFrom_(dctx, &cupmBlasHandle));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallCUPMBLAS(cupmBlasXaxpy(cupmBlasHandle, n, cupmScalarPtrCast(&alpha), DeviceArrayRead(dctx, xin), 1, DeviceArrayReadWrite(dctx, yin), 1));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(2 * n));
    if (n > 0) PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  } else {
    PetscCall(VecAXPY_Seq(yin, alpha, xin));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->axpy
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AXPY(Vec yin, PetscScalar alpha, Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(AXPYAsync(yin, alpha, xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct divides {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &lhs, const PetscScalar &rhs) const noexcept { return rhs == PetscScalar{0.0} ? rhs : lhs / rhs; }
};

} // namespace detail

// VecPointwiseDivideAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseDivideAsync(Vec wout, Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseBinaryDispatch_(VecPointwiseDivide_Seq, detail::divides{}, wout, xin, yin, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->pointwisedivide
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseDivide(Vec wout, Vec xin, Vec yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseDivideAsync(wout, xin, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecPointwiseMultAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseMultAsync(Vec wout, Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseBinaryDispatch_(VecPointwiseMult_Seq, thrust::multiplies<PetscScalar>{}, wout, xin, yin, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->pointwisemult
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseMult(Vec wout, Vec xin, Vec yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseMultAsync(wout, xin, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct MaximumRealPart {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &lhs, const PetscScalar &rhs) const noexcept { return thrust::maximum<PetscReal>{}(PetscRealPart(lhs), PetscRealPart(rhs)); }
};

} // namespace detail

// VecPointwiseMaxAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseMaxAsync(Vec wout, Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseBinaryDispatch_(VecPointwiseMax_Seq, detail::MaximumRealPart{}, wout, xin, yin, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->pointwisemax
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseMax(Vec wout, Vec xin, Vec yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseMaxAsync(wout, xin, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct MaximumAbsoluteValue {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &lhs, const PetscScalar &rhs) const noexcept { return thrust::maximum<PetscReal>{}(PetscAbsScalar(lhs), PetscAbsScalar(rhs)); }
};

} // namespace detail

// VecPointwiseMaxAbsAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseMaxAbsAsync(Vec wout, Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseBinaryDispatch_(VecPointwiseMaxAbs_Seq, detail::MaximumAbsoluteValue{}, wout, xin, yin, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->pointwisemaxabs
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseMaxAbs(Vec wout, Vec xin, Vec yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseMaxAbsAsync(wout, xin, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct MinimumRealPart {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &lhs, const PetscScalar &rhs) const noexcept { return thrust::minimum<PetscReal>{}(PetscRealPart(lhs), PetscRealPart(rhs)); }
};

} // namespace detail

// VecPointwiseMinAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseMinAsync(Vec wout, Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseBinaryDispatch_(VecPointwiseMin_Seq, detail::MinimumRealPart{}, wout, xin, yin, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->pointwisemin
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::PointwiseMin(Vec wout, Vec xin, Vec yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseMinAsync(wout, xin, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct Reciprocal {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &s) const noexcept
  {
    // yes all of this verbosity is needed because sometimes PetscScalar is a thrust::complex
    // and then it matters whether we do s ? true : false vs s == 0, as well as whether we wrap
    // everything in PetscScalar...
    return s == PetscScalar{0.0} ? s : PetscScalar{1.0} / s;
  }
};

} // namespace detail

// VecReciprocalAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::ReciprocalAsync(Vec xin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseUnary_(detail::Reciprocal{}, xin, nullptr, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->reciprocal
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Reciprocal(Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(ReciprocalAsync(xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct AbsoluteValue {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &s) const noexcept { return PetscAbsScalar(s); }
};

} // namespace detail

// VecAbsAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AbsAsync(Vec xin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseUnary_(detail::AbsoluteValue{}, xin, nullptr, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->abs
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Abs(Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(AbsAsync(xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct SquareRootAbsoluteValue {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &s) const noexcept { return PetscSqrtReal(PetscAbsScalar(s)); }
};

} // namespace detail

// VecSqrtAbsAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::SqrtAbsAsync(Vec xin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseUnary_(detail::SquareRootAbsoluteValue{}, xin, nullptr, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->sqrt
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::SqrtAbs(Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(SqrtAbsAsync(xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct Exponent {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &s) const noexcept { return PetscExpScalar(s); }
};

} // namespace detail

// VecExpAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::ExpAsync(Vec xin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseUnary_(detail::Exponent{}, xin, nullptr, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->exp
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Exp(Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(ExpAsync(xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct Logarithm {
  PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &s) const noexcept { return PetscLogScalar(s); }
};

} // namespace detail

// VecLogAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::LogAsync(Vec xin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseUnary_(detail::Logarithm{}, xin, nullptr, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->log
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Log(Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(LogAsync(xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->waxpy
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::WAXPYAsync(Vec win, PetscScalar alpha, Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscBool xiscupm, yiscupm;

  PetscFunctionBegin;
  PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(xin), &xiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(yin), &yiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  if (!xiscupm || !yiscupm) {
    PetscCall(VecWAXPY_Seq(win, alpha, xin, yin));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
  if (alpha == PetscScalar(0.0)) {
    PetscCall(CopyAsync(yin, win, dctx));
  } else if (const auto n = static_cast<cupmBlasInt_t>(win->map->n)) {
    cupmBlasHandle_t cupmBlasHandle;
    cupmStream_t     stream;
    PetscBool        xiscupm, yiscupm;

    PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(xin), &xiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
    PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(yin), &yiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
    if (!xiscupm || !yiscupm) {
      PetscCall(VecWAXPY_Seq(win, alpha, xin, yin));
      PetscFunctionReturn(PETSC_SUCCESS);
    }
    PetscCall(GetHandlesFrom_(dctx, &cupmBlasHandle, NULL, &stream));
    {
      const auto wptr = DeviceArrayWrite(dctx, win);

      PetscCall(PetscLogGpuTimeBegin());
      PetscCall(PetscCUPMMemcpyAsync(wptr.data(), DeviceArrayRead(dctx, yin).data(), n, cupmMemcpyDeviceToDevice, stream, true));
      PetscCallCUPMBLAS(cupmBlasXaxpy(cupmBlasHandle, n, cupmScalarPtrCast(&alpha), DeviceArrayRead(dctx, xin), 1, wptr.cupmdata(), 1));
      PetscCall(PetscLogGpuTimeEnd());
    }
    PetscCall(PetscLogGpuFlops(2 * n));
    PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->waxpy
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::WAXPY(Vec win, PetscScalar alpha, Vec xin, Vec yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(WAXPYAsync(win, alpha, xin, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace kernels
{

template <typename... Args>
PETSC_KERNEL_DECL static void MAXPY_kernel(const PetscInt size, PetscScalar *PETSC_RESTRICT xptr, const PetscScalar *PETSC_RESTRICT aptr, Args... yptr)
{
  constexpr int      N        = sizeof...(Args);
  const auto         tx       = threadIdx.x;
  const PetscScalar *yptr_p[] = {yptr...};

  PETSC_SHAREDMEM_DECL PetscScalar aptr_shmem[N];

  // load a to shared memory
  if (tx < N) aptr_shmem[tx] = aptr[tx];
  __syncthreads();

  ::Petsc::device::cupm::kernels::util::grid_stride_1D(size, [&](PetscInt i) {
  // these may look the same but give different results!
#if 0
    PetscScalar sum = 0.0;

  #pragma unroll
    for (auto j = 0; j < N; ++j) sum += aptr_shmem[j]*yptr_p[j][i];
    xptr[i] += sum;
#else
    auto sum = xptr[i];

  #pragma unroll
    for (auto j = 0; j < N; ++j) sum += aptr_shmem[j] * yptr_p[j][i];
    xptr[i] = sum;
#endif
  });
  return;
}

} // namespace kernels

namespace detail
{

// a helper-struct to gobble the size_t input, it is used with template parameter pack
// expansion such that
// typename repeat_type<MyType, IdxParamPack>...
// expands to
// MyType, MyType, MyType, ... [repeated sizeof...(IdxParamPack) times]
template <typename T, std::size_t>
struct repeat_type {
  using type = T;
};

} // namespace detail

template <device::cupm::DeviceType T>
template <std::size_t... Idx>
inline PetscErrorCode VecSeq_CUPM<T>::MAXPY_kernel_dispatch_(PetscDeviceContext dctx, cupmStream_t stream, PetscScalar *xptr, const PetscScalar *aptr, const Vec *yin, PetscInt size, util::index_sequence<Idx...>) noexcept
{
  PetscFunctionBegin;
  // clang-format off
  PetscCall(
    PetscCUPMLaunchKernel1D(
      size, 0, stream,
      kernels::MAXPY_kernel<typename detail::repeat_type<const PetscScalar *, Idx>::type...>,
      size, xptr, aptr, DeviceArrayRead(dctx, yin[Idx]).data()...
    )
  );
  // clang-format on
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
template <int N>
inline PetscErrorCode VecSeq_CUPM<T>::MAXPY_kernel_dispatch_(PetscDeviceContext dctx, cupmStream_t stream, PetscScalar *xptr, const PetscScalar *aptr, const Vec *yin, PetscInt size, PetscInt &yidx) noexcept
{
  PetscFunctionBegin;
  PetscCall(MAXPY_kernel_dispatch_(dctx, stream, xptr, aptr + yidx, yin + yidx, size, util::make_index_sequence<N>{}));
  yidx += N;
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecMAXPYAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::MAXPYAsync(Vec xin, PetscInt nv, const PetscScalar *alpha, Vec *yin, PetscDeviceContext dctx) noexcept
{
  const auto   n = xin->map->n;
  cupmStream_t stream;
  PetscBool    yiscupm = PETSC_TRUE;

  PetscFunctionBegin;
  for (PetscInt i = 0; i < nv && yiscupm; i++) PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(yin[i]), &yiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  if (!yiscupm) {
    PetscCall(VecMAXPY_Seq(xin, nv, alpha, yin));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
  PetscCall(GetHandlesFrom_(dctx, &stream));
  {
    const auto   xptr    = DeviceArrayReadWrite(dctx, xin);
    PetscScalar *d_alpha = nullptr;
    PetscInt     yidx    = 0;

    // placement of early-return is deliberate, we would like to capture the
    // DeviceArrayReadWrite() call (which calls PetscObjectStateIncreate()) before we bail
    if (!n || !nv) PetscFunctionReturn(PETSC_SUCCESS);
    PetscCall(PetscDeviceMalloc(dctx, PETSC_MEMTYPE_CUPM(), nv, &d_alpha));
    PetscCall(PetscCUPMMemcpyAsync(d_alpha, alpha, nv, cupmMemcpyHostToDevice, stream));
    PetscCall(PetscLogGpuTimeBegin());
    do {
      switch (nv - yidx) {
      case 7:
        PetscCall(MAXPY_kernel_dispatch_<7>(dctx, stream, xptr.data(), d_alpha, yin, n, yidx));
        break;
      case 6:
        PetscCall(MAXPY_kernel_dispatch_<6>(dctx, stream, xptr.data(), d_alpha, yin, n, yidx));
        break;
      case 5:
        PetscCall(MAXPY_kernel_dispatch_<5>(dctx, stream, xptr.data(), d_alpha, yin, n, yidx));
        break;
      case 4:
        PetscCall(MAXPY_kernel_dispatch_<4>(dctx, stream, xptr.data(), d_alpha, yin, n, yidx));
        break;
      case 3:
        PetscCall(MAXPY_kernel_dispatch_<3>(dctx, stream, xptr.data(), d_alpha, yin, n, yidx));
        break;
      case 2:
        PetscCall(MAXPY_kernel_dispatch_<2>(dctx, stream, xptr.data(), d_alpha, yin, n, yidx));
        break;
      case 1:
        PetscCall(MAXPY_kernel_dispatch_<1>(dctx, stream, xptr.data(), d_alpha, yin, n, yidx));
        break;
      default: // 8 or more
        PetscCall(MAXPY_kernel_dispatch_<8>(dctx, stream, xptr.data(), d_alpha, yin, n, yidx));
        break;
      }
    } while (yidx < nv);
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscDeviceFree(dctx, d_alpha));
    PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  }
  PetscCall(PetscLogGpuFlops(nv * 2 * n));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->maxpy
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::MAXPY(Vec xin, PetscInt nv, const PetscScalar *alpha, Vec *yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(MAXPYAsync(xin, nv, alpha, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Dot(Vec xin, Vec yin, PetscScalar *z) noexcept
{
  PetscBool yiscupm;

  PetscFunctionBegin;
  PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(yin), &yiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  if (!yiscupm) {
    PetscCall(VecDot_Seq(xin, yin, z));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  if (const auto n = static_cast<cupmBlasInt_t>(xin->map->n)) {
    PetscDeviceContext dctx;
    cupmBlasHandle_t   cupmBlasHandle;

    PetscCall(GetHandles_(&dctx, &cupmBlasHandle));
    // arguments y, x are reversed because BLAS complex conjugates the first argument, PETSc the
    // second
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallCUPMBLAS(cupmBlasXdot(cupmBlasHandle, n, DeviceArrayRead(dctx, yin), 1, DeviceArrayRead(dctx, xin), 1, cupmScalarPtrCast(z)));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(2 * n - 1));
  } else {
    *z = 0.0;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

#define MDOT_WORKGROUP_NUM  128
#define MDOT_WORKGROUP_SIZE MDOT_WORKGROUP_NUM

namespace kernels
{

PETSC_DEVICE_INLINE_DECL static PetscInt EntriesPerGroup(const PetscInt size) noexcept
{
  const auto group_entries = (size - 1) / gridDim.x + 1;
  // for very small vectors, a group should still do some work
  return group_entries ? group_entries : 1;
}

template <typename... ConstPetscScalarPointer>
PETSC_KERNEL_DECL static void MDot_kernel(const PetscScalar *PETSC_RESTRICT x, const PetscInt size, PetscScalar *PETSC_RESTRICT results, ConstPetscScalarPointer... y)
{
  constexpr int      N        = sizeof...(ConstPetscScalarPointer);
  const PetscScalar *ylocal[] = {y...};
  PetscScalar        sumlocal[N];

  PETSC_SHAREDMEM_DECL PetscScalar shmem[N * MDOT_WORKGROUP_SIZE];

  // HIP -- for whatever reason -- has threadIdx, blockIdx, blockDim, and gridDim as separate
  // types, so each of these go on separate lines...
  const auto tx       = threadIdx.x;
  const auto bx       = blockIdx.x;
  const auto bdx      = blockDim.x;
  const auto gdx      = gridDim.x;
  const auto worksize = EntriesPerGroup(size);
  const auto begin    = tx + bx * worksize;
  const auto end      = min((bx + 1) * worksize, size);

#pragma unroll
  for (auto i = 0; i < N; ++i) sumlocal[i] = 0;

  for (auto i = begin; i < end; i += bdx) {
    const auto xi = x[i]; // load only once from global memory!

#pragma unroll
    for (auto j = 0; j < N; ++j) sumlocal[j] += ylocal[j][i] * xi;
  }

#pragma unroll
  for (auto i = 0; i < N; ++i) shmem[tx + i * MDOT_WORKGROUP_SIZE] = sumlocal[i];

  // parallel reduction
  for (auto stride = bdx / 2; stride > 0; stride /= 2) {
    __syncthreads();
    if (tx < stride) {
#pragma unroll
      for (auto i = 0; i < N; ++i) shmem[tx + i * MDOT_WORKGROUP_SIZE] += shmem[tx + stride + i * MDOT_WORKGROUP_SIZE];
    }
  }
  // bottom N threads per block write to global memory
  // REVIEW ME: I am ~pretty~ sure we don't need another __syncthreads() here since each thread
  // writes to the same sections in the above loop that it is about to read from below, but
  // running this under the racecheck tool of cuda-memcheck reports a write-after-write hazard.
  __syncthreads();
  if (tx < N) results[bx + tx * gdx] = shmem[tx * MDOT_WORKGROUP_SIZE];
  return;
}

namespace
{

PETSC_KERNEL_DECL void sum_kernel(const PetscInt size, PetscScalar *PETSC_RESTRICT results)
{
  int         local_i = 0;
  PetscScalar local_results[8];

  // each thread sums up MDOT_WORKGROUP_NUM entries of the result, storing it in a local buffer
  //
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  // | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | ...
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  //  |  ______________________________________________________/
  //  | /            <- MDOT_WORKGROUP_NUM ->
  //  |/
  //  +
  //  v
  // *-*-*
  // | | | ...
  // *-*-*
  //
  ::Petsc::device::cupm::kernels::util::grid_stride_1D(size, [&](PetscInt i) {
    PetscScalar z_sum = 0;

    for (auto j = i * MDOT_WORKGROUP_SIZE; j < (i + 1) * MDOT_WORKGROUP_SIZE; ++j) z_sum += results[j];
    local_results[local_i++] = z_sum;
  });
  // if we needed more than 1 workgroup to handle the vector we should sync since other threads
  // may currently be reading from results
  if (size >= MDOT_WORKGROUP_SIZE) __syncthreads();
  // Local buffer is now written to global memory
  ::Petsc::device::cupm::kernels::util::grid_stride_1D(size, [&](PetscInt i) {
    const auto j = --local_i;

    if (j >= 0) results[i] = local_results[j];
  });
  return;
}

} // namespace

#if PetscDefined(USING_HCC)
namespace do_not_use
{

inline void silence_warning_function_sum_kernel_is_not_needed_and_will_not_be_emitted()
{
  (void)sum_kernel;
}

} // namespace do_not_use
#endif

} // namespace kernels

template <device::cupm::DeviceType T>
template <std::size_t... Idx>
inline PetscErrorCode VecSeq_CUPM<T>::MDot_kernel_dispatch_(PetscDeviceContext dctx, cupmStream_t stream, const PetscScalar *xarr, const Vec yin[], PetscInt size, PetscScalar *results, util::index_sequence<Idx...>) noexcept
{
  PetscFunctionBegin;
  // REVIEW ME: convert this kernel launch to PetscCUPMLaunchKernel1D(), it currently launches
  // 128 blocks of 128 threads every time which may be wasteful
  // clang-format off
  PetscCallCUPM(
    cupmLaunchKernel(
      kernels::MDot_kernel<typename detail::repeat_type<const PetscScalar *, Idx>::type...>,
      MDOT_WORKGROUP_NUM, MDOT_WORKGROUP_SIZE, 0, stream,
      xarr, size, results, DeviceArrayRead(dctx, yin[Idx]).data()...
    )
  );
  // clang-format on
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
template <int N>
inline PetscErrorCode VecSeq_CUPM<T>::MDot_kernel_dispatch_(PetscDeviceContext dctx, cupmStream_t stream, const PetscScalar *xarr, const Vec yin[], PetscInt size, PetscScalar *results, PetscInt &yidx) noexcept
{
  PetscFunctionBegin;
  PetscCall(MDot_kernel_dispatch_(dctx, stream, xarr, yin + yidx, size, results + yidx * MDOT_WORKGROUP_NUM, util::make_index_sequence<N>{}));
  yidx += N;
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::MDot_(std::false_type, Vec xin, PetscInt nv, const Vec yin[], PetscScalar *z, PetscDeviceContext dctx) noexcept
{
  // the largest possible size of a batch
  constexpr PetscInt batchsize = 8;
  // how many sub streams to create, if nv <= batchsize we can do this without looping, so we
  // do not create substreams. Note we don't create more than 8 streams, in practice we could
  // not get more parallelism with higher numbers.
  const auto   num_sub_streams = nv > batchsize ? std::min((nv + batchsize) / batchsize, batchsize) : 0;
  const auto   n               = xin->map->n;
  const auto   nwork           = nv * MDOT_WORKGROUP_NUM;
  PetscScalar *d_results;
  cupmStream_t stream;

  PetscFunctionBegin;
  PetscCall(GetHandlesFrom_(dctx, &stream));
  // allocate scratchpad memory for the results of individual work groups
  PetscCall(PetscDeviceMalloc(dctx, PETSC_MEMTYPE_CUPM(), nwork, &d_results));
  {
    const auto          xptr       = DeviceArrayRead(dctx, xin);
    PetscInt            yidx       = 0;
    auto                subidx     = 0;
    auto                cur_stream = stream;
    auto                cur_ctx    = dctx;
    PetscDeviceContext *sub        = nullptr;
    PetscStreamType     stype;

    // REVIEW ME: maybe PetscDeviceContextFork() should insert dctx into the first entry of
    // sub. Ideally the parent context should also join in on the fork, but it is extremely
    // fiddly to do so presently
    PetscCall(PetscDeviceContextGetStreamType(dctx, &stype));
    if (stype == PETSC_STREAM_DEFAULT || stype == PETSC_STREAM_DEFAULT_WITH_BARRIER) stype = PETSC_STREAM_NONBLOCKING;
    // If we have a default stream create nonblocking streams instead (as we can
    // locally exploit the parallelism). Otherwise use the prescribed stream type.
    PetscCall(PetscDeviceContextForkWithStreamType(dctx, stype, num_sub_streams, &sub));
    PetscCall(PetscLogGpuTimeBegin());
    do {
      if (num_sub_streams) {
        cur_ctx = sub[subidx++ % num_sub_streams];
        PetscCall(GetHandlesFrom_(cur_ctx, &cur_stream));
      }
      // REVIEW ME: Should probably try and load-balance these. Consider the case where nv = 9;
      // it is very likely better to do 4+5 rather than 8+1
      switch (nv - yidx) {
      case 7:
        PetscCall(MDot_kernel_dispatch_<7>(cur_ctx, cur_stream, xptr.data(), yin, n, d_results, yidx));
        break;
      case 6:
        PetscCall(MDot_kernel_dispatch_<6>(cur_ctx, cur_stream, xptr.data(), yin, n, d_results, yidx));
        break;
      case 5:
        PetscCall(MDot_kernel_dispatch_<5>(cur_ctx, cur_stream, xptr.data(), yin, n, d_results, yidx));
        break;
      case 4:
        PetscCall(MDot_kernel_dispatch_<4>(cur_ctx, cur_stream, xptr.data(), yin, n, d_results, yidx));
        break;
      case 3:
        PetscCall(MDot_kernel_dispatch_<3>(cur_ctx, cur_stream, xptr.data(), yin, n, d_results, yidx));
        break;
      case 2:
        PetscCall(MDot_kernel_dispatch_<2>(cur_ctx, cur_stream, xptr.data(), yin, n, d_results, yidx));
        break;
      case 1:
        PetscCall(MDot_kernel_dispatch_<1>(cur_ctx, cur_stream, xptr.data(), yin, n, d_results, yidx));
        break;
      default: // 8 or more
        PetscCall(MDot_kernel_dispatch_<8>(cur_ctx, cur_stream, xptr.data(), yin, n, d_results, yidx));
        break;
      }
    } while (yidx < nv);
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscDeviceContextJoin(dctx, num_sub_streams, PETSC_DEVICE_CONTEXT_JOIN_DESTROY, &sub));
  }

  PetscCall(PetscCUPMLaunchKernel1D(nv, 0, stream, kernels::sum_kernel, nv, d_results));
  // copy result of device reduction to host
  PetscCall(PetscCUPMMemcpyAsync(z, d_results, nv, cupmMemcpyDeviceToHost, stream));
  // do these now while final reduction is in flight
  PetscCall(PetscLogGpuFlops(nwork));
  PetscCall(PetscDeviceFree(dctx, d_results));
  PetscFunctionReturn(PETSC_SUCCESS);
}

#undef MDOT_WORKGROUP_NUM
#undef MDOT_WORKGROUP_SIZE

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::MDot_(std::true_type, Vec xin, PetscInt nv, const Vec yin[], PetscScalar *z, PetscDeviceContext dctx) noexcept
{
  // probably not worth it to run more than 8 of these at a time?
  const auto          n_sub = PetscMin(nv, 8);
  const auto          n     = static_cast<cupmBlasInt_t>(xin->map->n);
  const auto          xptr  = DeviceArrayRead(dctx, xin);
  PetscScalar        *d_z;
  PetscDeviceContext *subctx;
  cupmStream_t        stream;

  PetscFunctionBegin;
  PetscCall(GetHandlesFrom_(dctx, &stream));
  PetscCall(PetscDeviceMalloc(dctx, PETSC_MEMTYPE_CUPM(), nv, &d_z));
  PetscCall(PetscDeviceContextFork(dctx, n_sub, &subctx));
  PetscCall(PetscLogGpuTimeBegin());
  for (PetscInt i = 0; i < nv; ++i) {
    const auto            sub = subctx[i % n_sub];
    cupmBlasHandle_t      handle;
    cupmBlasPointerMode_t old_mode;

    PetscCall(GetHandlesFrom_(sub, &handle));
    PetscCallCUPMBLAS(cupmBlasGetPointerMode(handle, &old_mode));
    if (old_mode != CUPMBLAS_POINTER_MODE_DEVICE) PetscCallCUPMBLAS(cupmBlasSetPointerMode(handle, CUPMBLAS_POINTER_MODE_DEVICE));
    PetscCallCUPMBLAS(cupmBlasXdot(handle, n, DeviceArrayRead(sub, yin[i]), 1, xptr.cupmdata(), 1, cupmScalarPtrCast(d_z + i)));
    if (old_mode != CUPMBLAS_POINTER_MODE_DEVICE) PetscCallCUPMBLAS(cupmBlasSetPointerMode(handle, old_mode));
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(PetscDeviceContextJoin(dctx, n_sub, PETSC_DEVICE_CONTEXT_JOIN_DESTROY, &subctx));
  PetscCall(PetscCUPMMemcpyAsync(z, d_z, nv, cupmMemcpyDeviceToHost, stream));
  PetscCall(PetscDeviceFree(dctx, d_z));
  // REVIEW ME: flops?????
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->mdot
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::MDot(Vec xin, PetscInt nv, const Vec yin[], PetscScalar *z) noexcept
{
  PetscFunctionBegin;
  if (PetscUnlikely(nv == 1)) {
    // dot handles nv = 0 correctly
    PetscCall(Dot(xin, const_cast<Vec>(yin[0]), z));
  } else if (const auto n = xin->map->n) {
    PetscDeviceContext dctx;

    PetscCheck(nv > 0, PETSC_COMM_SELF, PETSC_ERR_LIB, "Number of vectors provided to %s %" PetscInt_FMT " not positive", PETSC_FUNCTION_NAME, nv);
    PetscCall(GetHandles_(&dctx));
    PetscCall(MDot_(std::integral_constant<bool, PetscDefined(USE_COMPLEX)>{}, xin, nv, yin, z, dctx));
    // REVIEW ME: double count of flops??
    PetscCall(PetscLogGpuFlops(nv * (2 * n - 1)));
    PetscCall(PetscDeviceContextSynchronize(dctx));
  } else {
    PetscCall(PetscArrayzero(z, nv));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecSetAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::SetAsync(Vec xin, PetscScalar alpha, PetscDeviceContext dctx) noexcept
{
  const auto   n = xin->map->n;
  cupmStream_t stream;

  PetscFunctionBegin;
  PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
  PetscCall(GetHandlesFrom_(dctx, &stream));
  {
    const auto xptr = DeviceArrayWrite(dctx, xin);

    if (alpha == PetscScalar(0.0)) {
      PetscCall(PetscCUPMMemsetAsync(xptr.data(), 0, n, stream));
    } else {
      const auto dptr = thrust::device_pointer_cast(xptr.data());

      PetscCallThrust(THRUST_CALL(thrust::fill, stream, dptr, dptr + n, alpha));
    }
  }
  if (n > 0) PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->set
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Set(Vec xin, PetscScalar alpha) noexcept
{
  PetscFunctionBegin;
  PetscCall(SetAsync(xin, alpha, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecScaleAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::ScaleAsync(Vec xin, PetscScalar alpha, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  if (PetscUnlikely(alpha == PetscScalar(1.0))) PetscFunctionReturn(PETSC_SUCCESS);
  PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
  if (PetscUnlikely(alpha == PetscScalar(0.0))) {
    PetscCall(SetAsync(xin, alpha, dctx));
  } else if (const auto n = static_cast<cupmBlasInt_t>(xin->map->n)) {
    cupmBlasHandle_t cupmBlasHandle;

    PetscCall(GetHandlesFrom_(dctx, &cupmBlasHandle));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallCUPMBLAS(cupmBlasXscal(cupmBlasHandle, n, cupmScalarPtrCast(&alpha), DeviceArrayReadWrite(dctx, xin), 1));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(n));
    PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  } else {
    PetscCall(MaybeIncrementEmptyLocalVec(xin));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->scale
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Scale(Vec xin, PetscScalar alpha) noexcept
{
  PetscFunctionBegin;
  PetscCall(ScaleAsync(xin, alpha, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->tdot
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::TDot(Vec xin, Vec yin, PetscScalar *z) noexcept
{
  PetscBool yiscupm;

  PetscFunctionBegin;
  PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(yin), &yiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  if (!yiscupm) {
    PetscCall(VecTDot_Seq(xin, yin, z));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  if (const auto n = static_cast<cupmBlasInt_t>(xin->map->n)) {
    PetscDeviceContext dctx;
    cupmBlasHandle_t   cupmBlasHandle;

    PetscCall(GetHandles_(&dctx, &cupmBlasHandle));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallCUPMBLAS(cupmBlasXdotu(cupmBlasHandle, n, DeviceArrayRead(dctx, xin), 1, DeviceArrayRead(dctx, yin), 1, cupmScalarPtrCast(z)));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(2 * n - 1));
  } else {
    *z = 0.0;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecCopyAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::CopyAsync(Vec xin, Vec yout, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  if (xin == yout) PetscFunctionReturn(PETSC_SUCCESS);
  if (const auto n = xin->map->n) {
    const auto xmask = xin->offloadmask;
    // silence buggy gcc warning: mode may be used uninitialized in this function
    auto         mode = cupmMemcpyDeviceToDevice;
    cupmStream_t stream;

    // translate from PetscOffloadMask to cupmMemcpyKind
    PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
    switch (const auto ymask = yout->offloadmask) {
    case PETSC_OFFLOAD_UNALLOCATED: {
      PetscBool yiscupm;

      PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(yout), &yiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
      if (yiscupm) {
        mode = PetscOffloadDevice(xmask) ? cupmMemcpyDeviceToDevice : cupmMemcpyHostToHost;
        break;
      }
    } // fall-through if unallocated and not cupm
#if PETSC_CPP_VERSION >= 17
      [[fallthrough]];
#endif
    case PETSC_OFFLOAD_CPU: {
      PetscBool yiscupm;

      PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(yout), &yiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
      if (yiscupm) {
        mode = PetscOffloadHost(xmask) ? cupmMemcpyHostToDevice : cupmMemcpyDeviceToDevice;
      } else {
        mode = PetscOffloadHost(xmask) ? cupmMemcpyHostToHost : cupmMemcpyDeviceToHost;
      }
      break;
    }
    case PETSC_OFFLOAD_BOTH:
    case PETSC_OFFLOAD_GPU:
      mode = PetscOffloadDevice(xmask) ? cupmMemcpyDeviceToDevice : cupmMemcpyHostToDevice;
      break;
    default:
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Incompatible offload mask %s", PetscOffloadMaskToString(ymask));
    }

    PetscCall(GetHandlesFrom_(dctx, &stream));
    switch (mode) {
    case cupmMemcpyDeviceToDevice: // the best case
    case cupmMemcpyHostToDevice: { // not terrible
      const auto yptr = DeviceArrayWrite(dctx, yout);
      const auto xptr = mode == cupmMemcpyDeviceToDevice ? DeviceArrayRead(dctx, xin).data() : HostArrayRead(dctx, xin).data();

      PetscCall(PetscLogGpuTimeBegin());
      PetscCall(PetscCUPMMemcpyAsync(yptr.data(), xptr, n, mode, stream));
      PetscCall(PetscLogGpuTimeEnd());
    } break;
    case cupmMemcpyDeviceToHost: // not great
    case cupmMemcpyHostToHost: { // worst case
      const auto   xptr = mode == cupmMemcpyDeviceToHost ? DeviceArrayRead(dctx, xin).data() : HostArrayRead(dctx, xin).data();
      PetscScalar *yptr;

      PetscCall(VecGetArrayWrite(yout, &yptr));
      if (mode == cupmMemcpyDeviceToHost) PetscCall(PetscLogGpuTimeBegin());
      PetscCall(PetscCUPMMemcpyAsync(yptr, xptr, n, mode, stream, /* force async */ true));
      if (mode == cupmMemcpyDeviceToHost) PetscCall(PetscLogGpuTimeEnd());
      PetscCall(VecRestoreArrayWrite(yout, &yptr));
    } break;
    default:
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_GPU, "Unknown cupmMemcpyKind %d", static_cast<int>(mode));
    }
    PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  } else {
    PetscCall(MaybeIncrementEmptyLocalVec(yout));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->copy
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Copy(Vec xin, Vec yout) noexcept
{
  PetscFunctionBegin;
  PetscCall(CopyAsync(xin, yout, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecSwapAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::SwapAsync(Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscBool yiscupm;

  PetscFunctionBegin;
  if (xin == yin) PetscFunctionReturn(PETSC_SUCCESS);
  PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(yin), &yiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  PetscCheck(yiscupm, PetscObjectComm(PetscObjectCast(yin)), PETSC_ERR_SUP, "Cannot swap with Y of type %s", PetscObjectCast(yin)->type_name);
  PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
  if (const auto n = static_cast<cupmBlasInt_t>(xin->map->n)) {
    cupmBlasHandle_t cupmBlasHandle;

    PetscCall(GetHandlesFrom_(dctx, &cupmBlasHandle));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallCUPMBLAS(cupmBlasXswap(cupmBlasHandle, n, DeviceArrayReadWrite(dctx, xin), 1, DeviceArrayReadWrite(dctx, yin), 1));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  } else {
    PetscCall(MaybeIncrementEmptyLocalVec(xin));
    PetscCall(MaybeIncrementEmptyLocalVec(yin));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->swap
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Swap(Vec xin, Vec yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(SwapAsync(xin, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecAXPYBYAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AXPBYAsync(Vec yin, PetscScalar alpha, PetscScalar beta, Vec xin, PetscDeviceContext dctx) noexcept
{
  PetscBool xiscupm;

  PetscFunctionBegin;
  PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(xin), &xiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
  if (!xiscupm) {
    PetscCall(VecAXPBY_Seq(yin, alpha, beta, xin));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
  if (alpha == PetscScalar(0.0)) {
    PetscCall(ScaleAsync(yin, beta, dctx));
  } else if (beta == PetscScalar(1.0)) {
    PetscCall(AXPYAsync(yin, alpha, xin, dctx));
  } else if (alpha == PetscScalar(1.0)) {
    PetscCall(AYPXAsync(yin, beta, xin, dctx));
  } else if (const auto n = static_cast<cupmBlasInt_t>(yin->map->n)) {
    PetscBool xiscupm;

    PetscCall(PetscObjectTypeCompareAny(PetscObjectCast(xin), &xiscupm, VECSEQCUPM(), VECMPICUPM(), ""));
    if (!xiscupm) {
      PetscCall(VecAXPBY_Seq(yin, alpha, beta, xin));
      PetscFunctionReturn(PETSC_SUCCESS);
    }

    const auto       betaIsZero = beta == PetscScalar(0.0);
    const auto       aptr       = cupmScalarPtrCast(&alpha);
    cupmBlasHandle_t cupmBlasHandle;

    PetscCall(GetHandlesFrom_(dctx, &cupmBlasHandle));
    {
      const auto xptr = DeviceArrayRead(dctx, xin);

      if (betaIsZero /* beta = 0 */) {
        // here we can get away with purely write-only as we memcpy into it first
        const auto   yptr = DeviceArrayWrite(dctx, yin);
        cupmStream_t stream;

        PetscCall(GetHandlesFrom_(dctx, &stream));
        PetscCall(PetscLogGpuTimeBegin());
        PetscCall(PetscCUPMMemcpyAsync(yptr.data(), xptr.data(), n, cupmMemcpyDeviceToDevice, stream));
        PetscCallCUPMBLAS(cupmBlasXscal(cupmBlasHandle, n, aptr, yptr.cupmdata(), 1));
      } else {
        const auto yptr = DeviceArrayReadWrite(dctx, yin);

        PetscCall(PetscLogGpuTimeBegin());
        PetscCallCUPMBLAS(cupmBlasXscal(cupmBlasHandle, n, cupmScalarPtrCast(&beta), yptr.cupmdata(), 1));
        PetscCallCUPMBLAS(cupmBlasXaxpy(cupmBlasHandle, n, aptr, xptr.cupmdata(), 1, yptr.cupmdata(), 1));
      }
    }
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops((betaIsZero ? 1 : 3) * n));
    PetscCall(PetscDeviceContextSynchronizeIfWithBarrier_Internal(dctx));
  } else {
    PetscCall(MaybeIncrementEmptyLocalVec(yin));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->axpby
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AXPBY(Vec yin, PetscScalar alpha, PetscScalar beta, Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(AXPBYAsync(yin, alpha, beta, xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// VecAXPBYPCZAsync_Private
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AXPBYPCZAsync(Vec zin, PetscScalar alpha, PetscScalar beta, PetscScalar gamma, Vec xin, Vec yin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PetscDeviceContextGetOptionalNullContext_Internal(&dctx));
  if (gamma != PetscScalar(1.0)) PetscCall(ScaleAsync(zin, gamma, dctx));
  PetscCall(AXPYAsync(zin, alpha, xin, dctx));
  PetscCall(AXPYAsync(zin, beta, yin, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->axpbypcz
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::AXPBYPCZ(Vec zin, PetscScalar alpha, PetscScalar beta, PetscScalar gamma, Vec xin, Vec yin) noexcept
{
  PetscFunctionBegin;
  PetscCall(AXPBYPCZAsync(zin, alpha, beta, gamma, xin, yin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->norm
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Norm(Vec xin, NormType type, PetscReal *z) noexcept
{
  PetscDeviceContext dctx;
  cupmBlasHandle_t   cupmBlasHandle;

  PetscFunctionBegin;
  PetscCall(GetHandles_(&dctx, &cupmBlasHandle));
  if (const auto n = static_cast<cupmBlasInt_t>(xin->map->n)) {
    const auto xptr      = DeviceArrayRead(dctx, xin);
    PetscInt   flopCount = 0;

    PetscCall(PetscLogGpuTimeBegin());
    switch (type) {
    case NORM_1_AND_2:
    case NORM_1:
      PetscCallCUPMBLAS(cupmBlasXasum(cupmBlasHandle, n, xptr.cupmdata(), 1, cupmRealPtrCast(z)));
      flopCount = std::max(n - 1, 0);
      if (type == NORM_1) break;
      ++z; // fall-through
#if PETSC_CPP_VERSION >= 17
      [[fallthrough]];
#endif
    case NORM_2:
    case NORM_FROBENIUS:
      PetscCallCUPMBLAS(cupmBlasXnrm2(cupmBlasHandle, n, xptr.cupmdata(), 1, cupmRealPtrCast(z)));
      flopCount += std::max(2 * n - 1, 0); // += in case we've fallen through from NORM_1_AND_2
      break;
    case NORM_INFINITY: {
      cupmBlasInt_t max_loc = 0;
      PetscScalar   xv      = 0.;
      cupmStream_t  stream;

      PetscCall(GetHandlesFrom_(dctx, &stream));
      PetscCallCUPMBLAS(cupmBlasXamax(cupmBlasHandle, n, xptr.cupmdata(), 1, &max_loc));
      PetscCall(PetscCUPMMemcpyAsync(&xv, xptr.data() + max_loc - 1, 1, cupmMemcpyDeviceToHost, stream));
      *z = PetscAbsScalar(xv);
      // REVIEW ME: flopCount = ???
    } break;
    }
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(flopCount));
  } else {
    z[0]                    = 0.0;
    z[type == NORM_1_AND_2] = 0.0;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

template <NormType wnormtype>
class ErrorWNormTransformBase {
public:
  using result_type = thrust::tuple<PetscReal, PetscReal, PetscReal, PetscInt, PetscInt, PetscInt>;

  constexpr explicit ErrorWNormTransformBase(PetscReal v) noexcept : ignore_max_{v} { }

protected:
  struct NormTuple {
    PetscReal norm;
    PetscInt  loc;
  };

  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL static NormTuple compute_norm_(PetscReal err, PetscReal tol) noexcept
  {
    if (tol > 0.) {
      const auto val = err / tol;

      return {wnormtype == NORM_INFINITY ? val : PetscSqr(val), 1};
    } else {
      return {0.0, 0};
    }
  }

  PetscReal ignore_max_;
};

template <NormType wnormtype>
struct ErrorWNormTransform : ErrorWNormTransformBase<wnormtype> {
  using base_type     = ErrorWNormTransformBase<wnormtype>;
  using result_type   = typename base_type::result_type;
  using argument_type = thrust::tuple<PetscScalar, PetscScalar, PetscScalar, PetscScalar>;

  using base_type::base_type;

  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL result_type operator()(const argument_type &x) const noexcept
  {
    const auto u     = thrust::get<0>(x); // with x.get<0>(), cuda-12.4.0 gives error: class "cuda::std::__4::tuple<PetscScalar, PetscScalar, PetscScalar, PetscScalar>" has no member "get"
    const auto y     = thrust::get<1>(x);
    const auto au    = PetscAbsScalar(u);
    const auto ay    = PetscAbsScalar(y);
    const auto skip  = au < this->ignore_max_ || ay < this->ignore_max_;
    const auto tola  = skip ? 0.0 : PetscRealPart(thrust::get<2>(x));
    const auto tolr  = skip ? 0.0 : PetscRealPart(thrust::get<3>(x)) * PetscMax(au, ay);
    const auto tol   = tola + tolr;
    const auto err   = PetscAbsScalar(u - y);
    const auto tup_a = this->compute_norm_(err, tola);
    const auto tup_r = this->compute_norm_(err, tolr);
    const auto tup_n = this->compute_norm_(err, tol);

    return {tup_n.norm, tup_a.norm, tup_r.norm, tup_n.loc, tup_a.loc, tup_r.loc};
  }
};

template <NormType wnormtype>
struct ErrorWNormETransform : ErrorWNormTransformBase<wnormtype> {
  using base_type     = ErrorWNormTransformBase<wnormtype>;
  using result_type   = typename base_type::result_type;
  using argument_type = thrust::tuple<PetscScalar, PetscScalar, PetscScalar, PetscScalar, PetscScalar>;

  using base_type::base_type;

  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL result_type operator()(const argument_type &x) const noexcept
  {
    const auto au    = PetscAbsScalar(thrust::get<0>(x));
    const auto ay    = PetscAbsScalar(thrust::get<1>(x));
    const auto skip  = au < this->ignore_max_ || ay < this->ignore_max_;
    const auto tola  = skip ? 0.0 : PetscRealPart(thrust::get<3>(x));
    const auto tolr  = skip ? 0.0 : PetscRealPart(thrust::get<4>(x)) * PetscMax(au, ay);
    const auto tol   = tola + tolr;
    const auto err   = PetscAbsScalar(thrust::get<2>(x));
    const auto tup_a = this->compute_norm_(err, tola);
    const auto tup_r = this->compute_norm_(err, tolr);
    const auto tup_n = this->compute_norm_(err, tol);

    return {tup_n.norm, tup_a.norm, tup_r.norm, tup_n.loc, tup_a.loc, tup_r.loc};
  }
};

template <NormType wnormtype>
struct ErrorWNormReduce {
  using value_type = typename ErrorWNormTransformBase<wnormtype>::result_type;

  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL value_type operator()(const value_type &lhs, const value_type &rhs) const noexcept
  {
    // cannot use lhs.get<0>() etc since the using decl above ambiguates the fact that
    // result_type is a template, so in order to fix this we would need to write:
    //
    // lhs.template get<0>()
    //
    // which is unseemly.
    if (wnormtype == NORM_INFINITY) {
      // clang-format off
      return {
        PetscMax(thrust::get<0>(lhs), thrust::get<0>(rhs)),
        PetscMax(thrust::get<1>(lhs), thrust::get<1>(rhs)),
        PetscMax(thrust::get<2>(lhs), thrust::get<2>(rhs)),
        thrust::get<3>(lhs) + thrust::get<3>(rhs),
        thrust::get<4>(lhs) + thrust::get<4>(rhs),
        thrust::get<5>(lhs) + thrust::get<5>(rhs)
      };
      // clang-format on
    } else {
      // clang-format off
      return {
        thrust::get<0>(lhs) + thrust::get<0>(rhs),
        thrust::get<1>(lhs) + thrust::get<1>(rhs),
        thrust::get<2>(lhs) + thrust::get<2>(rhs),
        thrust::get<3>(lhs) + thrust::get<3>(rhs),
        thrust::get<4>(lhs) + thrust::get<4>(rhs),
        thrust::get<5>(lhs) + thrust::get<5>(rhs)
      };
      // clang-format on
    }
  }
};

template <template <NormType> class WNormTransformType, typename Tuple, typename cupmStream_t>
inline PetscErrorCode ExecuteWNorm(Tuple &&first, Tuple &&last, NormType wnormtype, cupmStream_t stream, PetscReal ignore_max, PetscReal *norm, PetscInt *norm_loc, PetscReal *norma, PetscInt *norma_loc, PetscReal *normr, PetscInt *normr_loc) noexcept
{
  auto      begin = thrust::make_zip_iterator(std::forward<Tuple>(first));
  auto      end   = thrust::make_zip_iterator(std::forward<Tuple>(last));
  PetscReal n = 0, na = 0, nr = 0;
  PetscInt  n_loc = 0, na_loc = 0, nr_loc = 0;

  PetscFunctionBegin;
  // clang-format off
  if (wnormtype == NORM_INFINITY) {
    PetscCallThrust(
      thrust::tie(*norm, *norma, *normr, *norm_loc, *norma_loc, *normr_loc) = THRUST_CALL(
        thrust::transform_reduce,
        stream,
        std::move(begin),
        std::move(end),
        WNormTransformType<NORM_INFINITY>{ignore_max},
        thrust::make_tuple(n, na, nr, n_loc, na_loc, nr_loc),
        ErrorWNormReduce<NORM_INFINITY>{}
      )
    );
  } else {
    PetscCallThrust(
      thrust::tie(*norm, *norma, *normr, *norm_loc, *norma_loc, *normr_loc) = THRUST_CALL(
        thrust::transform_reduce,
        stream,
        std::move(begin),
        std::move(end),
        WNormTransformType<NORM_2>{ignore_max},
        thrust::make_tuple(n, na, nr, n_loc, na_loc, nr_loc),
        ErrorWNormReduce<NORM_2>{}
      )
    );
  }
  // clang-format on
  if (wnormtype == NORM_2) {
    *norm  = PetscSqrtReal(*norm);
    *norma = PetscSqrtReal(*norma);
    *normr = PetscSqrtReal(*normr);
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

} // namespace detail

// v->ops->errorwnorm
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::ErrorWnorm(Vec U, Vec Y, Vec E, NormType wnormtype, PetscReal atol, Vec vatol, PetscReal rtol, Vec vrtol, PetscReal ignore_max, PetscReal *norm, PetscInt *norm_loc, PetscReal *norma, PetscInt *norma_loc, PetscReal *normr, PetscInt *normr_loc) noexcept
{
  const auto         nl  = U->map->n;
  auto               ait = thrust::make_constant_iterator(static_cast<PetscScalar>(atol));
  auto               rit = thrust::make_constant_iterator(static_cast<PetscScalar>(rtol));
  PetscDeviceContext dctx;
  cupmStream_t       stream;

  PetscFunctionBegin;
  PetscCall(GetHandles_(&dctx, &stream));
  {
    const auto ConditionalDeviceArrayRead = [&](Vec v) {
      if (v) {
        return thrust::device_pointer_cast(DeviceArrayRead(dctx, v).data());
      } else {
        return thrust::device_ptr<PetscScalar>{nullptr};
      }
    };

    const auto uarr = DeviceArrayRead(dctx, U);
    const auto yarr = DeviceArrayRead(dctx, Y);
    const auto uptr = thrust::device_pointer_cast(uarr.data());
    const auto yptr = thrust::device_pointer_cast(yarr.data());
    const auto eptr = ConditionalDeviceArrayRead(E);
    const auto rptr = ConditionalDeviceArrayRead(vrtol);
    const auto aptr = ConditionalDeviceArrayRead(vatol);

    if (!vatol && !vrtol) {
      if (E) {
        // clang-format off
        PetscCall(
          detail::ExecuteWNorm<detail::ErrorWNormETransform>(
            thrust::make_tuple(uptr, yptr, eptr, ait, rit),
            thrust::make_tuple(uptr + nl, yptr + nl, eptr + nl, ait, rit),
            wnormtype, stream, ignore_max, norm, norm_loc, norma, norma_loc, normr, normr_loc
          )
        );
        // clang-format on
      } else {
        // clang-format off
        PetscCall(
          detail::ExecuteWNorm<detail::ErrorWNormTransform>(
            thrust::make_tuple(uptr, yptr, ait, rit),
            thrust::make_tuple(uptr + nl, yptr + nl, ait, rit),
            wnormtype, stream, ignore_max, norm, norm_loc, norma, norma_loc, normr, normr_loc
          )
        );
        // clang-format on
      }
    } else if (!vatol) {
      if (E) {
        // clang-format off
        PetscCall(
          detail::ExecuteWNorm<detail::ErrorWNormETransform>(
            thrust::make_tuple(uptr, yptr, eptr, ait, rptr),
            thrust::make_tuple(uptr + nl, yptr + nl, eptr + nl, ait, rptr + nl),
            wnormtype, stream, ignore_max, norm, norm_loc, norma, norma_loc, normr, normr_loc
          )
        );
        // clang-format on
      } else {
        // clang-format off
        PetscCall(
          detail::ExecuteWNorm<detail::ErrorWNormTransform>(
            thrust::make_tuple(uptr, yptr, ait, rptr),
            thrust::make_tuple(uptr + nl, yptr + nl, ait, rptr + nl),
            wnormtype, stream, ignore_max, norm, norm_loc, norma, norma_loc, normr, normr_loc
          )
        );
        // clang-format on
      }
    } else if (!vrtol) {
      if (E) {
        // clang-format off
          PetscCall(
            detail::ExecuteWNorm<detail::ErrorWNormETransform>(
              thrust::make_tuple(uptr, yptr, eptr, aptr, rit),
              thrust::make_tuple(uptr + nl, yptr + nl, eptr + nl, aptr + nl, rit),
              wnormtype, stream, ignore_max, norm, norm_loc, norma, norma_loc, normr, normr_loc
            )
          );
        // clang-format on
      } else {
        // clang-format off
          PetscCall(
            detail::ExecuteWNorm<detail::ErrorWNormTransform>(
              thrust::make_tuple(uptr, yptr, aptr, rit),
              thrust::make_tuple(uptr + nl, yptr + nl, aptr + nl, rit),
              wnormtype, stream, ignore_max, norm, norm_loc, norma, norma_loc, normr, normr_loc
            )
          );
        // clang-format on
      }
    } else {
      if (E) {
        // clang-format off
          PetscCall(
            detail::ExecuteWNorm<detail::ErrorWNormETransform>(
              thrust::make_tuple(uptr, yptr, eptr, aptr, rptr),
              thrust::make_tuple(uptr + nl, yptr + nl, eptr + nl, aptr + nl, rptr + nl),
              wnormtype, stream, ignore_max, norm, norm_loc, norma, norma_loc, normr, normr_loc
            )
          );
        // clang-format on
      } else {
        // clang-format off
          PetscCall(
            detail::ExecuteWNorm<detail::ErrorWNormTransform>(
              thrust::make_tuple(uptr, yptr, aptr, rptr),
              thrust::make_tuple(uptr + nl, yptr + nl, aptr + nl, rptr + nl),
              wnormtype, stream, ignore_max, norm, norm_loc, norma, norma_loc, normr, normr_loc
            )
          );
        // clang-format on
      }
    }
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{
struct dotnorm2_mult {
  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL thrust::tuple<PetscScalar, PetscScalar> operator()(const PetscScalar &s, const PetscScalar &t) const noexcept
  {
    const auto conjt = PetscConj(t);

    return {s * conjt, t * conjt};
  }
};

// it is positively __bananas__ that thrust does not define default operator+ for tuples... I
// would do it myself but now I am worried that they do so on purpose...
struct dotnorm2_tuple_plus {
  using value_type = thrust::tuple<PetscScalar, PetscScalar>;

  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL value_type operator()(const value_type &lhs, const value_type &rhs) const noexcept { return {thrust::get<0>(lhs) + thrust::get<0>(rhs), thrust::get<1>(lhs) + thrust::get<1>(rhs)}; }
};

} // namespace detail

// v->ops->dotnorm2
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::DotNorm2(Vec s, Vec t, PetscScalar *dp, PetscScalar *nm) noexcept
{
  PetscDeviceContext dctx;
  cupmStream_t       stream;

  PetscFunctionBegin;
  PetscCall(GetHandles_(&dctx, &stream));
  {
    PetscScalar dpt = 0.0, nmt = 0.0;
    const auto  sdptr = thrust::device_pointer_cast(DeviceArrayRead(dctx, s).data());

    // clang-format off
    PetscCallThrust(
      thrust::tie(*dp, *nm) = THRUST_CALL(
        thrust::inner_product,
        stream,
        sdptr, sdptr+s->map->n, thrust::device_pointer_cast(DeviceArrayRead(dctx, t).data()),
        thrust::make_tuple(dpt, nmt),
        detail::dotnorm2_tuple_plus{}, detail::dotnorm2_mult{}
      );
    );
    // clang-format on
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{
struct conjugate {
  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL PetscScalar operator()(const PetscScalar &x) const noexcept { return PetscConj(x); }
};

} // namespace detail

// v->ops->conjugate
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::ConjugateAsync(Vec xin, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  if (PetscDefined(USE_COMPLEX)) PetscCall(PointwiseUnary_(detail::conjugate{}, xin, nullptr, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->conjugate
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Conjugate(Vec xin) noexcept
{
  PetscFunctionBegin;
  PetscCall(ConjugateAsync(xin, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

namespace detail
{

struct real_part {
  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL thrust::tuple<PetscReal, PetscInt> operator()(const thrust::tuple<PetscScalar, PetscInt> &x) const noexcept { return {PetscRealPart(thrust::get<0>(x)), thrust::get<1>(x)}; }

  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL PetscReal operator()(const PetscScalar &x) const noexcept { return PetscRealPart(x); }
};

// deriving from Operator allows us to "store" an instance of the operator in the class but
// also take advantage of empty base class optimization if the operator is stateless
template <typename Operator>
class tuple_compare : Operator {
public:
  using tuple_type    = thrust::tuple<PetscReal, PetscInt>;
  using operator_type = Operator;

  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL tuple_type operator()(const tuple_type &x, const tuple_type &y) const noexcept
  {
    if (op_()(thrust::get<0>(y), thrust::get<0>(x))) {
      // if y is strictly greater/less than x, return y
      return y;
    } else if (thrust::get<0>(y) == thrust::get<0>(x)) {
      // if equal, prefer lower index
      return thrust::get<1>(y) < thrust::get<1>(x) ? y : x;
    }
    // otherwise return x
    return x;
  }

private:
  PETSC_NODISCARD PETSC_HOSTDEVICE_INLINE_DECL const operator_type &op_() const noexcept { return *this; }
};

} // namespace detail

template <device::cupm::DeviceType T>
template <typename TupleFuncT, typename UnaryFuncT>
inline PetscErrorCode VecSeq_CUPM<T>::MinMax_(TupleFuncT &&tuple_ftr, UnaryFuncT &&unary_ftr, Vec v, PetscInt *p, PetscReal *m) noexcept
{
  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQCUPM(), VECMPICUPM());
  if (p) *p = -1;
  if (const auto n = v->map->n) {
    PetscDeviceContext dctx;
    cupmStream_t       stream;

    PetscCall(GetHandles_(&dctx, &stream));
    // needed to:
    // 1. switch between transform_reduce and reduce
    // 2. strip the real_part functor from the arguments
#if PetscDefined(USE_COMPLEX)
  #define THRUST_MINMAX_REDUCE(...) THRUST_CALL(thrust::transform_reduce, __VA_ARGS__)
#else
  #define THRUST_MINMAX_REDUCE(s, b, e, real_part__, ...) THRUST_CALL(thrust::reduce, s, b, e, __VA_ARGS__)
#endif
    {
      const auto vptr = thrust::device_pointer_cast(DeviceArrayRead(dctx, v).data());

      if (p) {
        // clang-format off
        const auto zip = thrust::make_zip_iterator(
          thrust::make_tuple(std::move(vptr), thrust::make_counting_iterator(PetscInt{0}))
        );
        // clang-format on
        // need to use preprocessor conditionals since otherwise thrust complains about not being
        // able to convert a thrust::device_reference<PetscScalar> to a PetscReal on complex
        // builds...
        // clang-format off
        PetscCallThrust(
          thrust::tie(*m, *p) = THRUST_MINMAX_REDUCE(
            stream, zip, zip + n, detail::real_part{},
            thrust::make_tuple(*m, *p), std::forward<TupleFuncT>(tuple_ftr)
          );
        );
        // clang-format on
      } else {
        // clang-format off
        PetscCallThrust(
          *m = THRUST_MINMAX_REDUCE(
            stream, vptr, vptr + n, detail::real_part{},
            *m, std::forward<UnaryFuncT>(unary_ftr)
          );
        );
        // clang-format on
      }
    }
#undef THRUST_MINMAX_REDUCE
  }
  // REVIEW ME: flops?
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->max
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Max(Vec v, PetscInt *p, PetscReal *m) noexcept
{
  using tuple_functor = detail::tuple_compare<thrust::greater<PetscReal>>;
  using unary_functor = thrust::maximum<PetscReal>;

  PetscFunctionBegin;
  *m = PETSC_MIN_REAL;
  // use {} constructor syntax otherwise most vexing parse
  PetscCall(MinMax_(tuple_functor{}, unary_functor{}, v, p, m));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->min
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Min(Vec v, PetscInt *p, PetscReal *m) noexcept
{
  using tuple_functor = detail::tuple_compare<thrust::less<PetscReal>>;
  using unary_functor = thrust::minimum<PetscReal>;

  PetscFunctionBegin;
  *m = PETSC_MAX_REAL;
  // use {} constructor syntax otherwise most vexing parse
  PetscCall(MinMax_(tuple_functor{}, unary_functor{}, v, p, m));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->sum
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Sum(Vec v, PetscScalar *sum) noexcept
{
  PetscFunctionBegin;
  if (const auto n = v->map->n) {
    PetscDeviceContext dctx;
    cupmStream_t       stream;

    PetscCall(GetHandles_(&dctx, &stream));
    const auto dptr = thrust::device_pointer_cast(DeviceArrayRead(dctx, v).data());
    // REVIEW ME: why not cupmBlasXasum()?
    PetscCallThrust(*sum = THRUST_CALL(thrust::reduce, stream, dptr, dptr + n, PetscScalar{0.0}););
    // REVIEW ME: must be at least n additions
    PetscCall(PetscLogGpuFlops(n));
  } else {
    *sum = 0.0;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::ShiftAsync(Vec v, PetscScalar shift, PetscDeviceContext dctx) noexcept
{
  PetscFunctionBegin;
  PetscCall(PointwiseUnary_(device::cupm::functors::make_plus_equals(shift), v, nullptr, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::Shift(Vec v, PetscScalar shift) noexcept
{
  PetscFunctionBegin;
  PetscCall(ShiftAsync(v, shift, nullptr));
  PetscFunctionReturn(PETSC_SUCCESS);
}

template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::SetRandom(Vec v, PetscRandom rand) noexcept
{
  PetscFunctionBegin;
  if (const auto n = v->map->n) {
    PetscBool          iscurand;
    PetscDeviceContext dctx;

    PetscCall(GetHandles_(&dctx));
    PetscCall(PetscObjectTypeCompare(PetscObjectCast(rand), PETSCCURAND, &iscurand));
    if (iscurand) PetscCall(PetscRandomGetValues(rand, n, DeviceArrayWrite(dctx, v)));
    else PetscCall(PetscRandomGetValues(rand, n, HostArrayWrite(dctx, v)));
  } else {
    PetscCall(MaybeIncrementEmptyLocalVec(v));
  }
  // REVIEW ME: flops????
  // REVIEW ME: Timing???
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->setpreallocation
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::SetPreallocationCOO(Vec v, PetscCount ncoo, const PetscInt coo_i[]) noexcept
{
  PetscDeviceContext dctx;

  PetscFunctionBegin;
  PetscCall(GetHandles_(&dctx));
  PetscCall(VecSetPreallocationCOO_Seq(v, ncoo, coo_i));
  PetscCall(SetPreallocationCOO_CUPMBase(v, ncoo, coo_i, dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

// v->ops->setvaluescoo
template <device::cupm::DeviceType T>
inline PetscErrorCode VecSeq_CUPM<T>::SetValuesCOO(Vec x, const PetscScalar v[], InsertMode imode) noexcept
{
  auto               vv = const_cast<PetscScalar *>(v);
  PetscMemType       memtype;
  PetscDeviceContext dctx;
  cupmStream_t       stream;

  PetscFunctionBegin;
  PetscCall(GetHandles_(&dctx, &stream));
  PetscCall(PetscGetMemType(v, &memtype));
  if (PetscMemTypeHost(memtype)) {
    const auto size = VecIMPLCast(x)->coo_n;

    // If user gave v[] in host, we might need to copy it to device if any
    PetscCall(PetscDeviceMalloc(dctx, PETSC_MEMTYPE_CUPM(), size, &vv));
    PetscCall(PetscCUPMMemcpyAsync(vv, v, size, cupmMemcpyHostToDevice, stream));
  }

  if (const auto n = x->map->n) {
    const auto vcu = VecCUPMCast(x);

    PetscCall(PetscCUPMLaunchKernel1D(n, 0, stream, kernels::add_coo_values, vv, n, vcu->jmap1_d, vcu->perm1_d, imode, imode == INSERT_VALUES ? DeviceArrayWrite(dctx, x).data() : DeviceArrayReadWrite(dctx, x).data()));
  } else {
    PetscCall(MaybeIncrementEmptyLocalVec(x));
  }

  if (PetscMemTypeHost(memtype)) PetscCall(PetscDeviceFree(dctx, vv));
  PetscCall(PetscDeviceContextSynchronize(dctx));
  PetscFunctionReturn(PETSC_SUCCESS);
}

} // namespace impl

} // namespace cupm

} // namespace vec

} // namespace Petsc
