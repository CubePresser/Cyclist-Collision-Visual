/*******************************************************
         JonesJonathan - Jonathan Jones - 2018

------------- Cyclist Collision Visual ---------------
This is an OpenGL graphical simulation of a Car-Bike intersection.
The purpose of this simulation is to research how intersection angle, vehicle speeds
and blindspots affect the driver's visual of cyclists on intersecting roads.

For the purposes of this simulation, each unit (1.0f) is equivalent to 1 meter.

Default sizes (W X H X L):
Ground: 2000.0 x 2000.0 meters
Road: 4.0 meters wide (Typical single lane)
Car: 2.0 x 2.0 x 4.0 meters (Typical mid sized car)
Bike: 0.5 x 1.0 x 2.0 meters (Typical bike)

Default speeds:
Car: 18 m/s (40 mph)(65 km/h)
Bike: 7 m/s (17 mph)(25 km/h)

Default starting distance:
Car: 100m (328 ft)
Bike: 39m (128 ft)
*******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#include "Dependencies/glew.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "Dependencies/freeglut.h"
#include "Dependencies/glui.h"

// title of these windows:
const char *WINDOWTITLE = { "Cyclist Collision Visual - Jonathan Jones" };
const char *GLUITITLE   = { "User Interface Window" };


// what the glui package defines as true and false:
const int GLUITRUE  = { true  };
const int GLUIFALSE = { false };


// the escape key:
#define ESCAPE		0x1b

//Degrees to radians conversion
const float DEG_TO_RAD	= M_PI / 180.0;
const float RAD_TO_DEG	= 180.0 / M_PI;

// initial window size:
const int INIT_WINDOW_SIZE = { 800 };

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

//Depth Cue
const int DEPTHCUE = 0;


// minimum allowable scale factor:
const float MINSCALE = { 0.05f };


// active mouse buttons (or them together):
const int LEFT   = { 4 };
const int MIDDLE = { 2 };
const int RIGHT  = { 1 };

// which button:
enum ButtonVals
{
	PLAY,
	RESET,
	REPLAY,
	QUIT
};

// window background color (rgba):
const GLfloat BACKCOLOR[ ] = { .258, .525, .956, 1. };


// line width for the axes:
const GLfloat AXES_WIDTH   = { 3. };

//Fog parameters
const GLfloat FOGCOLOR[4] = { .0, .0, .0, 1. };
const GLenum  FOGMODE     = { GL_LINEAR };
const GLfloat FOGDENSITY  = { 0.30f };
const GLfloat FOGSTART    = { 1.5 };
const GLfloat FOGEND      = { 4. };


//Non-constant global variables
int		ActiveButton;			// current button that is down
GLuint	AxesList;				// list to hold the axes
int		AxesOn;					// != 0 means to draw the axes
int		ViewType = 0;			// 0 = Car view, 1 = Intersection view
int		DebugOn;				// != 0 means to print debugging info
int		MainWindow;				// window id for main graphics window
float	Scale, Scale2;			// scaling factors
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees

//Freezing the animation
bool	Frozen;

GLfloat Fov; //Field of view

GLuint	GroundList;
GLuint	RoadList;
GLuint	BikeList;

//Blind spot angles (With respect to the Z axis in the negative direction)
float	AngleIntersection;
float	LeadingAngle;
float	TrailingAngle;

//Globals for the moving objects in the scene
GLfloat	CarStart;
GLfloat CarDistance;
GLfloat	CarSpeed;
GLfloat BikeStart;
GLfloat BikeDistance;
GLfloat	BikeSpeed;

//Distance travelled since the last frame
GLfloat CarDistanceTravelled;
GLfloat BikeDistanceTravelled;

//Animation times
int animate_start_time; //Log the time in which animation starts so simulation time is essentially starting at t=0
int time_frozen;

//GLUI globals
GLUI *	Glui;				// instance of glui window
int	GluiWindow;				// the glut id for the glui window
GLfloat	RotMatrix[4][4];	// set by glui rotation widget
float	TransXYZ[3];		// set by glui translation widgets
bool	play;
float	Time;

//Structure to hold all the information needed for a slider on the GLUI panel
struct GLUI_SliderPackage
{
	GLUI_HSlider* slider;
	GLUI_EditText* edit_text;

};

enum SliderVals {
	FOV,
	AOI,
	LA,
	TA,
	CSTART,
	CSPEED,
	BSTART,
	BSPEED
};

struct GLUI_SliderPackage sliders[8];


// function prototypes:
void	Animate( );
void	Buttons( int );
void	Display( );
void	InitGlui();
void	InitGraphics( );
void	InitLists( );
void	Keyboard( unsigned char, int, int );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Reset( );
void	Replay( );
void	Resize( int, int );
void	Visibility( int );

void	Axes( float );

//Draw car and shadow
void	DrawShadow();
void	DrawCar(float);

void	UpdateGLUI(int);

// main program:

int main( int argc, char *argv[ ] )
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)

	glutInit( &argc, argv );


	// setup all the graphics stuff:

	InitGraphics( );


	// create the display structures that will not change:

	InitLists( );


	// init all the global variables used by Display( ):
	// this will also post a redisplay

	Reset( );


	// setup all the user interface stuff:

	//InitMenus( );
	InitGlui();


	// draw the scene once and wait for some interaction:
	// (this will never return)

	glutSetWindow( MainWindow );
	glutMainLoop( );


	// this is here to make the compiler happy:

	return 0;
}


//Update distance information with respect to speed for the Car and Bike in the scene
void Animate( )
{
	if (play)
	{
		float seconds = ((float)(glutGet(GLUT_ELAPSED_TIME) - animate_start_time) / 1000.f);
		float dt = seconds - Time;
		Time = seconds;

		//Distance += DeltaTime * Speed
		CarDistanceTravelled += (dt * CarSpeed);
		BikeDistanceTravelled += (dt * BikeSpeed);
	}

	// force a call to Display( ) next time it is convenient:
	Glui->sync_live();
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

void Buttons(int id)
{
	switch (id)
	{
	case PLAY:
		play = !play;
		Frozen = !Frozen;
		if (Frozen)
		{
			time_frozen = glutGet(GLUT_ELAPSED_TIME) - animate_start_time;
		}
		else
		{
			animate_start_time = glutGet(GLUT_ELAPSED_TIME) - time_frozen;
		}
		break;

	case RESET:
		Reset();
		UpdateGLUI(-1);
		Glui->sync_live();
		glutSetWindow(MainWindow);
		glutPostRedisplay();
		break;

	case REPLAY:
		Replay();
		glutSetWindow(MainWindow);
		glutPostRedisplay();
		break;

	case QUIT:
		// gracefully close the glui window:
		// gracefully close out the graphics:
		// gracefully close the graphics window:
		// gracefully exit the program:

		Glui->close();
		glutSetWindow(MainWindow);
		glFinish();
		glutDestroyWindow(MainWindow);
		exit(0);
		break;

	default:
		fprintf(stderr, "Don't know what to do with Button ID %d\n", id);
	}

}


/*************************************************
 * Function: 
 * Description: 
 * Returns: 
 * **********************************************/

//Draw the scene
void Display( )
{
	CarDistance = CarStart - CarDistanceTravelled;
	BikeDistance = BikeStart - BikeDistanceTravelled;

	GLfloat scale2;

	if( DebugOn != 0 )
	{
		fprintf( stderr, "Display\n" );
	}


	//Set window in which to draw graphics
	glutSetWindow( MainWindow );

	//Flush the background contents
	glDrawBuffer( GL_BACK );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glEnable( GL_DEPTH_TEST );

	//Specify shading to be flat:
	glShadeModel( GL_FLAT );


	// set the viewport to a square centered in the window:
	GLsizei vx = glutGet( GLUT_WINDOW_WIDTH );
	GLsizei vy = glutGet( GLUT_WINDOW_HEIGHT );
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = ( vx - v ) / 2;
	GLint yb = ( vy - v ) / 2;
	glViewport( xl, yb,  v, v );


	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluPerspective( (double)Fov, 1.,	0.1, 1000. );


	// place the objects into the scene:
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );


	//Check view type
	if (!ViewType) //Car interior
	{
		gluLookAt(0., 1.6, CarDistance, 0., 1.6, -CarDistanceTravelled, 0., 1., 0.); //Eye is positioned at the car looking out the front
	}
	else //Intersection
	{
		gluLookAt(0., 100., 100.,     0., 0., 0.,     0., 1., 0.);

		glTranslatef((GLfloat)TransXYZ[0], (GLfloat)TransXYZ[1], -(GLfloat)TransXYZ[2]);
		glRotatef((GLfloat)Yrot, 0., 1., 0.);
		glRotatef((GLfloat)Xrot, 1., 0., 0.);
		glMultMatrixf((const GLfloat *)RotMatrix);
		glScalef((GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale);
		scale2 = 1. + Scale2;
		if (scale2 < MINSCALE)
			scale2 = MINSCALE;
		glScalef((GLfloat)scale2, (GLfloat)scale2, (GLfloat)scale2);
	}

	// possibly draw the axes:
	if( AxesOn != 0 )
	{
		glPushMatrix();
		glTranslatef(0.f, 2.f, 0.f);
		glColor3f(1, 1, 1);
		glCallList( AxesList );
		glPopMatrix();
	}


	// since we are using glScalef( ), be sure normals get unitized:
	glEnable( GL_NORMALIZE );

	glCallList(GroundList);
	glCallList(RoadList); //Car Road

	//Draw bike road
	glPushMatrix();
	glRotatef(AngleIntersection, 0.f, 1.f, 0.f);
	glCallList(RoadList); //Bike road
	glPopMatrix();

	//Draw the Car
	glPushMatrix();
	glTranslatef(0, 0, CarDistance); //Move the car and shadow
	DrawCar(2.195f);
	glPopMatrix();

	//Draw bike
	glPushMatrix();
	glRotatef(AngleIntersection, 0.f, 1.f, 0.f);
	glTranslatef(0, 0, BikeDistance);
	glCallList(BikeList);
	glPopMatrix();

	//Draw blind spot shadow
	DrawShadow();

	//Reset projection matrix and set world coordinates 0-100
	glDisable( GL_DEPTH_TEST );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluOrtho2D( 0., 100.,     0., 100. );

	//Reset modelview matrix
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	// swap the double-buffered framebuffers:
	glutSwapBuffers( );


	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !
	glFlush( );
}

void InitGlui(void)
{
	GLUI_Panel *panel;
	GLUI_Translation *trans, *scale;
	GLUI_Rotation *rot;

	// setup the glui window:

	glutInitWindowPosition(INIT_WINDOW_SIZE + 50, 0);
	Glui = GLUI_Master.create_glui((char *)GLUITITLE);


	Glui->add_statictext((char *)GLUITITLE);
	Glui->add_separator();

	//Axes
	Glui->add_checkbox( "Axes", &AxesOn );

	//View
	Glui->add_checkbox("Exterior View", &ViewType);

	Glui->add_statictext("Field of View");
	sliders[FOV].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &Fov);
	sliders[FOV].slider->set_float_limits(0.f, 180.f);
	sliders[FOV].slider->set_w(500);
	sliders[FOV].slider->set_slider_val(Fov);
	sliders[FOV].edit_text = Glui->add_edittext("Degrees [0 - 180]: ", GLUI_EDITTEXT_FLOAT, &Fov, FOV, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	//Angle of intersection
	Glui->add_statictext("Angle of Intersection");
	sliders[AOI].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &AngleIntersection);
	sliders[AOI].slider->set_float_limits(0.f, 180.f);
	sliders[AOI].slider->set_w(500);
	sliders[AOI].slider->set_slider_val(AngleIntersection);
	sliders[AOI].edit_text = Glui->add_edittext("Degrees [0 - 180]: ", GLUI_EDITTEXT_FLOAT, &AngleIntersection, AOI, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	//Leading Angle
	Glui->add_statictext("Blindspot Leading Angle");
	sliders[LA].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &LeadingAngle);
	sliders[LA].slider->set_float_limits(0.f, 45.f);
	sliders[LA].slider->set_w(500);
	sliders[LA].slider->set_slider_val(LeadingAngle);
	sliders[LA].edit_text = Glui->add_edittext("Degrees [0 - 45]: ", GLUI_EDITTEXT_FLOAT, &LeadingAngle, LA, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	//Trailing Angle
	Glui->add_statictext("Blindspot Trailing Angle");
	sliders[TA].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &TrailingAngle);
	sliders[TA].slider->set_float_limits(0.f, 45.f);
	sliders[TA].slider->set_w(500);
	sliders[TA].slider->set_slider_val(TrailingAngle);
	sliders[TA].edit_text = Glui->add_edittext("Degrees [0 - 45]: ", GLUI_EDITTEXT_FLOAT, &TrailingAngle, TA, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	//Car start
	Glui->add_statictext("Car Starting Distance");
	sliders[CSTART].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &CarStart);
	sliders[CSTART].slider->set_float_limits(0.f, 1000.f);
	sliders[CSTART].slider->set_w(500);
	sliders[CSTART].slider->set_slider_val(CarStart);
	sliders[CSTART].edit_text = Glui->add_edittext("Meters [0. - 1000.]: ", GLUI_EDITTEXT_FLOAT, &CarStart, CSTART, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	//Car speed
	Glui->add_statictext("Car Speed");
	sliders[CSPEED].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &CarSpeed);
	sliders[CSPEED].slider->set_float_limits(0.f, 100.f);
	sliders[CSPEED].slider->set_w(500);
	sliders[CSPEED].slider->set_slider_val(CarSpeed);
	sliders[CSPEED].edit_text = Glui->add_edittext("Meters/Second [0 - 100]: ", GLUI_EDITTEXT_FLOAT, &CarSpeed, CSPEED, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	//Bike Start
	Glui->add_statictext("Bike Starting Distance");
	sliders[BSTART].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &BikeStart);
	sliders[BSTART].slider->set_float_limits(0.f, 1000.f);
	sliders[BSTART].slider->set_w(500);
	sliders[BSTART].slider->set_slider_val(BikeStart);
	sliders[BSTART].edit_text = Glui->add_edittext("Meters [0 - 1000]: ", GLUI_EDITTEXT_FLOAT, &BikeStart, BSTART, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	//Bike Speed
	Glui->add_statictext("Bike Speed");
	sliders[BSPEED].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &BikeSpeed);
	sliders[BSPEED].slider->set_float_limits(0.f, 100.f);
	sliders[BSPEED].slider->set_w(500);
	sliders[BSPEED].slider->set_slider_val(BikeSpeed);
	sliders[BSPEED].edit_text = Glui->add_edittext("Meters/Second [0 - 100]: ", GLUI_EDITTEXT_FLOAT, &BikeSpeed, BSPEED, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	panel = Glui->add_panel("Scene Transformation");

	rot = Glui->add_rotation_to_panel(panel, "Rotation", (float *)RotMatrix);

	rot->set_spin(1.0);

	Glui->add_column_to_panel(panel, GLUIFALSE);
	scale = Glui->add_translation_to_panel(panel, "Zoom", GLUI_TRANSLATION_Y, &Scale2);
	scale->set_speed(0.01f);

	Glui->add_column_to_panel(panel, GLUIFALSE);
	trans = Glui->add_translation_to_panel(panel, "Trans XY", GLUI_TRANSLATION_XY, &TransXYZ[0]);
	trans->set_speed(1.1f);

	Glui->add_column_to_panel(panel, FALSE);
	trans = Glui->add_translation_to_panel(panel, "Trans Z", GLUI_TRANSLATION_Z, &TransXYZ[2]);
	trans->set_speed(1.1f);

	panel = Glui->add_panel("", FALSE);

	Glui->add_button_to_panel(panel, "Play / Pause", PLAY, (GLUI_Update_CB)Buttons);

	Glui->add_column_to_panel(panel, FALSE);

	Glui->add_button_to_panel(panel, "Replay", REPLAY, (GLUI_Update_CB)Buttons);

	Glui->add_column_to_panel(panel, FALSE);

	Glui->add_button_to_panel(panel, "Reset", RESET, (GLUI_Update_CB)Buttons);

	Glui->add_column_to_panel(panel, FALSE);

	Glui->add_button_to_panel(panel, "Quit", QUIT, (GLUI_Update_CB)Buttons);


	// tell glui what graphics window it needs to post a redisplay to:

	Glui->set_main_gfx_window(MainWindow);


	// set the graphics window's idle function:

	GLUI_Master.set_glutIdleFunc(Animate);
}

/*
// initialize the glui window:
void InitMenus( )
{
	glutSetWindow( MainWindow );

	int axesmenu = glutCreateMenu( DoAxesMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int viewmenu = glutCreateMenu(DoViewMenu);
	glutAddMenuEntry("Car - Interior", 0);
	glutAddMenuEntry("Intersection", 1);

	int debugmenu = glutCreateMenu( DoDebugMenu );
	glutAddMenuEntry( "Off",  0 );
	glutAddMenuEntry( "On",   1 );

	int mainmenu = glutCreateMenu( DoMainMenu );
	glutAddSubMenu(   "Axes",          axesmenu);
	glutAddSubMenu(   "View",          viewmenu);
	glutAddMenuEntry( "Reset",         RESET );
	glutAddSubMenu(   "Debug",         debugmenu);
	glutAddMenuEntry( "Quit",          QUIT );

	// attach the pop-up menu to the right mouse button:
	glutAttachMenu( GLUT_RIGHT_BUTTON );
}
*/

// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions
void InitGraphics( )
{
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	// set the initial window configuration:
	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );

	// open the window and set its title:
	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );

	// set the framebuffer clear values:
	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

	// setup the callback functions:
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
	glutTimerFunc( -1, NULL, 0 );
	//glutIdleFunc( NULL );

	// init glew (a window must be open to do this):
	#ifdef WIN32
		GLenum err = glewInit( );
		if( err != GLEW_OK )
		{
			fprintf( stderr, "glewInit Error\n" );
		}
		else
			fprintf( stderr, "GLEW initialized OK\n" );
		fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	#endif

}


// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )
void InitLists( )
{
	glutSetWindow( MainWindow );

	//Grass
	GroundList = glGenLists(1);
	glNewList(GroundList, GL_COMPILE);
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(.235, .686, .113);
	glVertex3f(1000.f, 0.f, 1000.f);
	glVertex3f(1000.f, 0.f, -1000.f);
	glVertex3f(-1000.f, 0.f, 1000.f);
	glVertex3f(-1000.f, 0.f, -1000.f);
	glEnd();
	glEndList();

	//Road
	RoadList = glGenLists(1);
	glNewList(RoadList, GL_COMPILE);
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(.75f, .75f, .75f);
	glVertex3f(2.f, 0.05f, 1000.f);
	glVertex3f(2.f, 0.05f, -1000.f);
	glVertex3f(-2.f, 0.05f, 1000.f);
	glVertex3f(-2.f, 0.05f, -1000.f);
	glEnd();
	glEndList();

	//Bike
	BikeList = glGenLists(1);
	glNewList(BikeList, GL_COMPILE);
		//Top
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(0.25f, 1.f, 1.f);
		glVertex3f(0.25f, 1.f, -1.f);
		glVertex3f(-0.25f, 1.f, 1.f);
		glVertex3f(-0.25f, 1.f, -1.f);
		glEnd();
		//Left
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-0.25f, 1.f, 1.f);
		glVertex3f(-0.25f, 0.f, 1.f);
		glVertex3f(-0.25f, 1.f, -1.f);
		glVertex3f(-0.25f, 0.f, -1.f);
		glEnd();
		//Right
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(0.25f, 1.f, 1.f);
		glVertex3f(0.25f, 0.f, 1.f);
		glVertex3f(0.25f, 1.f, -1.f);
		glVertex3f(0.25f, 0.f, -1.f);
		glEnd();
	glEndList();

	//Axes
	AxesList = glGenLists( 1 );
	glNewList( AxesList, GL_COMPILE );
		glLineWidth( AXES_WIDTH );
			Axes( 20.0 );
		glLineWidth( 1. );
	glEndList( );
}


// the keyboard callback:
void Keyboard( unsigned char c, int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Keyboard: '%c' (0x%0x)\n", c, c );

	switch( c )
	{
		case 'r':
		case 'R':
			Buttons(RESET);
			break;
		case 'q':
		case 'Q':
		case ESCAPE:
			Buttons( QUIT );	// will not return here
			break;				// happy compiler
		case 'p':
		case 'P':
			Buttons( PLAY );
			break;

		default:
			fprintf( stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c );
	}

	// synchronize the GLUI display with the variables:
	Glui->sync_live();

	// force a call to Display( ):
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// called when the mouse button transitions down or up:
void MouseButton( int button, int state, int x, int y )
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if( DebugOn != 0 )
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


// called when the mouse moves while a button is down:
void MouseMotion( int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "MouseMotion: %d, %d\n", x, y );


	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if( ( ActiveButton & LEFT ) != 0 )
	{
		Xrot += ( ANGFACT*dy );
		Yrot += ( ANGFACT*dx );
	}


	if( ( ActiveButton & MIDDLE ) != 0 )
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:
		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

//Assign global variables their default values
// - Also used to "Reset" the scene
void Reset( )
{
	ActiveButton = 0;
	AxesOn = GLUIFALSE;
	DebugOn = GLUIFALSE;
	Scale  = 1.0;
	Xrot = Yrot = 0.;
	Fov = 90.f;

	TransXYZ[0] = TransXYZ[1] = TransXYZ[2] = 0.;

	RotMatrix[0][1] = RotMatrix[0][2] = RotMatrix[0][3] = 0.;
	RotMatrix[1][0] = RotMatrix[1][2] = RotMatrix[1][3] = 0.;
	RotMatrix[2][0] = RotMatrix[2][1] = RotMatrix[2][3] = 0.;
	RotMatrix[3][0] = RotMatrix[3][1] = RotMatrix[3][3] = 0.;
	RotMatrix[0][0] = RotMatrix[1][1] = RotMatrix[2][2] = RotMatrix[3][3] = 1.;

	//Perfect conditions initial values
	AngleIntersection = 69.f;
	LeadingAngle = 19.4f;
	TrailingAngle = 27.1f;

	CarStart = 100.f;
	CarSpeed = 18.f;
	BikeStart = 39.0f;
	BikeSpeed = 7.0f;

	Replay();
}

void Replay()
{
	//Animations specific values
	Time = 0.f;
	CarDistance = CarStart;
	CarDistanceTravelled = 0;

	BikeDistance = BikeStart;
	BikeDistanceTravelled = 0;

	play = false;

	animate_start_time = glutGet(GLUT_ELAPSED_TIME);
	time_frozen = 0;
	Frozen = true;
}


// called when user resizes the window:
void Resize( int width, int height )
{
	if( DebugOn != 0 )
		fprintf( stderr, "ReSize: %d, %d\n", width, height );

	// don't really need to do anything since window size is
	// checked each time in Display( ):
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// handle a change to the window's visibility:
void Visibility ( int state )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Visibility: %d\n", state );

	if( state == GLUT_VISIBLE )
	{
		glutSetWindow( MainWindow );
		glutPostRedisplay( );
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}



///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


// the stroke characters 'X' 'Y' 'Z' :

static float xx[ ] = {
		0.f, 1.f, 0.f, 1.f
	      };

static float xy[ ] = {
		-.5f, .5f, .5f, -.5f
	      };

static int xorder[ ] = {
		1, 2, -3, 4
		};

static float yx[ ] = {
		0.f, 0.f, -.5f, .5f
	      };

static float yy[ ] = {
		0.f, .6f, 1.f, 1.f
	      };

static int yorder[ ] = {
		1, 2, 3, -2, 4
		};

static float zx[ ] = {
		1.f, 0.f, 1.f, 0.f, .25f, .75f
	      };

static float zy[ ] = {
		.5f, .5f, -.5f, -.5f, 0.f, 0.f
	      };

static int zorder[ ] = {
		1, 2, 3, 4, -5, 6
		};

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)
void Axes( float length )
{
	glBegin( GL_LINE_STRIP );
		glVertex3f( length, 0., 0. );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., length, 0. );
	glEnd( );
	glBegin( GL_LINE_STRIP );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., 0., length );
	glEnd( );

	float fact = LENFRAC * length;
	float base = BASEFRAC * length;

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 4; i++ )
		{
			int j = xorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( base + fact*xx[j], fact*xy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 5; i++ )
		{
			int j = yorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( fact*yx[j], base + fact*yy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 6; i++ )
		{
			int j = zorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( 0.0, fact*zy[j], base + fact*zx[j] );
		}
	glEnd( );

}

//Draw shadow triangle
void DrawShadow()
{
	float angle_difference = 180.f - AngleIntersection;
	//If the car has passed the intersection or the leading edge never interesects with the road, do not draw shadow
	if (CarDistance < 0 || angle_difference < LeadingAngle)
	{
		return; //Don't draw the shadow
	}
	//Issues occur when leading angle or trailing angle are equal to 180 - AngleIntersection
	//At this point, the distance values become unpredictable and large
	float lAngle = LeadingAngle * DEG_TO_RAD;
	float tAngle = TrailingAngle * DEG_TO_RAD;
	float iAngle = AngleIntersection * DEG_TO_RAD;

	//CSED = Car Shadow Edge Distance
	float CSED_Numerator = (CarDistance * sin(iAngle));
	float CSED_Trail =  CSED_Numerator / sin((M_PI - (tAngle + iAngle))); //Distance from trailing edge of blindspot shadow to car
	float CSED_Lead = CSED_Numerator / sin((M_PI - (lAngle + iAngle))); //Distance from leading edge blindspot shadow to car

	if (angle_difference < TrailingAngle)
	{
		CSED_Trail = 100000.f; //Fixed distance on trailing edge of shadow to prevent visual issues when there is no intersection between the road and the trailing edge
	}

	//Cleaning things up so its easier to read the next set of operations
	float Opp_Lead = sin(lAngle);
	float Opp_Trail = sin(tAngle);
	float Adj_Lead = -cos(lAngle);
	float Adj_Trail = -cos(tAngle);

	//Draw shadow
	glColor3f(1.f, 0.f, 0.f);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(0.f, .1f, CarDistance);
	glVertex3f(CSED_Lead * Opp_Lead, .1f, (CSED_Lead * Adj_Lead) + CarDistance);
	glVertex3f(CSED_Trail * Opp_Trail, .1f, (CSED_Trail * Adj_Trail) + CarDistance);
	glEnd();
}

//Draw the car and its blinders
void DrawCar(float scaleFactor)
{
	//Calculate some trig here to avoid repeat calculations 
	float lAngle = LeadingAngle * DEG_TO_RAD;
	float tAngle = TrailingAngle * DEG_TO_RAD;
	float leadX = sin(lAngle) * scaleFactor, leadZ = (-cos(lAngle) * scaleFactor);
	float trailX = sin(tAngle) * scaleFactor, trailZ = (-cos(tAngle) * scaleFactor);

	float length = 4.f;
	float height = 2.f;
	float dash_height = height / 2.f;

	//Draw the blinders
	glColor3f(0.f, 0.f, 0.f);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(leadX, 0.f, leadZ);
	glVertex3f(leadX, height, leadZ);
	glVertex3f(trailX, 0.f, trailZ);
	glVertex3f(trailX, height, trailZ);
	glEnd();

	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(-leadX, 0.f, leadZ);
	glVertex3f(-leadX, height, leadZ);
	glVertex3f(-trailX, 0.f, trailZ);
	glVertex3f(-trailX, height, trailZ);
	glEnd();

	//Roof of car
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(trailX, height, leadZ);
	glVertex3f(trailX, height, leadZ + length);
	glVertex3f(-trailX, height, leadZ);
	glVertex3f(-trailX, height, leadZ + length);
	glEnd();

	//Seats
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3f(trailX, dash_height, leadZ);
	glVertex3f(trailX, dash_height, leadZ + length);
	glVertex3f(-trailX, dash_height, leadZ);
	glVertex3f(-trailX, dash_height, leadZ + length);
	glEnd();

	//Bottom of car
		//Right side
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(trailX, 0.f, leadZ);
		glVertex3f(trailX, dash_height, leadZ);
		glVertex3f(trailX, 0.f, leadZ + length);
		glVertex3f(trailX, dash_height, leadZ + length);
		glEnd();

		//Left Side
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(-trailX, 0.f, leadZ);
		glVertex3f(-trailX, dash_height, leadZ);
		glVertex3f(-trailX, 0.f, leadZ + length);
		glVertex3f(-trailX, dash_height, leadZ + length);
		glEnd();

		//Front
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(trailX, 0.f, leadZ);
		glVertex3f(trailX, dash_height, leadZ);
		glVertex3f(-trailX, 0.f, leadZ);
		glVertex3f(-trailX, dash_height, leadZ);
		glEnd();

		//Back
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(trailX, 0.f, leadZ + length);
		glVertex3f(trailX, height, leadZ + length);
		glVertex3f(-trailX, 0.f, leadZ + length);
		glVertex3f(-trailX, height, leadZ + length);
		glEnd();
}

void UpdateGLUI(int id)
{
	switch (id)
	{
		case FOV:
			sliders[FOV].slider->set_slider_val(Fov);
			break;
		case AOI:
			sliders[AOI].slider->set_slider_val(AngleIntersection);
			break;
		case LA:
			sliders[LA].slider->set_slider_val(LeadingAngle);
			break;
		case TA:
			sliders[TA].slider->set_slider_val(TrailingAngle);
			break;
		case CSTART:
			sliders[CSTART].slider->set_slider_val(CarStart);
			break;
		case CSPEED:
			sliders[CSPEED].slider->set_slider_val(CarSpeed);
			break;
		case BSTART:
			sliders[BSTART].slider->set_slider_val(BikeStart);
			break;
		case BSPEED:
			sliders[BSPEED].slider->set_slider_val(BikeSpeed);
			break;
		default:
			sliders[FOV].slider->set_slider_val(Fov);
			sliders[AOI].slider->set_slider_val(AngleIntersection);
			sliders[LA].slider->set_slider_val(LeadingAngle);
			sliders[TA].slider->set_slider_val(TrailingAngle);
			sliders[CSTART].slider->set_slider_val(CarStart);
			sliders[CSPEED].slider->set_slider_val(CarSpeed);
			sliders[BSTART].slider->set_slider_val(BikeStart);
			sliders[BSPEED].slider->set_slider_val(BikeSpeed);
	}
	Glui->sync_live();
}