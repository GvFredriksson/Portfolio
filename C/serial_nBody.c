#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "particle.h"

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

void InitParticles(struct particle *particles[], int npart){
    for (int i=0; i<npart; i++) {
    particles[i] = malloc(sizeof(struct particle));
	particles[i]->x	  = 100 * Random();
	particles[i]->y	  = 100 * Random();
	particles[i]->mass = 5.0; //Earth = 5.972 × 10^24 kg
	particles[i]->xvel   = 0.;
	particles[i]->yvel   = 0.;
	particles[i]->xforce   = 0.;
	particles[i]->yforce   = 0.;
    }
}

double min(double a, double b){
    return a < b ? a : b;
}
double max (double a, double b){
    return a > b ? a : b;
}

void ComputeForces(struct particle* particleCalc, struct particle *particles[], double G, int npart ){
    double dx, dy, hypotenuse, hypo2; //x distance, y distance, xy distance, distance^2
	double accel;
	
    for(int i = 0; i < npart; ++i){
        if(particleCalc!=particles[i]){
	        dx = particles[i]->x - particleCalc->x;
	        dy = particles[i]->y - particleCalc->y;
            
            hypotenuse = sqrt(pow(dx,2) + pow(dy,2));
            hypo2 = pow(hypotenuse,2);
            
            accel = G*particles[i]->mass*particleCalc->mass;
    
		    particleCalc->xforce += (accel/hypo2)*dx;
		    particleCalc->yforce += (accel/hypo2)*dy;
        }
    }
}

void main(int argc, char const *argv[])
{
    struct particle **particles;
    int num_particles;
    int num_steps;

    double G = .6; //normaly 6.6738 * 10^(-11) (The gravitational constant)
    double ratio = 1.0;
    double xmin = 0.0;
    double xmax = 0.0;
    double ymin = 0.0;
    double ymax = 0.0;

    double deltaTime = 0.2; // How much time each step counts as
    num_particles = 100;
    num_steps = 1;

    particles = malloc(num_particles * sizeof(struct particle*));

    InitParticles(particles, num_particles);
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    for(int step = 0; step < num_steps; step++){
        for(int i = 0; i < num_particles; i++){
            xmin = min(xmin, particles[i]->x);
            xmax = max(xmax, particles[i]->x);
            ymin = min(ymin, particles[i]->y);
            ymax = max(ymax, particles[i]->y);
            particles[i]->xforce = 0.0;
            particles[i]->yforce = 0.0;
        }

        for(int i = 0; i < num_particles; i++){
            ComputeForces(particles[i], particles, G,num_particles);
        }

        for(int i = 0; i < num_particles; i++){
            particles[i]->x += deltaTime*particles[i]->xvel;
            particles[i]->y += deltaTime*particles[i]->yvel;

            particles[i]->xvel += particles[i]->xforce * (deltaTime/particles[i]->mass);
            particles[i]->yvel += particles[i]->yforce * (deltaTime/particles[i]->mass);
        }
    }
    gettimeofday(&end, NULL);

    //for(int i = 0; i < num_particles; i++){
    //    printf("%f     %f\n", particles[i]->x, particles[i]->y);
    //}
    fprintf(stdout,"Time:%f seconds\n", (double)((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec))/1000000);
}
