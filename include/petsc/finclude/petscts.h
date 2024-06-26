!
!  Include file for Fortran use of the TS (timestepping) package in PETSc
!
#if !defined (PETSCTSDEF_H)
#define PETSCTSDEF_H

#include "petsc/finclude/petscsnes.h"

#define TS type(tTS)
#define TSAdapt type(tTSAdapt)
#define TSTrajectory type(tTSTrajectory)
#define TSGLLEAdapt type(tTSGLLEAdapt)

#define TSType character*(80)
#define TSAdaptType character*(80)
#define TSTrajectoryType character*(80)
#define TSEquationType PetscEnum
#define TSConvergedReason PetscEnum
#define TSExactFinalTimeOption PetscEnum
#define TSSundialsType PetscEnum
#define TSProblemType PetscEnum
#define TSSundialsGramSchmidtType PetscEnum
#define TSSundialsLmmType PetscEnum
#define TSTrajectoryMemoryType PetscEnum

#define TSEULER           'euler'
#define TSBEULER          'beuler'
#define TSPSEUDO          'pseudo'
#define TSCN              'cn'
#define TSSUNDIALS        'sundials'
#define TSRK              'rk'
#define TSPYTHON          'python'
#define TSTHETA           'theta'
#define TSALPHA           'alpha'
#define TSGLLE            'glle'
#define TSSSP             'ssp'
#define TSARKIMEX         'arkimex'
#define TSROSW            'rosw'
#define TSEIMEX           'eimex'
#define TSRADAU5          'radau5'
#define TSMPRK            'mprk'

#define TSTRAJECTORYBASIC 'basic'

#define TSSSPType character*(80)
#define TSSSPRKS2  'rks2'
#define TSSSPRKS3  'rks3'
#define TSSSPRK104 'rk104'

#define TSGLLEAdaptType character*(80)
#define TSGLLEADAPT_NONE 'none'
#define TSGLLEADAPT_SIZE 'size'
#define TSGLLEADAPT_BOTH 'both'

#define TSAdaptType character*(80)
#define TSADAPTNONE    'none'
#define TSADAPTBASIC   'basic'
#define TSADAPTDSP     'dsp'
#define TSADAPTCFL     'cfl'
#define TSADAPTGLEE    'glee'
#define TSADAPTHISTORY 'history'

#define TSRKType character*(80)
#define TSRK1FE   '1fe'
#define TSRK2A    '2a'
#define TSRK3     '3'
#define TSRK3BS   '3bs'
#define TSRK4     '4'
#define TSRK5F    '5f'
#define TSRK5DP   '5dp'
#define TSRK5BS   '5bs'
#define TSRK6VR   '6vr'
#define TSRK7VR   '7vr'
#define TSRK8VR   '8vr'

#define TSMPRKType   character*(80)
#define TSMPRKPM2     'pm2'
#define TSMPRKP2      'p2'
#define TSMPRKP3      'p3'

#define TSARKIMEXType character*(80)
#define TSARKIMEX1BEE   '1bee'
#define TSARKIMEXA2     'a2'
#define TSARKIMEXL2     'l2'
#define TSARKIMEXARS122 'ars122'
#define TSARKIMEX2C     '2c'
#define TSARKIMEX2D     '2d'
#define TSARKIMEX2E     '2e'
#define TSARKIMEXPRSSP2 'prssp2'
#define TSARKIMEX3      '3'
#define TSARKIMEXBPR3   'bpr3'
#define TSARKIMEXARS443 'ars443'
#define TSARKIMEX4      '4'
#define TSARKIMEX5      '5'

#define TSDIRKType character*(80)
#define TSDIRKS212      's212'
#define TSDIRKES122SAL  'es122sal'
#define TSDIRKES213SAL  'es213sal'
#define TSDIRKES324SAL  'es324sal'
#define TSDIRKES325SAL  'es325sal'
#define TSDIRK657A      '657a'
#define TSDIRKES648SA   'es648sa'
#define TSDIRK658A      '658a'
#define TSDIRKS659A     's659a'
#define TSDIRK7510SAL   '7510sal'
#define TSDIRKES7510SA  'es7510sa'
#define TSDIRK759A      '759a'
#define TSDIRKS7511SAL  's7511sal'
#define TSDIRK8614A     '8614a'
#define TSDIRK8616SAL   '8616sal'
#define TSDIRKES8516SAL 'es8516sal'

#define TSROSWType character*(80)
#define TSROSW2M          '2m'
#define TSROSW2P          '2p'
#define TSROSWRA3PW       'ra3pw'
#define TSROSWRA34PW2     'ra34pw2'
#define TSROSWRODAS3      'rodas3'
#define TSROSWSANDU3      'sandu3'
#define TSROSWASSP3P3S1C  'assp3p3s1c'
#define TSROSWLASSP3P4S2C 'lassp3p4s2c'
#define TSROSWLLSSP3P3S2C 'llssp3p3s2c'
#define TSROSWARK3        'ark3'
#define TSROSWTHETA1      'theta1'
#define TSROSWTHETA2      'theta2'
#define TSROSWGRK4T       'grk4t'
#define TSROSWSHAMP4      'shamp4'
#define TSROSWVELDD4      'veldd4'
#define TSROSW4L          '4l'

#define TSEIMEXType character*(80)
#define TSEIMEXS2     's2'
#define TSEIMEXS3     's3'
#define TSEIMEXS4     's4'

#endif
