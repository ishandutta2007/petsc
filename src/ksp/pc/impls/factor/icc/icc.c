/*
   Defines a Cholesky factorization preconditioner for any Mat implementation.
  Presently only provided for MPIRowbs format (i.e. BlockSolve).
*/

#include "src/ksp/pc/impls/factor/icc/icc.h"   /*I "petscpc.h" I*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCFactorSetZeroPivot_ICC"
PetscErrorCode PCFactorSetZeroPivot_ICC(PC pc,PetscReal z)
{
  PC_ICC *icc;

  PetscFunctionBegin;
  icc                 = (PC_ICC*)pc->data;
  icc->info.zeropivot = z;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCFactorSetShiftNonzero_ICC"
PetscErrorCode PCFactorSetShiftNonzero_ICC(PC pc,PetscReal shift)
{
  PC_ICC *dir;

  PetscFunctionBegin;
  dir = (PC_ICC*)pc->data;
  if (shift == (PetscReal) PETSC_DECIDE) {
    dir->info.shiftnz = 1.e-12;
  } else {
    dir->info.shiftnz = shift;
  }
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCFactorSetShiftPd_ICC"
PetscErrorCode PCFactorSetShiftPd_ICC(PC pc,PetscTruth shift)
{
  PC_ICC *dir;
 
  PetscFunctionBegin;
  dir = (PC_ICC*)pc->data;
  dir->info.shiftpd = shift;
  if (shift) dir->info.shift_fraction = 0.0;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCICCSetMatOrdering_ICC"
PetscErrorCode PCICCSetMatOrdering_ICC(PC pc,MatOrderingType ordering)
{
  PC_ICC         *dir = (PC_ICC*)pc->data;
  PetscErrorCode ierr;
 
  PetscFunctionBegin;
  ierr = PetscStrfree(dir->ordering);CHKERRQ(ierr);
  ierr = PetscStrallocpy(ordering,&dir->ordering);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCICCSetFill_ICC"
PetscErrorCode PCICCSetFill_ICC(PC pc,PetscReal fill)
{
  PC_ICC *dir;

  PetscFunctionBegin;
  dir            = (PC_ICC*)pc->data;
  dir->info.fill = fill;
  PetscFunctionReturn(0);
}
EXTERN_C_END

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCICCSetLevels_ICC"
PetscErrorCode PCICCSetLevels_ICC(PC pc,PetscInt levels)
{
  PC_ICC *icc;

  PetscFunctionBegin;
  icc = (PC_ICC*)pc->data;
  icc->info.levels = levels;
  PetscFunctionReturn(0);
}
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "PCICCSetMatOrdering"
/*@
    PCICCSetMatOrdering - Sets the ordering routine (to reduce fill) to 
    be used it the ICC factorization.

    Collective on PC

    Input Parameters:
+   pc - the preconditioner context
-   ordering - the matrix ordering name, for example, MATORDERING_ND or MATORDERING_RCM

    Options Database Key:
.   -pc_icc_mat_ordering_type <nd,rcm,...> - Sets ordering routine

    Level: intermediate

.seealso: PCLUSetMatOrdering()

.keywords: PC, ICC, set, matrix, reordering

@*/
PetscErrorCode PCICCSetMatOrdering(PC pc,MatOrderingType ordering)
{
  PetscErrorCode ierr,(*f)(PC,MatOrderingType);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCICCSetMatOrdering_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,ordering);CHKERRQ(ierr);
  } 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCICCSetLevels"
/*@
   PCICCSetLevels - Sets the number of levels of fill to use.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  levels - number of levels of fill

   Options Database Key:
.  -pc_icc_levels <levels> - Sets fill level

   Level: intermediate

   Concepts: ICC^setting levels of fill

@*/
PetscErrorCode PCICCSetLevels(PC pc,PetscInt levels)
{
  PetscErrorCode ierr,(*f)(PC,PetscInt);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  if (levels < 0) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,"negative levels");
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCICCSetLevels_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,levels);CHKERRQ(ierr);
  } 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCICCSetFill"
/*@
   PCICCSetFill - Indicate the amount of fill you expect in the factored matrix,
   where fill = number nonzeros in factor/number nonzeros in original matrix.

   Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  fill - amount of expected fill

   Options Database Key:
$  -pc_icc_fill <fill>

   Note:
   For sparse matrix factorizations it is difficult to predict how much 
   fill to expect. By running with the option -log_info PETSc will print the 
   actual amount of fill used; allowing you to set the value accurately for
   future runs. But default PETSc uses a value of 1.0

   Level: intermediate

.keywords: PC, set, factorization, direct, fill

.seealso: PCLUSetFill()
@*/
PetscErrorCode PCICCSetFill(PC pc,PetscReal fill)
{
  PetscErrorCode ierr,(*f)(PC,PetscReal);

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_COOKIE,1);
  if (fill < 1.0) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,"Fill factor cannot be less than 1.0");
  ierr = PetscObjectQueryFunction((PetscObject)pc,"PCICCSetFill_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(pc,fill);CHKERRQ(ierr);
  } 
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCSetup_ICC"
static PetscErrorCode PCSetup_ICC(PC pc)
{
  PC_ICC         *icc = (PC_ICC*)pc->data;
  IS             perm,cperm;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatGetOrdering(pc->pmat,icc->ordering,&perm,&cperm);CHKERRQ(ierr);

  if (!pc->setupcalled) {
    ierr = MatICCFactorSymbolic(pc->pmat,perm,&icc->info,&icc->fact);CHKERRQ(ierr);
  } else if (pc->flag != SAME_NONZERO_PATTERN) {
    ierr = MatDestroy(icc->fact);CHKERRQ(ierr);
    ierr = MatICCFactorSymbolic(pc->pmat,perm,&icc->info,&icc->fact);CHKERRQ(ierr);
  }
  ierr = ISDestroy(cperm);CHKERRQ(ierr);
  ierr = ISDestroy(perm);CHKERRQ(ierr);
  ierr = MatCholeskyFactorNumeric(pc->pmat,&icc->info,&icc->fact);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCDestroy_ICC"
static PetscErrorCode PCDestroy_ICC(PC pc)
{
  PC_ICC         *icc = (PC_ICC*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (icc->fact) {ierr = MatDestroy(icc->fact);CHKERRQ(ierr);}
  ierr = PetscStrfree(icc->ordering);CHKERRQ(ierr);
  ierr = PetscFree(icc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApply_ICC"
static PetscErrorCode PCApply_ICC(PC pc,Vec x,Vec y)
{
  PC_ICC         *icc = (PC_ICC*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatSolve(icc->fact,x,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);  
}

#undef __FUNCT__  
#define __FUNCT__ "PCApplySymmetricLeft_ICC"
static PetscErrorCode PCApplySymmetricLeft_ICC(PC pc,Vec x,Vec y)
{
  PetscErrorCode ierr;
  PC_ICC         *icc = (PC_ICC*)pc->data;

  PetscFunctionBegin;
  ierr = MatForwardSolve(icc->fact,x,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCApplySymmetricRight_ICC"
static PetscErrorCode PCApplySymmetricRight_ICC(PC pc,Vec x,Vec y)
{
  PetscErrorCode ierr;
  PC_ICC         *icc = (PC_ICC*)pc->data;

  PetscFunctionBegin;
  ierr = MatBackwardSolve(icc->fact,x,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCGetFactoredMatrix_ICC"
static PetscErrorCode PCGetFactoredMatrix_ICC(PC pc,Mat *mat)
{
  PC_ICC *icc = (PC_ICC*)pc->data;

  PetscFunctionBegin;
  *mat = icc->fact;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCSetFromOptions_ICC"
static PetscErrorCode PCSetFromOptions_ICC(PC pc)
{
  PC_ICC         *icc = (PC_ICC*)pc->data;
  char           tname[256];
  PetscTruth     flg;
  PetscErrorCode ierr;
  PetscFList     ordlist;

  PetscFunctionBegin;
  ierr = MatOrderingRegisterAll(PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsHead("ICC Options");CHKERRQ(ierr);
    ierr = PetscOptionsReal("-pc_icc_levels","levels of fill","PCICCSetLevels",icc->info.levels,&icc->info.levels,&flg);CHKERRQ(ierr);
    ierr = PetscOptionsReal("-pc_icc_fill","Expected fill in factorization","PCICCSetFill",icc->info.fill,&icc->info.fill,&flg);CHKERRQ(ierr);
    ierr = MatGetOrderingList(&ordlist);CHKERRQ(ierr);
    ierr = PetscOptionsList("-pc_icc_mat_ordering_type","Reorder to reduce nonzeros in ICC","PCICCSetMatOrdering",ordlist,icc->ordering,tname,256,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PCICCSetMatOrdering(pc,tname);CHKERRQ(ierr);
    }
    ierr = PetscOptionsName("-pc_factor_shiftnonzero","Shift added to diagonal","PCFactorSetShiftNonzero",&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PCFactorSetShiftNonzero(pc,(PetscReal)PETSC_DECIDE);CHKERRQ(ierr);
    }
    ierr = PetscOptionsReal("-pc_factor_shiftnonzero","Shift added to diagonal","PCFactorSetShiftNonzero",icc->info.shiftnz,&icc->info.shiftnz,0);CHKERRQ(ierr);
    ierr = PetscOptionsName("-pc_factor_shiftpd","Manteuffel shift applied to diagonal","PCICCSetShift",&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PCFactorSetShiftPd(pc,PETSC_TRUE);CHKERRQ(ierr);
    } else {
      ierr = PCFactorSetShiftPd(pc,PETSC_FALSE);CHKERRQ(ierr);
    }
    ierr = PetscOptionsReal("-pc_factor_zeropivot","Pivot is considered zero if less than","PCFactorSetZeroPivot",icc->info.zeropivot,&icc->info.zeropivot,0);CHKERRQ(ierr);
 
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PCView_ICC"
static PetscErrorCode PCView_ICC(PC pc,PetscViewer viewer)
{
  PC_ICC         *icc = (PC_ICC*)pc->data;
  PetscErrorCode ierr;
  PetscTruth     isstring,iascii;

  PetscFunctionBegin;
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_STRING,&isstring);CHKERRQ(ierr);
  ierr = PetscTypeCompare((PetscObject)viewer,PETSC_VIEWER_ASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    if (icc->info.levels == 1) {
        ierr = PetscViewerASCIIPrintf(viewer,"  ICC: %D level of fill\n",(PetscInt)icc->info.levels);CHKERRQ(ierr);
    } else {
        ierr = PetscViewerASCIIPrintf(viewer,"  ICC: %D levels of fill\n",(PetscInt)icc->info.levels);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPrintf(viewer,"  ICC: max fill ratio allocated %g\n",icc->info.fill);CHKERRQ(ierr);
    if (icc->info.shiftpd) {ierr = PetscViewerASCIIPrintf(viewer,"  ICC: using Manteuffel shift\n");CHKERRQ(ierr);}
  } else if (isstring) {
    ierr = PetscViewerStringSPrintf(viewer," lvls=%D",(PetscInt)icc->info.levels);CHKERRQ(ierr);CHKERRQ(ierr);
  } else {
    SETERRQ1(PETSC_ERR_SUP,"Viewer type %s not supported for PCICC",((PetscObject)viewer)->type_name);
  }
  PetscFunctionReturn(0);
}

/*MC
     PCICC - Incomplete Cholesky factorization preconditioners.

   Options Database Keys:
+  -pc_icc_levels <k> - number of levels of fill for ICC(k)
.  -pc_icc_in_place - only for ICC(0) with natural ordering, reuses the space of the matrix for
                      its factorization (overwrites original matrix)
.  -pc_icc_fill <nfill> - expected amount of fill in factored matrix compared to original matrix, nfill > 1
-  -pc_icc_mat_ordering_type <natural,nd,1wd,rcm,qmd> - set the row/column ordering of the factored matrix

   Level: beginner

  Concepts: incomplete Cholesky factorization

   Notes: Only implemented for some matrix formats. Not implemented in parallel

          For BAIJ matrices this implements a point block ICC.

          The Manteuffel shift is only implemented for matrices with block size 1

          By default, the Manteuffel is applied (for matrices with block size 1). Call PCICCSetShift(pc,PETSC_FALSE);
          to turn off the shift.


.seealso:  PCCreate(), PCSetType(), PCType (for list of available types), PC, PCSOR, MatOrderingType,
           PCFactorSetZeroPivot(), PCFactorSetShiftNonzero(), PCFactorSetShiftPd(), 
           PCICCSetFill(), PCICCSetMatOrdering(), PCICCSetReuseOrdering(), 
           PCICCSetLevels()

M*/

EXTERN_C_BEGIN
#undef __FUNCT__  
#define __FUNCT__ "PCCreate_ICC"
PetscErrorCode PCCreate_ICC(PC pc)
{
  PetscErrorCode ierr;
  PC_ICC         *icc;

  PetscFunctionBegin;
  ierr = PetscNew(PC_ICC,&icc);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory(pc,sizeof(PC_ICC));CHKERRQ(ierr);

  icc->fact	          = 0;
  ierr = PetscStrallocpy(MATORDERING_NATURAL,&icc->ordering);CHKERRQ(ierr);
  ierr = MatFactorInfoInitialize(&icc->info);CHKERRQ(ierr);
  icc->info.levels	  = 0;
  icc->info.fill          = 1.0;
  icc->implctx            = 0;

  icc->info.dtcol              = PETSC_DEFAULT;
  icc->info.shiftnz            = 0.0;
  icc->info.shiftpd            = PETSC_TRUE;
  icc->info.shift_fraction     = 0.0;
  icc->info.zeropivot          = 1.e-12;
  pc->data	               = (void*)icc;

  pc->ops->apply	       = PCApply_ICC;
  pc->ops->setup               = PCSetup_ICC;
  pc->ops->destroy	       = PCDestroy_ICC;
  pc->ops->setfromoptions      = PCSetFromOptions_ICC;
  pc->ops->view                = PCView_ICC;
  pc->ops->getfactoredmatrix   = PCGetFactoredMatrix_ICC;
  pc->ops->applysymmetricleft  = PCApplySymmetricLeft_ICC;
  pc->ops->applysymmetricright = PCApplySymmetricRight_ICC;

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCFactorSetZeroPivot_C","PCFactorSetZeroPivot_ICC",
                    PCFactorSetZeroPivot_ICC);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCFactorSetShiftNonzero_C","PCFactorSetShiftNonzero_ICC",
                    PCFactorSetShiftNonzero_ICC);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCFactorSetShiftPd_C","PCFactorSetShiftPd_ICC",
                    PCFactorSetShiftPd_ICC);CHKERRQ(ierr);

  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCICCSetLevels_C","PCICCSetLevels_ICC",
                    PCICCSetLevels_ICC);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCICCSetFill_C","PCICCSetFill_ICC",
                    PCICCSetFill_ICC);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunctionDynamic((PetscObject)pc,"PCICCSetMatOrdering_C","PCICCSetMatOrdering_ICC",
                    PCICCSetMatOrdering_ICC);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
EXTERN_C_END


