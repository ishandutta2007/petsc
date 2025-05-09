#include <petsc/private/dmdaimpl.h> /*I   "petscdmda.h"   I*/

/*@C
  DMDAVecGetArray - Returns a multiple dimension array that shares data with
  the underlying vector and is indexed using the global or local dimensions of a `DMDA`.

  Logically Collective

  Input Parameters:
+ da  - the `DMDA`
- vec - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`

  Output Parameter:
. array - the array

  Level: intermediate

  Notes:
  Call `DMDAVecRestoreArray()` once you have finished accessing the vector entries.

  In C, the indexing is "backwards" from what expects: array[k][j][i] NOT array[i][j][k]!

  If `vec` is a local vector (obtained with DMCreateLocalVector() etc) then the ghost point locations are accessible. If it is
  a global vector then the ghost points are not accessible. Of course, with a local vector you will have had to do the
  appropriate `DMGlobalToLocalBegin()` and `DMGlobalToLocalEnd()` to have correct values in the ghost locations.

  The accessible indices are `array[zs:zs+zm-1][ys:ys+ym-1][xs:xs+xm-1]` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

  Fortran Notes:
  Use `DMDAVecGetArray()` and pass for the array type `PetscScalar`,pointer :: array(:,...,:) of the appropriate
  dimension. For a `DMDA` created with a dof of 1 use the dimension of the `DMDA`, for a `DMDA` created with a dof greater than 1 use one more than the
  dimension of the `DMDA`.

  The order of the indices is `array(xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` (when dof is 1) otherwise
  `array(0:dof-1,xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecRestoreArray()`, `DMDAVecRestoreArrayDOF()`
          `DMDAVecGetArrayDOF()`, `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`, `DMDAVecGetArrayRead()`, `DMDAVecRestoreArrayRead()`,
          `DMStagVecGetArray()`
@*/
PetscErrorCode DMDAVecGetArray(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(da, DM_CLASSID, 1, DMDA);
  PetscValidHeaderSpecific(vec, VEC_CLASSID, 2);
  PetscAssertPointer(array, 3);
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecGetArray1d(vec, gxm * dof, gxs * dof, (PetscScalar **)array));
  } else if (dim == 2) {
    PetscCall(VecGetArray2d(vec, gym, gxm * dof, gys, gxs * dof, (PetscScalar ***)array));
  } else if (dim == 3) {
    PetscCall(VecGetArray3d(vec, gzm, gym, gxm * dof, gzs, gys, gxs * dof, (PetscScalar ****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMDAVecRestoreArray - Restores a multiple dimension array obtained with `DMDAVecGetArray()`

  Logically Collective

  Input Parameters:
+ da    - the `DMDA`
. vec   - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`
- array - the `array` pointer

  Level: intermediate

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecGetArray()`,
          `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`, `DMDAVecGetArrayRead()`, `DMDAVecRestoreArrayRead()`,
          `DMStagVecRestoreArray()`
@*/
PetscErrorCode DMDAVecRestoreArray(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(da, DM_CLASSID, 1, DMDA);
  PetscValidHeaderSpecific(vec, VEC_CLASSID, 2);
  PetscAssertPointer(array, 3);
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecRestoreArray1d(vec, gxm * dof, gxs * dof, (PetscScalar **)array));
  } else if (dim == 2) {
    PetscCall(VecRestoreArray2d(vec, gym, gxm * dof, gys, gxs * dof, (PetscScalar ***)array));
  } else if (dim == 3) {
    PetscCall(VecRestoreArray3d(vec, gzm, gym, gxm * dof, gzs, gys, gxs * dof, (PetscScalar ****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMDAVecGetArrayWrite - Returns a multiple dimension array that shares data with
  the underlying vector and is indexed using the global or local dimensions of a `DMDA`.

  Logically Collective

  Input Parameters:
+ da  - the `DMDA`
- vec - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`

  Output Parameter:
. array - the array

  Level: intermediate

  Notes:
  Call `DMDAVecRestoreArray()` once you have finished accessing the vector entries.

  In C, the indexing is "backwards" from what expects: array[k][j][i] NOT array[i][j][k]!

  if `vec` is a local vector (obtained with `DMCreateLocalVector()` etc) then the ghost point locations are accessible. If it is
  a global vector then the ghost points are not accessible. Of course with the local vector you will have had to do the
  appropriate `DMGlobalToLocalBegin()` and `DMGlobalToLocalEnd()` to have correct values in the ghost locations.

  The accessible indices are `array[zs:zs+zm-1][ys:ys+ym-1][xs:xs+xm-1]` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

  Fortran Notes:
  Use `DMDAVecGetArrayWrite()` and pass for the array type PetscScalar,pointer :: array(:,...,:) of the appropriate
  dimension. For a `DMDA` created with a dof of 1 use the dimension of the `DMDA`, for a `DMDA` created with a dof greater than 1 use one more than the
  dimension of the `DMDA`.

  The order of the indices is `array(xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` (when dof is 1) otherwise
  `array(0:dof-1,xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

  Developer Note:
  This has code duplication with `DMDAVecGetArray()` and `DMDAVecGetArrayRead()`

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecRestoreArrayWrite()`, `DMDAVecRestoreArrayDOF()`
          `DMDAVecGetArrayDOF()`, `DMDAVecGetArray()`, `DMDAVecRestoreArray()`, `DMDAVecGetArrayRead()`, `DMDAVecRestoreArrayRead()`
@*/
PetscErrorCode DMDAVecGetArrayWrite(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(da, DM_CLASSID, 1, DMDA);
  PetscValidHeaderSpecific(vec, VEC_CLASSID, 2);
  PetscAssertPointer(array, 3);
  if (da->localSection) {
    PetscCall(VecGetArrayWrite(vec, (PetscScalar **)array));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecGetArray1dWrite(vec, gxm * dof, gxs * dof, (PetscScalar **)array));
  } else if (dim == 2) {
    PetscCall(VecGetArray2dWrite(vec, gym, gxm * dof, gys, gxs * dof, (PetscScalar ***)array));
  } else if (dim == 3) {
    PetscCall(VecGetArray3dWrite(vec, gzm, gym, gxm * dof, gzs, gys, gxs * dof, (PetscScalar ****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMDAVecRestoreArrayWrite - Restores a multiple dimension array obtained with `DMDAVecGetArrayWrite()`

  Logically Collective

  Input Parameters:
+ da    - the `DMDA`
. vec   - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`
- array - the `array` pointer

  Level: intermediate

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecGetArrayWrite()`,
          `DMDAVecGetArray()`, `DMDAVecRestoreArray()`, `DMDAVecGetArrayRead()`, `DMDAVecRestoreArrayRead()`
@*/
PetscErrorCode DMDAVecRestoreArrayWrite(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(da, DM_CLASSID, 1, DMDA);
  PetscValidHeaderSpecific(vec, VEC_CLASSID, 2);
  PetscAssertPointer(array, 3);
  if (da->localSection) {
    PetscCall(VecRestoreArray(vec, (PetscScalar **)array));
    PetscFunctionReturn(PETSC_SUCCESS);
  }
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecRestoreArray1dWrite(vec, gxm * dof, gxs * dof, (PetscScalar **)array));
  } else if (dim == 2) {
    PetscCall(VecRestoreArray2dWrite(vec, gym, gxm * dof, gys, gxs * dof, (PetscScalar ***)array));
  } else if (dim == 3) {
    PetscCall(VecRestoreArray3dWrite(vec, gzm, gym, gxm * dof, gzs, gys, gxs * dof, (PetscScalar ****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMDAVecGetArrayDOF - Returns a multiple dimension array that shares data with
  the underlying vector and is indexed using the global or local dimensions of a `DMDA`

  Logically Collective

  Input Parameters:
+ da  - the `DMDA`
- vec - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`

  Output Parameter:
. array - the `array` pointer

  Level: intermediate

  Notes:
  Call `DMDAVecRestoreArrayDOF()` once you have finished accessing the vector entries.

  In C, the indexing is "backwards" from what expects: array[k][j][i][DOF] NOT array[i][j][k][DOF]

  The accessible indices are `array[zs:zs+zm-1][ys:ys+ym-1][xs:xs+xm-1][0:ndof-1]` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

  Fortran Notes:
  Use `DMDAVecGetArray()` and pass for the array type PetscScalar,pointer :: array(:,...,:) of the appropriate
  dimension. For a `DMDA` created with a dof of 1 use the dimension of the `DMDA`, for a `DMDA` created with a dof greater than 1 use one more than the
  dimension of the `DMDA`.

  The order of the indices is `array(xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` (when ndof is 1) otherwise
  `array(0:dof-1,xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecRestoreArray()`, `DMDAVecGetArray()`, `DMDAVecRestoreArrayDOF()`,
          `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`, `DMDAVecGetArrayRead()`, `DMDAVecRestoreArrayRead()`, `DMDAVecGetArrayDOFRead()`
@*/
PetscErrorCode DMDAVecGetArrayDOF(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecGetArray2d(vec, gxm, dof, gxs, 0, (PetscScalar ***)array));
  } else if (dim == 2) {
    PetscCall(VecGetArray3d(vec, gym, gxm, dof, gys, gxs, 0, (PetscScalar ****)array));
  } else if (dim == 3) {
    PetscCall(VecGetArray4d(vec, gzm, gym, gxm, dof, gzs, gys, gxs, 0, (PetscScalar *****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMDAVecRestoreArrayDOF - Restores a multiple dimension array obtained with `DMDAVecGetArrayDOF()`

  Logically Collective

  Input Parameters:
+ da    - the `DMDA`
. vec   - vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`
- array - the `array` point

  Level: intermediate

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecGetArray()`, `DMDAVecGetArrayDOF()`,
          `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`, `DMDAVecGetArrayRead()`, `DMDAVecRestoreArrayRead()`
@*/
PetscErrorCode DMDAVecRestoreArrayDOF(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  }

  if (dim == 1) {
    PetscCall(VecRestoreArray2d(vec, gxm, dof, gxs, 0, (PetscScalar ***)array));
  } else if (dim == 2) {
    PetscCall(VecRestoreArray3d(vec, gym, gxm, dof, gys, gxs, 0, (PetscScalar ****)array));
  } else if (dim == 3) {
    PetscCall(VecRestoreArray4d(vec, gzm, gym, gxm, dof, gzs, gys, gxs, 0, (PetscScalar *****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMDAVecGetArrayRead - Returns a multiple dimension array that shares data with
  the underlying vector and is indexed using the global or local dimensions of a `DMDA`.

  Not Collective

  Input Parameters:
+ da  - the `DMDA`
- vec - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`

  Output Parameter:
. array - the array

  Level: intermediate

  Notes:
  Call `DMDAVecRestoreArrayRead()` once you have finished accessing the vector entries.

  In C, the indexing is "backwards" from what expects: array[k][j][i] NOT array[i][j][k]!

  If `vec` is a local vector (obtained with `DMCreateLocalVector()` etc) then the ghost point locations are accessible. If it is
  a global vector then the ghost points are not accessible. Of course with the local vector you will have had to do the
  appropriate `DMGlobalToLocalBegin()` and `DMGlobalToLocalEnd()` to have correct values in the ghost locations.

  The accessible indices are `array[zs:zs+zm-1][ys:ys+ym-1][xs:xs+xm-1]` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

  Fortran Notes:
  Use `DMDAVecGetArrayRead()` and pass for the array type `PetscScalar`,pointer :: array(:,...,:) of the appropriate
  dimension. For a `DMDA` created with a dof of 1 use the dimension of the `DMDA`, for a `DMDA` created with a dof greater than 1 use one more than the
  dimension of the `DMDA`.

  The order of the indices is `array(xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` (when dof is 1) otherwise
  `array(0:dof-1,xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`,
`DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecRestoreArrayRead()`,
`DMDAVecRestoreArrayDOF()`, `DMDAVecGetArrayDOF()`, `DMDAVecGetArray()`,
`DMDAVecRestoreArray()`, `DMStagVecGetArrayRead()`
@*/
PetscErrorCode DMDAVecGetArrayRead(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(da, DM_CLASSID, 1, DMDA);
  PetscValidHeaderSpecific(vec, VEC_CLASSID, 2);
  PetscAssertPointer(array, 3);
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecGetArray1dRead(vec, gxm * dof, gxs * dof, (PetscScalar **)array));
  } else if (dim == 2) {
    PetscCall(VecGetArray2dRead(vec, gym, gxm * dof, gys, gxs * dof, (PetscScalar ***)array));
  } else if (dim == 3) {
    PetscCall(VecGetArray3dRead(vec, gzm, gym, gxm * dof, gzs, gys, gxs * dof, (PetscScalar ****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMDAVecRestoreArrayRead - Restores a multiple dimension array obtained with `DMDAVecGetArrayRead()`

  Not Collective

  Input Parameters:
+ da    - the `DMDA`
. vec   - vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`
- array - the `array` pointer

  Level: intermediate

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecGetArrayRead()`,
          `DMDAVecGetArray()`, `DMDAVecRestoreArray()`, `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`,
          `DMStagVecRestoreArrayRead()`
@*/
PetscErrorCode DMDAVecRestoreArrayRead(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscValidHeaderSpecificType(da, DM_CLASSID, 1, DMDA);
  PetscValidHeaderSpecific(vec, VEC_CLASSID, 2);
  PetscAssertPointer(array, 3);
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecRestoreArray1dRead(vec, gxm * dof, gxs * dof, (PetscScalar **)array));
  } else if (dim == 2) {
    PetscCall(VecRestoreArray2dRead(vec, gym, gxm * dof, gys, gxs * dof, (PetscScalar ***)array));
  } else if (dim == 3) {
    PetscCall(VecRestoreArray3dRead(vec, gzm, gym, gxm * dof, gzs, gys, gxs * dof, (PetscScalar ****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMDAVecGetArrayDOFRead - Returns a multiple dimension array that shares data with
  the underlying vector and is indexed using the global or local dimensions of a `DMDA`

  Not Collective

  Input Parameters:
+ da  - the `DMDA`
- vec - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`

  Output Parameter:
. array - the array

  Level: intermediate

  Notes:
  Call `DMDAVecRestoreArrayDOFRead()` once you have finished accessing the vector entries.

  In C, the indexing is "backwards" from what expects: array[k][j][i][DOF] NOT array[i][j][k][DOF]!

  The accessible indices are `array[zs:zs+zm-1][ys:ys+ym-1][xs:xs+xm-1]` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

  Fortran Notes:
  Use  `DMDAVecGetArrayRead()` and pass for the array type PetscScalar,pointer :: array(:,...,:) of the appropriate
  dimension. For a `DMDA` created with a dof of 1 use the dimension of the `DMDA`, for a `DMDA` created with a dof greater than 1 use one more than the
  dimension of the `DMDA`.

  The order of the indices is `array(xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` (when dof is 1) otherwise
  `array(0:dof-1,xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecRestoreArray()`, `DMDAVecGetArray()`, `DMDAVecGetArrayDOF()`,
          `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`, `DMDAVecGetArrayRead()`, `DMDAVecRestoreArrayRead()`
@*/
PetscErrorCode DMDAVecGetArrayDOFRead(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecGetArray2dRead(vec, gxm, dof, gxs, 0, (PetscScalar ***)array));
  } else if (dim == 2) {
    PetscCall(VecGetArray3dRead(vec, gym, gxm, dof, gys, gxs, 0, (PetscScalar ****)array));
  } else if (dim == 3) {
    PetscCall(VecGetArray4dRead(vec, gzm, gym, gxm, dof, gzs, gys, gxs, 0, (PetscScalar *****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMDAVecRestoreArrayDOFRead - Restores a multiple dimension array obtained with `DMDAVecGetArrayDOFRead()`

  Not Collective

  Input Parameters:
+ da    - the `DMDA`
. vec   - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`
- array - the `array` pointer

  Level: intermediate

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecGetArray()`, `DMDAVecGetArrayDOF()`, `DMDAVecRestoreArrayDOF()`,
          `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`, `DMDAVecGetArrayRead()`, `DMDAVecRestoreArrayRead()`
@*/
PetscErrorCode DMDAVecRestoreArrayDOFRead(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  }

  if (dim == 1) {
    PetscCall(VecRestoreArray2dRead(vec, gxm, dof, gxs, 0, (PetscScalar ***)array));
  } else if (dim == 2) {
    PetscCall(VecRestoreArray3dRead(vec, gym, gxm, dof, gys, gxs, 0, (PetscScalar ****)array));
  } else if (dim == 3) {
    PetscCall(VecRestoreArray4dRead(vec, gzm, gym, gxm, dof, gzs, gys, gxs, 0, (PetscScalar *****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  DMDAVecGetArrayDOFWrite - Returns a multiple dimension array that shares data with
  the underlying vector and is indexed using the global or local dimensions of a `DMDA`

  Not Collective

  Input Parameters:
+ da  - the `DMDA`
- vec - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`

  Output Parameter:
. array - the array

  Level: intermediate

  Notes:
  Call `DMDAVecRestoreArrayDOFWrite()` once you have finished accessing the vector entries.

  In C, the indexing is "backwards" from what expects: array[k][j][i][DOF] NOT array[i][j][k][DOF]!

  The accessible indices are `array[zs:zs+zm-1][ys:ys+ym-1][xs:xs+xm-1][0:dof-1]` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

  Fortran Notes:
  Use  `DMDAVecGetArrayWrite()` and pass for the array type PetscScalar,pointer :: array(:,...,:) of the appropriate
  dimension. For a `DMDA` created with a dof of 1 use the dimension of the `DMDA`, for a `DMDA` created with a dof greater than 1 use one more than the
  dimension of the `DMDA`.

  The order of the indices is `array(xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` (when dof is 1) otherwise
  `array(0:dof-1,xs:xs+xm-1,ys:ys+ym-1,zs:zs+zm-1)` where the values are obtained from
  `DMDAGetCorners()` for a global vector or `DMDAGetGhostCorners()` for a local vector.

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecRestoreArray()`, `DMDAVecGetArray()`, `DMDAVecGetArrayDOF()`,
          `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`
@*/
PetscErrorCode DMDAVecGetArrayDOFWrite(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  } else PetscCheck(N == gxm * gym * gzm * dof, PETSC_COMM_SELF, PETSC_ERR_ARG_INCOMP, "Vector local size %" PetscInt_FMT " is not compatible with DMDA local sizes %" PetscInt_FMT " %" PetscInt_FMT, N, xm * ym * zm * dof, gxm * gym * gzm * dof);

  if (dim == 1) {
    PetscCall(VecGetArray2dWrite(vec, gxm, dof, gxs, 0, (PetscScalar ***)array));
  } else if (dim == 2) {
    PetscCall(VecGetArray3dWrite(vec, gym, gxm, dof, gys, gxs, 0, (PetscScalar ****)array));
  } else if (dim == 3) {
    PetscCall(VecGetArray4dWrite(vec, gzm, gym, gxm, dof, gzs, gys, gxs, 0, (PetscScalar *****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  DMDAVecRestoreArrayDOFWrite - Restores a multiple dimension array obtained with `DMDAVecGetArrayDOFWrite()`

  Not Collective

  Input Parameters:
+ da    - the `DMDA`
. vec   - a vector the same size as one obtained with `DMCreateGlobalVector()` or `DMCreateLocalVector()`
- array - the `array` pointer

  Level: intermediate

.seealso: [](sec_struct), [](sec_struct_set), `DM`, `DMDA`, `DMDAGetGhostCorners()`, `DMDAGetCorners()`, `VecGetArray()`, `VecRestoreArray()`, `DMDAVecGetArray()`, `DMDAVecGetArrayDOF()`, `DMDAVecRestoreArrayDOF()`,
          `DMDAVecGetArrayWrite()`, `DMDAVecRestoreArrayWrite()`
@*/
PetscErrorCode DMDAVecRestoreArrayDOFWrite(DM da, Vec vec, void *array)
{
  PetscInt xs, ys, zs, xm, ym, zm, gxs, gys, gzs, gxm, gym, gzm, N, dim, dof;

  PetscFunctionBegin;
  PetscCall(DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm));
  PetscCall(DMDAGetGhostCorners(da, &gxs, &gys, &gzs, &gxm, &gym, &gzm));
  PetscCall(DMDAGetInfo(da, &dim, NULL, NULL, NULL, NULL, NULL, NULL, &dof, NULL, NULL, NULL, NULL, NULL));

  /* Handle case where user passes in global vector as opposed to local */
  PetscCall(VecGetLocalSize(vec, &N));
  if (N == xm * ym * zm * dof) {
    gxm = xm;
    gym = ym;
    gzm = zm;
    gxs = xs;
    gys = ys;
    gzs = zs;
  }

  if (dim == 1) {
    PetscCall(VecRestoreArray2dWrite(vec, gxm, dof, gxs, 0, (PetscScalar ***)array));
  } else if (dim == 2) {
    PetscCall(VecRestoreArray3dWrite(vec, gym, gxm, dof, gys, gxs, 0, (PetscScalar ****)array));
  } else if (dim == 3) {
    PetscCall(VecRestoreArray4dWrite(vec, gzm, gym, gxm, dof, gzs, gys, gxs, 0, (PetscScalar *****)array));
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_CORRUPT, "DMDA dimension not 1, 2, or 3, it is %" PetscInt_FMT, dim);
  PetscFunctionReturn(PETSC_SUCCESS);
}
