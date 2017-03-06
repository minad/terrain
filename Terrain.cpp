#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_opengl.h"
#include "Matrix.h"
#include "Vector.h"

struct GLDriver {
#define GL_PROC(ret, name, args)\
ret (*name)args;
#include "GLFuncs.h"
#undef GL_PROC
};

enum {
	SCREEN_WIDTH = 640,
	SCREEN_HEIGHT = 480,
	SCREEN_BPP = 32,
	AREA_SIZE = 256,
};

SDL_Surface *surface;
GLDriver glDriver;
GLDriver* driver = &glDriver;

/* Ambient Light Values */
GLfloat LightAmbient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
/* Diffuse Light Values */
GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
/* Light Position */
GLfloat LightPosition[] = { 0.0f, 0.0f, 2.0f, 1.0f };

short height[AREA_SIZE][AREA_SIZE];

void
quit (int exitCode)
{
	SDL_Quit ();
	exit (exitCode);
}

void
resizeWindow (int width, int height)
{
	if (height == 0)
		height = 1;
	driver->glViewport (0, 0, width, height);
	driver->glMatrixMode(GL_PROJECTION);

	driver->glLoadMatrixf(perspectiveMatrix
			      (45.0f, (float) width / height, 0.1f,
			       200.0f));

	driver->glMatrixMode(GL_MODELVIEW);
	driver->glLoadIdentity();
}

void
handleKeyPress (SDL_keysym * keysym)
{
	switch (keysym->sym) {
	case SDLK_ESCAPE:
		quit (0);
		break;

	case SDLK_F1:
		SDL_WM_ToggleFullScreen (surface);
		break;
	}
}

bool initGLDriver() {
#define GL_PROC(ret, name, args)\
	driver->name = (ret (*)args)SDL_GL_GetProcAddress(#name);
#include "GLFuncs.h"
#undef GL_PROC
	return true;
}

void initHeights() {
    float h;
    for (int x = 0; x < AREA_SIZE; ++x) {
        for(int y = 0; y < AREA_SIZE; ++y) {
            height[x][y] = (short)(10.0 * sin(x/24.0) + 7.0 * cos((y-50.0)/18.0));
        }
    }
}

bool
initGL ()
{
	if (!initGLDriver())
		return false;

	initHeights();

	driver->glShadeModel (GL_SMOOTH);
	driver->glClearColor (0, 0, 0, 0);
	driver->glClearDepth (1);
	driver->glEnable (GL_DEPTH_TEST);
	//driver->glEnable (GL_BLEND);
	//driver->glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	//driver->glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//driver->glHint (GL_POINT_SMOOTH_HINT, GL_NICEST);

    /* Setup The Ambient Light */
    driver->glLightfv( GL_LIGHT1, GL_AMBIENT, LightAmbient );

    /* Setup The Diffuse Light */
    driver->glLightfv( GL_LIGHT1, GL_DIFFUSE, LightDiffuse );

    /* Position The Light */
    driver->glLightfv( GL_LIGHT1, GL_POSITION, LightPosition );

    /* Enable Light One */
    driver->glEnable( GL_LIGHT1 );
driver->glEnable( GL_LIGHTING );

	return true;
}

void
drawScene (float frameTime)
{
	driver->glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//driver->glLoadIdentity();
	driver->glLoadMatrixf(translationMatrix(-13, 0, -42).rotateX(.35));

	float scale = .1;

	driver->glColor3f(0, 0.5, 0.1);
	driver->glBegin(GL_QUADS);
	for (int x = 0; x < AREA_SIZE - 1; ++x)
	{
		for (int z = 0; z < AREA_SIZE - 1; ++z)
		{
      			driver->glNormal3f( 1.0f, 0.0f, 0.5f );
			driver->glVertex3f(scale * (x+1),  scale * height[x+1][z],    scale * z);
      			driver->glNormal3f( 1.0f, 0.0f, 0.5f );
			driver->glVertex3f(scale * x,      scale * height[x][z],      scale * z);
      			driver->glNormal3f( 0.0f, 0.0f, 0.5f );
			driver->glVertex3f(scale * x,      scale * height[x][z+1],    scale * (z+1));
      			driver->glNormal3f( 0.0f, 0.0f, 0.5f );
			driver->glVertex3f(scale * (x+1),  scale * height[x+1][z+1],  scale * (z+1));
		}
	}
	driver->glEnd();

	SDL_GL_SwapBuffers ();
}

int
main (int argc, char *argv[])
{
	if (SDL_Init (SDL_INIT_VIDEO) < 0) {
		fprintf (stderr, "Video initialization failed: %s\n",
			 SDL_GetError ());
		quit (1);
	}

	const SDL_VideoInfo *
		videoInfo = SDL_GetVideoInfo ();

	if (!videoInfo) {
		fprintf (stderr, "Video query failed: %s\n", SDL_GetError ());
		quit (1);
	}

	int
	  videoFlags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_RESIZABLE;
	if (videoInfo->hw_available)
		videoFlags |= SDL_HWSURFACE;
	else
		videoFlags |= SDL_SWSURFACE;

	if (videoInfo->blit_hw)
		videoFlags |= SDL_HWACCEL;

	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

	SDL_WM_SetCaption("Particle", NULL);

	surface = SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
				   videoFlags);

	if (!surface) {
		fprintf (stderr, "Video mode set failed: %s\n",
			 SDL_GetError ());
		quit (1);
	}

	if ((SDL_EnableKeyRepeat (100, SDL_DEFAULT_REPEAT_INTERVAL))) {
		fprintf (stderr, "Setting keyboard repeat failed: %s\n",
			 SDL_GetError ());
		quit (1);
	}

	if (!initGL ()) {
		fprintf (stderr, "Could not initialize OpenGL.\n");
		quit (1);
	}

	resizeWindow (SCREEN_WIDTH, SCREEN_HEIGHT);

	bool done = false, active = true;
	int lastTime = SDL_GetTicks(), frames = 0, lastFrameTime = SDL_GetTicks();
	while (!done) {
		/* handle the events in the queue */
		SDL_Event event;
		while (SDL_PollEvent (&event)) {
			switch (event.type) {
			case SDL_ACTIVEEVENT:
				if (event.active.gain == 0)
					active = false;
				else {
					active = true;
					 lastTime = SDL_GetTicks();
				}
				break;

			case SDL_VIDEORESIZE:
				surface = SDL_SetVideoMode (event.resize.w,
							    event.resize.h,
							    SCREEN_BPP,
							    videoFlags);
				if (!surface) {
					fprintf (stderr,
						 "Could not get a surface after resize: %s\n",
						 SDL_GetError ());
					quit (1);
				}
				resizeWindow (event.resize.w, event.resize.h);
				break;

			case SDL_KEYDOWN:
				handleKeyPress (&event.key.keysym);
				break;

			case SDL_QUIT:
				done = true;
				break;
			}
		}

		if (active) {
			int time =  SDL_GetTicks();

			if (++frames % 100 == 99) {
				printf("%.2f FPS\n", 1000.f * frames / (time - lastFrameTime));
				lastFrameTime = time;
				frames = 0;
			}

			drawScene ((time - lastTime) * .001f);
			lastTime = time;
		}
	}

	quit (0);
	return 0;
}
