#include "quad_tree.h"
#include "particle.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


struct node* CreateNode(struct particle* particleP, double xmin, double xmax, double ymin, double ymax){
    struct node* root;
    root = malloc(sizeof(struct node));
    root->particleP=particleP;
    root->totalmass = particleP->mass;
    root->centerx = particleP->x;
    root->centery = particleP->y;
    root->xmin = xmin;
    root->xmax = xmax;
    root->ymin = ymin;
    root->ymax = ymax;
    root->NE = NULL;
    root->NW = NULL;
    root->SE = NULL;
    root->SW = NULL;
    root->diag = sqrt((pow(xmax - xmin, 2) + pow(ymax - ymin,2)));

    return root;
}

enum quadrant { NE, NW, SE, SW };
/* Calculate which quadrant x y belong to */
enum quadrant getQuadrant(double x, double y, double xmin, double xmax, double ymin, double ymax){
    if(y > (ymin + 0.5*(ymax - ymin))){
		if(x > (xmin + 0.5*(xmax - xmin))){
			return NE;
		} else{
			return NW;
		}
	} else {
        if(x > (xmin + 0.5*(xmax - xmin))){
			return SE;
		} else{
			return SW;
		}		
	}
}

void InsertParticle(struct particle* particleP, struct node* nodep){
    enum quadrant quad;
	double xmid, ymid;
	
	xmid = nodep->xmin + 0.5*(nodep->xmax - nodep->xmin);
	ymid = nodep->ymin + 0.5*(nodep->ymax - nodep->ymin);

    /* If nodep->particleP is not null its a leaf so the traversal has reached bottom */
    if(nodep->particleP != NULL){
        quad = getQuadrant(nodep->particleP->x, nodep->particleP->y, nodep->xmin, nodep->xmax, nodep->ymin, nodep->ymax);
        if(quad == NE){
            nodep->NE = CreateNode(nodep->particleP, xmid, nodep->xmax, ymid, nodep->ymax);
        }else if(quad == NW){
            nodep->NW = CreateNode(nodep->particleP, nodep->xmin, xmid, ymid, nodep->ymax);
        }else if(quad == SE){
            nodep->SE = CreateNode(nodep->particleP, nodep->xmin, xmid, nodep->ymin, ymid);
        }else if(quad == SW){
            nodep->SW = CreateNode(nodep->particleP, xmid, nodep->xmax, nodep->ymin, ymid);
        }
        nodep->particleP = NULL;
    }

    quad = getQuadrant(particleP->x, particleP->y, nodep->xmin, nodep->xmax, nodep->ymin, nodep->ymax);

    /* Add particlePs mass and coordinates to the node, for later approximations */
    nodep->centerx = (nodep->totalmass*nodep->centerx + particleP->mass*particleP->x)/(nodep->totalmass + particleP->mass);
    nodep->centery = (nodep->totalmass*nodep->centery + particleP->mass*particleP->y)/(nodep->totalmass + particleP->mass);
    nodep->totalmass += particleP->mass;

    /* Traverse deeper in the tree depending which quadrant particleP belongs to */
    if(quad == NE){
        if(nodep->NE == NULL){
            nodep->NE = CreateNode(particleP, xmid, nodep->xmax, ymid, nodep->ymax);
        } else {
            InsertParticle(particleP, nodep->NE);
        }
    }else if(quad == NW){
        if(nodep->NW == NULL){
            nodep->NW = CreateNode(particleP, nodep->xmin, xmid, ymid, nodep->ymax);
        } else {
            InsertParticle(particleP, nodep->NW);
        }
    }else if(quad == SE){
        if(nodep->SE == NULL){
            nodep->SE = CreateNode(particleP, nodep->xmin, xmid, nodep->ymin, ymid);
        } else {
            InsertParticle(particleP, nodep->SE);
        }
    }else if(quad == SW){
        if(nodep->SW == NULL){
            nodep->SW = CreateNode(particleP, xmid, nodep->xmax, nodep->ymin, ymid);
        } else {
            InsertParticle(particleP, nodep->SW);
        }
    }
}

/* Calculate the force applied to particleP over the entire tree */
void CalcForce(struct node* nodep, struct particle* particleP, double G, double ratiothreshold ){
    double dx, dy, hypotenuse, hypo2; //x distance, y distance, xy distance, distance^2
	double accel;
		
	dx = nodep->centerx - particleP->x;
	dy = nodep->centery - particleP->y;
    hypotenuse = sqrt(pow(dx,2) + pow(dy,2));
    hypo2 = pow(hypotenuse,2);
	
    /* Calculate forces iff the node is far from particleP(over ratiothreshold) then take the node force, 
    or if the node has a particle and that particle isn't particleP, then calculate it for that individual particle*/
	if((nodep->particleP!=particleP)&&(((hypotenuse/nodep->diag) > ratiothreshold) || (nodep->particleP))){

        /*acceleration = gravitational konstant * mass in node P * mass of particle P*/
        accel = G*nodep->totalmass*particleP->mass;

        /*The force on the body is exponentially dependent on the distance: force / distance^2  */
		particleP->xforce += (accel/hypo2)*dx;
		particleP->yforce += (accel/hypo2)*dy;		
		
	} else {
		if(nodep->NE) { CalcForce(nodep->NE, particleP, G, ratiothreshold); }
		if(nodep->NW) { CalcForce(nodep->NW, particleP, G, ratiothreshold); }
		if(nodep->SE) { CalcForce(nodep->SE, particleP, G, ratiothreshold); }
		if(nodep->SW) { CalcForce(nodep->SW, particleP, G, ratiothreshold); }
	}
	return;
}


/* Free the root and all its branches */
void DestroyTree(struct node* nodep){
    if(nodep!=NULL){
        if(nodep->NE!=NULL){
            DestroyTree(nodep->NE);
        }
        if(nodep->NW!=NULL){
            DestroyTree(nodep->NW);
        }
        if(nodep->SE!=NULL){
            DestroyTree(nodep->SE);
        }
        if(nodep->SW!=NULL){
            DestroyTree(nodep->SW);
        }
    }
    free(nodep);
}