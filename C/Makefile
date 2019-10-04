########################################################################
# Makefile for Project
#
########################################################################

CC         =  mpicc
CCFLAGS    =  -O3
LIBS       =  -lmpi -lm


all:
	@echo "Usage: make file for nbody simulation project"

Paral:		nBody_Barnes_Hut_Parall.c
	$(CC) $(CCFLAGS) -o Paral nBody_Barnes_Hut_Parall.c quad_tree.c $(LIBS)

Serial:		nBody_Barnes_Hut_Serial.c
	$(CC) $(CCFLAGS) -o Serial nBody_Barnes_Hut_Serial.c quad_tree.c $(LIBS)

Serial2:		serial_nBody.c
	$(CC) $(CCFLAGS) -o Serial2 serial_nBody.c $(LIBS)

clean:
	$(RM) Paral Serial Serial2


