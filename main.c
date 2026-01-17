#include <SDL3/SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1400
#define SCREEN_HEIGHT 800
#define NUMBER_OF_PARTICLES 100
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x00000000
#define COLOR_GREEN 0x003ca123
#define COLOR_BLUE 0x004d91ff
#define COLOR_RED 0xffff3c3c
#define COLOR_YELLOW 0xffffe100
#define COLOR_MAGENTA 0xffff00ff
#define COLOR_CYAN 0xff00ffff
#define COLOR_ORANGE 0xffffa500
#define COLOR_PURPLE 0xff800080
#define COLOR_LIME 0xff32cd32
#define COLOR_PINK 0xffff69b4
#define NUM_COLORS 8
#define SPEED_LIMIT 8


Uint32 colors[NUM_COLORS] = {COLOR_RED, COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN, COLOR_ORANGE, COLOR_PURPLE, COLOR_LIME, COLOR_PINK};

int gBestIndex = 0;     // index of particle with best fitness
double gBestValue = INFINITY;  // best fitness so far
double gBestX, gBestY;  // position of global best

struct Circle
{
    double x, y;
    double r;
};

struct Particle
{
    double x, y;
    double vx, vy;
    double pBest;
    double pBestX;
    double pBestY;
    bool isGbest;
    Uint32 color;
};

void limitSpeed(struct Particle *p) {
    double speed = sqrt(p->vx * p->vx + p->vy * p->vy);

    if (speed > SPEED_LIMIT) {
        double scale = SPEED_LIMIT / speed;
        p->vx *= scale;
        p->vy *= scale;
    }
}

void generateParticles(struct Particle particles[])
{
    for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
        // spawn inside screen bounds
        particles[i].x = rand() % SCREEN_WIDTH;
        particles[i].y = rand() % SCREEN_HEIGHT;

        // random velocity init for each particle
        particles[i].vx = ((double)rand() / RAND_MAX) - 0.5;
        particles[i].vy = ((double)rand() / RAND_MAX) - 0.5;

        // PBest
        particles[i].pBest = INFINITY;
        particles[i].pBestX = particles[i].x;
        particles[i].pBestY = particles[i].y;
        
        // random color
        particles[i].color = colors[rand() % NUM_COLORS];
    }
}

void drawCircle(struct Circle circle, SDL_Surface *surface, Uint32 color)
{
    double radius_squared = pow(circle.r, 2);
    for (double x = circle.x - circle.r; x <= circle.x + circle.r; x++)
    {
        for (double y = circle.y - circle.r; y <= circle.y + circle.r; y++)
        {
            // if it satisfies the eqn of a circle
            double distance_squared = pow(x - circle.x, 2) + pow(y - circle.y, 2);
            if (distance_squared <= radius_squared)
            {
                SDL_Rect point = {x, y, 1, 1};
                SDL_FillSurfaceRect(surface, &point, color);
            }
        }
    }
}

void moveCircle(struct Circle *circle)
{
    float cursor_x, cursor_y;
    SDL_GetMouseState(&cursor_x, &cursor_y);
    circle->x = cursor_x;
    circle->y = cursor_y;
}

void drawParticles(SDL_Surface *surface, struct Particle particles[]) {
    struct Circle point;
    for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
        point.x = (int)particles[i].x;
        point.y = (int)particles[i].y;
        if (particles[i].isGbest){
            point.r = 8;
            drawCircle(point, surface, COLOR_BLUE);
        }
        else {
            point.r = 2;
            drawCircle(point, surface, particles[i].color);

        }
    }
}


void enforceMinimumDistance(struct Particle particles[], double minDist) {
    for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
        for (int j = i + 1; j < NUMBER_OF_PARTICLES; j++) {
            double dx = particles[i].x - particles[j].x;
            double dy = particles[i].y - particles[j].y;
            double dist = sqrt(dx*dx + dy*dy);

            if (dist < minDist && dist > 0) {
                double overlap = (minDist - dist) / 2;
                double nx = dx / dist;
                double ny = dy / dist;
                particles[i].x += nx * overlap;
                particles[i].y += ny * overlap;
                particles[j].x -= nx * overlap;
                particles[j].y -= ny * overlap;
            }
        }
    }
}

void updateBests(struct Particle particles[], struct Circle goalCircle) {
    for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
        // distance to goal
        double dx = particles[i].x - goalCircle.x;
        double dy = particles[i].y - goalCircle.y;
        double fitness = sqrt(dx*dx + dy*dy);

        // update pBest
        if (fitness < particles[i].pBest || particles[i].pBest == 0) {
            particles[i].pBest = fitness;
            particles[i].pBestX = particles[i].x;
            particles[i].pBestY = particles[i].y;
        }

        // update gBest
        if (fitness < gBestValue) {
            gBestValue = fitness;
            gBestIndex = i;
            gBestX = particles[i].x;
            gBestY = particles[i].y;
        }
    }

    // mark gBest particle
    for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
        particles[i].isGbest = (i == gBestIndex);
    }
}

void updateVelocity(struct Particle particles[]) {
    double jitter = 0.1; // jitter factor in velocity
    double c1 = 0.8;  // cognitive OR toward pBest
    double c2 = 0.1; // social OR toward gBest
    double w = 3; // inertia (gives momentum to the particles)

    for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
        double r1 = (double)rand() / RAND_MAX;
        double r2 = (double)rand() / RAND_MAX;
        // apply inertia
        particles[i].vx *= w;
        particles[i].vy *= w;
        // move toward pBest
        particles[i].vx += c1 * r1 * (particles[i].pBestX - particles[i].x);
        particles[i].vy += c1 * r1 * (particles[i].pBestY - particles[i].y);
        //move toward gbest if not the gBest particle itself
        if (!particles[i].isGbest) {
            particles[i].vx += c2 * r2 * (gBestX - particles[i].x);
            particles[i].vy += c2 * r2 * (gBestY - particles[i].y);
        }
        // add random wobble to the velocity
        particles[i].vx += ((double)rand()/RAND_MAX - 0.5) * 0.5 * jitter;;
        particles[i].vy += ((double)rand()/RAND_MAX - 0.5) * 0.5 * jitter;;

        //speed limit
        limitSpeed(&particles[i]);
    }
}

void updatePosition(struct Particle particles[]) {
    for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        // keep inside screen
        if (particles[i].x < 0) {
            particles[i].x = 0;
            particles[i].vx *= -0.8;
        }
        if (particles[i].x > SCREEN_WIDTH) {
            particles[i].x = SCREEN_WIDTH;
            particles[i].vx *= -0.8;
        }
        if (particles[i].y < 0) {
            particles[i].y = 0;
            particles[i].vy *= -0.8;
        }
        if (particles[i].y > SCREEN_HEIGHT) {
            particles[i].y = SCREEN_HEIGHT;
            particles[i].vy *= -0.8;
        }
    }
    // repel each other
    enforceMinimumDistance(particles, 20);
}

int main() {
    printf("Particle Swarm Optimization Visualization");

    // initializing SDL
    SDL_Init(SDL_INIT_VIDEO); 
    SDL_Window *window = SDL_CreateWindow("PSO Visualization", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    
    // goal circle
    struct Circle goalCircle = {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2, 20};
    struct Particle particles[NUMBER_OF_PARTICLES]; // array to store the particles
    generateParticles(particles);
    int quit = 0;
    SDL_Event event;
    int mousePressed = 0;
    int mouseMoving = 0;
    // main loop
    while (!quit)
    {
        // re-draw the surface black
        SDL_Rect black_rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_FillSurfaceRect(surface, &black_rect, COLOR_BLACK);

        updateBests(particles, goalCircle);

        // draw food source (goalCircle)
        drawCircle(goalCircle, surface, COLOR_GREEN);
        // update particles velocity
        updateVelocity(particles);

        // update particles position
        updatePosition(particles);

        //draw particles based on new position
        drawParticles(surface, particles);


        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    SDL_Log("SDL Quit.");
                    quit = 1;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    mousePressed = 1;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    mousePressed = 0;
                    break;
                default:
                    break;
            }
            // if the mouse button is pressed and moving
            if (mousePressed)
            {
                moveCircle(&goalCircle);
                updateBests(particles, goalCircle);
            }
                for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
                    particles[i].pBest = INFINITY;
                }
                gBestValue = INFINITY;

        }
        //updating the window surface
        SDL_UpdateWindowSurface(window);
        SDL_Delay(10);
    }
    return 0;
}

