/*
 *		This Code Was Created By Jeff Molofee 2000
 *		A HUGE Thanks To Fredric Echols For Cleaning Up
 *		And Optimizing The Base Code, Making It More Flexible!
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit My Site At nehe.gamedev.net
 */

#include <windows.h>		
#include <gl/gl.h>			
#include <gl/glu.h>			
#include <gl/wglext.h>		//WGL extensions
#include <gl/glext.h>		//GL extensions

#include <math.h>
#include "Level.h"

#include "svl/SVL.h"
#include "svl/SVLgl.h"

/******************************************************************/
#define CHECK_FOR_MULTISAMPLE true

typedef struct {									// Structure For Keyboard Stuff
	BOOL keyDown [256];								// Holds TRUE / FALSE For Each Key
} Keys;												// Keys

typedef struct {									// Contains Information Vital To Applications
	HINSTANCE		hInstance;						// Application Instance
	const char*		className;						// Application ClassName
} Application;										// Application

typedef struct {									// Window Creation Info
	Application*		application;				// Application Structure
	char*				title;						// Window Title
	int					width;						// Width
	int					height;						// Height
	int					bitsPerPixel;				// Bits Per Pixel
	BOOL				isFullScreen;				// FullScreen?
} GL_WindowInit;									// GL_WindowInit

typedef struct {									// Contains Information Vital To A Window
	Keys*				keys;						// Key Structure
	HWND				hWnd;						// Window Handle
	HDC					hDC;						// Device Context
	HGLRC				hRC;						// Rendering Context
	GL_WindowInit		init;						// Window Init
	BOOL				isVisible;					// Window Visible?
	DWORD				lastTickCount;				// Tick Counter
} GL_Window;

HDC			hDC=NULL;		
HGLRC		hRC=NULL;		
HWND		hWnd=NULL;		
HINSTANCE	hInstance;		

bool	keys[256];			
bool	active=TRUE;		
bool	fullscreen=TRUE;	

GLfloat	rotx;				
GLfloat	roty;	

GLfloat rotn;

Start   Currentpos;
Start   Nextpos;
GLfloat playprog;

GLuint  acquiredTargets = 0;

bool NeedUpdate;

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	

GL_Window* g_window;

// Declarations We'll Use
#define WGL_SAMPLE_BUFFERS_ARB	0x2041
#define WGL_SAMPLES_ARB		0x2042

bool	arbMultisampleSupported	= false;
int	arbMultisampleFormat	= 0;

bool WGLisExtensionSupported(const char *extension)
{
	const size_t extlen = strlen(extension);
	const char *supported = NULL;

	// Try To Use wglGetExtensionStringARB On Current DC, If Possible
	PROC wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");

	if (wglGetExtString)
		supported = ((char*(__stdcall*)(HDC))wglGetExtString)(wglGetCurrentDC());

	// If That Failed, Try Standard Opengl Extensions String
	if (supported == NULL)
		supported = (char*)glGetString(GL_EXTENSIONS);

	// If That Failed Too, Must Be No Extensions Supported
	if (supported == NULL)
		return false;

	// Begin Examination At Start Of String, Increment By 1 On False Match
	for (const char* p = supported; ; p++)
	{
		// Advance p Up To The Next Possible Match
		p = strstr(p, extension);

		if (p == NULL)
			return false;						// No Match

		// Make Sure That Match Is At The Start Of The String Or That
		// The Previous Char Is A Space, Or Else We Could Accidentally
		// Match "wglFunkywglExtension" With "wglExtension"

		// Also, Make Sure That The Following Character Is Space Or NULL
		// Or Else "wglExtensionTwo" Might Match "wglExtension"
		if ((p==supported || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' '))
			return true;						// Match
	}
}

bool InitMultisample(HINSTANCE hInstance,HWND hWnd,PIXELFORMATDESCRIPTOR pfd)
{  
	// See If The String Exists In WGL!
	if (!WGLisExtensionSupported("WGL_ARB_multisample"))
	{
		arbMultisampleSupported=false;
		return false;
	}

	// Get Our Pixel Format
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
		(PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

	
	if(!wglChoosePixelFormatARB)
	{
		HMODULE hm = LoadLibrary("opengl32.dll");
		if(hm)
		{
			wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)GetProcAddress(hm,"wglChoosePixelFormat");
			FreeLibrary(hm);
		}
	}

	if (!wglChoosePixelFormatARB)
	{
		// We Didn't Find Support For Multisampling, Set Our Flag And Exit Out.
		arbMultisampleSupported=false;
		return false;
	}

	// Get Our Current Device Context. We Need This In Order To Ask The OpenGL Window What Attributes We Have
	HDC hDC = GetDC(hWnd);

	int pixelFormat;
	bool valid;
	UINT numFormats;
	float fAttributes[] = {0,0};

	// These Attributes Are The Bits We Want To Test For In Our Sample
	// Everything Is Pretty Standard, The Only One We Want To 
	// Really Focus On Is The SAMPLE BUFFERS ARB And WGL SAMPLES
	// These Two Are Going To Do The Main Testing For Whether Or Not
	// We Support Multisampling On This Hardware
	int iAttributes[] = { WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
		WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,24,
		WGL_ALPHA_BITS_ARB,8,
		WGL_DEPTH_BITS_ARB,16,
		WGL_STENCIL_BITS_ARB,0,
		WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
		WGL_SAMPLES_ARB, 4 ,						// Check For 4x Multisampling
		0,0};

	// First We Check To See If We Can Get A Pixel Format For 4 Samples
	valid = wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);
 
	// if We Returned True, And Our Format Count Is Greater Than 1
	if (valid && numFormats >= 1)
	{
		arbMultisampleSupported	= true;
		arbMultisampleFormat	= pixelFormat;	
		return arbMultisampleSupported;
	}

	// Our Pixel Format With 4 Samples Failed, Test For 2 Samples
	iAttributes[19] = 2;
	valid = wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);
	if (valid && numFormats >= 1)
	{
		arbMultisampleSupported	= true;
		arbMultisampleFormat	= pixelFormat;	 
		return arbMultisampleSupported;
	}
	  
	// Return The Valid Format
	return  arbMultisampleSupported;
}

BOOL Initialize (GL_Window* window, Keys* keys)										
{
	glShadeModel(GL_SMOOTH);							
	glClearColor(0.7f, 0.7f, 0.7f, 0.5f);				
	glClearDepth(1.0f);									
	glEnable(GL_DEPTH_TEST);							
	glDepthFunc(GL_LEQUAL);								
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	glEnable (GL_LINE_SMOOTH); 
	glEnable (GL_POLYGON_SMOOTH); 

	return TRUE;										
}

void DrawCubicPiece(int neighbors[6],int adjacent[6], GLfloat x, GLfloat y, GLfloat z, Cell cell)
{
	int type = cell.F;

	bool shared_sides[6] = {
		false,false,false,false,false,false
	};

	bool has_side[6] = {
		true,true,true,true,true,true,
	};

	bool sides[6] = {
		true,true,true,true,true,true,
	};

	bool edges[12] = {
		true,true,true,true,
		true,true,true,true,
		true,true,true,true,
	};

	for(int i=0;i<6;i++)
	{
		if(neighbors[i])
		{
			shared_sides[i]=true;

			Cell neighbor = Cells[neighbors[i]-1];

			if((i!=1)&&(neighbor.Y!=cell.Y)&&(neighbor.F>=4)&&(neighbor.F<8))
			{
				shared_sides[i]=false;
			}
		}
		if(adjacent[i]) shared_sides[i]=true;
	}

	switch(type)
	{
	case 4:
		has_side[0]=false;
		has_side[2]=false;
		has_side[4]=false;
		has_side[5]=false;
		break;
	case 5:
		has_side[0]=false;
		has_side[3]=false;
		has_side[4]=false;
		has_side[5]=false;
		break;
	case 6:
		has_side[0]=false;
		has_side[2]=false;
		has_side[3]=false;
		has_side[4]=false;
	case 7:
		has_side[0]=false;
		has_side[2]=false;
		has_side[3]=false;
		has_side[5]=false;
		break;
		break;

	}

	// sides:
	//  top    = 0
	//  bottom = 1
	//  front  = 2
	//  back   = 3
	//  left   = 4
	//  right  = 5
	// edges:
	//  top square     = 0..3
	//		front   = 0
	//		back    = 1
	//		left    = 2
	//		right   = 3
	//  bottom square  = 4..7
	//		front   = 4
	//		back    = 5
	//		left    = 6
	//		right   = 7
	//  vertical edges = 8..11
	//		front left   = 8
	//		front right  = 9
	//		back left    = 10
	//		back right   = 11

	if(shared_sides[0] || !has_side[0]) // top
	{
		edges[0]=false;
		edges[1]=false;
		edges[2]=false;
		edges[3]=false;

		sides[0]=false;
	}

	if(shared_sides[1] || !has_side[1]) // bottom
	{
		edges[4]=false;
		edges[5]=false;
		edges[6]=false;
		edges[7]=false;

		sides[1]=false;
	}

	if(shared_sides[2] || !has_side[2]) // front
	{
		edges[0]=false;
		edges[4]=false;
		edges[8]=false;
		edges[9]=false;

		sides[2]=false;
	}

	if(shared_sides[3] || !has_side[3]) // back
	{
		edges[1]=false;
		edges[5]=false;
		edges[10]=false;
		edges[11]=false;

		sides[3]=false;
	}

	if(shared_sides[4] || !has_side[4]) // left
	{
		edges[2]=false;
		edges[6]=false;
		edges[8]=false;
		edges[10]=false;

		sides[4]=false;
	}

	if(shared_sides[5] || !has_side[5]) // right
	{
		edges[3]=false;
		edges[7]=false;
		edges[9]=false;
		edges[11]=false;

		sides[5]=false;
	}

	if(shared_sides[0] && shared_sides[2]) // top front
	{
		edges[0] = true;
	}
	if(shared_sides[0] && shared_sides[3])
	{
		edges[1] = true;
	}
	if(shared_sides[0] && shared_sides[4])
	{
		edges[2] = true;
	}
	if(shared_sides[0] && shared_sides[5])
	{
		edges[3] = true;
	}
	if(shared_sides[1] && shared_sides[2])
	{
		edges[4] = true;
	}
	if(shared_sides[1] && shared_sides[3])
	{
		edges[5] = true;
	}
	if(shared_sides[1] && shared_sides[4])
	{
		edges[6] = true;
	}
	if(shared_sides[1] && shared_sides[5])
	{
		edges[7] = true;
	}
	if(shared_sides[2] && shared_sides[4])
	{
		edges[8] = true;
	}
	if(shared_sides[2] && shared_sides[5])
	{
		edges[9] = true;
	}
	if(shared_sides[3] && shared_sides[4])
	{
		edges[10] = true;
	}
	if(shared_sides[3] && shared_sides[5])
	{
		edges[11] = true;
	}

	glPopMatrix();
	glPushMatrix();
	glTranslatef(x,y,z);						
	glColor3f(1.0f,1.0f,1.0f);						
	if((type>=4)&&(type<8))
	{
		glPushMatrix();
		switch(type)
		{
		case 4:
			glRotatef(270,0,1,0);
			break;
		case 5:
			glRotatef(90,0,1,0);
			break;
		case 6:
			//glRotatef(100,0,1,0);
			break;
		case 7:
			glRotatef(180,0,1,0);
			break;
		}

		glBegin(GL_QUADS);	

			glVertex3f(-1.0f,-1.0f, 1.0f);					
			glVertex3f(-1.0f,-0.5f, 1.0f);					
			glVertex3f(-1.0f,-0.5f,-1.0f);					
			glVertex3f(-1.0f,-1.0f,-1.0f);

			glVertex3f(-0.5f,-0.5f, 1.0f);					
			glVertex3f(-1.0f,-0.5f, 1.0f);					
			glVertex3f(-1.0f,-0.5f,-1.0f);					
			glVertex3f(-0.5f,-0.5f,-1.0f);

			glVertex3f(-1.0f,-1.0f, 1.0f);					
			glVertex3f(-1.0f,-0.5f, 1.0f);					
			glVertex3f( 1.0f,-0.5f, 1.0f);					
			glVertex3f( 1.0f,-1.0f, 1.0f);
			glVertex3f(-1.0f,-1.0f,-1.0f);					
			glVertex3f(-1.0f,-0.5f,-1.0f);					
			glVertex3f( 1.0f,-0.5f,-1.0f);					
			glVertex3f( 1.0f,-1.0f,-1.0f);

			glVertex3f(-0.5f,-0.5f, 1.0f);					
			glVertex3f(-0.5f, 0.0f, 1.0f);					
			glVertex3f(-0.5f, 0.0f,-1.0f);					
			glVertex3f(-0.5f,-0.5f,-1.0f);

			glVertex3f( 0.0f, 0.0f, 1.0f);					
			glVertex3f(-0.5f, 0.0f, 1.0f);					
			glVertex3f(-0.5f, 0.0f,-1.0f);					
			glVertex3f( 0.0f, 0.0f,-1.0f);

			glVertex3f(-0.5f,-0.5f, 1.0f);					
			glVertex3f(-0.5f, 0.0f, 1.0f);					
			glVertex3f( 1.0f, 0.0f, 1.0f);					
			glVertex3f( 1.0f,-0.5f, 1.0f);
			glVertex3f(-0.5f,-0.5f,-1.0f);					
			glVertex3f(-0.5f, 0.0f,-1.0f);					
			glVertex3f( 1.0f, 0.0f,-1.0f);					
			glVertex3f( 1.0f,-0.5f,-1.0f);

			glVertex3f(-0.0f, 0.0f, 1.0f);					
			glVertex3f(-0.0f, 0.5f, 1.0f);					
			glVertex3f(-0.0f, 0.5f,-1.0f);					
			glVertex3f(-0.0f, 0.0f,-1.0f);

			glVertex3f( 0.5f, 0.5f, 1.0f);					
			glVertex3f( 0.0f, 0.5f, 1.0f);					
			glVertex3f( 0.0f, 0.5f,-1.0f);					
			glVertex3f( 0.5f, 0.5f,-1.0f);

			glVertex3f( 0.0f, 0.0f, 1.0f);					
			glVertex3f( 0.0f, 0.5f, 1.0f);					
			glVertex3f( 1.0f, 0.5f, 1.0f);					
			glVertex3f( 1.0f, 0.0f, 1.0f);
			glVertex3f( 0.0f, 0.0f,-1.0f);					
			glVertex3f( 0.0f, 0.5f,-1.0f);					
			glVertex3f( 1.0f, 0.5f,-1.0f);					
			glVertex3f( 1.0f, 0.0f,-1.0f);

			glVertex3f( 0.5f, 0.5f, 1.0f);					
			glVertex3f( 0.5f, 1.0f, 1.0f);					
			glVertex3f( 0.5f, 1.0f,-1.0f);					
			glVertex3f( 0.5f, 0.5f,-1.0f);

			glVertex3f( 1.0f, 1.0f, 1.0f);					
			glVertex3f( 0.5f, 1.0f, 1.0f);					
			glVertex3f( 0.5f, 1.0f,-1.0f);					
			glVertex3f( 1.0f, 1.0f,-1.0f);

			glVertex3f( 0.5f, 0.5f, 1.0f);					
			glVertex3f( 0.5f, 1.0f, 1.0f);					
			glVertex3f( 1.0f, 1.0f, 1.0f);					
			glVertex3f( 1.0f, 0.5f, 1.0f);
			glVertex3f( 0.5f, 0.5f,-1.0f);					
			glVertex3f( 0.5f, 1.0f,-1.0f);					
			glVertex3f( 1.0f, 1.0f,-1.0f);					
			glVertex3f( 1.0f, 0.5f,-1.0f);

		glEnd();
		glPopMatrix();
	}
	else if (type==2)
	{
		glBegin(GL_QUADS);
			glVertex3f( 0.0,-1.0,-0.9);
			glVertex3f( 0.0, 1.0,-0.9);
			glVertex3f(-0.6,-1.0,-0.6);
			glVertex3f(-0.6, 1.0,-0.6);
			glVertex3f(-0.6,-1.0,-0.6);
			glVertex3f(-0.6, 1.0,-0.6);
			glVertex3f(-0.9,-1.0, 0.0);
			glVertex3f(-0.9, 1.0, 0.0);
			glVertex3f(-0.9,-1.0, 0.0);
			glVertex3f(-0.9, 1.0, 0.0);
			glVertex3f(-0.6,-1.0, 0.6);
			glVertex3f(-0.6, 1.0, 0.6);
			glVertex3f(-0.6,-1.0, 0.6);
			glVertex3f(-0.6, 1.0, 0.6);
			glVertex3f( 0.0,-1.0, 0.9);
			glVertex3f( 0.0, 1.0, 0.9);
			glVertex3f( 0.0,-1.0, 0.9);
			glVertex3f( 0.0, 1.0, 0.9);
			glVertex3f( 0.6,-1.0, 0.6);
			glVertex3f( 0.6, 1.0, 0.6);
			glVertex3f( 0.6,-1.0, 0.6);
			glVertex3f( 0.6, 1.0, 0.6);
			glVertex3f( 0.9,-1.0, 0.0);
			glVertex3f( 0.9, 1.0, 0.0);
			glVertex3f( 0.9,-1.0, 0.0);
			glVertex3f( 0.9, 1.0, 0.0);
			glVertex3f( 0.6,-1.0,-0.6);
			glVertex3f( 0.6, 1.0,-0.6);
			glVertex3f( 0.6,-1.0,-0.6);
			glVertex3f( 0.6, 1.0,-0.6);
			glVertex3f( 0.0,-1.0,-0.9);
			glVertex3f( 0.0, 1.0,-0.9);
			glVertex3f( 1.0,-1.0,-0.0);
			glVertex3f( 1.0,-1.0,-1.0);
			glVertex3f( 0.6,-1.0,-0.6);
			glVertex3f( 0.9,-1.0,-0.0);
			glVertex3f( 1.0,-1.0, 1.0);
			glVertex3f( 1.0,-1.0,-0.0);
			glVertex3f( 0.9,-1.0,-0.0);
			glVertex3f( 0.6,-1.0, 0.6);
			glVertex3f( 0.0,-1.0, 1.0);
			glVertex3f( 1.0,-1.0, 1.0);
			glVertex3f( 0.6,-1.0, 0.6);
			glVertex3f( 0.0,-1.0, 0.9);
			glVertex3f(-1.0,-1.0, 1.0);
			glVertex3f( 0.0,-1.0, 1.0);
			glVertex3f( 0.0,-1.0, 0.9);
			glVertex3f(-0.6,-1.0, 0.6);
			glVertex3f(-1.0,-1.0,-0.0);
			glVertex3f(-1.0,-1.0, 1.0);
			glVertex3f(-0.6,-1.0, 0.6);
			glVertex3f(-0.9,-1.0,-0.0);
			glVertex3f(-1.0,-1.0,-1.0);
			glVertex3f(-1.0,-1.0,-0.0);
			glVertex3f(-0.9,-1.0,-0.0);
			glVertex3f(-0.6,-1.0,-0.6);
			glVertex3f( 0.0,-1.0,-1.0);
			glVertex3f(-1.0,-1.0,-1.0);
			glVertex3f(-0.6,-1.0,-0.6);
			glVertex3f( 0.0,-1.0,-0.9);
			glVertex3f( 1.0,-1.0,-1.0);
			glVertex3f( 0.0,-1.0,-1.0);
			glVertex3f( 0.0,-1.0,-0.9);
			glVertex3f( 0.6,-1.0,-0.6);
			glVertex3f(-1.0, 1.0,-0.0);
			glVertex3f(-1.0, 1.0,-1.0);
			glVertex3f(-0.6, 1.0,-0.6);
			glVertex3f(-0.9, 1.0,-0.0);
			glVertex3f(-1.0, 1.0, 1.0);
			glVertex3f(-1.0, 1.0,-0.0);
			glVertex3f(-0.9, 1.0,-0.0);
			glVertex3f(-0.6, 1.0, 0.6);
			glVertex3f( 0.0, 1.0, 1.0);
			glVertex3f(-1.0, 1.0, 1.0);
			glVertex3f(-0.6, 1.0, 0.6);
			glVertex3f( 0.0, 1.0, 0.9);
			glVertex3f( 1.0, 1.0, 1.0);
			glVertex3f( 0.0, 1.0, 1.0);
			glVertex3f( 0.0, 1.0, 0.9);
			glVertex3f( 0.6, 1.0, 0.6);
			glVertex3f( 1.0, 1.0,-0.0);
			glVertex3f( 1.0, 1.0, 1.0);
			glVertex3f( 0.6, 1.0, 0.6);
			glVertex3f( 0.9, 1.0,-0.0);
			glVertex3f( 1.0, 1.0,-1.0);
			glVertex3f( 1.0, 1.0,-0.0);
			glVertex3f( 0.9, 1.0,-0.0);
			glVertex3f( 0.6, 1.0,-0.6);
			glVertex3f( 0.0, 1.0,-1.0);
			glVertex3f( 1.0, 1.0,-1.0);
			glVertex3f( 0.6, 1.0,-0.6);
			glVertex3f( 0.0, 1.0,-0.9);
			glVertex3f(-1.0, 1.0,-1.0);
			glVertex3f( 0.0, 1.0,-1.0);
			glVertex3f( 0.0, 1.0,-0.9);
			glVertex3f(-0.6, 1.0,-0.6);
		glEnd();
	}
	glBegin(GL_QUADS);
		if(sides[0]&&(type!=2))
		{
			glVertex3f( 1.0f, 1.0f, 1.0f);					
			glVertex3f(-1.0f, 1.0f, 1.0f);					
			glVertex3f(-1.0f, 1.0f,-1.0f);					
			glVertex3f( 1.0f, 1.0f,-1.0f);
		}
		if(sides[1]&&(type!=2))
		{
			glVertex3f( 1.0f,-1.0f, 1.0f);					
			glVertex3f(-1.0f,-1.0f, 1.0f);					
			glVertex3f(-1.0f,-1.0f,-1.0f);					
			glVertex3f( 1.0f,-1.0f,-1.0f);					
		}
		if(sides[2]) 
		{
			glVertex3f( 1.0f, 1.0f,-1.0f);					
			glVertex3f(-1.0f, 1.0f,-1.0f);					
			glVertex3f(-1.0f,-1.0f,-1.0f);					
			glVertex3f( 1.0f,-1.0f,-1.0f);					
		}
		if(sides[3])
		{
			glVertex3f( 1.0f,-1.0f, 1.0f);					
			glVertex3f(-1.0f,-1.0f, 1.0f);					
			glVertex3f(-1.0f, 1.0f, 1.0f);					
			glVertex3f( 1.0f, 1.0f, 1.0f);					
		}
		if(sides[4])
		{
			glVertex3f(-1.0f, 1.0f,-1.0f);					
			glVertex3f(-1.0f, 1.0f, 1.0f);					
			glVertex3f(-1.0f,-1.0f, 1.0f);					
			glVertex3f(-1.0f,-1.0f,-1.0f);					
		}
		if(sides[5])
		{
			glVertex3f( 1.0f, 1.0f, 1.0f);					
			glVertex3f( 1.0f, 1.0f,-1.0f);					
			glVertex3f( 1.0f,-1.0f,-1.0f);					
			glVertex3f( 1.0f,-1.0f, 1.0f);					
		}
	glEnd();											

	glPopMatrix();
	glPushMatrix();
	glTranslatef(x,y,z);						
	glLineWidth(3.0f);
	glColor3f(0.2f,0.2f,0.2f);

	if((type>=4)&&(type<8))
	{
		glPushMatrix();
		switch(type)
		{
		case 4:
			glRotatef(270,0,1,0);
			break;
		case 5:
			glRotatef(90,0,1,0);
			break;
		case 6:
			//glRotatef(100,0,1,0);
			break;
		case 7:
			glRotatef(180,0,1,0);
			break;
		}

		glBegin(GL_LINES);	

			glVertex3f(-1.01f,-1.01f, 1.01f);					
			glVertex3f(-1.01f,-0.49f, 1.01f);					
			glVertex3f(-1.01f,-1.01f,-1.01f);					
			glVertex3f(-1.01f,-0.49f,-1.01f);					

			glVertex3f(-0.51f,-0.49f, 1.01f);					
			glVertex3f(-1.01f,-0.49f, 1.01f);					
			glVertex3f(-1.01f,-0.49f, 1.01f);					
			glVertex3f(-1.01f,-0.49f,-1.01f);					
			glVertex3f(-1.01f,-0.49f,-1.01f);					
			glVertex3f(-0.51f,-0.49f,-1.01f);
			glVertex3f(-0.51f,-0.49f,-1.01f);
			glVertex3f(-0.51f,-0.49f, 1.01f);					

			// ---
			glVertex3f(-0.51f,-0.49f, 1.01f);					
			glVertex3f(-0.51f, 0.01f, 1.01f);					
			glVertex3f(-0.51f, 0.01f,-1.01f);					
			glVertex3f(-0.51f,-0.49f,-1.01f);					

			glVertex3f(-0.01f, 0.01f, 1.01f);					
			glVertex3f(-0.51f, 0.01f, 1.01f);
			glVertex3f(-0.51f, 0.01f, 1.01f);					
			glVertex3f(-0.51f, 0.01f,-1.01f);					
			glVertex3f(-0.51f, 0.01f,-1.01f);					
			glVertex3f(-0.01f, 0.01f,-1.01f);
			glVertex3f(-0.01f, 0.01f,-1.01f);
			glVertex3f(-0.01f, 0.01f, 1.01f);					

			// ---
			glVertex3f(-0.01f, 0.01f, 1.01f);					
			glVertex3f(-0.01f, 0.51f, 1.01f);					
			glVertex3f(-0.01f, 0.51f,-1.01f);					
			glVertex3f(-0.01f, 0.01f,-1.01f);					

			glVertex3f( 0.49f, 0.51f, 1.01f);					
			glVertex3f(-0.01f, 0.51f, 1.01f);					
			glVertex3f(-0.01f, 0.51f, 1.01f);					
			glVertex3f(-0.01f, 0.51f,-1.01f);					
			glVertex3f(-0.01f, 0.51f,-1.01f);					
			glVertex3f( 0.49f, 0.51f,-1.01f);
			glVertex3f( 0.49f, 0.51f,-1.01f);
			glVertex3f( 0.49f, 0.51f, 1.01f);					

			// ---
			glVertex3f( 0.49f, 0.51f, 1.01f);					
			glVertex3f( 0.49f, 1.01f, 1.01f);					
			glVertex3f( 0.49f, 1.01f,-1.01f);					
			glVertex3f( 0.49f, 0.51f,-1.01f);					

			glVertex3f( 1.01f, 1.01f, 1.01f);					
			glVertex3f( 0.49f, 1.01f, 1.01f);					
			glVertex3f( 0.49f, 1.01f, 1.01f);					
			glVertex3f( 0.49f, 1.01f,-1.01f);					
			glVertex3f( 0.49f, 1.01f,-1.01f);					
			glVertex3f( 1.01f, 1.01f,-1.01f);
			glVertex3f( 1.01f, 1.01f,-1.01f);
			glVertex3f( 1.01f, 1.01f, 1.01f);					

			//glVertex3f( 1.01f, 1.00f, 1.01f);					
			//glVertex3f( 1.01f,-1.01f, 1.01f);					
			//glVertex3f( 1.01f,-1.01f,-1.01f);					
			//glVertex3f( 1.01f, 1.00f,-1.01f);					

			glVertex3f(-1.01f,-1.01f,-1.01f);					
			glVertex3f(-1.01f,-1.01f, 1.01f);	

			if(sides[1])
			{
				glVertex3f(-1.01f,-1.01f,-1.01f);					
				glVertex3f( 1.01f,-1.01f,-1.01f);	
				glVertex3f(-1.01f,-1.01f, 1.01f);					
				glVertex3f( 1.01f,-1.01f, 1.01f);	
			}

		glEnd();
		glPopMatrix();
	}
	else if(type==2)
	{
		glBegin(GL_LINES);
			glVertex3f( 0.00,-1,-0.89);
			glVertex3f( 0.59,-1,-0.59);
			glVertex3f( 0.59,-1,-0.59);
			glVertex3f( 0.89,-1, 0.00);
			glVertex3f( 0.89,-1, 0.00);
			glVertex3f( 0.59,-1, 0.59);
			glVertex3f( 0.59,-1, 0.59);
			glVertex3f( 0.00,-1, 0.89);
			glVertex3f( 0.00,-1, 0.89);
			glVertex3f(-0.59,-1, 0.59);
			glVertex3f(-0.59,-1, 0.59);
			glVertex3f(-0.89,-1, 0.00);
			glVertex3f(-0.89,-1, 0.00);
			glVertex3f(-0.59,-1,-0.59);
			glVertex3f(-0.59,-1,-0.59);
			glVertex3f( 0.00,-1,-0.89);

			glVertex3f( 0.00,1,-0.89);
			glVertex3f( 0.59,1,-0.59);
			glVertex3f( 0.59,1,-0.59);
			glVertex3f( 0.89,1, 0.00);
			glVertex3f( 0.89,1, 0.00);
			glVertex3f( 0.59,1, 0.59);
			glVertex3f( 0.59,1, 0.59);
			glVertex3f( 0.00,1, 0.89);
			glVertex3f( 0.00,1, 0.89);
			glVertex3f(-0.59,1, 0.59);
			glVertex3f(-0.59,1, 0.59);
			glVertex3f(-0.89,1, 0.00);
			glVertex3f(-0.89,1, 0.00);
			glVertex3f(-0.59,1,-0.59);
			glVertex3f(-0.59,1,-0.59);
			glVertex3f( 0.00,1,-0.89);

			glVertex3f( 0.00,-1,-0.89);
			glVertex3f( 0.00, 1,-0.89);

			glVertex3f( 0.59,-1,-0.59);
			glVertex3f( 0.59, 1,-0.59);

			glVertex3f( 0.89,-1, 0.00);
			glVertex3f( 0.89, 1, 0.00);

			glVertex3f( 0.59,-1, 0.59);
			glVertex3f( 0.59, 1, 0.59);

			glVertex3f( 0.00,-1, 0.89);
			glVertex3f( 0.00, 1, 0.89);

			glVertex3f(-0.59,-1, 0.59);
			glVertex3f(-0.59, 1, 0.59);

			glVertex3f(-0.89,-1, 0.00);
			glVertex3f(-0.89, 1, 0.00);

			glVertex3f(-0.59,-1,-0.59);
			glVertex3f(-0.59, 1,-0.59);
		glEnd();
	}
	glBegin(GL_LINES);
		if(edges[0])
		{
			glVertex3f( 1.01f, 1.01f,-1.01f);					
			glVertex3f(-1.01f, 1.01f,-1.01f);					
		}
		if(edges[1])
		{
			glVertex3f( 1.01f, 1.01f, 1.01f);					
			glVertex3f(-1.01f, 1.01f, 1.01f);					
		}
		if(edges[2])
		{
			glVertex3f(-1.01f, 1.01f, 1.01f);					
			glVertex3f(-1.01f, 1.01f,-1.01f);					
		}
		if(edges[3])
		{
			glVertex3f( 1.01f, 1.01f, 1.01f);					
			glVertex3f( 1.01f, 1.01f,-1.01f);					
		}
 		if(edges[4])
		{
			glVertex3f( 1.01f,-1.01f,-1.01f);					
			glVertex3f(-1.01f,-1.01f,-1.01f);					
		}
		if(edges[5])
		{
			glVertex3f( 1.01f,-1.01f, 1.01f);					
			glVertex3f(-1.01f,-1.01f, 1.01f);					
		}
		if(edges[6])
		{
			glVertex3f(-1.01f,-1.01f, 1.01f);					
			glVertex3f(-1.01f,-1.01f,-1.01f);					
		}
		if(edges[7])
		{
			glVertex3f( 1.01f,-1.01f, 1.01f);					
			glVertex3f( 1.01f,-1.01f,-1.01f);					
		}

		if(edges[8])
		{
			glVertex3f(-1.01f, 1.01f,-1.01f);					
			glVertex3f(-1.01f,-1.01f,-1.01f);					
		}
		if(edges[9])
		{
			glVertex3f( 1.01f, 1.01f,-1.01f);					
			glVertex3f( 1.01f,-1.01f,-1.01f);					
		}
		if(edges[10])
		{
			glVertex3f(-1.01f, 1.01f, 1.01f);					
			glVertex3f(-1.01f,-1.01f, 1.01f);					
		}
		if(edges[11])
		{
			glVertex3f( 1.01f, 1.01f, 1.01f);					
			glVertex3f( 1.01f,-1.01f, 1.01f);					
		}
	glEnd();
}

int  CheckNeighbor(int x, int y, int z, bool checkStairs)
{
	if(x<0) return 0;
	if(y<0) return 0;
	if(z<0) return 0;

	if(x>=Size.X) return 0;
	if(y>=Size.Y) return 0;
	if(z>=Size.Z) return 0;

	if(checkStairs&&(y<Size.Y))
	{
		if((Grid[x][y+1][z]>=4)&&(Grid[x][y+1][z]<8))
			return GridNumber[x][y+1][z]+1;
	}

	if((Grid[x][y][z]>0)&&(Grid[x][y][z]!=128))
	{
		if(Grid[x][y][z]==129)
		{
			return GridNumber[x][y][z]+1;
		}
		return GridNumber[x][y][z]+1;
	}

	return 0;
}

void DrawGrid()
{
	GLfloat xoff = Size.X;
	GLfloat yoff = Size.Y;
	GLfloat zoff = Size.Z;

	Real tx = floor(rotx) * 3.1415926536 / 180;
	Real ty = floor(roty) * 3.1415926536 / 180;

	Mat4 m1 = Mat4(
		1,    0,       0,   0,
		0, cos(tx), sin(tx),0,
		0,-sin(tx), cos(tx),0,
		0,    0,       0,   1)
			* Mat4(
		 cos(ty),0,-sin(ty),0,
		    0,   1,    0,   0,
		 sin(ty),0, cos(ty),0,
		    0,   0,    0,   1);
	//m1 = m1.Transpose();

	for(int i=0;i<NumCells;i++)
	{
		int x = Cells[i].X;
		int y = Cells[i].Y;
		int z = Cells[i].Z;

		if(Cells[i].F==130)
		{
			Cells[i].F = 2;
		}

		if((Cells[i].F>=4)&&(Cells[i].F<8))
		{
			GridAdjacent[x][y][z][0] = CheckNeighbor(x,y+1,z,false);
			GridAdjacent[x][y][z][1] = CheckNeighbor(x,y-1,z,false);
			GridAdjacent[x][y][z][2] = CheckNeighbor(x,y,z+1,false);
			GridAdjacent[x][y][z][3] = CheckNeighbor(x,y,z-1,false);
			GridAdjacent[x][y][z][4] = CheckNeighbor(x-1,y,z,false);
			GridAdjacent[x][y][z][5] = CheckNeighbor(x+1,y,z,false);

			GridNeighbor[x][y][z][1] = CheckNeighbor(x,y-1,z,false);

			switch(Cells[i].F)
			{
			case 4:
				GridNeighbor[x][y][z][2] = CheckNeighbor(x,y-1,z+1,true);
				GridNeighbor[x][y][z][3] = CheckNeighbor(x,y,z-1,true);
				GridNeighbor[x][y][z][4] = CheckNeighbor(x-1,y,z,true);
				GridNeighbor[x][y][z][5] = CheckNeighbor(x+1,y,z,true);
				break;
			case 5:
				GridNeighbor[x][y][z][2] = CheckNeighbor(x,y,z+1,true);
				GridNeighbor[x][y][z][3] = CheckNeighbor(x,y-1,z-1,true);
				GridNeighbor[x][y][z][4] = CheckNeighbor(x-1,y,z,true);
				GridNeighbor[x][y][z][5] = CheckNeighbor(x+1,y,z,true);
				break;
			case 6:
				GridNeighbor[x][y][z][2] = CheckNeighbor(x,y,z+1,true);
				GridNeighbor[x][y][z][3] = CheckNeighbor(x,y,z-1,true);
				GridNeighbor[x][y][z][4] = CheckNeighbor(x-1,y-1,z,true);
				GridNeighbor[x][y][z][5] = CheckNeighbor(x+1,y,z,true);
				break;
			case 7:
				GridNeighbor[x][y][z][2] = CheckNeighbor(x,y,z+1,true);
				GridNeighbor[x][y][z][3] = CheckNeighbor(x,y,z-1,true);
				GridNeighbor[x][y][z][4] = CheckNeighbor(x-1,y,z,true);
				GridNeighbor[x][y][z][5] = CheckNeighbor(x+1,y-1,z,true);
				break;
			}
		}
		else if(Cells[i].F==128)
		{
			GridNeighbor[x][y][z][0] = 0;
			GridNeighbor[x][y][z][1] = 0;
			GridNeighbor[x][y][z][2] = 0;
			GridNeighbor[x][y][z][3] = 0;
			GridNeighbor[x][y][z][4] = 0;
			GridNeighbor[x][y][z][5] = 0;

			GridAdjacent[x][y][z][0] = 0;
			GridAdjacent[x][y][z][1] = 0;
			GridAdjacent[x][y][z][2] = 0;
			GridAdjacent[x][y][z][3] = 0;
			GridAdjacent[x][y][z][4] = 0;
			GridAdjacent[x][y][z][5] = 0;
		}
		else
		{
			if(x==1 && y==0 && z==0)
			{
				x=1;
			}

			GridNeighbor[x][y][z][0] = CheckNeighbor(x,y+1,z,false);
			GridNeighbor[x][y][z][1] = CheckNeighbor(x,y-1,z,false);
			GridNeighbor[x][y][z][2] = CheckNeighbor(x,y,z+1,true);
			GridNeighbor[x][y][z][3] = CheckNeighbor(x,y,z-1,true);
			GridNeighbor[x][y][z][4] = CheckNeighbor(x-1,y,z,true);
			GridNeighbor[x][y][z][5] = CheckNeighbor(x+1,y,z,true);

			GridAdjacent[x][y][z][0] = CheckNeighbor(x,y+1,z,false);
			GridAdjacent[x][y][z][1] = CheckNeighbor(x,y-1,z,false);
			GridAdjacent[x][y][z][2] = CheckNeighbor(x,y,z+1,false);
			GridAdjacent[x][y][z][3] = CheckNeighbor(x,y,z-1,false);
			GridAdjacent[x][y][z][4] = CheckNeighbor(x-1,y,z,false);
			GridAdjacent[x][y][z][5] = CheckNeighbor(x+1,y,z,false);
		}
	}

	for(int i=0;i<NumCells;i++)
	{
		for(int j=0;j<NumCells;j++)
		{
			if((i!=j)&&(Cells[i].F!=128)&&(Cells[j].F!=128))
			{
				// front j with back i
				if( (!GridNeighbor[Cells[j].X][Cells[j].Y][Cells[j].Z][0])&&
					(!GridNeighbor[Cells[i].X][Cells[i].Y][Cells[i].Z][0])&&
					(!GridNeighbor[Cells[j].X][Cells[j].Y][Cells[j].Z][3])&&
					(!GridNeighbor[Cells[i].X][Cells[i].Y][Cells[i].Z][2])&&
					((Cells[i].F<4)||(Cells[i].F>=8))&&
					((Cells[j].F<4)||(Cells[j].F>=8)))
				{
					Vec4 celli_a(Cells[i].X*2-1,Cells[i].Y*2+1,Cells[i].Z*2+1,1);
					Vec4 celli_b(Cells[i].X*2+1,Cells[i].Y*2+1,Cells[i].Z*2+1,1);

					Vec4 cellj_a(Cells[j].X*2-1,Cells[j].Y*2+1,Cells[j].Z*2-1,1);
					Vec4 cellj_b(Cells[j].X*2+1,Cells[j].Y*2+1,Cells[j].Z*2-1,1);

					Vec4 ti_a (m1 * celli_a);
					Vec4 ti_b (m1 * celli_b);
					Vec4 tj_a (m1 * cellj_a);
					Vec4 tj_b (m1 * cellj_b);

					Vec4 td_a = tj_a - ti_a;
					Vec4 td_b = tj_b - ti_b;

					Vec4 td = td_a + td_b;

					Real l = len(Vec2(td[0],td[1]));

					if((i==3)&&(j==1))
					{
						l=l * 1.00001; 
					}

					if((l<0.1)&&(Cells[j].Y!=Cells[i].Y)&&(len(celli_a-cellj_a)>1))
					{
						GridNeighbor[Cells[j].X][Cells[j].Y][Cells[j].Z][3] = i+1;
						GridNeighbor[Cells[i].X][Cells[i].Y][Cells[i].Z][2] = j+1;
					}
				}

				// left j with right i
				if( (!GridNeighbor[Cells[j].X][Cells[j].Y][Cells[j].Z][0])&&
					(!GridNeighbor[Cells[i].X][Cells[i].Y][Cells[i].Z][0])&&
					(!GridNeighbor[Cells[j].X][Cells[j].Y][Cells[j].Z][4])&&
					(!GridNeighbor[Cells[i].X][Cells[i].Y][Cells[i].Z][5])&&
					((Cells[i].F<4)||(Cells[i].F>=8))&&
					((Cells[j].F<4)||(Cells[j].F>=8)))
				{
					Vec4 celli_a(Cells[i].X*2+1,Cells[i].Y*2+1,Cells[i].Z*2-1,1);
					Vec4 celli_b(Cells[i].X*2+1,Cells[i].Y*2+1,Cells[i].Z*2+1,1);

					Vec4 cellj_a(Cells[j].X*2-1,Cells[j].Y*2+1,Cells[j].Z*2-1,1);
					Vec4 cellj_b(Cells[j].X*2-1,Cells[j].Y*2+1,Cells[j].Z*2+1,1);

					//Vec4 ti_a (celli_a * m1);
					//Vec4 ti_b (celli_b * m1);
					//Vec4 tj_a (cellj_a * m1);
					//Vec4 tj_b (cellj_b * m1);
					Vec4 ti_a (m1 * celli_a);
					Vec4 ti_b (m1 * celli_b);
					Vec4 tj_a (m1 * cellj_a);
					Vec4 tj_b (m1 * cellj_b);

					Vec4 td_a = tj_a - ti_a;
					Vec4 td_b = tj_b - ti_b;

					Vec4 td = td_a + td_b;

					Real l = len(Vec2(td[0],td[1]));

					if((i==3)&&(j==1))
					{
						l=l * 1.00001; 
					}

					if((l<0.25)&&(Cells[j].Y!=Cells[i].Y))
					{
						GridNeighbor[Cells[j].X][Cells[j].Y][Cells[j].Z][4] = i+1;
						GridNeighbor[Cells[i].X][Cells[i].Y][Cells[i].Z][5] = j+1;
					}
				}
			}
		}
	}
	for(int i=0;i<NumCells;i++)
	{
		int x = Cells[i].X;
		int y = Cells[i].Y;
		int z = Cells[i].Z;
		if((Cells[i].F!=128)&&(Cells[i].F!=129))
		{
			DrawCubicPiece(GridNeighbor[x][y][z],GridAdjacent[x][y][z],2*x-xoff,2*y-yoff,zoff-2*z,Cells[i]);
		}
	}

	// Occlusion testing for holes (Perspective Abscence)
	for(int i=0;i<NumCells;i++)
	{
		if((Cells[i].F==2)||(Cells[i].F==130))
		{
			GLboolean result;

			// disable updates to color and depth buffer (optional)
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
			// enable occlusion test
			glEnable(GL_OCCLUSION_TEST_HP);
			// render bounding geometry

			int x = Cells[i].X*2-xoff;
			int y = Cells[i].Y*2-yoff;
			int z = zoff-Cells[i].Z*2;

			glPopMatrix();
			glPushMatrix();
			glTranslatef(x,y,z);						
			glBegin(GL_QUADS);
				glVertex3f( 0.0,1.02,-0.9);
				glVertex3f( 0.9,1.02,-0.6);
				glVertex3f( 0.9,1.02, 0.00);
				glVertex3f( 0.0,1.02, 0.00);


				glVertex3f( 0.9,1.02, 0.0);
				glVertex3f( 0.6,1.02, 0.6);
				glVertex3f( 0.0,1.02, 0.9);
				glVertex3f( 0.0,1.02, 0.0);

				glVertex3f( 0.0,1.02, 0.9);
				glVertex3f(-0.6,1.02, 0.6);
				glVertex3f(-0.9,1.02, 0.0);
				glVertex3f( 0.0,1.02, 0.0);

				glVertex3f(-0.9,1.02, 0.0);
				glVertex3f(-0.6,1.02,-0.6);
				glVertex3f( 0.0,1.02,-0.9);
				glVertex3f( 0.0,1.02, 0.0);
			glEnd();

			// disable occlusion test
			glDisable(GL_OCCLUSION_TEST_HP);
			// enable updates to color and depth buffer
			glDepthMask(GL_TRUE);
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			// read occlusion test result
			glGetBooleanv(GL_OCCLUSION_TEST_RESULT_HP, &result);

			Cells[i].F = (result)?2:130;
			Grid[Cells[i].X][Cells[i].Y][Cells[i].Z]=Cells[i].F;
		}

		if((Cells[i].F==128)||(Cells[i].F==129))
		{
			GLboolean result;

			// disable updates to color and depth buffer (optional)
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
			// enable occlusion test
			glEnable(GL_OCCLUSION_TEST_HP);
			// render bounding geometry

			int x = Cells[i].X*2-xoff;
			int y = Cells[i].Y*2-yoff;
			int z = zoff-Cells[i].Z*2;

			glPopMatrix();
			glPushMatrix();
			glTranslatef(x,y,z);						
			glBegin(GL_QUADS);
				glVertex3f(-0.95,1.00,-0.95);
				glVertex3f(-0.95,1.00, 0.95);
				glVertex3f( 0.95,1.00, 0.95);
				glVertex3f( 0.95,1.00,-0.95);
				glVertex3f(-0.95,-1.00,-0.95);
				glVertex3f(-0.95,-1.00, 0.95);
				glVertex3f( 0.95,-1.00, 0.95);
				glVertex3f( 0.95,-1.00,-0.95);

				glVertex3f(-0.95,-0.95,1.00);
				glVertex3f(-0.95, 0.95,1.00);
				glVertex3f( 0.95, 0.95,1.00);
				glVertex3f( 0.95,-0.95,1.00);
				glVertex3f(-0.95,-0.95,-1.00);
				glVertex3f(-0.95, 0.95,-1.00);
				glVertex3f( 0.95, 0.95,-1.00);
				glVertex3f( 0.95,-0.95,-1.00);

				glVertex3f(1.00,-0.95,-0.95);
				glVertex3f(1.00,-0.95, 0.95);
				glVertex3f(1.00, 0.95, 0.95);
				glVertex3f(1.00, 0.95,-0.95);
				glVertex3f(-1.00,-0.95,-0.95);
				glVertex3f(-1.00,-0.95, 0.95);
				glVertex3f(-1.00, 0.95, 0.95);
				glVertex3f(-1.00, 0.95,-0.95);
			glEnd();

			// disable occlusion test
			glDisable(GL_OCCLUSION_TEST_HP);
			// enable updates to color and depth buffer
			glDepthMask(GL_TRUE);
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			// read occlusion test result
			glGetBooleanv(GL_OCCLUSION_TEST_RESULT_HP, &result);

			if(result)
			{
				Cells[i].F=128;
				Grid[Cells[i].X][Cells[i].Y][Cells[i].Z]=Cells[i].F;
			}
			else
			{
				Cells[i].F = 129;
				Grid[Cells[i].X][Cells[i].Y][Cells[i].Z]=Cells[i].F;
			}
		}
	}

	NeedUpdate=false;
}

void DrawTarget(GLfloat x, GLfloat y, GLfloat z)
{
	glPopMatrix();
	glPushMatrix();
	glTranslatef(x,y,z);
	glRotatef(rotn,0,1,0);
	glColor3f(0.4f,0.4f,0.4f);

	glBegin(GL_TRIANGLES);	

		// Front Side
		glVertex3f(-0.8f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.8f, 0.0f);					
		glVertex3f( 0.0f, 0.0f,-0.3f);					

		glVertex3f( 0.0f, 0.8f, 0.0f);					
		glVertex3f( 0.8f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.0f,-0.3f);					

		glVertex3f( 0.8f, 0.0f, 0.0f);					
		glVertex3f( 0.0f,-0.8f, 0.0f);					
		glVertex3f( 0.0f, 0.0f,-0.3f);					

		glVertex3f( 0.0f,-0.8f, 0.0f);					
		glVertex3f(-0.8f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.0f,-0.3f);					

		// Back Side
		glVertex3f( 0.8f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.8f, 0.0f);					
		glVertex3f( 0.0f, 0.0f, 0.3f);					

		glVertex3f( 0.0f, 0.8f, 0.0f);					
		glVertex3f(-0.8f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.0f, 0.3f);					

		glVertex3f(-0.8f, 0.0f, 0.0f);					
		glVertex3f( 0.0f,-0.8f, 0.0f);					
		glVertex3f( 0.0f, 0.0f, 0.3f);					

		glVertex3f( 0.0f,-0.8f, 0.0f);					
		glVertex3f( 0.8f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.0f, 0.3f);					
	glEnd();											

	glLineWidth(3.0f);
	glColor3f(0.2f,0.2f,0.2f);
	glBegin(GL_LINES);									
	
		glVertex3f(-0.81f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.81f, 0.00f);	

		glVertex3f( 0.00f, 0.81f, 0.00f);					
		glVertex3f( 0.81f, 0.00f, 0.00f);					

		glVertex3f( 0.81f, 0.00f, 0.00f);					
		glVertex3f( 0.00f,-0.81f, 0.00f);					

		glVertex3f( 0.00f,-0.81f, 0.00f);					
		glVertex3f(-0.81f, 0.00f, 0.00f);					

	glEnd();											
	glLineWidth(1.5f);
	glBegin(GL_LINES);									

		// Front Side
		glVertex3f(-0.81f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.00f,-0.31f);					

		glVertex3f( 0.00f, 0.81f, 0.00f);	
		glVertex3f( 0.00f, 0.00f,-0.31f);					

		glVertex3f( 0.81f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.00f,-0.31f);					

		glVertex3f( 0.00f,-0.81f, 0.00f);					
		glVertex3f( 0.00f, 0.00f,-0.31f);					

		// Back Side
		glVertex3f(-0.81f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.00f, 0.31f);					

		glVertex3f( 0.00f, 0.81f, 0.00f);	
		glVertex3f( 0.00f, 0.00f, 0.31f);					

		glVertex3f( 0.81f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.00f, 0.31f);					

		glVertex3f( 0.00f,-0.81f, 0.00f);					
		glVertex3f( 0.00f, 0.00f, 0.31f);					

	glEnd();
}

void DrawTargets()
{
	GLfloat xoff = Size.X;
	GLfloat yoff = Size.Y;
	GLfloat zoff = Size.Z;

	for(int i=0;i<NumTargets;i++)
	{
		if((Targets[i].N <= acquiredTargets)&&(!Targets[i].A))
		{
			DrawTarget(Targets[i].X*2-xoff,Targets[i].Y*2-yoff+2,zoff-Targets[i].Z*2);
		}
	}
}

void DrawPlayer(GLfloat x, GLfloat y, GLfloat z, GLfloat d)
{
	GLfloat xoff = Size.X;
	GLfloat yoff = Size.Y;
	GLfloat zoff = Size.Z;

	glPopMatrix();
	glPushMatrix();

	glTranslatef(2*x-xoff,2*y-yoff+2,zoff-2*z);
	glRotatef(180 - d * 360/4,0,1,0);
	glColor3f(0.8f,0.8f,0.8f);

	glBegin(GL_TRIANGLES);	

		// Front Side
		glVertex3f(-0.6f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.6f, 0.0f);					
		glVertex3f( 0.0f, 0.0f,-0.8f);					

		glVertex3f( 0.0f, 0.6f, 0.0f);					
		glVertex3f( 0.6f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.0f,-0.8f);					

		glVertex3f( 0.6f, 0.0f, 0.0f);					
		glVertex3f( 0.0f,-0.6f, 0.0f);					
		glVertex3f( 0.0f, 0.0f,-0.8f);					

		glVertex3f( 0.0f,-0.6f, 0.0f);					
		glVertex3f(-0.6f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.0f,-0.8f);					

		// Back Side
		glVertex3f( 0.6f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.6f, 0.0f);					
		glVertex3f( 0.0f, 0.0f, 0.6f);					

		glVertex3f( 0.0f, 0.6f, 0.0f);					
		glVertex3f(-0.6f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.0f, 0.6f);					

		glVertex3f(-0.6f, 0.0f, 0.0f);					
		glVertex3f( 0.0f,-0.6f, 0.0f);					
		glVertex3f( 0.0f, 0.0f, 0.6f);					

		glVertex3f( 0.0f,-0.6f, 0.0f);					
		glVertex3f( 0.6f, 0.0f, 0.0f);					
		glVertex3f( 0.0f, 0.0f, 0.6f);					
	glEnd();											

	glLineWidth(3.0f);
	glColor3f(0.2f,0.2f,0.2f);
	glBegin(GL_LINES);									
	
		glVertex3f(-0.61f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.61f, 0.00f);	

		glVertex3f( 0.00f, 0.61f, 0.00f);					
		glVertex3f( 0.61f, 0.00f, 0.00f);					

		glVertex3f( 0.61f, 0.00f, 0.00f);					
		glVertex3f( 0.00f,-0.61f, 0.00f);					

		glVertex3f( 0.00f,-0.61f, 0.00f);					
		glVertex3f(-0.61f, 0.00f, 0.00f);					

	glEnd();											
	glLineWidth(1.5f);
	glBegin(GL_LINES);									

		// Front Side
		glVertex3f(-0.61f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.00f,-0.81f);					

		glVertex3f( 0.00f, 0.61f, 0.00f);	
		glVertex3f( 0.00f, 0.00f,-0.81f);					

		glVertex3f( 0.61f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.00f,-0.81f);					

		glVertex3f( 0.00f,-0.61f, 0.00f);					
		glVertex3f( 0.00f, 0.00f,-0.81f);					

		// Back Side
		glVertex3f(-0.61f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.00f, 0.61f);					

		glVertex3f( 0.00f, 0.61f, 0.00f);	
		glVertex3f( 0.00f, 0.00f, 0.61f);					

		glVertex3f( 0.61f, 0.00f, 0.00f);					
		glVertex3f( 0.00f, 0.00f, 0.61f);					

		glVertex3f( 0.00f,-0.61f, 0.00f);					
		glVertex3f( 0.00f, 0.00f, 0.61f);					

	glEnd();
}

void Draw (void)							
{
	glEnable(GL_MULTISAMPLE_ARB);							// Enable Our Multisampling

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glLoadIdentity();									
	glTranslatef(0,0,-50);		
	glRotatef(rotx,1,0,0);		
	glRotatef(roty,0,1,0);		
	glPushMatrix();

	DrawGrid();
	DrawTargets();

	if( (Grid[Currentpos.X][Currentpos.Y][Currentpos.Z]>=4)&&
		(Grid[Currentpos.X][Currentpos.Y][Currentpos.Z]<8)&&
		(Grid[Nextpos.X][Nextpos.Y][Nextpos.Z]>=4)&&
		(Grid[Nextpos.X][Nextpos.Y][Nextpos.Z]<8))
	{
		GLfloat td = Nextpos.Y - Currentpos.Y;
		if(td>0)
		{
			switch(Currentpos.D)
			{
			case 0: DrawPlayer(Currentpos.X,         Currentpos.Y+playprog-0.5,Currentpos.Z-playprog,Currentpos.D); break;
			case 1: DrawPlayer(Currentpos.X-playprog,Currentpos.Y+playprog-0.5,Currentpos.Z,         Currentpos.D); break;
			case 2: DrawPlayer(Currentpos.X,         Currentpos.Y+playprog-0.5,Currentpos.Z+playprog,Currentpos.D); break;
			case 3: DrawPlayer(Currentpos.X+playprog,Currentpos.Y+playprog-0.5,Currentpos.Z,         Currentpos.D); break;
			}
		}
		else
		{
			switch(Currentpos.D)
			{
			case 0: DrawPlayer(Currentpos.X,         Currentpos.Y-playprog-0.5,Currentpos.Z-playprog,Currentpos.D); break;
			case 1: DrawPlayer(Currentpos.X-playprog,Currentpos.Y-playprog-0.5,Currentpos.Z,         Currentpos.D); break;
			case 2: DrawPlayer(Currentpos.X,         Currentpos.Y-playprog-0.5,Currentpos.Z+playprog,Currentpos.D); break;
			case 3: DrawPlayer(Currentpos.X+playprog,Currentpos.Y-playprog-0.5,Currentpos.Z,         Currentpos.D); break;
			}
		}
	}
	else
	if( (Grid[Currentpos.X][Currentpos.Y][Currentpos.Z]>=4)&&
		(Grid[Currentpos.X][Currentpos.Y][Currentpos.Z]<8))
	{
		if(playprog<=0.5)
		{
			GLfloat td = Nextpos.Y - Currentpos.Y;
			if(td>=0)
			{
				switch(Currentpos.D)
				{
				case 0: DrawPlayer(Currentpos.X,         Currentpos.Y+playprog-0.5,Currentpos.Z-playprog,Currentpos.D); break;
				case 1: DrawPlayer(Currentpos.X-playprog,Currentpos.Y+playprog-0.5,Currentpos.Z,         Currentpos.D); break;
				case 2: DrawPlayer(Currentpos.X,         Currentpos.Y+playprog-0.5,Currentpos.Z+playprog,Currentpos.D); break;
				case 3: DrawPlayer(Currentpos.X+playprog,Currentpos.Y+playprog-0.5,Currentpos.Z,         Currentpos.D); break;
				}
			}
			else
			{
				switch(Currentpos.D)
				{
				case 0: DrawPlayer(Currentpos.X,         Currentpos.Y-playprog-0.5,Currentpos.Z-playprog,Currentpos.D); break;
				case 1: DrawPlayer(Currentpos.X-playprog,Currentpos.Y-playprog-0.5,Currentpos.Z,         Currentpos.D); break;
				case 2: DrawPlayer(Currentpos.X,         Currentpos.Y-playprog-0.5,Currentpos.Z+playprog,Currentpos.D); break;
				case 3: DrawPlayer(Currentpos.X+playprog,Currentpos.Y-playprog-0.5,Currentpos.Z,         Currentpos.D); break;
				}
			}
		}
		else
		{
			switch(Currentpos.D)
			{
			case 0: DrawPlayer(Nextpos.X,           Nextpos.Y,Nextpos.Z+1-playprog,Currentpos.D); break;
			case 1: DrawPlayer(Nextpos.X+1-playprog,Nextpos.Y,Nextpos.Z,           Currentpos.D); break;
			case 2: DrawPlayer(Nextpos.X,           Nextpos.Y,Nextpos.Z-1+playprog,Currentpos.D); break;
			case 3: DrawPlayer(Nextpos.X-1+playprog,Nextpos.Y,Nextpos.Z,           Currentpos.D); break;
			}
		}
	}
	else
	if( (Grid[Nextpos.X][Nextpos.Y][Nextpos.Z]>=4)&&
		(Grid[Nextpos.X][Nextpos.Y][Nextpos.Z]<8))
	{
		if(playprog<=0.5)
		{
			switch(Currentpos.D)
			{
				case 0: DrawPlayer(Currentpos.X,         Currentpos.Y,Currentpos.Z-playprog,Currentpos.D); break;
				case 1: DrawPlayer(Currentpos.X-playprog,Currentpos.Y,Currentpos.Z,         Currentpos.D); break;
				case 2: DrawPlayer(Currentpos.X,         Currentpos.Y,Currentpos.Z+playprog,Currentpos.D); break;
				case 3: DrawPlayer(Currentpos.X+playprog,Currentpos.Y,Currentpos.Z,         Currentpos.D); break;
			}
		}
		else
		{
			GLfloat td = Nextpos.Y - Currentpos.Y;
			if(td>0)
			{
				switch(Currentpos.D)
				{
				case 0: DrawPlayer(Currentpos.X,         Currentpos.Y+playprog-0.5,Currentpos.Z-playprog,Currentpos.D); break;
				case 1: DrawPlayer(Currentpos.X-playprog,Currentpos.Y+playprog-0.5,Currentpos.Z,         Currentpos.D); break;
				case 2: DrawPlayer(Currentpos.X,         Currentpos.Y+playprog-0.5,Currentpos.Z+playprog,Currentpos.D); break;
				case 3: DrawPlayer(Currentpos.X+playprog,Currentpos.Y+playprog-0.5,Currentpos.Z,         Currentpos.D); break;
				}
			}
			else
			{
				switch(Currentpos.D)
				{
				case 0: DrawPlayer(Currentpos.X,         Currentpos.Y-playprog+0.5,Currentpos.Z-playprog,Currentpos.D); break;
				case 1: DrawPlayer(Currentpos.X-playprog,Currentpos.Y-playprog+0.5,Currentpos.Z,         Currentpos.D); break;
				case 2: DrawPlayer(Currentpos.X,         Currentpos.Y-playprog+0.5,Currentpos.Z+playprog,Currentpos.D); break;
				case 3: DrawPlayer(Currentpos.X+playprog,Currentpos.Y-playprog+0.5,Currentpos.Z,         Currentpos.D); break;
				}
			}
		}
	}
	else
	{
#define DOUBLE_DRAW
#ifdef DOUBLE_DRAW
		switch(Currentpos.D)
		{
		case 0:
			DrawPlayer(Currentpos.X,Currentpos.Y,Currentpos.Z-  playprog,Currentpos.D);
			DrawPlayer(Nextpos.X   ,Nextpos.Y   ,Nextpos.Z   +1-playprog,Currentpos.D);
			break;
		case 1:
			DrawPlayer(Currentpos.X-  playprog,Currentpos.Y,Currentpos.Z,Currentpos.D);
			DrawPlayer(Nextpos.X   +1-playprog,Nextpos.Y   ,Nextpos.Z   ,Currentpos.D);
			break;
		case 2:
			DrawPlayer(Currentpos.X,Currentpos.Y,Currentpos.Z+  playprog,Currentpos.D);
			DrawPlayer(Nextpos.X   ,Nextpos.Y   ,Nextpos.Z   -1+playprog,Currentpos.D);
			break;
		case 3:
			DrawPlayer(Currentpos.X+  playprog,Currentpos.Y,Currentpos.Z,Currentpos.D);
			DrawPlayer(Nextpos.X   -1+playprog,Nextpos.Y   ,Nextpos.Z   ,Currentpos.D);
			break;
		}
#else
		GLfloat tx = Currentpos.X + playprog*(Nextpos.X-Currentpos.X);
		GLfloat ty = Currentpos.Y + playprog*(Nextpos.Y-Currentpos.Y);
		GLfloat tz = Currentpos.Z + playprog*(Nextpos.Z-Currentpos.Z);
		DrawPlayer(tx,ty,tz,Currentpos.D);
#endif
	}
	glPopMatrix();

	glDisable(GL_MULTISAMPLE_ARB);
}

void Deinitialize (void)										// Any User DeInitialization Goes Here
{
}

void TerminateApplication (GL_Window* window);
void ToggleFullscreen (GL_Window* window);

bool falling=false;
int fallTimes=0;

void Update (DWORD milliseconds)								// Perform Motion Updates Here
{
	if(GetKeyState(VK_LEFT)&0x8000)  roty-=milliseconds * 0.05f;	
	if(GetKeyState(VK_RIGHT)&0x8000) roty+=milliseconds * 0.05f;	

	if(GetKeyState(VK_UP)&0x8000)    rotx-=milliseconds * 0.05f;	
	if(GetKeyState(VK_DOWN)&0x8000)  rotx+=milliseconds * 0.05f;	

	if(rotx<0.1) rotx=0.1;
	if(rotx<CameraMin.X) rotx = CameraMin.X;
	if(rotx>CameraMax.X) rotx = CameraMax.X;

	if((CameraMin.Y!=0)||(CameraMax.Y!=360))
	{
		if(roty<CameraMin.Y) roty = CameraMin.Y;
		if(roty>CameraMax.Y) roty = CameraMax.Y;
	}

	if (GetKeyState(VK_ESCAPE)&0x8000)					// Is ESC Being Pressed?
	{
		TerminateApplication (g_window);						// Terminate The Program
	}

	if (GetKeyState(VK_F1)&0x8000)						// Is F1 Being Pressed?
	{
		ToggleFullscreen (g_window);							// Toggle Fullscreen Mode
	}

	rotn += milliseconds * 0.025f;

	if(Grid[Currentpos.X][Currentpos.Y][Currentpos.Z]==128)
	{
		Currentpos = Startpos;
	}

	int neighbor = 0;

	if(falling||(fallTimes>0))
	{
		Real tx = floor(rotx) * 3.1415926536 / 180;
		Real ty = floor(roty) * 3.1415926536 / 180;

		Mat4 m1 = Mat4(
			1,    0,       0,   0,
			0, cos(tx), sin(tx),0,
			0,-sin(tx), cos(tx),0,
			0,    0,       0,   1)
				* Mat4(
			 cos(ty),0,-sin(ty),0,
				0,   1,    0,   0,
			 sin(ty),0, cos(ty),0,
				0,   0,    0,   1);

		Vec4 cell_n(Currentpos.X*2,Currentpos.Y*2-1,Currentpos.Z*2,1);
		Vec4 ti_n (m1 * cell_n);

		int mn = NumCells;
		int md = 10000;

		int j= GridNumber[Currentpos.X][Currentpos.Y][Currentpos.Z];

		if(falling)
		{
			for(int i=0;i<NumCells;i++)
			{
	#ifdef FULL_CHECK
				if((i!=j)&&(Cells[j].F!=128))
				{
					Vec4 cell_a(Cells[i].X*2-1,Cells[i].Y*2+1,Cells[i].Z*2+1,1);
					Vec4 cell_b(Cells[i].X*2+1,Cells[i].Y*2+1,Cells[i].Z*2+1,1);
					Vec4 cell_c(Cells[i].X*2+1,Cells[i].Y*2+1,Cells[i].Z*2-1,1);
					Vec4 cell_d(Cells[i].X*2-1,Cells[i].Y*2+1,Cells[i].Z*2-1,1);

					Vec4 ti_a (m1 * cell_a);
					Vec4 ti_b (m1 * cell_b);
					Vec4 ti_c (m1 * cell_c);
					Vec4 ti_d (m1 * cell_d);

					if( (ti_n[1]>ti_a[1])&&(ti_n[1]>ti_b[1])&&(ti_n[1]>ti_c[1])&&(ti_n[1]>ti_d[1]))
					{
						if(
							((ti_n[0]>=ti_a[0])&&(ti_n[0]<=ti_b[0]))||
							((ti_n[0]>=ti_b[0])&&(ti_n[0]<=ti_c[0]))||
							((ti_n[0]>=ti_c[0])&&(ti_n[0]<=ti_d[0]))||
							((ti_n[0]>=ti_d[0])&&(ti_n[0]<=ti_a[0])))
						{
							int da = abs(ti_a[1] - ti_n[1]);
							int db = abs(ti_b[1] - ti_n[1]);
							int dc = abs(ti_c[1] - ti_n[1]);
							int dd = abs(ti_d[1] - ti_n[1]);

							if(db<da) da=db;
							if(dc<da) da=dc;
							if(dd<da) da=dd;

							if(da<md) {
								md = da;
								mn = i;
							}
						}
					}
				}
	#else
				if((i!=j)&&(Cells[j].F!=128)&&
					(GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][2]!=(j+1))&&
					(GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][3]!=(j+1))&&
					(GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][4]!=(j+1))&&
					(GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][5]!=(j+1))&&
					(GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][6]!=(j+1))&&
					(GridNeighbor[Cells[j].X][Cells[j].Y][Cells[j].Z][0]==0))
				{
					Vec4 cell_a(Cells[i].X*2-1,Cells[i].Y*2+1,Cells[i].Z*2+1,1);
					Vec4 cell_b(Cells[i].X*2+1,Cells[i].Y*2+1,Cells[i].Z*2+1,1);
					Vec4 cell_c(Cells[i].X*2+1,Cells[i].Y*2+1,Cells[i].Z*2-1,1);
					Vec4 cell_d(Cells[i].X*2-1,Cells[i].Y*2+1,Cells[i].Z*2-1,1);
					Vec4 ti_a (m1 * cell_a);
					Vec4 ti_b (m1 * cell_b);
					Vec4 ti_c (m1 * cell_c);
					Vec4 ti_d (m1 * cell_d);

					int mnx = ti_a[0];
					int mxx = ti_a[0];

					if(ti_b[0]<mnx) mnx=ti_b[0];
					if(ti_c[0]<mnx) mnx=ti_c[0];
					if(ti_d[0]<mnx) mnx=ti_d[0];
					if(ti_b[0]>mxx) mxx=ti_b[0];
					if(ti_c[0]>mxx) mxx=ti_c[0];
					if(ti_d[0]>mxx) mxx=ti_d[0];

					if((ti_n[0]>mnx)&&(ti_n[0]<mxx)&&(ti_a[1]<ti_n[1]))
					{
						int da = abs(ti_a[1] - ti_n[1]);
						int db = abs(ti_b[1] - ti_n[1]);
						int dc = abs(ti_c[1] - ti_n[1]);
						int dd = abs(ti_d[1] - ti_n[1]);

						if(db<da) da=db;
						if(dc<da) da=dc;
						if(dd<da) da=dd;

						if(da<md) {
							md = da;
							mn = i;
						}
					}
				}
	#endif
			}

			falling=false;
			if((mn==NumCells)||(Cells[mn].F==0)||(Cells[mn].F==128))
			{
				Currentpos = Startpos;
			}
			else
			{
				Nextpos.X = Cells[mn].X;
				Nextpos.Y = Cells[mn].Y;
				Nextpos.Z = Cells[mn].Z;
			}
		}
		fallTimes-=milliseconds;
	}
	else
	{
		switch(Currentpos.D)
		{
		case 0:	// forward (z+1 or z-1?)
			neighbor = GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][3];
			break;
		case 1:	// left
			neighbor = GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][4];
			break;
		case 2:	// backwards
			neighbor = GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][2];
			break;
		case 3:	// right
			neighbor = GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][5];
			break;
		}
		if(neighbor==128) neighbor=0;

		playprog += milliseconds * 0.001f;
		if((playprog>=1)||(neighbor==0)||NeedUpdate)
		{
			playprog=0;

			if(neighbor!=0)
			{
				if(Cells[neighbor-1].F==2)
				{
					falling=true;
					fallTimes=1000;
					return;
				}
				else
				{
					Currentpos.X = Cells[neighbor-1].X;
					Currentpos.Y = Cells[neighbor-1].Y;
					Currentpos.Z = Cells[neighbor-1].Z;
				}

				// check if caught a target
				for(int i=0;i<NumTargets;i++)
				{
					if( (Targets[i].X==Currentpos.X)&&
						(Targets[i].Y==Currentpos.Y)&&
						(Targets[i].Z==Currentpos.Z)&&
						(Targets[i].N<=acquiredTargets)&&
						(!Targets[i].A))
					{
						Targets[i].A=true;
						acquiredTargets++;
					}
				}
			}

			for(int i=+1;i>-3;i--)
			{
				switch((Currentpos.D+i+4)%4)
				{
				case 0:	// forward (z+1 or z-1?)
					neighbor = GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][3];
					break;
				case 1:	// left
					neighbor = GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][4];
					break;
				case 2:	// backwards
					neighbor = GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][2];
					break;
				case 3:	// right
					neighbor = GridNeighbor[Currentpos.X][Currentpos.Y][Currentpos.Z][5];
					break;
				}
				if(neighbor==128) neighbor=0;
				if(neighbor>0) 
				{
					Currentpos.D = ((Currentpos.D+i+4)%4); 
					break; 
				}
			}

			if(neighbor!=0)
			{
				Nextpos.X = Cells[neighbor-1].X;
				Nextpos.Y = Cells[neighbor-1].Y;
				Nextpos.Z = Cells[neighbor-1].Z;
			}
		}
	}
}

bool mouseLook = false;
POINT cPos;
void MouseDown(DWORD button)
{
	if(button == VK_LBUTTON)
	{
		mouseLook=true;
		SetCapture(g_window->hWnd);
		GetCursorPos(&cPos);
		ShowCursor(0);
	}
}

void MouseMove(POINT newPos) // newPos contains relative coordinates!
{
	if(mouseLook)
	{
		GetCursorPos(&newPos);

		int xdiff = newPos.x - cPos.x;
		int ydiff = newPos.y - cPos.y;

		if(xdiff || ydiff)
		{
			rotx += ydiff * 0.25f;
			roty += xdiff * 0.25f;

			if(rotx<0.1) rotx=0.1;
			if(rotx<CameraMin.X) rotx = CameraMin.X;
			if(rotx>CameraMax.X) rotx = CameraMax.X;

			if((CameraMin.Y!=0)||(CameraMax.Y!=360))
			{
				if(roty<CameraMin.Y) roty = CameraMin.Y;
				if(roty>CameraMax.Y) roty = CameraMax.Y;
			}

			SetCursorPos(cPos.x,cPos.y);
		}
	}
}

void MouseUp(DWORD button)
{
	if(button == VK_LBUTTON)
	{
		if(mouseLook)
		{
			while(ShowCursor(1)) {}
			ReleaseCapture();
			SetCursorPos(cPos.x,cPos.y);
			mouseLook=false;
		}
	}
}

/***********************************************
*                                              *
*    Jeff Molofee's Revised OpenGL Basecode    *
*  Huge Thanks To Maxwell Sayles & Peter Puck  *
*            http://nehe.gamedev.net           *
*                     2001                     *
*                                              *
***********************************************/

#include <windows.h>													// Header File For The Windows Library
#include <gl/gl.h>														// Header File For The OpenGL32 Library
#include <gl/glu.h>														// Header File For The GLu32 Library

BOOL DestroyWindowGL (GL_Window* window);
BOOL CreateWindowGL (GL_Window* window);
//ENDROACH

#define WM_TOGGLEFULLSCREEN (WM_USER+1)									// Application Define Message For Toggling

static BOOL g_isProgramLooping;											// Window Creation Loop, For FullScreen/Windowed Toggle																		// Between Fullscreen / Windowed Mode
static BOOL g_createFullScreen;											// If TRUE, Then Create Fullscreen

void TerminateApplication (GL_Window* window)							// Terminate The Application
{

	PostMessage (window->hWnd, WM_QUIT, 0, 0);							// Send A WM_QUIT Message
	g_isProgramLooping = FALSE;											// Stop Looping Of The Program
}

void ToggleFullscreen (GL_Window* window)								// Toggle Fullscreen/Windowed
{
	PostMessage (window->hWnd, WM_TOGGLEFULLSCREEN, 0, 0);				// Send A WM_TOGGLEFULLSCREEN Message
}

void ReshapeGL (int width, int height)									// Reshape The Window When It's Moved Or Resized
{
	if (height==0)										
	{
		height=1;										
	}

	glViewport(0,0,width,height);						

	glMatrixMode(GL_PROJECTION);						
	glLoadIdentity();									

	GLfloat aratio = (GLfloat)width/(GLfloat)height;
	GLfloat viewSize = sqrt((GLfloat)((Size.X*Size.X)+(Size.Y*Size.Y)+(Size.Z*Size.Z)));
	
	if(width>height)
	{
		glOrtho(-viewSize*aratio,viewSize*aratio,-viewSize,viewSize,0.1,1000);
	}
	else
	{
		glOrtho(-viewSize,viewSize,-viewSize/aratio,viewSize/aratio,0.1,1000);
	}

	glMatrixMode(GL_MODELVIEW);							
	glLoadIdentity();									
}

BOOL ChangeScreenResolution (int width, int height, int bitsPerPixel)	// Change The Screen Resolution
{
	DEVMODE dmScreenSettings;											// Device Mode
	ZeroMemory (&dmScreenSettings, sizeof (DEVMODE));					// Make Sure Memory Is Cleared
	dmScreenSettings.dmSize				= sizeof (DEVMODE);				// Size Of The Devmode Structure
	dmScreenSettings.dmPelsWidth		= width;						// Select Screen Width
	dmScreenSettings.dmPelsHeight		= height;						// Select Screen Height
	dmScreenSettings.dmBitsPerPel		= bitsPerPixel;					// Select Bits Per Pixel
	dmScreenSettings.dmFields			= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	if (ChangeDisplaySettings (&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
		return FALSE;													// Display Change Failed, Return False
	}
	return TRUE;														// Display Change Was Successful, Return True
}

BOOL CreateWindowGL (GL_Window* window)									// This Code Creates Our OpenGL Window
{
	DWORD windowStyle = WS_OVERLAPPEDWINDOW;							// Define Our Window Style
	DWORD windowExtendedStyle = WS_EX_APPWINDOW;						// Define The Window's Extended Style

	PIXELFORMATDESCRIPTOR pfd =											// pfd Tells Windows How We Want Things To Be
	{
		sizeof (PIXELFORMATDESCRIPTOR),									// Size Of This Pixel Format Descriptor
		1,																// Version Number
		PFD_DRAW_TO_WINDOW |											// Format Must Support Window
		PFD_SUPPORT_OPENGL |											// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,												// Must Support Double Buffering
		PFD_TYPE_RGBA,													// Request An RGBA Format
		window->init.bitsPerPixel,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,												// Color Bits Ignored
		1,																// Alpha Buffer
		0,																// Shift Bit Ignored
		0,																// No Accumulation Buffer
		0, 0, 0, 0,														// Accumulation Bits Ignored
		16,																// 16Bit Z-Buffer (Depth Buffer)  
		0,																// No Stencil Buffer
		0,																// No Auxiliary Buffer
		PFD_MAIN_PLANE,													// Main Drawing Layer
		0,																// Reserved
		0, 0, 0															// Layer Masks Ignored
	};

	RECT windowRect = {0, 0, window->init.width, window->init.height};	// Define Our Window Coordinates

	GLuint PixelFormat;													// Will Hold The Selected Pixel Format

	if (window->init.isFullScreen == TRUE)								// Fullscreen Requested, Try Changing Video Modes
	{
		if (ChangeScreenResolution (window->init.width, window->init.height, window->init.bitsPerPixel) == FALSE)
		{
			// Fullscreen Mode Failed.  Run In Windowed Mode Instead
			MessageBox (HWND_DESKTOP, "Mode Switch Failed.\nRunning In Windowed Mode.", "Error", MB_OK | MB_ICONEXCLAMATION);
			window->init.isFullScreen = FALSE;							// Set isFullscreen To False (Windowed Mode)
		}
		else															// Otherwise (If Fullscreen Mode Was Successful)
		{
			ShowCursor (FALSE);											// Turn Off The Cursor
			windowStyle = WS_POPUP;										// Set The WindowStyle To WS_POPUP (Popup Window)
			windowExtendedStyle |= WS_EX_TOPMOST;						// Set The Extended Window Style To WS_EX_TOPMOST
		}																// (Top Window Covering Everything Else)
	}
	else																// If Fullscreen Was Not Selected
	{
		// Adjust Window, Account For Window Borders
		AdjustWindowRectEx (&windowRect, windowStyle, 0, windowExtendedStyle);
	}

	// Create The OpenGL Window
	window->hWnd = CreateWindowEx (windowExtendedStyle,					// Extended Style
								   window->init.application->className,	// Class Name
								   window->init.title,					// Window Title
								   windowStyle,							// Window Style
								   0, 0,								// Window X,Y Position
								   windowRect.right - windowRect.left,	// Window Width
								   windowRect.bottom - windowRect.top,	// Window Height
								   HWND_DESKTOP,						// Desktop Is Window's Parent
								   0,									// No Menu
								   window->init.application->hInstance, // Pass The Window Instance
								   window);

	if (window->hWnd == 0)												// Was Window Creation A Success?
	{
		return FALSE;													// If Not Return False
	}

	window->hDC = GetDC (window->hWnd);									// Grab A Device Context For This Window
	if (window->hDC == 0)												// Did We Get A Device Context?
	{
			// Failed
		DestroyWindow (window->hWnd);									// Destroy The Window
		window->hWnd = 0;												// Zero The Window Handle
		return FALSE;													// Return False
	}

//ROACH
	/*
	Our first pass, Multisampling hasn't been created yet, so we create a window normally
	If it is supported, then we're on our second pass
	that means we want to use our pixel format for sampling
	so set PixelFormat to arbMultiSampleformat instead
  */
	if(!arbMultisampleSupported)
	{
		PixelFormat = ChoosePixelFormat (window->hDC, &pfd);				// Find A Compatible Pixel Format
		if (PixelFormat == 0)												// Did We Find A Compatible Format?
		{
			// Failed
			ReleaseDC (window->hWnd, window->hDC);							// Release Our Device Context
			window->hDC = 0;												// Zero The Device Context
			DestroyWindow (window->hWnd);									// Destroy The Window
			window->hWnd = 0;												// Zero The Window Handle
			return FALSE;													// Return False
		}

	}
	else
	{
		PixelFormat = arbMultisampleFormat;
	}
//ENDROACH

	if (SetPixelFormat (window->hDC, PixelFormat, &pfd) == FALSE)		// Try To Set The Pixel Format
	{
		// Failed
		ReleaseDC (window->hWnd, window->hDC);							// Release Our Device Context
		window->hDC = 0;												// Zero The Device Context
		DestroyWindow (window->hWnd);									// Destroy The Window
		window->hWnd = 0;												// Zero The Window Handle
		return FALSE;													// Return False
	}

	window->hRC = wglCreateContext (window->hDC);						// Try To Get A Rendering Context
	if (window->hRC == 0)												// Did We Get A Rendering Context?
	{
		// Failed
		ReleaseDC (window->hWnd, window->hDC);							// Release Our Device Context
		window->hDC = 0;												// Zero The Device Context
		DestroyWindow (window->hWnd);									// Destroy The Window
		window->hWnd = 0;												// Zero The Window Handle
		return FALSE;													// Return False
	}

	// Make The Rendering Context Our Current Rendering Context
	if (wglMakeCurrent (window->hDC, window->hRC) == FALSE)
	{
		// Failed
		wglDeleteContext (window->hRC);									// Delete The Rendering Context
		window->hRC = 0;												// Zero The Rendering Context
		ReleaseDC (window->hWnd, window->hDC);							// Release Our Device Context
		window->hDC = 0;												// Zero The Device Context
		DestroyWindow (window->hWnd);									// Destroy The Window
		window->hWnd = 0;												// Zero The Window Handle
		return FALSE;													// Return False
	}

	
//ROACH
	/*
	Now that our window is created, we want to queary what samples are available
	we call our InitMultiSample window
	if we return a valid context, we want to destroy our current window
	and create a new one using the multisample interface.
	*/
	if(!arbMultisampleSupported && CHECK_FOR_MULTISAMPLE)
	{
	
		if(InitMultisample(window->init.application->hInstance,window->hWnd,pfd))
		{
			
			DestroyWindowGL (window);
			return CreateWindowGL(window);
		}
	}

//ENDROACH

	ShowWindow (window->hWnd, SW_NORMAL);								// Make The Window Visible
	window->isVisible = TRUE;											// Set isVisible To True

	ReshapeGL (window->init.width, window->init.height);				// Reshape Our GL Window

	ZeroMemory (window->keys, sizeof (Keys));							// Clear All Keys

	window->lastTickCount = GetTickCount ();							// Get Tick Count

	return TRUE;														// Window Creating Was A Success
																		// Initialization Will Be Done In WM_CREATE
}

BOOL DestroyWindowGL (GL_Window* window)								// Destroy The OpenGL Window & Release Resources
{
	if (window->hWnd != 0)												// Does The Window Have A Handle?
	{	
		if (window->hDC != 0)											// Does The Window Have A Device Context?
		{
			wglMakeCurrent (window->hDC, 0);							// Set The Current Active Rendering Context To Zero
			if (window->hRC != 0)										// Does The Window Have A Rendering Context?
			{
				wglDeleteContext (window->hRC);							// Release The Rendering Context
				window->hRC = 0;										// Zero The Rendering Context

			}
			ReleaseDC (window->hWnd, window->hDC);						// Release The Device Context
			window->hDC = 0;											// Zero The Device Context
		}
		DestroyWindow (window->hWnd);									// Destroy The Window
		window->hWnd = 0;												// Zero The Window Handle
	}

	if (window->init.isFullScreen)										// Is Window In Fullscreen Mode
	{
		ChangeDisplaySettings (NULL,0);									// Switch Back To Desktop Resolution
		ShowCursor (TRUE);												// Show The Cursor
	}	
	return TRUE;														// Return True
}

// Process Window Message Callbacks
LRESULT CALLBACK WindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Get The Window Context
	GL_Window* window = (GL_Window*)(GetWindowLong (hWnd, GWL_USERDATA));

	switch (uMsg)														// Evaluate Window Message
	{
		case WM_SYSCOMMAND:												// Intercept System Commands
		{
			switch (wParam)												// Check System Calls
			{
				case SC_SCREENSAVE:										// Screensaver Trying To Start?
				case SC_MONITORPOWER:									// Monitor Trying To Enter Powersave?
				return 0;												// Prevent From Happening
			}
			break;														// Exit
		}
		return 0;														// Return

		case WM_CREATE:													// Window Creation
		{
			CREATESTRUCT* creation = (CREATESTRUCT*)(lParam);			// Store Window Structure Pointer
			window = (GL_Window*)(creation->lpCreateParams);
			SetWindowLong (hWnd, GWL_USERDATA, (LONG)(window));
		}
		return 0;														// Return

		case WM_CLOSE:													// Closing The Window
			TerminateApplication(window);								// Terminate The Application
		return 0;														// Return

		case WM_SIZE:													// Size Action Has Taken Place
			switch (wParam)												// Evaluate Size Action
			{
				case SIZE_MINIMIZED:									// Was Window Minimized?
					window->isVisible = FALSE;							// Set isVisible To False
				return 0;												// Return

				case SIZE_MAXIMIZED:									// Was Window Maximized?
					window->isVisible = TRUE;							// Set isVisible To True
					ReshapeGL (LOWORD (lParam), HIWORD (lParam));		// Reshape Window - LoWord=Width, HiWord=Height
				return 0;												// Return

				case SIZE_RESTORED:										// Was Window Restored?
					window->isVisible = TRUE;							// Set isVisible To True
					ReshapeGL (LOWORD (lParam), HIWORD (lParam));		// Reshape Window - LoWord=Width, HiWord=Height
				return 0;												// Return
			}
		break;															// Break

		case WM_KEYDOWN:												// Update Keyboard Buffers For Keys Pressed
			if ((wParam >= 0) && (wParam <= 255))						// Is Key (wParam) In A Valid Range?
			{
				window->keys->keyDown [wParam] = TRUE;					// Set The Selected Key (wParam) To True
				return 0;												// Return
			}
		break;															// Break

		case WM_KEYUP:													// Update Keyboard Buffers For Keys Released
			if ((wParam >= 0) && (wParam <= 255))						// Is Key (wParam) In A Valid Range?
			{
				window->keys->keyDown [wParam] = FALSE;					// Set The Selected Key (wParam) To False
				return 0;												// Return
			}
		break;															// Break

		case WM_TOGGLEFULLSCREEN:										// Toggle FullScreen Mode On/Off
			g_createFullScreen = (g_createFullScreen == TRUE) ? FALSE : TRUE;
			PostMessage (hWnd, WM_QUIT, 0, 0);
		break;	// Break

		case WM_MOUSEMOVE:
			{
				POINT pt;
				pt.x = LOWORD(lParam); 
				pt.y = HIWORD(lParam); 
				MouseMove(pt);
			}
			break;
		case WM_LBUTTONDOWN:
			MouseDown(VK_LBUTTON);
			break;
		case WM_LBUTTONUP:
			MouseUp(VK_LBUTTON);
			break;
		case WM_RBUTTONDOWN:
			MouseDown(VK_RBUTTON);
			break;
		case WM_RBUTTONUP:
			MouseUp(VK_RBUTTON);
			break;
		case WM_MBUTTONDOWN:
			MouseDown(VK_MBUTTON);
			break;
		case WM_MBUTTONUP:
			MouseUp(VK_MBUTTON);
			break;
	}

	return DefWindowProc (hWnd, uMsg, wParam, lParam);					// Pass Unhandled Messages To DefWindowProc
}

BOOL RegisterWindowClass (Application* application)						// Register A Window Class For This Application.
{																		// TRUE If Successful
	// Register A Window Class
	WNDCLASSEX windowClass;												// Window Class
	ZeroMemory (&windowClass, sizeof (WNDCLASSEX));						// Make Sure Memory Is Cleared
	windowClass.cbSize			= sizeof (WNDCLASSEX);					// Size Of The windowClass Structure
	windowClass.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraws The Window For Any Movement / Resizing
	windowClass.lpfnWndProc		= (WNDPROC)(WindowProc);				// WindowProc Handles Messages
	windowClass.hInstance		= application->hInstance;				// Set The Instance
	windowClass.hbrBackground	= (HBRUSH)(COLOR_APPWORKSPACE);			// Class Background Brush Color
	windowClass.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	windowClass.lpszClassName	= application->className;				// Sets The Applications Classname
	if (RegisterClassEx (&windowClass) == 0)							// Did Registering The Class Fail?
	{
		// NOTE: Failure, Should Never Happen
		MessageBox (HWND_DESKTOP, "RegisterClassEx Failed!", "Error", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;													// Return False (Failure)
	}
	return TRUE;														// Return True (Success)
}

// Program Entry (WinMain)
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Application			application;									// Application Structure
	GL_Window			window;											// Window Structure
	Keys				keys;											// Key Structure
	BOOL				isMessagePumpActive;							// Message Pump Active?
	MSG					msg;											// Window Message Structure
	DWORD				tickCount;										// Used For The Tick Counter

	g_window = &window;

	// Fill Out Application Data
	application.className = "OpenGL";									// Application Class Name
	application.hInstance = hInstance;									// Application Instance

	// Fill Out Window
	ZeroMemory (&window, sizeof (GL_Window));							// Make Sure Memory Is Zeroed
	window.keys					= &keys;								// Window Key Structure
	window.init.application		= &application;							// Window Application
	window.init.title			= "Echoclone - Alpha 3";	// Window Title
	window.init.width			= 640;									// Window Width
	window.init.height			= 480;									// Window Height
	window.init.bitsPerPixel	= 32;									// Bits Per Pixel
	window.init.isFullScreen	= TRUE;									// Fullscreen? (Set To TRUE)

	ZeroMemory (&keys, sizeof (Keys));									// Zero keys Structure


	// Ask The User If They Want To Start In FullScreen Mode?
	//if (MessageBox (HWND_DESKTOP, "Would You Like To Run In Fullscreen Mode?", "Start FullScreen?", MB_YESNO | MB_ICONQUESTION) == IDNO)
	{
		window.init.isFullScreen = FALSE;								// If Not, Run In Windowed Mode
	}

	// Register A Class For Our Window To Use
	if (RegisterWindowClass (&application) == FALSE)					// Did Registering A Class Fail?
	{
		// Failure
		MessageBox (HWND_DESKTOP, "Error Registering Window Class!", "Error", MB_OK | MB_ICONEXCLAMATION);
		return -1;														// Terminate Application
	}

	g_isProgramLooping = TRUE;											// Program Looping Is Set To TRUE
	g_createFullScreen = window.init.isFullScreen;						// g_createFullScreen Is Set To User Default
	
	while (g_isProgramLooping)											// Loop Until WM_QUIT Is Received
	{
		// Create A Window
		window.init.isFullScreen = g_createFullScreen;					// Set Init Param Of Window Creation To Fullscreen?
		if (CreateWindowGL (&window) == TRUE)							// Was Window Creation Successful?
		{
			bool canInitialize=true;
			bool needLoadLevel=false;

			if(lpCmdLine)
			{
				if(strlen(lpCmdLine)>0)
				{
					if(!LoadLevel(lpCmdLine))
					{
						needLoadLevel=true;
					}
				}
				else needLoadLevel=true;
			}
			else needLoadLevel=true;

			if(needLoadLevel)
			{
				char fnamebuffer[1024] = "";
				OPENFILENAME fn = {
					sizeof(OPENFILENAME),
					window.hWnd,
					hInstance,
					"All Files (*.*)\0*.*\0\0",
					NULL,
					0,
					0,
					fnamebuffer,
					1023,
					NULL,
					0,
					NULL,
					"Chroose a level file...",
					OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST,
					0,0,
					"",
					NULL,
					NULL,
					NULL,
				};
				if(GetOpenFileName(&fn))
				{
					if(!LoadLevel(fn.lpstrFile))
					{
						// Failure
						TerminateApplication (&window);							// Close Window, This Will Handle The Shutdown
						canInitialize=false;
					}
				}
				else
				{
					// Failure
					TerminateApplication (&window);							// Close Window, This Will Handle The Shutdown
					canInitialize=false;
				}
			}

			if(canInitialize)
			{
				rotx = Camera.X;
				roty = Camera.Y;

				Currentpos = Startpos;
				Nextpos = Startpos;
				playprog=1;
				Currentpos.D -=4;
				NeedUpdate=true;
				ReshapeGL(window.init.width,window.init.height);
				Update(0);

				// At This Point We Should Have A Window That Is Setup To Render OpenGL
				if ( Initialize (&window, &keys) == FALSE)					// Call User Intialization
				{
					// Failure
					TerminateApplication (&window);							// Close Window, This Will Handle The Shutdown
				}
				else														// Otherwise (Start The Message Pump)
				{	// Initialize was a success
					isMessagePumpActive = TRUE;								// Set isMessagePumpActive To TRUE
					while (isMessagePumpActive == TRUE)						// While The Message Pump Is Active
					{
						// Success Creating Window.  Check For Window Messages
						if (PeekMessage (&msg, window.hWnd, 0, 0, PM_REMOVE) != 0)
						{
							// Check For WM_QUIT Message
							if (msg.message != WM_QUIT)						// Is The Message A WM_QUIT Message?
							{
								DispatchMessage (&msg);						// If Not, Dispatch The Message
							}
							else											// Otherwise (If Message Is WM_QUIT)
							{
								isMessagePumpActive = FALSE;				// Terminate The Message Pump
							}
						}
						else												// If There Are No Messages
						{
							if (window.isVisible == FALSE)					// If Window Is Not Visible
							{
								WaitMessage ();								// Application Is Minimized Wait For A Message
							}
							else											// If Window Is Visible
							{
								// Process Application Loop
								tickCount = GetTickCount ();				// Get The Tick Count
								Update (tickCount - window.lastTickCount);	// Update The Counter
								window.lastTickCount = tickCount;			// Set Last Count To Current Count
								Draw ();									// Draw Our Scene

								SwapBuffers (window.hDC);					// Swap Buffers (Double Buffering)
							}
						}
					}														// Loop While isMessagePumpActive == TRUE
				}	
			}// If (Initialize (...

			// Application Is Finished
			Deinitialize ();											// User Defined DeInitialization

			DestroyWindowGL (&window);									// Destroy The Active Window
		}
		else															// If Window Creation Failed
		{
			// Error Creating Window
			MessageBox (HWND_DESKTOP, "Error Creating OpenGL Window", "Error", MB_OK | MB_ICONEXCLAMATION);
			g_isProgramLooping = FALSE;									// Terminate The Loop
		}
	}																	// While (isProgramLooping)

	UnregisterClass (application.className, application.hInstance);		// UnRegister Window Class
	return 0;
}																		// End Of WinMain()
