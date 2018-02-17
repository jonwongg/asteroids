/*
 *	asteroids.c
 *
 *	An OpenGL implementation of the Asteroids video game
 *	By Jon Wong
 *
 *	Oct. 14, 2011
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD2DEG 180.0/M_PI
#define DEG2RAD M_PI/180.0

#define myTranslate2D(x,y) glTranslated(x, y, 0.0)
#define myScale2D(x,y) glScalef(x, y, 1.0)
#define myRotate2D(angle) glRotatef(RAD2DEG*angle, 0.0, 0.0, 1.0)


#define MAX_PHOTONS	8
#define MAX_ASTEROIDS	30
#define MAX_VERTICES	16
#define STAR_COUNT	30

#define drawCircle() glCallList(circle)


#define circleForm(a, b, c, d) (((a - b) * (a - b)) + ((c - d) * (c - d)))

/* -- display list for drawing a circle ------------------------------------- */

static GLuint	circle;

void
buildCircle() {
    GLint   i;

    circle = glGenLists(1);
    glNewList(circle, GL_COMPILE);
      glBegin(GL_POLYGON);
        for(i=0; i<40; i++)
            glVertex2d(cos(i*M_PI/20.0), sin(i*M_PI/20.0));
      glEnd();
    glEndList();
}


/* -- type definitions ------------------------------------------------------ */

typedef struct Coords {
	double		x, y;
} Coords;

typedef struct {
	double	x, y, r, phi, dx, dy, velocityMax, acceleration;
	Coords coords[3];
} Ship;

typedef struct {
	int	active;
	double	x, y, dx, dy;
} Photon;

typedef struct {
	int	active, nVertices;
	double	x, y, r, phi, dx, dy, dphi;
	Coords	coords[MAX_VERTICES];
} Asteroid;

typedef struct {
	int	active;
	double x, y;
} Star;

/* -- function prototypes --------------------------------------------------- */

static void	myDisplay(void);
static void	myTimer(int value);
static void	myKey(unsigned char key, int x, int y);
static void	keyPress(int key, int x, int y);
static void	keyRelease(int key, int x, int y);
static void	myReshape(int w, int h);

static void	init(void);
static void	initAsteroid(Asteroid *a, double x, double y, double size);
static void	drawShip(Ship *s);
static void	drawPhoton(Photon *p);
static void	drawAsteroid(Asteroid *a);
static void	drawStar();

static double	myRandom(double min, double max);


/* -- global variables ------------------------------------------------------ */

static int	up=0, down=0, left=0, right=0;	/* state of cursor keys */
static int	gameStart=0, circularAsteroids=0,
		shipVertexCrash=0, isPaused=0, points=0;
static double	xMax, yMax;
static Ship	ship;
static Photon	photons[MAX_PHOTONS];
static Asteroid	asteroids[MAX_ASTEROIDS];
static Star	stars[STAR_COUNT];

/* -- main ------------------------------------------------------------------ */

int
main(int argc, char *argv[])
{
    srand((unsigned int) time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Asteroids");
    buildCircle();
    glutDisplayFunc(myDisplay);
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(myKey);
    glutSpecialFunc(keyPress);
    glutSpecialUpFunc(keyRelease);
    glutReshapeFunc(myReshape);
    glutTimerFunc(33, myTimer, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    init();
    
    glutMainLoop();
    
    return 0;
}


/* -- callback functions ---------------------------------------------------- */

void
myDisplay()
{
    /*
     *	display callback function
     */

    int	i;

    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();
    //identity matrix is pushed on to the matrix stack
    glPushMatrix();

    //transformations applied to ship and it is drawn
    myTranslate2D(ship.x, ship.y);
    myRotate2D(ship.phi);

    drawShip(&ship);
    //pops matrix
    glPopMatrix();

    //draws starry background
    for (i=0; i<STAR_COUNT; i++)
    {
        if(stars[i].active)
	{
  	   glPushMatrix();
   	   myTranslate2D(stars[i].x, stars[i].y);
	   drawStar();
	   glPopMatrix();
	}
    }

    //each photon that has been fired is drawn
    for (i=0; i<MAX_PHOTONS; i++)
    {
    	if (photons[i].active)
	{
	    glPushMatrix();
	    myTranslate2D(photons[i].x, photons[i].y);
            drawPhoton(&photons[i]);
	    glPopMatrix();
	}
    }

    //draws each asteroid
    for (i=0; i<MAX_ASTEROIDS; i++)
    {
    	if (asteroids[i].active)
	{
	    glPushMatrix();
	    myTranslate2D(asteroids[i].x, asteroids[i].y);
	    myRotate2D(asteroids[i].phi);
            drawAsteroid(&asteroids[i]);
	    glPopMatrix();
	}
    }


    glutSwapBuffers();
}

void
myTimer(int value)
{
    /*
     *	timer callback function
     */
   int i, j, k, length;

   //displays text for a new game
   if(gameStart == 0)
   {
      char welcome[] = "Press S to start!";
      //glRasterPos2f(x, y) sets position of the text
      glRasterPos2f(38, 60);
      length = (int)strlen(welcome);
      //individual characters of the string array are processed
      for(i = 0; i < length; i++)
         glutBitmapCharacter(GLUT_BITMAP_8_BY_13, welcome[i]);
   }


   if(gameStart == 1 && isPaused%2 == 0 && shipVertexCrash == 0)
   {
    //displays the score text
    char score[10], score_str[] = "Score: ";
    glRasterPos2f(1, 3);
    length = (int)strlen(score_str);
    for(i = 0; i < length; i++)
       glutBitmapCharacter(GLUT_BITMAP_8_BY_13, score_str[i]);
    //displays the score digits
    sprintf(score, "%i", points);
    glRasterPos2f(8, 3);
    length = (int)strlen(score);
    for(i = 0; i < length; i++)
       glutBitmapCharacter(GLUT_BITMAP_8_BY_13, score[i]);


    /* advance the ship */
    //checks for turns
    if(left == 1)
       ship.phi += 0.08;

    if(right == 1)
       ship.phi -= 0.08;

    //adjusts velocity of the ship toward the direction it is facing
    if(up == 1)
    {
       ship.dx -= ship.acceleration * sin(ship.phi) * 0.30;
       ship.dy += ship.acceleration * cos(ship.phi) * 0.30;

       if(sqrt(ship.dx*ship.dx + ship.dy*ship.dy) > ship.velocityMax)
       {
	  ship.dx += ship.acceleration * sin(ship.phi) * 0.30;
	  ship.dy -= ship.acceleration * cos(ship.phi) * 0.30;
       }
    }

    //adjusts velocity of the ship away from the direction it is facing
    if(down == 1)
    {
       ship.dx += ship.acceleration * sin(ship.phi) * 0.30;
       ship.dy -= ship.acceleration * cos(ship.phi) * 0.30;

       if(sqrt(ship.dx*ship.dx + ship.dy*ship.dy) > ship.velocityMax)
       {
	  ship.dx -= ship.acceleration * sin(ship.phi) * 0.30;
	  ship.dy += ship.acceleration * cos(ship.phi) * 0.30;
       }

    }

    //moves the ship
    ship.x += ship.dx * 0.30;
    ship.y += ship.dy * 0.30;

    //wrap-around handling for the ship
    if(ship.x > xMax)
       ship.x = 0;
    if(ship.x < 0)
       ship.x = xMax;
    if(ship.y > yMax)
       ship.y = 0;
    if(ship.y < 0)
       ship.y = yMax;


    /* advance photon laser shots, eliminating those that have gone past
      the window boundaries */
    //every photon is checked in similar fashion to the ship against the borders
    for(i = 0; i < MAX_PHOTONS; i++)
    {
	if(photons[i].active)
	{
	    photons[i].x += photons[i].dx * 0.30;
	    photons[i].y += photons[i].dy * 0.30;

	    if(photons[i].x > xMax || photons[i].y > yMax || photons[i].x < 0 || photons[i].y < 0)
	    {
	       photons[i].active = 0;
	       photons[i].x = -5;
	       photons[i].y = -5;
	    }
	}
    }

    /* advance asteroids */
    for(i = 0; i < MAX_ASTEROIDS; i++)
    {
	if(asteroids[i].active)
	{
            //moves asteroids by their linear and rotational velocities
	    asteroids[i].x += asteroids[i].dx * 0.30;
	    asteroids[i].y += asteroids[i].dy * 0.30;
	    asteroids[i].phi += asteroids[i].dphi;
	    
            //wrap-around handling for the asteroids
	    if(asteroids[i].x > xMax)
	       asteroids[i].x = 0;
	    if(asteroids[i].x < 0)
	       asteroids[i].x = xMax;
	    if(asteroids[i].y > yMax)
	       asteroids[i].y = 0;
	    if(asteroids[i].y < 0)
	       asteroids[i].y = yMax;
	}
    }


    /* test for and handle collisions */
    for(i = 0; i < MAX_PHOTONS; i++)
    {
	for(j = 0; j < MAX_ASTEROIDS; j++)
	{
	   //tests if the bullet has entered the radius of the asteroid
	   if(circleForm(photons[i].x, asteroids[j].x, photons[i].y, asteroids[j].y) <= pow(asteroids[j].r, 2))
	   {
                    //points are awarded for the destruction
		    points += 40.0 * asteroids[j].r;
	            photons[i].active = 0;
	            photons[i].x = -5;
	            photons[i].y = -5;

		    //checks size of the asteroid and creates new ones if necessary
	            if(asteroids[j].r > 3.0)
	            {   
		       int spawn_count = 0;
		       for(k = 0; k < MAX_ASTEROIDS; k++)
		       {
		          if(!asteroids[k].active && spawn_count < (int)myRandom(2.0, 4.0))
		          {
		             asteroids[k].active = 1;
	     	             initAsteroid(&asteroids[k], asteroids[j].x, asteroids[j].y, 1.0);
		             spawn_count++;
		          }
		       }
	            }
	      
	            asteroids[j].active = 0;
	            asteroids[j].x = -10;
	            asteroids[j].y = -10;
	   }
	}
    }


    for(i = 0; i < MAX_ASTEROIDS; i++)
    {
       //checks all drawn asteroids
       if(asteroids[i].active)
       {
	  //performs circle-circle test
	  if(circleForm(ship.x, asteroids[i].x, ship.y, asteroids[i].y) <= pow(ship.r + asteroids[i].r, 2))
	  {
	     //ship has crashed
	     shipVertexCrash = 1;
	  }
       }
    }

   }

   //a pause has occurred
   if(gameStart == 1 && isPaused%2 == 1)
   {
      char paused[] = "Game paused. Press P to resume.";
      glRasterPos2f(35, 60);
      length = (int)strlen(paused);
      for(i = 0; i < length; i++)
         glutBitmapCharacter(GLUT_BITMAP_8_BY_13, paused[i]);
   }

   //the player has crashed
   if(gameStart == 1 && shipVertexCrash == 1)
   {
      char gameOver[] = "GAME OVER";
      glRasterPos2f(45, 60);
      length = (int)strlen(gameOver);
      for(i = 0; i < length; i++)
         glutBitmapCharacter(GLUT_BITMAP_8_BY_13, gameOver[i]);

      init();

      gameStart = 0;
      shipVertexCrash = 0;
   }

    glutPostRedisplay();
    glutTimerFunc(33, myTimer, value);		/* 30 frames per second */
}

void
myKey(unsigned char key, int x, int y)
{
    /*
     *	keyboard callback function; add code here for firing the laser,
     *	starting and/or pausing the game, etc.
     */
    int i;

    //toggles circular asteroids
    if(key == 't' || key == 'T')
       circularAsteroids++;

    //spacebar fires the photon laser
    if(key == ' ')
    {
       for(i = 0; i < MAX_PHOTONS; i++)
       {
	  if(!photons[i].active)
	  {
             photons[i].active = 1;
             photons[i].x = ship.x - ship.r * sin(ship.phi);
             photons[i].y = ship.y + ship.r * cos(ship.phi);
             photons[i].dx = -9 * sin(ship.phi);
             photons[i].dy = 9 * cos(ship.phi);
	     break;
	  }
       }
    }

    //key to begin the game
    if((key == 's' || key == 'S') && gameStart == 0)
    {
       //the game has begun and asteroids are drawn
       gameStart = 1;
       for(i = 0; i < MAX_ASTEROIDS/3; i++)
       {
	  double spawnRand = myRandom(0.0, 4.1);
	  double xSpawn, ySpawn;
	  switch((int)spawnRand)
	  {
	     case 1:
		xSpawn = myRandom(0, 100);
		ySpawn = 0;
		break;

	     case 2:
		xSpawn = 0;
		ySpawn = myRandom(0, 100);
		break;

	     case 3:
		xSpawn = myRandom(0, 100);
		ySpawn = yMax;
		break;

	     case 4:
		xSpawn = xMax;
		ySpawn = myRandom(0, 100);
		break;
	  }

	  if(!asteroids[i].active)
	     initAsteroid(&asteroids[i], xSpawn, ySpawn, myRandom(2.5, 3.8));
       }
    }

    //increments the pause counter
    if((key == 'p' || key == 'P') && gameStart == 1)
    {
       isPaused++;
    }
}

void
keyPress(int key, int x, int y)
{
    /*
     *	this function is called when a special key is pressed; we are
     *	interested in the cursor keys only
     */

    switch (key)
    {
        case 100:
            left = 1; break;
        case 101:
            up = 1; break;
	case 102:
            right = 1; break;
        case 103:
            down = 1; break;
    }
}

void
keyRelease(int key, int x, int y)
{
    /*
     *	this function is called when a special key is released; we are
     *	interested in the cursor keys only
     */

    switch (key)
    {
        case 100:
            left = 0; break;
        case 101:
            up = 0; break;
	case 102:
            right = 0; break;
        case 103:
            down = 0; break;
    }
}

void
myReshape(int w, int h)
{
    /*
     *	reshape callback function; the upper and lower boundaries of the
     *	window are at 100.0 and 0.0, respectively; the aspect ratio is
     *  determined by the aspect ratio of the viewport
     */

    xMax = 100.0*w/h;
    yMax = 100.0;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, xMax, 0.0, yMax, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
}


/* -- other functions ------------------------------------------------------- */

void
init()
{
    /*
     * set parameters including the numbers of asteroids and photons present,
     * the maximum velocity of the ship, the velocity of the laser shots, the
     * ship's coordinates and velocity, etc.
     */
    int i;

    ship.x = 50;
    ship.y = 50;
    ship.r = 1.2;
    ship.velocityMax = 4.8;
    ship.acceleration = 0.6;
    ship.phi = 0;
    ship.dx = 0;
    ship.dy = 0;

    //coordinates of each ship vertex
    ship.coords[0].x = 0.0;
    ship.coords[0].y = ship.r;
    ship.coords[1].x = ship.r/2;
    ship.coords[1].y = -ship.r;
    ship.coords[2].x = -ship.r/2;
    ship.coords[2].y = -ship.r;


    //clears all asteroids
    for(i = 0; i < MAX_ASTEROIDS; i++)
    {
       if(asteroids[i].active == 1)
       {
          asteroids[i].active = 0;
	  asteroids[i].x = -10;
	  asteroids[i].y = -10;
       }
    }

    //clears all photons
    for(i = 0; i < MAX_PHOTONS; i++)
    {
       if(photons[i].active == 1)
       {
	  photons[i].active = 0;
	  photons[i].x = -5;
	  photons[i].y = -5;
       }
    }

    //draws the background
    for(i = 0; i < STAR_COUNT; i++)
    {
       if(stars[i].active == 0)
       {
	  stars[i].active = 1;
	  stars[i].x = myRandom(0.0, 100.0);
	  stars[i].y = myRandom(0.0, 100.0);
       }
    }

    //points are reset
    points = 0;
}

void
initAsteroid(
	Asteroid *a,
	double x, double y, double size)
{
    /*
     *	generate an asteroid at the given position; velocity, rotational
     *	velocity, and shape are generated randomly; size serves as a scale
     *	parameter that allows generating asteroids of different sizes; feel
     *	free to adjust the parameters according to your needs
     */

    double	theta, r, p1, p2, largest_r;
    int		i;
        
    a->x = x;
    a->y = y;
    a->phi = 0.0;
    a->dx = myRandom(-2.0, 2.0);
    a->dy = myRandom(-2.0, 2.0);
    a->dphi = myRandom(-0.2, 0.2);
   
    largest_r = 0.0;

 
    a->nVertices = 6+rand()%(MAX_VERTICES-6);
    for (i=0; i<=a->nVertices; i++)
    {
	theta = 2.0*M_PI*i/a->nVertices;
	r = size*myRandom(2.0, 3.0);
	if(r > largest_r)
	   largest_r = r;
	a->coords[i].x = -r*sin(theta);
	a->coords[i].y = r*cos(theta);
    }

    //will keep track of the largest radius
    a->r = largest_r;
    a->active = 1;
}

void
drawShip(Ship *s)
{
   glBegin(GL_TRIANGLES);

   glVertex2f(ship.coords[0].x, ship.coords[0].y);
   glVertex2f(ship.coords[1].x, ship.coords[1].y);
   glVertex2f(ship.coords[2].x, ship.coords[2].y);

   glEnd();
}

void
drawPhoton(Photon *p)
{

   glBegin(GL_TRIANGLES);
   glVertex2f(0.0, 0.14);
   glVertex2f(-0.14, -0.14);
   glVertex2f(0.14, -0.14);
   glEnd();
}

void
drawAsteroid(Asteroid *a)
{
   if(circularAsteroids%2 == 0)
   {
      int i;

      glBegin(GL_POLYGON);

      for(i = 0; i <= a->nVertices; i++)
	 glVertex2f(a->coords[i].x, a->coords[i].y);

      glEnd();
   }

   else
      drawCircle();
}

void
drawStar()
{
   int i;

   //draws stars for the background
   glBegin(GL_LINE_LOOP);
   for(i = 0; i < 10; i++)
   {
      double separation = M_PI/2 + i*2*M_PI/10;
      double divisor = 4;
      if(i%2 == 0)
         divisor = 1;
      else
	 divisor = 4;

      glVertex2f((cos(separation)/divisor), (sin(separation)/divisor));
   }
   glEnd();
}


/* -- helper function ------------------------------------------------------- */

double
myRandom(double min, double max)
{
	double	d;
	
	/* return a random number uniformly draw from [min,max] */
	d = min+(max-min)*(rand()%0x7fff)/32767.0;
	
	return d;
}
