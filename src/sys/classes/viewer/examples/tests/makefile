
CFLAGS	        =
FFLAGS	        =
CPPFLAGS        =
FPPFLAGS        =
LOCDIR  = src/sys/classes/viewer/examples/tests/
EXAMPLESC       = ex3.c
MANSEC          = Viewer

include ${PETSC_DIR}/conf/variables
include ${PETSC_DIR}/conf/rules

ex3: ex3.o   chkopts
	-${CLINKER} -o ex3 ex3.o  ${PETSC_SYS_LIB}
	${RM} -f ex3.o

runex3:
	-@${MPIEXEC} -n 1 ./ex3

TESTEXAMPLES_C_X	  =
TESTEXAMPLES_C_X_MPIUNI = ex3.PETSc runex3 ex3.rm

include ${PETSC_DIR}/conf/test

