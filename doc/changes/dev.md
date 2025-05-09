# Changes: Development

% STYLE GUIDELINES:
% * Capitalize sentences
% * Use imperative, e.g., Add, Improve, Change, etc.
% * Don't use a period (.) at the end of entries
% * If multiple sentences are needed, use a period or semicolon to divide sentences, but not at the end of the final sentence

```{rubric} General:
```

```{rubric} Configure/Build:
```

```{rubric} Sys:
```

- Deprecate `PetscSSEIsEnabled()`

```{rubric} Event Logging:
```

```{rubric} PetscViewer:
```

- Add `PetscViewerHDF5SetCompress()` and `PetscViewerHDF5GetCompress()`

```{rubric} PetscDraw:
```

```{rubric} AO:
```

```{rubric} IS:
```

```{rubric} VecScatter / PetscSF:
```

```{rubric} PF:
```

```{rubric} Vec:
```

```{rubric} PetscSection:
```

```{rubric} PetscPartitioner:
```

```{rubric} Mat:
```

- Add `MatConstantDiagonalGetConstant()`

```{rubric} MatCoarsen:
```

```{rubric} PC:
```

- Add `PCMatApplyTranspose()`
- Remove `PC_ApplyMultiple`

```{rubric} KSP:
```

- Add `MatLMVMGetLastUpdate()`
- Add `MatLMVMMultAlgorithm`, `MatLMVMSetMultAlgorithm()`, and `MatLMVMGetMultAlgorithm()`
- Add `MatLMVMSymBroydenGetPhi()` and `MatLMVMSymBroydenSetPhi()`
- Add `MatLMVMSymBadBroydenGetPsi()` and `MatLMVMSymBadBroydenSetPsi()`
- Deprecate `KSP_CONVERGED_RTOL_NORMAL` in favor of `KSP_CONVERGED_RTOL_NORMAL_EQUATIONS` and `KSP_CONVERGED_ATOL_NORMAL` in favor of `KSP_CONVERGED_ATOL_NORMAL_EQUATIONS`

```{rubric} SNES:
```

```{rubric} SNESLineSearch:
```

```{rubric} TS:
```

```{rubric} TAO:
```

- Add ``TaoBRGNSetRegularizationType()``, ``TaoBRGNGetRegularizationType()``

```{rubric} PetscRegressor:
```

- Add new component to support regression and classification machine learning tasks: [](ch_regressor)
- Add `PetscRegressor` type `PETSCREGRESSORLINEAR` for solving linear regression problems with optional regularization

```{rubric} DM/DA:
```

```{rubric} DMSwarm:
```

```{rubric} DMPlex:
```

- Add `DMPlexGetTransform()`, `DMPlexSetTransform()`, `DMPlexGetSaveTransform()`, and `DMPlexSetSaveTransform()`
- Add `DMPlexGetCoordinateMap()` and `DMPlexSetCoordinateMap()`
- Add `DMPlexTransformCohesiveExtrudeGetUnsplit()`
- Add `DMFieldCreateDefaultFaceQuadrature()`
- Rename `DMPlexComputeResidual_Internal()` to `DMPlexComputeResidualForKey()`
- Rename `DMPlexComputeJacobian_Internal()` to `DMPlexComputeJacobianByKey()`
- Rename `DMPlexComputeJacobian_Action_Internal()` to `DMPlexComputeJacobianActionByKey()`
- Rename `DMPlexComputeResidual_Hybrid_Internal()` to `DMPlexComputeResidualHybridByKey()`
- Rename `DMPlexComputeJacobian_Hybrid_Internal()` to `DMPlexComputeJacobianHybridByKey()`


```{rubric} FE/FV:
```

- Add `PetscFEExpandFaceQuadrature()`

```{rubric} DMNetwork:
```

```{rubric} DMStag:
```

```{rubric} DT:
```

```{rubric} Fortran:
```
