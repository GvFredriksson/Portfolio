#ifndef QUAD_T
#define QUAD_T

struct node {
    double totalmass;
    double centerx, centery;
    double xmin, xmax;
    double ymin, ymax;
    double diag;
    struct particle* particleP;
    struct node* NE;
    struct node* NW;
    struct node* SE;
    struct node* SW;
};

#endif


struct node* CreateNode(struct particle* particleP, double xmin, double xmax, double ymin, double ymax);
void InsertParticle(struct particle* particleP, struct node* nodep);
void CalcForce(struct node* nodep, struct particle* particleP, double G, double ratiothreshold );
void DestroyTree(struct node* nodep);