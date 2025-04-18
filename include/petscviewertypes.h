/*
     PetscViewers are objects where other objects can be looked at or stored.
*/

#pragma once

/* MANSEC = Sys */
/* SUBMANSEC = Viewer */

/*S
     PetscViewer - Abstract PETSc object for displaying (in ASCII, binary, graphically etc)
                   PETSc objects and their data

   Level: beginner

.seealso: [](sec_viewers), `PetscViewerType`, `PETSCVIEWERASCII`, `PetscViewerCreate()`, `PetscViewerSetType()`
S*/
typedef struct _p_PetscViewer *PetscViewer;
