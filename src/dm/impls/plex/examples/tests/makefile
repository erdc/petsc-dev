
CFLAGS	        =
FFLAGS	        =
CPPFLAGS        =
FPPFLAGS        =
LOCDIR          = src/dm/impls/plex/examples/tests/
EXAMPLESC       = ex1.c
EXAMPLESF       = ex1f90.F ex2f90.F
MANSEC          = DM

include ${PETSC_DIR}/conf/variables
include ${PETSC_DIR}/conf/rules

ex1: ex1.o  chkopts
	-${CLINKER} -o ex1 ex1.o ${PETSC_DM_LIB}
	${RM} -f ex1.o

ex1f90: ex1f90.o  chkopts
	-${FLINKER} -o ex1f90 ex1f90.o  ${PETSC_DM_LIB}
	${RM} -f ex1f90.o

ex2f90: ex2f90.o  chkopts
	-${FLINKER} -o ex2f90 ex2f90.o  ${PETSC_DM_LIB}
	${RM} -f ex2f90.o

ex3: ex3.o  chkopts
	-${CLINKER} -o ex3 ex3.o ${PETSC_DM_LIB}
	${RM} -f ex3.o

#--------------------------------------------------------------------------
runex1:
	-@${MPIEXEC} -n 1 ./ex1 -dim 3 -ctetgen_verbose 4 -dm_view ::ascii_info_detail -info -info_exclude null > ex1_0.tmp 2>&1;\
	   if (${DIFF} output/ex1_0.out ex1_0.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex1, diffs above \n========================================="; fi ;\
	   ${RM} -f ex1_0.tmp
runex1_2:
	-@${MPIEXEC} -n 1 ./ex1 -dim 3 -ctetgen_verbose 4 -refinement_limit 0.0625 -dm_view ::ascii_info_detail -info -info_exclude null > ex1_1.tmp 2>&1;\
	   if (${DIFF} output/ex1_1.out ex1_1.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex1_2, diffs above \n========================================="; fi ;\
	   ${RM} -f ex1_1.tmp

runex1f90:
	-@${MPIEXEC} -n 1 ./ex1f90 > ex1f90_0.tmp 2>&1;\
	   if (${DIFF} output/ex1f90_0.out ex1f90_0.tmp) then true; \
	   else echo ${PWD} ; echo "Possible problem with with runex1f90, diffs above \n========================================="; fi;\
	   ${RM} -f ex1f90_0.tmp

runex2f90:
	-@${MPIEXEC} -n 1 ./ex2f90 > ex2f90_0.tmp 2>&1;\
	   if (${DIFF} output/ex2f90_0.out ex2f90_0.tmp) then true; \
	   else echo ${PWD} ; echo "Possible problem with with runex2f90, diffs above \n========================================="; fi;\
	   ${RM} -f ex2f90_0.tmp

runex3:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_0.tmp 2>&1;\
	   if (${DIFF} output/ex3_0.out ex3_0.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_0.tmp
runex3_2:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_1.tmp 2>&1;\
	   if (${DIFF} output/ex3_1.out ex3_1.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3_2, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_1.tmp
runex3_3:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_2.tmp 2>&1;\
	   if (${DIFF} output/ex3_2.out ex3_2.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3_3, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_2.tmp
runex3_4:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_3.tmp 2>&1;\
	   if (${DIFF} output/ex3_3.out ex3_3.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3_4, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_3.tmp
runex3_5:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_4.tmp 2>&1;\
	   if (${DIFF} output/ex3_4.out ex3_4.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3_5, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_4.tmp
runex3_6:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_5.tmp 2>&1;\
	   if (${DIFF} output/ex3_5.out ex3_5.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3_6, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_5.tmp
runex3_7:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_6.tmp 2>&1;\
	   if (${DIFF} output/ex3_6.out ex3_6.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3_7, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_6.tmp
runex3_8:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_7.tmp 2>&1;\
	   if (${DIFF} output/ex3_7.out ex3_7.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3_8, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_7.tmp
runex3_9:
	-@${MPIEXEC} -n 1 ./ex3 > ex3_8.tmp 2>&1;\
	   if (${DIFF} output/ex3_8.out ex3_8.tmp) then true ;  \
	   else echo ${PWD} ; echo "Possible problem with with runex3_9, diffs above \n========================================="; fi ;\
	   ${RM} -f ex3_8.tmp

TESTEXAMPLES_CTETGEN = ex1.PETSc runex1 runex1_2 ex1.rm ex3.PETSc runex3 runex3_2 runex3_3 runex3_4 runex3_5 runex3_6 runex3_7 runex3_8 runex3_9 ex3.rm
TESTEXAMPLES_FORTRAN = ex1f90.PETSc runex1f90 ex1f90.rm ex2f90.PETSc runex2f90 ex2f90.rm

include ${PETSC_DIR}/conf/test
