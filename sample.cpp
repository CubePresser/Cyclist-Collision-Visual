#include <stdio.h>
	// yes, I know stdio.h is not good C++, but I like the *printf()
#include <stdlib.h>
#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "freeglut.h"
#include "glui.h"


// title of these windows:

const char *WINDOWTITLE = { "Projectile Motion with Collisions -- Joe Graphics" };
const char *GLUITITLE   = { "User Interface Window" };


// sizes:

const float RADIUS  = { 0.50f };

const float XLEFT   = { -5.f };
const float XRIGHT  = {  5.f };
const float YBOTTOM = { -3.f };
const float YTOP    = { 10.f };

// flags to indicate which surface was hit:
// (these are #define'd so that they can be used in a switch statement)

#define NOTHING_HIT	-1
#define HIT_LEFT	 0
#define HIT_RIGHT	 1
#define HIT_FLOOR1	 2
#define HIT_FLOOR2	 3

const float EPSILON = { 0.001f };

const float DT = { 0.002f };


// what the glui package defines as true and false:

const int GLUITRUE  = { true  };
const int GLUIFALSE = { false };


// the escape key:

#define ESCAPE		0x1b


// initial window size:

const int INIT_WINDOW_SIZE = { 600 };



// multiplication factors for input interaction:
//  (these are known from previous experience)

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };


// able to use the left mouse for either rotation or scaling,
// in case have only a 2-button mouse:

enum LeftButton
{
	ROTATE,
	SCALE
};


// minimum allowable scale factor:

const float MINSCALE = { 0.05f };


// active mouse buttons (or them together):

const int LEFT   = { 4 };
const int MIDDLE = { 2 };
const int RIGHT  = { 1 };


// which projection:

enum Projections
{
	ORTHO,
	PERSP
};


// which button:

enum ButtonVals
{
	GO,
	RESET,
	QUIT
};


// window background color (rgba):

const float BACKCOLOR[] = { 0., 0., 0., 0. };


// color and line width for the axes:

const GLfloat AXES_COLOR[] = { 1., .5, 0. };
const GLfloat AXES_WIDTH   = { 3. };



// fog parameters:

const GLfloat FOGCOLOR[4] = { .0, .0, .0, 1. };
const GLenum  FOGMODE     = { GL_LINEAR };
const GLfloat FOGDENSITY  = { 0.30f };
const GLfloat FOGSTART    = { 1.5 };
const GLfloat FOGEND      = { 4. };



//
// non-constant global variables:
//

int	ActiveButton;		// current button that is down
float	Angle;
GLuint	AxesList;		// list to hold the axes
int	AxesOn;			// ON or OFF
GLuint	BallList;
float	CoefRest;
int	Debug;			// ON means print debug info
int	DepthCueOn;		// TRUE means to use intensity depth cueing
float	Dt;
GLUI *	Glui;			// instance of glui window
int	GluiWindow;		// the glut id for the glui window
float	Gravity;
int	LeftButton;		// either ROTATE or SCALE
int	MainWindow;		// window id for main graphics window
bool	MoveBall;
GLfloat	RotMatrix[4][4];	// set by glui rotation widget
float	Scale, Scale2;		// scaling factors
float	TransXYZ[3];		// set by glui translation widgets
float	Velocity;
float	Vxnow, Vynow;
int	Xmouse, Ymouse;		// mouse values
float	Xnow, Ynow;
float	Xrot, Yrot;		// rotation angles in degrees
int	WhichProjection;	// ORTHO or PERSP


//
// function prototypes:
//

void	Animate( void );
void	Axes( float );
void	Bounce( float );
void	Buttons( int );
void	Display( void );
void	DoRasterString( float, float, float, char * );
void	DoStrokeString( float, float, float, float, char * );
float	ElapsedSeconds( void );
void	HsvRgb( float[3], float [3] );
void	InitGlui( void );
void	InitGraphics( void );
void	InitLists( void );
void	Keyboard( unsigned char, int, int );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Reset( void );
void	Resize( int, int );
void	Visibility( int );

void	Arrow( float [3], float [3] );
void	cross( float [3], float [3], float [3] );
float	dot( float [3], float [3] );
float	unit( float [3], float [3] );


//
// main program:
//

int
main( int argc, char *argv[] )
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)

	glutInit( &argc, argv );


	// setup all the graphics stuff:

	InitGraphics();


	// create the display structures that will not change:

	InitLists();


	// init all the global variables used by Display():
	// this will also post a redisplay
	// it is important to call this before InitGlui():
	// so that the variables that glui will control are correct
	// when each glui widget is created

	Reset();


	// setup all the user interface stuff:

	InitGlui();


	// draw the scene once and wait for some interaction:
	// (will never return)

	glutMainLoop();


	// this is here to make the compiler happy:

	return 0;
}



//
// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display() from here -- let glutMainLoop() do it
//

void
Animate( void )
{
	if (MoveBall)
	{
		Bounce(Dt);
		float v = sqrt(Vxnow*Vxnow + Vynow*Vynow);
		if (v < EPSILON)
			GLUI_Master.set_glutIdleFunc(Animate);
	}

	glutSetWindow( MainWindow );
	glutPostRedisplay();
}




//
// glui buttons callback:
//

void
Buttons( int id )
{
	switch( id )
	{
		case GO:
			Vxnow = Velocity * cos( Angle * (M_PI/180.) );
			Vynow = Velocity * sin( Angle * (M_PI/180.) );
			GLUI_Master.set_glutIdleFunc( Animate );
			MoveBall = true;
			break;

		case RESET:
			Reset();
			MoveBall = false;
			GLUI_Master.set_glutIdleFunc( Animate );
			Glui->sync_live();
			glutSetWindow( MainWindow );
			glutPostRedisplay();
			break;

		case QUIT:
			// gracefully close the glui window:
			// gracefully close out the graphics:
			// gracefully close the graphics window:
			// gracefully exit the program:

			Glui->close();
			glutSetWindow( MainWindow );
			glFinish();
			glutDestroyWindow( MainWindow );
			exit( 0 );
			break;

		default:
			fprintf( stderr, "Don't know what to do with Button ID %d\n", id );
	}

}



//
// draw the complete scene:
//

void
Display( void )
{
	GLsizei vx, vy, v;		// viewport dimensions
	GLint xl, yb;		// lower-left corner of viewport
	GLfloat scale2;		// real glui scale factor

	glutSetWindow( MainWindow );
	glDrawBuffer( GL_BACK );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glDisable( GL_DEPTH_TEST );
	glShadeModel( GL_FLAT );


	// set the viewport to a square centered in the window:

	vx = glutGet( GLUT_WINDOW_WIDTH );
	vy = glutGet( GLUT_WINDOW_HEIGHT );
	v = vx < vy ? vx : vy;			// minimum dimension
	xl = ( vx - v ) / 2;
	yb = ( vy - v ) / 2;
	glViewport( xl, yb,  v, v );


	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( -10., 10.,  -10., 10. );

	// place the objects into the scene:

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	
	glTranslatef( (GLfloat)TransXYZ[0], (GLfloat)TransXYZ[1], 0. );
	glScalef( (GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale );
	scale2 = 1. + Scale2;		// because glui translation starts at 0.
	if( scale2 < MINSCALE )
		scale2 = MINSCALE;
	glScalef( (GLfloat)scale2, (GLfloat)scale2, (GLfloat)scale2 );

	if( AxesOn )
		glCallList( AxesList );

	glLineWidth( 3. );
	glColor3f( 1., 0., 0. );
	glBegin( GL_LINE_STRIP );
		glVertex2f( XLEFT, YTOP );
		glVertex2f( XLEFT, YBOTTOM );
		glVertex2f( XRIGHT, YBOTTOM );
		glVertex2f( XRIGHT, YTOP );
	glEnd();

	glPushMatrix();
		glColor3f( 1., 1., 1. );
		glTranslatef( XLEFT+RADIUS, YTOP/2., 0. );
		glRotatef( Angle,  0., 0., 1. );
		float tail[3] = { 0., 0., 0. };
		float head[3] = { 1., 0., 0. };
		Arrow( tail, head );
	glPopMatrix();

	glPushMatrix();
		glColor3f( 0., 1., 0. );
		glTranslatef( Xnow, Ynow, 0. );
		glCallList( BallList );
	glPopMatrix();


	glutSwapBuffers();
	glFlush();
}



//
// use glut to display a string of characters using a raster font:
//

void
DoRasterString( float x, float y, float z, char *s )
{
	char c;			// one character to print

	glRasterPos3f( (GLfloat)x, (GLfloat)y, (GLfloat)z );
	for( ; ( c = *s ) != '\0'; s++ )
	{
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, c );
	}
}



//
// use glut to display a string of characters using a stroke font:
//

void
DoStrokeString( float x, float y, float z, float ht, char *s )
{
	char c;			// one character to print
	float sf;		// the scale factor

	glPushMatrix();
		glTranslatef( (GLfloat)x, (GLfloat)y, (GLfloat)z );
		sf = ht / ( 119.05 + 33.33 );
		glScalef( (GLfloat)sf, (GLfloat)sf, (GLfloat)sf );
		for( ; ( c = *s ) != '\0'; s++ )
		{
			glutStrokeCharacter( GLUT_STROKE_ROMAN, c );
		}
	glPopMatrix();
}



//
// return the number of seconds since the start of the program:
//

float
ElapsedSeconds( void )
{
	// get # of milliseconds since the start of the program:

	int ms = glutGet( GLUT_ELAPSED_TIME );

	// convert it to seconds:

	return (float)ms / 1000.f;
}



//
// initialize the glui window:
//

void
InitGlui( void )
{
	GLUI_Panel *panel;
	GLUI_Translation *trans, *scale;

	if( Debug )
		fprintf( stderr, "InitGlui\n" );


	// setup the glui window:

	glutInitWindowPosition( INIT_WINDOW_SIZE + 50, 0 );
	Glui = GLUI_Master.create_glui( (char *) GLUITITLE );


	Glui->add_statictext( (char *) GLUITITLE );
	Glui->add_separator();

	// Glui->add_checkbox( "Axes", &AxesOn );

	Glui->add_statictext( "Initial Velocity Angle:" );
	GLUI_HSlider *slider = Glui->add_slider( false, GLUI_HSLIDER_FLOAT, &Angle );
    	slider->set_float_limits( -75, 75. );
    	slider->set_w( 200 );
	Glui->add_separator();

	Glui->add_statictext( "Coefficient of Restitution:" );
	slider = Glui->add_slider( false, GLUI_HSLIDER_FLOAT, &CoefRest );
    	slider->set_float_limits( 0., 2. );
    	slider->set_w( 200 );
	Glui->add_separator();

	Glui->add_statictext( "Gravity:" );
	slider = Glui->add_slider( false, GLUI_HSLIDER_FLOAT, &Gravity );
    	slider->set_float_limits( -20., 20. );
    	slider->set_w( 200 );
	Glui->add_separator();
	
	Glui->add_statictext( "Speed:" );
	slider = Glui->add_slider( false, GLUI_HSLIDER_FLOAT, &Dt );
    	slider->set_float_limits( 0., 5.*DT );
    	slider->set_w( 200 );
	Glui->add_separator();


	panel = Glui->add_panel( "Scene Transformation" );

		Glui->add_column_to_panel( panel, GLUIFALSE );
		scale = Glui->add_translation_to_panel( panel, "Scale",  GLUI_TRANSLATION_Y , &Scale2 );
		scale->set_speed( 0.01f );

		Glui->add_column_to_panel( panel, FALSE );
		trans = Glui->add_translation_to_panel( panel, "Trans XY", GLUI_TRANSLATION_XY, &TransXYZ[0] );
		trans->set_speed( 0.1f );

	Glui->add_checkbox( "Debug", &Debug );

	panel = Glui->add_panel( "", FALSE );

	Glui->add_button_to_panel( panel, "Reset", RESET, (GLUI_Update_CB) Buttons );

	Glui->add_column_to_panel( panel, FALSE );

	Glui->add_button_to_panel( panel, "Go !", GO, (GLUI_Update_CB) Buttons );

	Glui->add_column_to_panel( panel, FALSE );

	Glui->add_button_to_panel( panel, "Quit", QUIT, (GLUI_Update_CB) Buttons );


	// tell glui what graphics window it needs to post a redisplay to:

	Glui->set_main_gfx_window( MainWindow );


	// set the graphics window's idle function:

	MoveBall = false;
	GLUI_Master.set_glutIdleFunc( Animate );
}



//
// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions
//

void
InitGraphics( void )
{
	if( Debug )
		fprintf( stderr, "InitGraphics\n" );


	// setup the display mode:
	// ( *must* be done before call to glutCreateWindow() )
	// ask for color, double-buffering, and z-buffering:

	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );


	// set the initial window configuration:

	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );


	// open the window and set its title:

	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );


	// setup the clear values:

	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );


	// setup the callback routines:


	// DisplayFunc -- redraw the window
	// ReshapeFunc -- handle the user resizing the window
	// KeyboardFunc -- handle a keyboard input
	// MouseFunc -- handle the mouse button going down or up
	// MotionFunc -- handle the mouse moving with a button down
	// PassiveMotionFunc -- handle the mouse moving with a button up
	// VisibilityFunc -- handle a change in window visibility
	// EntryFunc	-- handle the cursor entering or leaving the window
	// SpecialFunc -- handle special keys on the keyboard
	// SpaceballMotionFunc -- handle spaceball translation
	// SpaceballRotateFunc -- handle spaceball rotation
	// SpaceballButtonFunc -- handle spaceball button hits
	// ButtonBoxFunc -- handle button box hits
	// DialsFunc -- handle dial rotations
	// TabletMotionFunc -- handle digitizing tablet motion
	// TabletButtonFunc -- handle digitizing tablet button hits
	// MenuStateFunc -- declare when a pop-up menu is in use
	// TimerFunc -- trigger something to happen a certain time from now
	// IdleFunc -- what to do when nothing else is going on

	glutSetWindow( MainWindow );
	glutDisplayFunc( Display );
	glutReshapeFunc( Resize );
	glutKeyboardFunc( Keyboard );
	glutMouseFunc( MouseButton );
	glutMotionFunc( MouseMotion );
	glutPassiveMotionFunc( NULL );
	glutVisibilityFunc( Visibility );
	glutEntryFunc( NULL );
	glutSpecialFunc( NULL );
	glutSpaceballMotionFunc( NULL );
	glutSpaceballRotateFunc( NULL );
	glutSpaceballButtonFunc( NULL );
	glutButtonBoxFunc( NULL );
	glutDialsFunc( NULL );
	glutTabletMotionFunc( NULL );
	glutTabletButtonFunc( NULL );
	glutMenuStateFunc( NULL );
	glutTimerFunc( 0, NULL, 0 );

	// DO NOT SET THE GLUT IDLE FUNCTION HERE !!
	// glutIdleFunc( NULL );
	// let glui take care of it in InitGlui()
}




//
// initialize the display lists that will not change:
//

void
InitLists( void )
{
	BallList = glGenLists( 1 );
	glNewList( BallList, GL_COMPILE );
		glBegin( GL_TRIANGLE_FAN );
			glVertex2f( 0., 0. );
			int numsegs = 20;
			float dang = 2.*M_PI/(float)numsegs;
			float ang;
			int i;
			for( i = 0, ang = 0.; i <= numsegs; i++, ang += dang )
			{
				glVertex2f( RADIUS*cos(ang), RADIUS*sin(ang) );
			}
		glEnd();
	glEndList();


	AxesList = glGenLists( 1 );
	glNewList( AxesList, GL_COMPILE );
		glColor3fv( AXES_COLOR );
		glLineWidth( AXES_WIDTH );
			Axes( 4. );
		glLineWidth( 1. );
	glEndList();
}



//
// the keyboard callback:
//

void
Keyboard( unsigned char c, int x, int y )
{
	if( Debug )
		fprintf( stderr, "Keyboard: '%c' (0x%0x)\n", c, c );

	switch( c )
	{
		case 'q':
		case 'Q':
		case ESCAPE:
			Buttons( QUIT );	// will not return here
			break;			// happy compiler

		case 'r':
		case 'R':
			LeftButton = ROTATE;
			break;

		case 's':
		case 'S':
			LeftButton = SCALE;
			break;

		case '+':		// go faster
			Dt *= 1.10f;
			break;

		case '-':		// go slower
			Dt *= 0.90f;
			break;

		default:
			fprintf( stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c );
	}


	// synchronize the GLUI display with the variables:

	Glui->sync_live();


	// force a call to Display():

	glutSetWindow( MainWindow );
	glutPostRedisplay();
}



//
// called when the mouse button transitions down or up:
//

void
MouseButton( int button, int state, int x, int y )
{
	int b;			// LEFT, MIDDLE, or RIGHT

	if( Debug )
		fprintf( stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y );

	
	// get the proper button bit mask:

	switch( button )
	{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;

		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		default:
			b = 0;
			fprintf( stderr, "Unknown mouse button: %d\n", button );
	}


	// button down sets the bit, up clears the bit:

	if( state == GLUT_DOWN )
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}
}



//
// called when the mouse moves while a button is down:
//

void
MouseMotion( int x, int y )
{
	int dx, dy;		// change in mouse coordinates

	if( Debug )
		fprintf( stderr, "MouseMotion: %d, %d\n", x, y );


	dx = x - Xmouse;		// change in mouse coords
	dy = y - Ymouse;

	if( ActiveButton & LEFT )
	{
		switch( LeftButton )
		{
			case ROTATE:
				Xrot += ( ANGFACT*dy );
				Yrot += ( ANGFACT*dx );
				break;

			case SCALE:
				Scale += SCLFACT * (float) ( dx - dy );
				if( Scale < MINSCALE )
					Scale = MINSCALE;
				break;
		}
	}


	if( ActiveButton & MIDDLE )
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:

		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay();
}



//
// reset the transformations and the colors:
//
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene
//

void
Reset( void )
{
	ActiveButton = 0;
	Angle = 0.;
	AxesOn = GLUIFALSE;
	CoefRest = 1.f;
	Debug = GLUITRUE;
	DepthCueOn = FALSE;
	Dt = DT;
	Gravity = -9.8f;
	LeftButton = ROTATE;
	MoveBall = false;
	Scale  = 1.0;
	Scale2 = 0.0;		// because add 1. to it in Display()
	Velocity = 10.f;
	Vxnow = Vynow = 0.;
	Xnow = XLEFT+RADIUS;
	Ynow = YTOP/2.;
	Xrot = Yrot = 0.;
	TransXYZ[0] = TransXYZ[1] = TransXYZ[2] = 0.;

	                  RotMatrix[0][1] = RotMatrix[0][2] = RotMatrix[0][3] = 0.;
	RotMatrix[1][0]                   = RotMatrix[1][2] = RotMatrix[1][3] = 0.;
	RotMatrix[2][0] = RotMatrix[2][1]                   = RotMatrix[2][3] = 0.;
	RotMatrix[3][0] = RotMatrix[3][1] = RotMatrix[3][3]                   = 0.;
	RotMatrix[0][0] = RotMatrix[1][1] = RotMatrix[2][2] = RotMatrix[3][3] = 1.;
}



//
// called when user resizes the window:
//

void
Resize( int width, int height )
{
	if( Debug )
		fprintf( stderr, "ReSize: %d, %d\n", width, height );

	// don't really need to do anything since window size is
	// checked each time in Display():

	glutSetWindow( MainWindow );
	glutPostRedisplay();
}


//
// handle a change to the window's visibility:
//

void
Visibility ( int state )
{
	if( Debug )
		fprintf( stderr, "Visibility: %d\n", state );

	if( state == GLUT_VISIBLE )
	{
		glutSetWindow( MainWindow );
		glutPostRedisplay();
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}







// the stroke characters 'X' 'Y' 'Z' :

static float xx[] = {
		0., 1., 0., 1.
	      };

static float xy[] = {
		-.5, .5, .5, -.5
	      };

static int xorder[] = {
		1, 2, -3, 4
		};


static float yx[] = {
		0., 0., -.5, .5
	      };

static float yy[] = {
		0., .6f, 1., 1.
	      };

static int yorder[] = {
		1, 2, 3, -2, 4
		};


static float zx[] = {
		1., 0., 1., 0., .25, .75
	      };

static float zy[] = {
		.5, .5, -.5, -.5, 0., 0.
	      };

static int zorder[] = {
		1, 2, 3, 4, -5, 6
		};


// fraction of the length to use as height of the characters:

#define LENFRAC		0.10


// fraction of length to use as start location of the characters:

#define BASEFRAC	1.10


//
//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)
//

void
Axes( float length )
{
	int i, j;			// counters
	float fact;			// character scale factor
	float base;			// character start location


	glBegin( GL_LINE_STRIP );
		glVertex3f( length, 0., 0. );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., length, 0. );
	glEnd();
	glBegin( GL_LINE_STRIP );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., 0., length );
	glEnd();

	fact = LENFRAC * length;
	base = BASEFRAC * length;

	glBegin( GL_LINE_STRIP );
		for( i = 0; i < 4; i++ )
		{
			j = xorder[i];
			if( j < 0 )
			{
				
				glEnd();
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( base + fact*xx[j], fact*xy[j], 0.0 );
		}
	glEnd();

	glBegin( GL_LINE_STRIP );
		for( i = 0; i < 5; i++ )
		{
			j = yorder[i];
			if( j < 0 )
			{
				
				glEnd();
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( fact*yx[j], base + fact*yy[j], 0.0 );
		}
	glEnd();

	glBegin( GL_LINE_STRIP );
		for( i = 0; i < 6; i++ )
		{
			j = zorder[i];
			if( j < 0 )
			{
				
				glEnd();
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( 0.0, fact*zy[j], base + fact*zx[j] );
		}
	glEnd();

}




//
// routine to convert HSV to RGB
//
// Reference:  Foley, van Dam, Feiner, Hughes,
//		"Computer Graphics Principles and Practices,"
//		Additon-Wesley, 1990, pp592-593.
//	       
//


void
HsvRgb( float hsv[3], float rgb[3] )
{
	float h, s, v;			// hue, sat, value
	float r, g, b;			// red, green, blue
	float i, f, p, q, t;		// interim values


	// guarantee valid input:

	h = hsv[0] / 60.;
	while( h >= 6. )	h -= 6.;
	while( h <  0. ) 	h += 6.;

	s = hsv[1];
	if( s < 0. )
		s = 0.;
	if( s > 1. )
		s = 1.;

	v = hsv[2];
	if( v < 0. )
		v = 0.;
	if( v > 1. )
		v = 1.;


	// if sat==0, then is a gray:

	if( s == 0.0 )
	{
		rgb[0] = rgb[1] = rgb[2] = v;
		return;
	}


	// get an rgb from the hue itself:
	
	i = floor( h );
	f = h - i;
	p = v * ( 1. - s );
	q = v * ( 1. - s*f );
	t = v * ( 1. - ( s * (1.-f) ) );

	switch( (int) i )
	{
		case 0:
			r = v;	g = t;	b = p;
			break;
	
		case 1:
			r = q;	g = v;	b = p;
			break;
	
		case 2:
			r = p;	g = v;	b = t;
			break;
	
		case 3:
			r = p;	g = q;	b = v;
			break;
	
		case 4:
			r = t;	g = p;	b = v;
			break;
	
		case 5:
			r = v;	g = p;	b = q;
			break;
	}


	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;

}



void
Bounce( float dt )
{
	while( dt > EPSILON )
	{
		float tmin = dt;	// minimum time to do something
		int which = NOTHING_HIT; // which reason was it for doing the something

		// these four collision times are computed using your projectile motion equations:

		float tleft = 0.;	// time to hit the left wall
		if( tleft > 0.  &&  tleft < tmin )
		{
			//tmin = ?????;
			//which = ?????;
		}
		float tright = 0.;	// time to hit the right wall
		if( tright > 0.  &&  tright < tmin )
		{
			//?????
		}
		float tfloor1 = 0.;	// time to hit the floor
		if( tfloor1 > 0.  &&  tfloor1 < tmin )
		{
			//?????
		}
		float tfloor2 = 0.;	// time to hit the floor (note there are 2 answers)
		if( tfloor2 > 0.  &&  tfloor2 < tmin )
		{
			//?????
		}

		// tmin is now set to the smallest of:
		//	dt, tleft, tright, tfloor1, tfloor2
		// which are positive

		// which is set to:
		//	NOTHING_HIT, HIT_LEFT, HIT_RIGHT, HIT_FLOOR1, or HIT_FLOOR2
		// to show what was the first thing hit

		// take a time step of time length tmin, using the projectile motion equations:
		// if a bounce is going to occur, tmin takes the ball right up to the surface:

		Xnow = Xnow + Vxnow*dt;
		Vxnow = Vxnow;
		Ynow = Ynow + Vynow*dt + 0.5*Gravity*dt*dt;
		Vynow = Vynow + Gravity*dt;


		// a "bounce trick":

		if( Ynow < YBOTTOM+RADIUS )
			Ynow = YBOTTOM+RADIUS;		// floating point roundoff sometimes messes up

		if( Xnow < XLEFT+RADIUS )
			Xnow = XLEFT+RADIUS;

		if( Xnow > XRIGHT-RADIUS )
			Xnow = XRIGHT-RADIUS;


		// depending on what caused the bounce, change the proper velocity component:
		// if nothing was hit in this time step, just return:

		switch( which )
		{
			case NOTHING_HIT:
				return;

			case HIT_LEFT:
				Vxnow = -CoefRest * Vxnow;
				break;

			case HIT_RIGHT:
				//?????
				break;

			case HIT_FLOOR1:
				//?????
				break;

			case HIT_FLOOR2:
				//?????
				break;
		}

		dt -= tmin;	// after the bounce, we might still have some time step left
	}
}


// size of wings as fraction of length:

#define WINGS	0.10

// axes:

#define X	1
#define Y	2
#define Z	3


// x, y, z, axes:

static float axx[3] = { 1., 0., 0. };
static float ayy[3] = { 0., 1., 0. };
static float azz[3] = { 0., 0., 1. };

void
Arrow( float tail[3], float head[3] )
{
	float u[3], v[3], w[3];		/* arrow coordinate system	*/
	float d;			/* wing distance		*/
	float x, y, z;			/* point to plot		*/
	float mag;			/* magnitude of major direction	*/
	float f;			/* fabs of magnitude		*/
	int axis;			/* which axis is the major	*/


	/* set w direction in u-v-w coordinate system:			*/

	w[0] = head[0] - tail[0];
	w[1] = head[1] - tail[1];
	w[2] = head[2] - tail[2];


	/* determine major direction:					*/

	axis = X;
	mag = fabs( w[0] );
	if( (f=fabs(w[1]))  > mag )
	{
		axis = Y;
		mag = f;
	}
	if( (f=fabs(w[2]))  > mag )
	{
		axis = Z;
		mag = f;
	}


	/* set size of wings and turn w into a unit vector:		*/

	d = WINGS * unit( w, w );


	/* draw the shaft of the arrow:					*/

	glBegin( GL_LINE_STRIP );
		glVertex3fv( tail );
		glVertex3fv( head );
	glEnd();

	/* draw two sets of wings in the non-major directions:		*/

	if( axis != X )
	{
		cross( w, axx, v );
		(void) unit( v, v );
		cross( v, w, u  );
		x = head[0] + d * ( u[0] - w[0] );
		y = head[1] + d * ( u[1] - w[1] );
		z = head[2] + d * ( u[2] - w[2] );
		glBegin( GL_LINE_STRIP );
			glVertex3fv( head );
			glVertex3f( x, y, z );
		glEnd();
		x = head[0] + d * ( -u[0] - w[0] );
		y = head[1] + d * ( -u[1] - w[1] );
		z = head[2] + d * ( -u[2] - w[2] );
		glBegin( GL_LINE_STRIP );
			glVertex3fv( head );
			glVertex3f( x, y, z );
		glEnd();
	}


	if( axis != Y )
	{
		cross( w, ayy, v );
		(void) unit( v, v );
		cross( v, w, u  );
		x = head[0] + d * ( u[0] - w[0] );
		y = head[1] + d * ( u[1] - w[1] );
		z = head[2] + d * ( u[2] - w[2] );
		glBegin( GL_LINE_STRIP );
			glVertex3fv( head );
			glVertex3f( x, y, z );
		glEnd();
		x = head[0] + d * ( -u[0] - w[0] );
		y = head[1] + d * ( -u[1] - w[1] );
		z = head[2] + d * ( -u[2] - w[2] );
		glBegin( GL_LINE_STRIP );
			glVertex3fv( head );
			glVertex3f( x, y, z );
		glEnd();
	}



	if( axis != Z )
	{
		cross( w, azz, v );
		(void) unit( v, v );
		cross( v, w, u  );
		x = head[0] + d * ( u[0] - w[0] );
		y = head[1] + d * ( u[1] - w[1] );
		z = head[2] + d * ( u[2] - w[2] );
		glBegin( GL_LINE_STRIP );
			glVertex3fv( head );
			glVertex3f( x, y, z );
		glEnd();
		x = head[0] + d * ( -u[0] - w[0] );
		y = head[1] + d * ( -u[1] - w[1] );
		z = head[2] + d * ( -u[2] - w[2] );
		glBegin( GL_LINE_STRIP );
			glVertex3fv( head );
			glVertex3f( x, y, z );
		glEnd();
	}


	/* done:							*/

}



float
dot( float v1[3], float v2[3] )
{
	return( v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2] );
}



void
cross( float v1[3], float v2[3], float vout[3] )
{
	float tmp[3];

	tmp[0] = v1[1]*v2[2] - v2[1]*v1[2];
	tmp[1] = v2[0]*v1[2] - v1[0]*v2[2];
	tmp[2] = v1[0]*v2[1] - v2[0]*v1[1];

	vout[0] = tmp[0];
	vout[1] = tmp[1];
	vout[2] = tmp[2];
}



float
unit( float vin[3], float vout[3] )
{
	float dist, f ;

	dist = vin[0]*vin[0] + vin[1]*vin[1] + vin[2]*vin[2];

	if( dist > 0.0 )
	{
		dist = sqrt( dist );
		f = 1. / dist;
		vout[0] = f * vin[0];
		vout[1] = f * vin[1];
		vout[2] = f * vin[2];
	}
	else
	{
		vout[0] = vin[0];
		vout[1] = vin[1];
		vout[2] = vin[2];
	}

	return( dist );
}
