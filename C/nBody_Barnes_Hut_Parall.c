#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "particle.h"
#include "quad_tree.h"



/*
 * pRNG based on http://www.cs.wm.edu/~va/software/park/park.html
 */
#define MODULUS    2147483647
#define MULTIPLIER 48271
#define DEFAULT    123456789

static long seed = DEFAULT;
double dt, dt_old;

double Random(void)
/* ----------------------------------------------------------------
 * Random returns a pseudo-random real number uniformly distributed 
 * between 0.0 and 1.0. 
 * ----------------------------------------------------------------
 */
{
  const long Q = MODULUS / MULTIPLIER;
  const long R = MODULUS % MULTIPLIER;
        long t;

  t = MULTIPLIER * (seed % Q) - R * (seed / Q);
  if (t > 0) 
    seed = t;
  else 
    seed = t + MODULUS;
  return ((double) seed / MODULUS);
}

//----------------------------------------------------------------------

void InitParticles(struct particle *particles[], int npart){
    for (int i=0; i<npart; i++) {
	particles[i]->x	  = 100 * Random();
	particles[i]->y	  = 100 * Random();
	particles[i]->mass = 5.0; //Earth = 5.972 × 10^24 kg
	particles[i]->xvel   = 0.0;
	particles[i]->yvel   = 0.0;
	particles[i]->xforce   = 0.0;
	particles[i]->yforce   = 0.0;
    }
}

double min(double a, double b){
    return a < b ? a : b;
}
double max (double a, double b){
    return a > b ? a : b;
}

struct particle **particles;
int size;
int my_rank;
int num_particles;
int num_steps;

void main(int argc, char **argv){
    double G = .6; //normaly 6.6738 * 10^(-11) (The gravitational constant)
    double ratio = 0.5;
    double xmin = 0.0;
    double xmax = 0.0;
    double ymin = 0.0;
    double ymax = 0.0;

    num_particles = 100;
    num_steps = 1;

    double deltaTime = 0.2; // How much time each step counts as
    double time;
    double startTime;
    double endTime;
    struct node *root_node;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    MPI_Bcast(&num_particles, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num_steps, 1, MPI_INT, 0, MPI_COMM_WORLD);

    particles = malloc(num_particles * sizeof(struct particle*));
    for(int i = 0; i<num_particles; ++i){
        particles[i] = malloc(sizeof(struct particle));
    }
    if(my_rank==0){
        InitParticles(particles, num_particles);
    }

    for(int i = 0; i < num_particles; i++){
        MPI_Bcast(&particles[i]->x, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&particles[i]->y, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&particles[i]->xvel, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&particles[i]->yvel, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&particles[i]->xforce, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&particles[i]->yforce, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&particles[i]->mass, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }   

    double setForceZeroTime;
    double treeTime;
    double forceTime;
    double bcastTime;
    double calcTime;
    double destroyTreeTime;

    startTime = MPI_Wtime();
    /* Simulate for num_steps steps */
    for(int step = 0; step < num_steps; step++){

        setForceZeroTime = MPI_Wtime();
        /* Set forces to zero and get the simulation borders, keeps the border dynamic */
        for(int i = 0; i < num_particles; i++){
            xmin = min(xmin, particles[i]->x);
            xmax = max(xmax, particles[i]->x);
            ymin = min(ymin, particles[i]->y);
            ymax = max(ymax, particles[i]->y);
            particles[i]->xforce = 0.0;
            particles[i]->yforce = 0.0;
        }
        
        treeTime = MPI_Wtime();
        /* Build the tree */
        root_node = CreateNode(particles[0], xmin, xmax, ymin, ymax);
        for(int i = 1; i < num_particles; i++){
            InsertParticle(particles[i], root_node);
        }

        forceTime = MPI_Wtime();
        /* Calculate the forces beeing directed at each particle */
        for(int i = my_rank; i < num_particles; i+=size){
            CalcForce(root_node, particles[i], G, ratio);
        }

        bcastTime = MPI_Wtime();
        /* Broadcast the forces so every thread has the same */
        for(int i = 0; i < num_particles; i++){
            MPI_Bcast(&particles[i]->xforce, 1, MPI_DOUBLE, i%size, MPI_COMM_WORLD);
            MPI_Bcast(&particles[i]->yforce, 1, MPI_DOUBLE, i%size, MPI_COMM_WORLD);
        }

        calcTime = MPI_Wtime();
        /* Update velocities of all particles */
        for(int i = 0; i < num_particles; i++){
            particles[i]->x += deltaTime*particles[i]->xvel;
            particles[i]->y += deltaTime*particles[i]->yvel;

            particles[i]->xvel += particles[i]->xforce * (deltaTime/particles[i]->mass);
            particles[i]->yvel += particles[i]->yforce * (deltaTime/particles[i]->mass);
        }

        destroyTreeTime = MPI_Wtime();
        /* Destroy the tree so it can be rebuilt next iteration */
        DestroyTree(root_node);

    }
    endTime = MPI_Wtime();
    if(my_rank==0){
        printf("\nSet forces to zero took %f seconds\n", treeTime-setForceZeroTime);
        printf("\nBuild Tree took %f seconds\n", forceTime-treeTime);
        printf("\nCalc forces took %f seconds\n", bcastTime-forceTime);
        printf("\nBcast forces took %f seconds\n", calcTime-bcastTime);
        printf("\nCalc speeds took %f seconds\n", destroyTreeTime-calcTime);
        printf("\nDestroy the tree took %f seconds\n", endTime-destroyTreeTime);
        printf("\nSimulation took %f seconds\n", endTime-startTime);
        for(int i = 0; i < num_particles; i++){
            //printf("%f     %f\n", particles[i]->x, particles[i]->y);
        }
    }

    MPI_Finalize();
}