#include <../src/sys/classes/viewer/impls/ascii/asciiimpl.h> /*I "petscviewer.h" I*/

#define QUEUESTRINGSIZE 8192

static PetscErrorCode PetscViewerFileClose_ASCII(PetscViewer viewer)
{
  PetscMPIInt        rank;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  int                err;

  PetscFunctionBegin;
  PetscCheck(!vascii->sviewer, PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_WRONGSTATE, "Cannot call with outstanding call to PetscViewerRestoreSubViewer()");
  PetscCallMPI(MPI_Comm_rank(PetscObjectComm((PetscObject)viewer), &rank));
  if (rank == 0 && vascii->fd != stderr && vascii->fd != PETSC_STDOUT) {
    if (vascii->fd && vascii->closefile) {
      err = fclose(vascii->fd);
      PetscCheck(!err, PETSC_COMM_SELF, PETSC_ERR_SYS, "fclose() failed on file");
    }
    if (vascii->storecompressed) {
      char  par[PETSC_MAX_PATH_LEN], buf[PETSC_MAX_PATH_LEN];
      FILE *fp;
      PetscCall(PetscStrncpy(par, "gzip ", sizeof(par)));
      PetscCall(PetscStrlcat(par, vascii->filename, sizeof(par)));
#if defined(PETSC_HAVE_POPEN)
      PetscCall(PetscPOpen(PETSC_COMM_SELF, NULL, par, "r", &fp));
      PetscCheck(!fgets(buf, 1024, fp), PETSC_COMM_SELF, PETSC_ERR_LIB, "Error from compression command %s %s", par, buf);
      PetscCall(PetscPClose(PETSC_COMM_SELF, fp));
#else
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP_SYS, "Cannot run external programs on this machine");
#endif
    }
  }
  PetscCall(PetscFree(vascii->filename));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerDestroy_ASCII(PetscViewer viewer)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  PetscViewerLink   *vlink;
  PetscBool          flg;

  PetscFunctionBegin;
  PetscCheck(!vascii->sviewer, PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_WRONGSTATE, "Cannot call with outstanding call to PetscViewerRestoreSubViewer()");
  PetscCall(PetscViewerFileClose_ASCII(viewer));
  PetscCall(PetscFree(vascii));

  /* remove the viewer from the list in the MPI Communicator */
  if (Petsc_Viewer_keyval == MPI_KEYVAL_INVALID) PetscCallMPI(MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN, Petsc_DelViewer, &Petsc_Viewer_keyval, NULL));

  PetscCallMPI(MPI_Comm_get_attr(PetscObjectComm((PetscObject)viewer), Petsc_Viewer_keyval, (void **)&vlink, (PetscMPIInt *)&flg));
  if (flg) {
    if (vlink && vlink->viewer == viewer) {
      if (vlink->next) {
        PetscCallMPI(MPI_Comm_set_attr(PetscObjectComm((PetscObject)viewer), Petsc_Viewer_keyval, vlink->next));
      } else {
        PetscCallMPI(MPI_Comm_delete_attr(PetscObjectComm((PetscObject)viewer), Petsc_Viewer_keyval));
      }
      PetscCall(PetscFree(vlink));
    } else {
      while (vlink && vlink->next) {
        if (vlink->next->viewer == viewer) {
          PetscViewerLink *nv = vlink->next;
          vlink->next         = vlink->next->next;
          PetscCall(PetscFree(nv));
        }
        vlink = vlink->next;
      }
    }
  }

  if (Petsc_Viewer_Stdout_keyval != MPI_KEYVAL_INVALID) {
    PetscViewer aviewer;
    PetscCallMPI(MPI_Comm_get_attr(PetscObjectComm((PetscObject)viewer), Petsc_Viewer_Stdout_keyval, (void **)&aviewer, (PetscMPIInt *)&flg));
    if (flg && aviewer == viewer) PetscCallMPI(MPI_Comm_delete_attr(PetscObjectComm((PetscObject)viewer), Petsc_Viewer_Stdout_keyval));
  }
  if (Petsc_Viewer_Stderr_keyval != MPI_KEYVAL_INVALID) {
    PetscViewer aviewer;
    PetscCallMPI(MPI_Comm_get_attr(PetscObjectComm((PetscObject)viewer), Petsc_Viewer_Stderr_keyval, (void **)&aviewer, (PetscMPIInt *)&flg));
    if (flg && aviewer == viewer) PetscCallMPI(MPI_Comm_delete_attr(PetscObjectComm((PetscObject)viewer), Petsc_Viewer_Stderr_keyval));
  }
  PetscCall(PetscObjectComposeFunction((PetscObject)viewer, "PetscViewerFileSetName_C", NULL));
  PetscCall(PetscObjectComposeFunction((PetscObject)viewer, "PetscViewerFileGetName_C", NULL));
  PetscCall(PetscObjectComposeFunction((PetscObject)viewer, "PetscViewerFileGetMode_C", NULL));
  PetscCall(PetscObjectComposeFunction((PetscObject)viewer, "PetscViewerFileSetMode_C", NULL));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerDestroy_ASCII_SubViewer(PetscViewer viewer)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  PetscCall(PetscViewerRestoreSubViewer(vascii->bviewer, 0, &viewer));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscViewerASCIIGetPointer - Extracts the file pointer from an ASCII `PetscViewer`.

  Not Collective, depending on the viewer the value may be meaningless except for process 0 of the viewer; No Fortran Support

  Input Parameter:
. viewer - `PetscViewer` context, obtained from `PetscViewerASCIIOpen()`

  Output Parameter:
. fd - file pointer

  Level: intermediate

  Note:
  For the standard `PETSCVIEWERASCII` the value is valid only on MPI rank 0 of the viewer

.seealso: [](sec_viewers), `PETSCVIEWERASCII`, `PetscViewerASCIIOpen()`, `PetscViewerDestroy()`, `PetscViewerSetType()`,
          `PetscViewerCreate()`, `PetscViewerASCIIPrintf()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerFlush()`
@*/
PetscErrorCode PetscViewerASCIIGetPointer(PetscViewer viewer, FILE **fd)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  PetscCheck(!vascii->fileunit, PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_WRONGSTATE, "Cannot request file pointer for viewers that use Fortran files");
  *fd = vascii->fd;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerFileGetMode_ASCII(PetscViewer viewer, PetscFileMode *mode)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  *mode = vascii->mode;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerFileSetMode_ASCII(PetscViewer viewer, PetscFileMode mode)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  vascii->mode = mode;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*
   If petsc_history is on, then all Petsc*Printf() results are saved
   if the appropriate (usually .petschistory) file.
*/
PETSC_INTERN FILE *petsc_history;

/*@
  PetscViewerASCIISetTab - Causes `PetscViewer` to tab in a number of times before printing

  Not Collective, but only first processor in set has any effect; No Fortran Support

  Input Parameters:
+ viewer - obtained with `PetscViewerASCIIOpen()`
- tabs   - number of tabs

  Level: developer

  Note:
  `PetscViewerASCIIPushTab()` and `PetscViewerASCIIPopTab()` are the preferred usage

.seealso: [](sec_viewers), `PETSCVIEWERASCII`, `PetscPrintf()`, `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`,
          `PetscViewerASCIIGetTab()`,
          `PetscViewerASCIIPopTab()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`, `PetscViewerASCIIGetPointer()`,
          `PetscViewerASCIIPushTab()`
@*/
PetscErrorCode PetscViewerASCIISetTab(PetscViewer viewer, PetscInt tabs)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii) ascii->tab = tabs;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerASCIIGetTab - Return the number of tabs used by `PetscViewer`.

  Not Collective, meaningful on first processor only; No Fortran Support

  Input Parameter:
. viewer - obtained with `PetscViewerASCIIOpen()`

  Output Parameter:
. tabs - number of tabs

  Level: developer

.seealso: [](sec_viewers), `PETSCVIEWERASCII`, `PetscPrintf()`, `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`,
          `PetscViewerASCIISetTab()`,
          `PetscViewerASCIIPopTab()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`, `PetscViewerASCIIGetPointer()`, `PetscViewerASCIIPushTab()`
@*/
PetscErrorCode PetscViewerASCIIGetTab(PetscViewer viewer, PetscInt *tabs)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii && tabs) *tabs = ascii->tab;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerASCIIAddTab - Add to the number of times a `PETSCVIEWERASCII` viewer tabs before printing

  Not Collective, but only first processor in set has any effect; No Fortran Support

  Input Parameters:
+ viewer - obtained with `PetscViewerASCIIOpen()`
- tabs   - number of tabs

  Level: developer

  Note:
  `PetscViewerASCIIPushTab()` and `PetscViewerASCIIPopTab()` are the preferred usage

.seealso: [](sec_viewers), `PETSCVIEWERASCII`, `PetscPrintf()`, `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`,
          `PetscViewerASCIIPopTab()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`, `PetscViewerASCIIGetPointer()`, `PetscViewerASCIIPushTab()`
@*/
PetscErrorCode PetscViewerASCIIAddTab(PetscViewer viewer, PetscInt tabs)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii) ascii->tab += tabs;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerASCIISubtractTab - Subtracts from the number of times a `PETSCVIEWERASCII` viewer tabs before printing

  Not Collective, but only first processor in set has any effect; No Fortran Support

  Input Parameters:
+ viewer - obtained with `PetscViewerASCIIOpen()`
- tabs   - number of tabs

  Level: developer

  Note:
  `PetscViewerASCIIPushTab()` and `PetscViewerASCIIPopTab()` are the preferred usage

.seealso: [](sec_viewers), `PETSCVIEWERASCII`, `PetscPrintf()`, `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`,
          `PetscViewerASCIIPopTab()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`, `PetscViewerASCIIGetPointer()`,
          `PetscViewerASCIIPushTab()`
@*/
PetscErrorCode PetscViewerASCIISubtractTab(PetscViewer viewer, PetscInt tabs)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii) ascii->tab -= tabs;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerASCIIPushSynchronized - Allows calls to `PetscViewerASCIISynchronizedPrintf()` for this viewer

  Collective

  Input Parameter:
. viewer - obtained with `PetscViewerASCIIOpen()`

  Level: intermediate

  Note:
  See documentation of `PetscViewerASCIISynchronizedPrintf()` for more details how the synchronized output should be done properly.

.seealso: [](sec_viewers), `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerFlush()`, `PetscViewerASCIIPopSynchronized()`,
          `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`
@*/
PetscErrorCode PetscViewerASCIIPushSynchronized(PetscViewer viewer)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCheck(!ascii->sviewer, PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_WRONGSTATE, "Cannot call with outstanding call to PetscViewerRestoreSubViewer()");
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii) ascii->allowsynchronized++;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerASCIIPopSynchronized - Undoes most recent `PetscViewerASCIIPushSynchronized()` for this viewer

  Collective

  Input Parameter:
. viewer - obtained with `PetscViewerASCIIOpen()`

  Level: intermediate

  Note:
  See documentation of `PetscViewerASCIISynchronizedPrintf()` for more details how the synchronized output should be done properly.

.seealso: [](sec_viewers), `PetscViewerASCIIPushSynchronized()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerFlush()`,
          `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`
@*/
PetscErrorCode PetscViewerASCIIPopSynchronized(PetscViewer viewer)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCheck(!ascii->sviewer, PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_WRONGSTATE, "Cannot call with outstanding call to PetscViewerRestoreSubViewer()");
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii) {
    ascii->allowsynchronized--;
    PetscCheck(ascii->allowsynchronized >= 0, PETSC_COMM_SELF, PETSC_ERR_PLIB, "Called more times than PetscViewerASCIIPushSynchronized()");
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerASCIIPushTab - Adds one more tab to the amount that `PetscViewerASCIIPrintf()`
  lines are tabbed.

  Not Collective, but only first MPI rank in the viewer has any effect; No Fortran Support

  Input Parameter:
. viewer - obtained with `PetscViewerASCIIOpen()`

  Level: developer

.seealso: [](sec_viewers), `PetscPrintf()`, `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`,
          `PetscViewerASCIIPopTab()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`, `PetscViewerASCIIGetPointer()`
@*/
PetscErrorCode PetscViewerASCIIPushTab(PetscViewer viewer)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii) ascii->tab++;
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerASCIIPopTab - Removes one tab from the amount that `PetscViewerASCIIPrintf()` lines are tabbed that was provided by
  `PetscViewerASCIIPushTab()`

  Not Collective, but only first MPI rank in the viewer has any effect; No Fortran Support

  Input Parameter:
. viewer - obtained with `PetscViewerASCIIOpen()`

  Level: developer

.seealso: [](sec_viewers), `PetscPrintf()`, `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`,
          `PetscViewerASCIIPushTab()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`, `PetscViewerASCIIGetPointer()`
@*/
PetscErrorCode PetscViewerASCIIPopTab(PetscViewer viewer)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii) {
    PetscCheck(ascii->tab > 0, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONGSTATE, "More tabs popped than pushed");
    ascii->tab--;
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerASCIIUseTabs - Turns on or off the use of tabs with the `PETSCVIEWERASCII` `PetscViewer`

  Not Collective, but only first MPI rank in the viewer has any effect; No Fortran Support

  Input Parameters:
+ viewer - obtained with `PetscViewerASCIIOpen()`
- flg    - `PETSC_TRUE` or `PETSC_FALSE`

  Level: developer

.seealso: [](sec_viewers), `PetscPrintf()`, `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`,
          `PetscViewerASCIIPopTab()`, `PetscViewerASCIISynchronizedPrintf()`, `PetscViewerASCIIPushTab()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`, `PetscViewerASCIIGetPointer()`
@*/
PetscErrorCode PetscViewerASCIIUseTabs(PetscViewer viewer, PetscBool flg)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  if (iascii) {
    if (flg) ascii->tab = ascii->tab_store;
    else {
      ascii->tab_store = ascii->tab;
      ascii->tab       = 0;
    }
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

#if defined(PETSC_USE_FORTRAN_BINDINGS)

  #if defined(PETSC_HAVE_FORTRAN_CAPS)
    #define petscviewerasciiopenwithfileunit_  PETSCVIEWERASCIIOPENWITHFILEUNIT
    #define petscviewerasciisetfileunit_       PETSCVIEWERASCIISETFILEUNIT
    #define petscviewerasciistdoutsetfileunit_ PETSCVIEWERASCIISTDOUTSETFILEUNIT
    #define petscfortranprinttofileunit_       PETSCFORTRANPRINTTOFILEUNIT
  #elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
    #define petscviewerasciiopenwithfileunit_  petscviewerasciiopenwithfileunit
    #define petscviewerasciisetfileunit_       petscviewerasciisetfileunit
    #define petscviewerasciistdoutsetfileunit_ petscviewerasciistdoutsetfileunit
    #define petscfortranprinttofileunit_       petscfortranprinttofileunit
  #endif

  #if defined(__cplusplus)
extern "C" void petscfortranprinttofileunit_(int *, const char *, PetscErrorCode *, PETSC_FORTRAN_CHARLEN_T);
  #else
extern void petscfortranprinttofileunit_(int *, const char *, PetscErrorCode *, PETSC_FORTRAN_CHARLEN_T);
  #endif

  #define PETSCDEFAULTBUFFERSIZE 8 * 1024

static int PETSC_VIEWER_ASCII_STDOUT_fileunit = 0;

// PetscClangLinter pragma disable: -fdoc-synopsis-macro-explicit-synopsis-valid-header
/*MC
  PetscViewerASCIIStdoutSetFileUnit - sets `PETSC_VIEWER_STDOUT_()` to write to a Fortran IO unit

  Synopsis:
  #include <petscviewer.h>
  void PetscViewerASCIIStdoutSetFileUnit(PetscInt unit, PetscErrorCode ierr)

  Input Parameter:
. unit - the unit number

  Output Parameter:
. ierr - the error code

  Level: intermediate

  Notes:
  Can be called before `PetscInitialize()`

  Immediately changes the output for all `PETSC_VIEWER_STDOUT_()` viewers

  This may not work currently with some viewers that (improperly) use the `fd` directly instead of `PetscViewerASCIIPrintf()`

  With this option, for example, `-log_options` results will be saved to the Fortran file

  Any process may call this but only the unit passed on the first process is used

  Fortran Note:
  Only for Fortran

  Developer Note:
  `PetscViewerASCIIWORLDSetFilename()` and `PetscViewerASCIIWORLDSetFILE()` could be added

.seealso: `PetscViewerASCIISetFILE()`, `PETSCVIEWERASCII`, `PetscViewerASCIIOpenWithFileUnit()`, `PetscViewerASCIIStdoutSetFileUnit()`,
          `PETSC_VIEWER_STDOUT_()`, `PetscViewerASCIIGetStdout()`
M*/
PETSC_EXTERN void petscviewerasciistdoutsetfileunit_(int *unit, PetscErrorCode *ierr)
{
  #if defined(PETSC_USE_FORTRAN_BINDINGS)
  PETSC_VIEWER_ASCII_STDOUT_fileunit = *unit;
  #endif
}

  #include <petsc/private/ftnimpl.h>

// PetscClangLinter pragma disable: -fdoc-synopsis-macro-explicit-synopsis-valid-header
/*MC
  PetscViewerASCIISetFileUnit - sets the `PETSCVIEWERASCII` `PetscViewer` to write to a Fortran IO unit

  Synopsis:
  #include <petscviewer.h>
  void PetscViewerASCIISetFileUnit(PetscViewer lab, PetscInt unit, PetscErrorCode ierr)

  Input Parameters:
+ lab  - the viewer
- unit - the unit number

  Output Parameter:
. ierr - the error code

  Level: intermediate

  Note:
  `PetscViewerDestroy()` does not close the unit for this `PetscViewer`

  Fortran Notes:
  Only for Fortran, use  `PetscViewerASCIISetFILE()` for C

.seealso: `PetscViewerASCIISetFILE()`, `PETSCVIEWERASCII`, `PetscViewerASCIIOpenWithFileUnit()`, `PetscViewerASCIIStdoutSetFileUnit()`
M*/
PETSC_EXTERN void petscviewerasciisetfileunit_(PetscViewer *lab, int *unit, PetscErrorCode *ierr)
{
  PetscViewer_ASCII *vascii;
  PetscViewer        v;

  PetscPatchDefaultViewers_Fortran(lab, v);
  vascii = (PetscViewer_ASCII *)v->data;
  if (vascii->mode == FILE_MODE_READ) {
    *ierr = PETSC_ERR_ARG_WRONGSTATE;
    return;
  }
  vascii->fileunit = *unit;
}

// PetscClangLinter pragma disable: -fdoc-synopsis-macro-explicit-synopsis-valid-header
/*MC
  PetscViewerASCIIOpenWithFileUnit - opens a `PETSCVIEWERASCII` to write to a Fortran IO unit

  Synopsis:
  #include <petscviewer.h>
  void PetscViewerASCIIOpenWithFileUnit((MPI_Fint comm, integer unit, PetscViewer viewer, PetscErrorCode ierr)

  Input Parameters:
+ comm - the `MPI_Comm` to share the viewer
- unit - the unit number

  Output Parameters:
+ lab  - the viewer
- ierr - the error code

  Level: intermediate

  Note:
  `PetscViewerDestroy()` does not close the unit for this `PetscViewer`

  Fortran Notes:
  Only for Fortran, use  `PetscViewerASCIIOpenWithFILE()` for C

.seealso: `PetscViewerASCIISetFileUnit()`, `PetscViewerASCIISetFILE()`, `PETSCVIEWERASCII`, `PetscViewerASCIIOpenWithFILE()`
M*/
PETSC_EXTERN void petscviewerasciiopenwithfileunit_(MPI_Fint *comm, int *unit, PetscViewer *lab, PetscErrorCode *ierr)
{
  *ierr = PetscViewerCreate(MPI_Comm_f2c(*(MPI_Fint *)&*comm), lab);
  if (*ierr) return;
  *ierr = PetscViewerSetType(*lab, PETSCVIEWERASCII);
  if (*ierr) return;
  *ierr = PetscViewerFileSetMode(*lab, FILE_MODE_WRITE);
  if (*ierr) return;
  petscviewerasciisetfileunit_(lab, unit, ierr);
}

static PetscErrorCode PetscVFPrintfFortran(int unit, const char format[], va_list Argp)
{
  PetscErrorCode ierr;
  char           str[PETSCDEFAULTBUFFERSIZE];
  size_t         len;

  PetscFunctionBegin;
  PetscCall(PetscVSNPrintf(str, sizeof(str), format, NULL, Argp));
  PetscCall(PetscStrlen(str, &len));
  petscfortranprinttofileunit_(&unit, str, &ierr, (int)len);
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscFPrintfFortran(int unit, const char str[])
{
  PetscErrorCode ierr;
  size_t         len;

  PetscFunctionBegin;
  PetscCall(PetscStrlen(str, &len));
  petscfortranprinttofileunit_(&unit, str, &ierr, (int)len);
  PetscFunctionReturn(PETSC_SUCCESS);
}

#else

/* these will never be used; but are needed to link with */
static PetscErrorCode PetscVFPrintfFortran(int unit, const char format[], va_list Argp)
{
  PetscFunctionBegin;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscFPrintfFortran(int unit, const char str[])
{
  PetscFunctionBegin;
  PetscFunctionReturn(PETSC_SUCCESS);
}
#endif

/*@
  PetscViewerASCIIGetStdout - Creates a `PETSCVIEWERASCII` `PetscViewer` shared by all processes
  in a communicator that prints to `stdout`. Error returning version of `PETSC_VIEWER_STDOUT_()`

  Collective

  Input Parameter:
. comm - the MPI communicator to share the `PetscViewer`

  Output Parameter:
. viewer - the viewer

  Level: beginner

  Note:
  Use `PetscViewerDestroy()` to destroy it

  Developer Note:
  This should be used in all PETSc source code instead of `PETSC_VIEWER_STDOUT_()` since it allows error checking

.seealso: [](sec_viewers), `PetscViewerASCIIGetStderr()`, `PETSC_VIEWER_DRAW_()`, `PetscViewerASCIIOpen()`, `PETSC_VIEWER_STDERR_`, `PETSC_VIEWER_STDOUT_WORLD`,
          `PETSC_VIEWER_STDOUT_SELF`
@*/
PetscErrorCode PetscViewerASCIIGetStdout(MPI_Comm comm, PetscViewer *viewer)
{
  PetscBool flg;
  MPI_Comm  ncomm;

  PetscFunctionBegin;
  PetscAssertPointer(viewer, 2);
  PetscCall(PetscSpinlockLock(&PetscViewerASCIISpinLockStdout));
  PetscCall(PetscCommDuplicate(comm, &ncomm, NULL));
  if (Petsc_Viewer_Stdout_keyval == MPI_KEYVAL_INVALID) PetscCallMPI(MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN, MPI_COMM_NULL_DELETE_FN, &Petsc_Viewer_Stdout_keyval, NULL));
  PetscCallMPI(MPI_Comm_get_attr(ncomm, Petsc_Viewer_Stdout_keyval, (void **)viewer, (PetscMPIInt *)&flg));
  if (!flg) { /* PetscViewer not yet created */
#if defined(PETSC_USE_FORTRAN_BINDINGS)
    PetscCallMPI(MPI_Bcast(&PETSC_VIEWER_ASCII_STDOUT_fileunit, 1, MPI_INT, 0, comm));
    if (PETSC_VIEWER_ASCII_STDOUT_fileunit) {
      PetscErrorCode ierr;
      MPI_Fint       fcomm = MPI_Comm_c2f(ncomm);

      petscviewerasciiopenwithfileunit_(&fcomm, &PETSC_VIEWER_ASCII_STDOUT_fileunit, viewer, &ierr);
    } else
#endif
    {
      PetscCall(PetscViewerCreate(ncomm, viewer));
      PetscCall(PetscViewerSetType(*viewer, PETSCVIEWERASCII));
      PetscCall(PetscViewerFileSetName(*viewer, "stdout"));
    }
    PetscCall(PetscObjectRegisterDestroy((PetscObject)*viewer));
    PetscCallMPI(MPI_Comm_set_attr(ncomm, Petsc_Viewer_Stdout_keyval, (void *)*viewer));
  }
  PetscCall(PetscCommDestroy(&ncomm));
  PetscCall(PetscSpinlockUnlock(&PetscViewerASCIISpinLockStdout));
#if defined(PETSC_USE_FORTRAN_BINDINGS)
  ((PetscViewer_ASCII *)(*viewer)->data)->fileunit = PETSC_VIEWER_ASCII_STDOUT_fileunit;
#endif
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscViewerASCIIPrintf - Prints to a file, only from the first
  processor in the `PetscViewer` of type `PETSCVIEWERASCII`

  Not Collective, but only the first MPI rank in the viewer has any effect

  Input Parameters:
+ viewer - obtained with `PetscViewerASCIIOpen()`
- format - the usual printf() format string

  Level: developer

  Fortran Notes:
  The call sequence is `PetscViewerASCIIPrintf`(`PetscViewer`, character(*), int ierr) from Fortran.
  That is, you can only pass a single character string from Fortran.

.seealso: [](sec_viewers), `PetscPrintf()`, `PetscSynchronizedPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerASCIIPushTab()`, `PetscViewerASCIIPopTab()`, `PetscViewerASCIISynchronizedPrintf()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`, `PetscViewerASCIIGetPointer()`, `PetscViewerASCIIPushSynchronized()`
@*/
PetscErrorCode PetscViewerASCIIPrintf(PetscViewer viewer, const char format[], ...)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;
  PetscMPIInt        rank;
  PetscInt           tab = 0, intab = ascii->tab;
  FILE              *fd = ascii->fd;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCheck(!ascii->sviewer, PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_WRONGSTATE, "Cannot call with outstanding call to PetscViewerRestoreSubViewer()");
  PetscAssertPointer(format, 2);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  PetscCheck(iascii, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Not ASCII PetscViewer");
  PetscCallMPI(MPI_Comm_rank(PetscObjectComm((PetscObject)viewer), &rank));
  if (rank) PetscFunctionReturn(PETSC_SUCCESS);

  if (ascii->bviewer) { /* pass string up to parent viewer */
    char   *string;
    va_list Argp;
    size_t  fullLength;

    PetscCall(PetscCalloc1(QUEUESTRINGSIZE, &string));
    for (; tab < ascii->tab; tab++) { string[2 * tab] = string[2 * tab + 1] = ' '; }
    va_start(Argp, format);
    PetscCall(PetscVSNPrintf(string + 2 * intab, QUEUESTRINGSIZE - 2 * intab, format, &fullLength, Argp));
    va_end(Argp);
    PetscCall(PetscViewerASCIISynchronizedPrintf(ascii->bviewer, "%s", string));
    PetscCall(PetscFree(string));
  } else { /* write directly to file */
    va_list Argp;

    tab = intab;
    while (tab--) {
      if (!ascii->fileunit) PetscCall(PetscFPrintf(PETSC_COMM_SELF, fd, "  "));
      else PetscCall(PetscFPrintfFortran(ascii->fileunit, "   "));
    }

    va_start(Argp, format);
    if (!ascii->fileunit) PetscCall((*PetscVFPrintf)(fd, format, Argp));
    else PetscCall(PetscVFPrintfFortran(ascii->fileunit, format, Argp));
    va_end(Argp);
    PetscCall(PetscFFlush(fd));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@
  PetscViewerFileSetName - Sets the name of the file the `PetscViewer` should use.

  Collective

  Input Parameters:
+ viewer - the `PetscViewer`; for example, of type `PETSCVIEWERASCII` or `PETSCVIEWERBINARY`
- name   - the name of the file it should use

  Level: advanced

  Note:
  This will have no effect on viewers that are not related to files

.seealso: [](sec_viewers), `PetscViewerCreate()`, `PetscViewerSetType()`, `PetscViewerASCIIOpen()`, `PetscViewerBinaryOpen()`, `PetscViewerDestroy()`,
          `PetscViewerASCIIGetPointer()`, `PetscViewerASCIIPrintf()`, `PetscViewerASCIISynchronizedPrintf()`
@*/
PetscErrorCode PetscViewerFileSetName(PetscViewer viewer, const char name[])
{
  char filename[PETSC_MAX_PATH_LEN];

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscAssertPointer(name, 2);
  PetscCall(PetscStrreplace(PetscObjectComm((PetscObject)viewer), name, filename, sizeof(filename)));
  PetscTryMethod(viewer, "PetscViewerFileSetName_C", (PetscViewer, const char[]), (viewer, filename));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscViewerFileGetName - Gets the name of the file the `PetscViewer` is using

  Not Collective

  Input Parameter:
. viewer - the `PetscViewer`

  Output Parameter:
. name - the name of the file it is using

  Level: advanced

  Note:
  This will have no effect on viewers that are not related to files

.seealso: [](sec_viewers), `PetscViewerCreate()`, `PetscViewerSetType()`, `PetscViewerASCIIOpen()`, `PetscViewerBinaryOpen()`, `PetscViewerFileSetName()`
@*/
PetscErrorCode PetscViewerFileGetName(PetscViewer viewer, const char *name[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscAssertPointer(name, 2);
  PetscUseMethod(viewer, "PetscViewerFileGetName_C", (PetscViewer, const char **), (viewer, name));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerFileGetName_ASCII(PetscViewer viewer, const char **name)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  *name = vascii->filename;
  PetscFunctionReturn(PETSC_SUCCESS);
}

#include <errno.h>
static PetscErrorCode PetscViewerFileSetName_ASCII(PetscViewer viewer, const char name[])
{
  size_t             len;
  char               fname[PETSC_MAX_PATH_LEN], *gz = NULL;
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  PetscBool          isstderr, isstdout;
  PetscMPIInt        rank;

  PetscFunctionBegin;
  PetscCall(PetscViewerFileClose_ASCII(viewer));
  if (!name) PetscFunctionReturn(PETSC_SUCCESS);
  PetscCall(PetscStrallocpy(name, &vascii->filename));

  /* Is this file to be compressed */
  vascii->storecompressed = PETSC_FALSE;

  PetscCall(PetscStrstr(vascii->filename, ".gz", &gz));
  if (gz) {
    PetscCall(PetscStrlen(gz, &len));
    if (len == 3) {
      PetscCheck(vascii->mode == FILE_MODE_WRITE, PetscObjectComm((PetscObject)viewer), PETSC_ERR_SUP, "Cannot open ASCII PetscViewer file that is compressed; uncompress it manually first");
      *gz                     = 0;
      vascii->storecompressed = PETSC_TRUE;
    }
  }
  PetscCallMPI(MPI_Comm_rank(PetscObjectComm((PetscObject)viewer), &rank));
  if (rank == 0) {
    PetscCall(PetscStrcmp(name, "stderr", &isstderr));
    PetscCall(PetscStrcmp(name, "stdout", &isstdout));
    /* empty filename means stdout */
    if (name[0] == 0) isstdout = PETSC_TRUE;
    if (isstderr) vascii->fd = PETSC_STDERR;
    else if (isstdout) vascii->fd = PETSC_STDOUT;
    else {
      PetscCall(PetscFixFilename(name, fname));
      switch (vascii->mode) {
      case FILE_MODE_READ:
        vascii->fd = fopen(fname, "r");
        break;
      case FILE_MODE_WRITE:
        vascii->fd = fopen(fname, "w");
        break;
      case FILE_MODE_APPEND:
        vascii->fd = fopen(fname, "a");
        break;
      case FILE_MODE_UPDATE:
        vascii->fd = fopen(fname, "r+");
        if (!vascii->fd) vascii->fd = fopen(fname, "w+");
        break;
      case FILE_MODE_APPEND_UPDATE:
        /* I really want a file which is opened at the end for updating,
           not a+, which opens at the beginning, but makes writes at the end.
        */
        vascii->fd = fopen(fname, "r+");
        if (!vascii->fd) vascii->fd = fopen(fname, "w+");
        else {
          int ret = fseek(vascii->fd, 0, SEEK_END);
          PetscCheck(!ret, PETSC_COMM_SELF, PETSC_ERR_LIB, "fseek() failed with error code %d", ret);
        }
        break;
      default:
        SETERRQ(PetscObjectComm((PetscObject)viewer), PETSC_ERR_SUP, "Unsupported file mode %s", PetscFileModes[vascii->mode]);
      }
      PetscCheck(vascii->fd, PETSC_COMM_SELF, PETSC_ERR_FILE_OPEN, "Cannot open PetscViewer file: %s due to \"%s\"", fname, strerror(errno));
    }
  }
  PetscCall(PetscLogObjectState((PetscObject)viewer, "File: %s", name));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerGetSubViewer_ASCII(PetscViewer viewer, MPI_Comm subcomm, PetscViewer *outviewer)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data, *ovascii;

  PetscFunctionBegin;
  PetscCheck(!vascii->sviewer, PETSC_COMM_SELF, PETSC_ERR_ORDER, "SubViewer already obtained from PetscViewer and not restored");
  PetscCall(PetscViewerASCIIPushSynchronized(viewer));
  /*
     The following line is a bug; it does another PetscViewerASCIIPushSynchronized() on viewer, but if it is removed the code won't work
     because it relies on this behavior in other places. In particular this line causes the synchronized flush to occur when the viewer is destroyed
     (since the count never gets to zero) in some examples this displays information that otherwise would be lost

     This code also means another call to PetscViewerASCIIPopSynchronized() must be made after the PetscViewerRestoreSubViewer(), see, for example,
     PCView_GASM().
  */
  PetscCall(PetscViewerASCIIPushSynchronized(viewer));
  PetscCall(PetscViewerFlush(viewer));
  PetscCall(PetscViewerCreate(subcomm, outviewer));
  PetscCall(PetscViewerSetType(*outviewer, PETSCVIEWERASCII));
  PetscCall(PetscViewerASCIIPushSynchronized(*outviewer));
  ovascii            = (PetscViewer_ASCII *)(*outviewer)->data;
  ovascii->fd        = vascii->fd;
  ovascii->fileunit  = vascii->fileunit;
  ovascii->closefile = PETSC_FALSE;

  vascii->sviewer                                      = *outviewer;
  (*outviewer)->format                                 = viewer->format;
  ((PetscViewer_ASCII *)((*outviewer)->data))->bviewer = viewer;
  (*outviewer)->ops->destroy                           = PetscViewerDestroy_ASCII_SubViewer;
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerRestoreSubViewer_ASCII(PetscViewer viewer, MPI_Comm comm, PetscViewer *outviewer)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)viewer->data;

  PetscFunctionBegin;
  PetscCheck(ascii->sviewer, PETSC_COMM_SELF, PETSC_ERR_ORDER, "SubViewer never obtained from PetscViewer");
  PetscCheck(ascii->sviewer == *outviewer, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "This PetscViewer did not generate this SubViewer");

  PetscCall(PetscViewerASCIIPopSynchronized(*outviewer));
  ascii->sviewer             = NULL;
  (*outviewer)->ops->destroy = PetscViewerDestroy_ASCII;
  PetscCall(PetscViewerDestroy(outviewer));
  PetscCall(PetscViewerFlush(viewer));
  PetscCall(PetscViewerASCIIPopSynchronized(viewer));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerView_ASCII(PetscViewer v, PetscViewer viewer)
{
  PetscViewer_ASCII *ascii = (PetscViewer_ASCII *)v->data;

  PetscFunctionBegin;
  if (ascii->fileunit) PetscCall(PetscViewerASCIIPrintf(viewer, "Fortran FILE UNIT: %d\n", ascii->fileunit));
  else if (ascii->filename) PetscCall(PetscViewerASCIIPrintf(viewer, "Filename: %s\n", ascii->filename));
  PetscFunctionReturn(PETSC_SUCCESS);
}

static PetscErrorCode PetscViewerFlush_ASCII(PetscViewer viewer)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  MPI_Comm           comm;
  PetscMPIInt        rank, size;
  FILE              *fd = vascii->fd;

  PetscFunctionBegin;
  PetscCheck(!vascii->sviewer, PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_WRONGSTATE, "Cannot call with outstanding call to PetscViewerRestoreSubViewer()");
  PetscCall(PetscObjectGetComm((PetscObject)viewer, &comm));
  PetscCallMPI(MPI_Comm_rank(comm, &rank));
  PetscCallMPI(MPI_Comm_size(comm, &size));

  if (!vascii->bviewer && rank == 0 && (vascii->mode != FILE_MODE_READ)) PetscCall(PetscFFlush(vascii->fd));

  if (vascii->allowsynchronized) {
    PetscMPIInt tag, i, j, n = 0, dummy = 0;
    char       *message;
    MPI_Status  status;

    PetscCall(PetscCommDuplicate(comm, &comm, &tag));

    /* First processor waits for messages from all other processors */
    if (rank == 0) {
      /* flush my own messages that I may have queued up */
      PrintfQueue next = vascii->petsc_printfqueuebase, previous;
      for (i = 0; i < vascii->petsc_printfqueuelength; i++) {
        if (!vascii->bviewer) {
          if (!vascii->fileunit) PetscCall(PetscFPrintf(comm, fd, "%s", next->string));
          else PetscCall(PetscFPrintfFortran(vascii->fileunit, next->string));
        } else {
          PetscCall(PetscViewerASCIISynchronizedPrintf(vascii->bviewer, "%s", next->string));
        }
        previous = next;
        next     = next->next;
        PetscCall(PetscFree(previous->string));
        PetscCall(PetscFree(previous));
      }
      vascii->petsc_printfqueue       = NULL;
      vascii->petsc_printfqueuelength = 0;
      for (i = 1; i < size; i++) {
        /* to prevent a flood of messages to process zero, request each message separately */
        PetscCallMPI(MPI_Send(&dummy, 1, MPI_INT, i, tag, comm));
        PetscCallMPI(MPI_Recv(&n, 1, MPI_INT, i, tag, comm, &status));
        for (j = 0; j < n; j++) {
          size_t size;

          PetscCallMPI(MPI_Recv(&size, 1, MPIU_SIZE_T, i, tag, comm, &status));
          PetscCall(PetscMalloc1(size, &message));
          PetscCallMPI(MPI_Recv(message, (PetscMPIInt)size, MPI_CHAR, i, tag, comm, &status));
          if (!vascii->bviewer) {
            if (!vascii->fileunit) PetscCall(PetscFPrintf(comm, fd, "%s", message));
            else PetscCall(PetscFPrintfFortran(vascii->fileunit, message));
          } else {
            PetscCall(PetscViewerASCIISynchronizedPrintf(vascii->bviewer, "%s", message));
          }
          PetscCall(PetscFree(message));
        }
      }
    } else { /* other processors send queue to processor 0 */
      PrintfQueue next = vascii->petsc_printfqueuebase, previous;

      PetscCallMPI(MPI_Recv(&dummy, 1, MPI_INT, 0, tag, comm, &status));
      PetscCallMPI(MPI_Send(&vascii->petsc_printfqueuelength, 1, MPI_INT, 0, tag, comm));
      for (i = 0; i < vascii->petsc_printfqueuelength; i++) {
        PetscCallMPI(MPI_Send(&next->size, 1, MPIU_SIZE_T, 0, tag, comm));
        PetscCallMPI(MPI_Send(next->string, (PetscMPIInt)next->size, MPI_CHAR, 0, tag, comm));
        previous = next;
        next     = next->next;
        PetscCall(PetscFree(previous->string));
        PetscCall(PetscFree(previous));
      }
      vascii->petsc_printfqueue       = NULL;
      vascii->petsc_printfqueuelength = 0;
    }
    PetscCall(PetscCommDestroy(&comm));
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*MC
   PETSCVIEWERASCII - A viewer that prints to `stdout`, `stderr`, or an ASCII file

  Level: beginner

.seealso: [](sec_viewers), `PETSC_VIEWER_STDOUT_()`, `PETSC_VIEWER_STDOUT_SELF`, `PETSC_VIEWER_STDOUT_WORLD`, `PetscViewerCreate()`, `PetscViewerASCIIOpen()`,
          `PetscViewerMatlabOpen()`, `VecView()`, `DMView()`, `PetscViewerMatlabPutArray()`, `PETSCVIEWERBINARY`, `PETSCVIEWERMATLAB`,
          `PetscViewerFileSetName()`, `PetscViewerFileSetMode()`, `PetscViewerFormat`, `PetscViewerType`, `PetscViewerSetType()`
M*/
PETSC_EXTERN PetscErrorCode PetscViewerCreate_ASCII(PetscViewer viewer)
{
  PetscViewer_ASCII *vascii;

  PetscFunctionBegin;
  PetscCall(PetscNew(&vascii));
  viewer->data = (void *)vascii;

  viewer->ops->destroy          = PetscViewerDestroy_ASCII;
  viewer->ops->flush            = PetscViewerFlush_ASCII;
  viewer->ops->getsubviewer     = PetscViewerGetSubViewer_ASCII;
  viewer->ops->restoresubviewer = PetscViewerRestoreSubViewer_ASCII;
  viewer->ops->view             = PetscViewerView_ASCII;
  viewer->ops->read             = PetscViewerASCIIRead;

  /* defaults to stdout unless set with PetscViewerFileSetName() */
  vascii->fd        = PETSC_STDOUT;
  vascii->mode      = FILE_MODE_WRITE;
  vascii->bviewer   = NULL;
  vascii->subviewer = NULL;
  vascii->sviewer   = NULL;
  vascii->tab       = 0;
  vascii->tab_store = 0;
  vascii->filename  = NULL;
  vascii->closefile = PETSC_TRUE;

  PetscCall(PetscObjectComposeFunction((PetscObject)viewer, "PetscViewerFileSetName_C", PetscViewerFileSetName_ASCII));
  PetscCall(PetscObjectComposeFunction((PetscObject)viewer, "PetscViewerFileGetName_C", PetscViewerFileGetName_ASCII));
  PetscCall(PetscObjectComposeFunction((PetscObject)viewer, "PetscViewerFileGetMode_C", PetscViewerFileGetMode_ASCII));
  PetscCall(PetscObjectComposeFunction((PetscObject)viewer, "PetscViewerFileSetMode_C", PetscViewerFileSetMode_ASCII));
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscViewerASCIISynchronizedPrintf - Prints synchronized output to the specified `PETSCVIEWERASCII` file from
  several processors.  Output of the first processor is followed by that of the
  second, etc.

  Not Collective, must call collective `PetscViewerFlush()` to get the results flushed

  Input Parameters:
+ viewer - the `PETSCVIEWERASCII` `PetscViewer`
- format - the usual printf() format string

  Level: intermediate

  Notes:
  You must have previously called `PetscViewerASCIIPushSynchronized()` to allow this routine to be called.
  Then you can do multiple independent calls to this routine.

  The actual synchronized print is then done using `PetscViewerFlush()`.
  `PetscViewerASCIIPopSynchronized()` should be then called if we are already done with the synchronized output
  to conclude the "synchronized session".

  So the typical calling sequence looks like
.vb
    PetscViewerASCIIPushSynchronized(viewer);
    PetscViewerASCIISynchronizedPrintf(viewer, ...);
    PetscViewerASCIISynchronizedPrintf(viewer, ...);
    ...
    PetscViewerFlush(viewer);
    PetscViewerASCIISynchronizedPrintf(viewer, ...);
    PetscViewerASCIISynchronizedPrintf(viewer, ...);
    ...
    PetscViewerFlush(viewer);
    PetscViewerASCIIPopSynchronized(viewer);
.ve

  Fortran Notes:
  Can only print a single character* string

.seealso: [](sec_viewers), `PetscViewerASCIIPushSynchronized()`, `PetscViewerFlush()`, `PetscViewerASCIIPopSynchronized()`,
          `PetscSynchronizedPrintf()`, `PetscViewerASCIIPrintf()`, `PetscViewerASCIIOpen()`,
          `PetscViewerCreate()`, `PetscViewerDestroy()`, `PetscViewerSetType()`
@*/
PetscErrorCode PetscViewerASCIISynchronizedPrintf(PetscViewer viewer, const char format[], ...)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  PetscMPIInt        rank;
  PetscInt           tab = 0;
  MPI_Comm           comm;
  PetscBool          iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscAssertPointer(format, 2);
  PetscCall(PetscObjectTypeCompare((PetscObject)viewer, PETSCVIEWERASCII, &iascii));
  PetscCheck(iascii, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Not ASCII PetscViewer");
  PetscCheck(vascii->allowsynchronized, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONGSTATE, "First call PetscViewerASCIIPushSynchronized() to allow this call");

  PetscCall(PetscObjectGetComm((PetscObject)viewer, &comm));
  PetscCallMPI(MPI_Comm_rank(comm, &rank));

  if (vascii->bviewer) {
    char   *string;
    va_list Argp;
    size_t  fullLength;

    PetscCall(PetscCalloc1(QUEUESTRINGSIZE, &string));
    for (; tab < vascii->tab; tab++) { string[2 * tab] = string[2 * tab + 1] = ' '; }
    va_start(Argp, format);
    PetscCall(PetscVSNPrintf(string + 2 * tab, QUEUESTRINGSIZE - 2 * tab, format, &fullLength, Argp));
    va_end(Argp);
    PetscCall(PetscViewerASCIISynchronizedPrintf(vascii->bviewer, "%s", string));
    PetscCall(PetscFree(string));
  } else if (rank == 0) { /* First processor prints immediately to fp */
    va_list Argp;
    FILE   *fp = vascii->fd;

    tab = vascii->tab;
    while (tab--) {
      if (!vascii->fileunit) PetscCall(PetscFPrintf(PETSC_COMM_SELF, fp, "  "));
      else PetscCall(PetscFPrintfFortran(vascii->fileunit, "   "));
    }

    va_start(Argp, format);
    if (!vascii->fileunit) PetscCall((*PetscVFPrintf)(fp, format, Argp));
    else PetscCall(PetscVFPrintfFortran(vascii->fileunit, format, Argp));
    va_end(Argp);
    PetscCall(PetscFFlush(fp));
    if (petsc_history) {
      va_start(Argp, format);
      PetscCall((*PetscVFPrintf)(petsc_history, format, Argp));
      va_end(Argp);
      PetscCall(PetscFFlush(petsc_history));
    }
  } else { /* other processors add to queue */
    char       *string;
    va_list     Argp;
    size_t      fullLength;
    PrintfQueue next;

    PetscCall(PetscNew(&next));
    if (vascii->petsc_printfqueue) {
      vascii->petsc_printfqueue->next = next;
      vascii->petsc_printfqueue       = next;
    } else {
      vascii->petsc_printfqueuebase = vascii->petsc_printfqueue = next;
    }
    vascii->petsc_printfqueuelength++;
    next->size = QUEUESTRINGSIZE;
    PetscCall(PetscCalloc1(next->size, &next->string));
    string = next->string;

    tab = vascii->tab;
    tab *= 2;
    while (tab--) *string++ = ' ';
    va_start(Argp, format);
    PetscCall(PetscVSNPrintf(string, next->size - 2 * vascii->tab, format, &fullLength, Argp));
    va_end(Argp);
    if (fullLength > next->size - 2 * vascii->tab) {
      PetscCall(PetscFree(next->string));
      next->size = fullLength + 2 * vascii->tab;
      PetscCall(PetscCalloc1(next->size, &next->string));
      string = next->string;
      tab    = 2 * vascii->tab;
      while (tab--) *string++ = ' ';
      va_start(Argp, format);
      PetscCall(PetscVSNPrintf(string, next->size - 2 * vascii->tab, format, NULL, Argp));
      va_end(Argp);
    }
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

/*@C
  PetscViewerASCIIRead - Reads from a `PETSCVIEWERASCII` file

  Only MPI rank 0 in the `PetscViewer` may call this

  Input Parameters:
+ viewer - the `PETSCVIEWERASCII` viewer
. data   - location to write the data, treated as an array of type indicated by `datatype`
. num    - number of items of data to read
- dtype  - type of data to read

  Output Parameter:
. count - number of items of data actually read, or `NULL`

  Level: beginner

.seealso: [](sec_viewers), `PetscViewerASCIIOpen()`, `PetscViewerPushFormat()`, `PetscViewerDestroy()`, `PetscViewerCreate()`, `PetscViewerFileSetMode()`, `PetscViewerFileSetName()`
          `VecView()`, `MatView()`, `VecLoad()`, `MatLoad()`, `PetscViewerBinaryGetDescriptor()`,
          `PetscViewerBinaryGetInfoPointer()`, `PetscFileMode`, `PetscViewer`, `PetscViewerBinaryRead()`
@*/
PetscErrorCode PetscViewerASCIIRead(PetscViewer viewer, void *data, PetscInt num, PetscInt *count, PetscDataType dtype)
{
  PetscViewer_ASCII *vascii = (PetscViewer_ASCII *)viewer->data;
  FILE              *fd     = vascii->fd;
  PetscInt           i;
  int                ret = 0;
  PetscMPIInt        rank;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  PetscCallMPI(MPI_Comm_rank(PetscObjectComm((PetscObject)viewer), &rank));
  PetscCheck(rank == 0, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Can only be called from process 0 in the PetscViewer");
  for (i = 0; i < num; i++) {
    if (dtype == PETSC_CHAR) ret = fscanf(fd, "%c", &(((char *)data)[i]));
    else if (dtype == PETSC_STRING) ret = fscanf(fd, "%s", &(((char *)data)[i]));
    else if (dtype == PETSC_INT) ret = fscanf(fd, "%" PetscInt_FMT, &(((PetscInt *)data)[i]));
    else if (dtype == PETSC_ENUM) ret = fscanf(fd, "%d", &(((int *)data)[i]));
    else if (dtype == PETSC_INT64) ret = fscanf(fd, "%" PetscInt64_FMT, &(((PetscInt64 *)data)[i]));
    else if (dtype == PETSC_LONG) ret = fscanf(fd, "%ld", &(((long *)data)[i]));
    else if (dtype == PETSC_COUNT) ret = fscanf(fd, "%" PetscCount_FMT, &(((PetscCount *)data)[i]));
    else if (dtype == PETSC_FLOAT) ret = fscanf(fd, "%f", &(((float *)data)[i]));
    else if (dtype == PETSC_DOUBLE) ret = fscanf(fd, "%lg", &(((double *)data)[i]));
#if defined(PETSC_USE_REAL___FLOAT128)
    else if (dtype == PETSC___FLOAT128) {
      double tmp;
      ret                     = fscanf(fd, "%lg", &tmp);
      ((__float128 *)data)[i] = tmp;
    }
#endif
    else
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Data type %d not supported", (int)dtype);
    PetscCheck(ret, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Conversion error for data type %d", (int)dtype);
    if (ret < 0) break; /* Proxy for EOF, need to check for it in configure */
  }
  if (count) *count = i;
  else PetscCheck(ret >= 0, PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Insufficient data, read only %" PetscInt_FMT " < %" PetscInt_FMT " items", i, num);
  PetscFunctionReturn(PETSC_SUCCESS);
}
