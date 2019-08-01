//
//  main.cpp
//  FionaOgre
//
//  Adapted from Hyun Joon Shin and Ross Tredinnick's FionaUT project
// This is an OGRE-only version of that original work created by Ross Tredinnick 3/2013


#define WITH_FIONA

#include "GL/glew.h"
#include "FionaOgre.h"
#include "FionaUT.h"

#ifdef WIN32
#include "FionaUtil.h"
#endif

#include <Kit3D/glslUtils.h>
#include <Kit3D/glUtils.h>

#include "soil/SOIL.h"
#include "FionaGoPro.h"
#include "WordCake.h"
#include "OculusPerCal.h"

#ifdef LINUX_BUILD
#define FALSE 0
#endif
class FionaScene;
FionaScene* scene = NULL;

extern bool cmp(const std::string& a, const std::string& b);
extern std::string getst(char* argv[], int& i, int argc);
/*static bool cmp(const char* a, const char* b)
{
	if( strlen(a)!=strlen(b) ) return 0;
	size_t len = MIN(strlen(a),strlen(b));
	for(size_t i=0; i<len; i++)
		if(toupper(a[i]) != toupper(b[i])) return 0;
	return 1;
}
*/
static bool cmpExt(const std::string& fn, const std::string& ext)
{
	std::string extt = fn.substr(fn.rfind('.')+1,100);
	std::cout<<"The extension: "<<extt<<std::endl;
	return cmp(ext.c_str(),extt.c_str());
}


jvec3 curJoy(0,0,0);
jvec3 pos(0,0,0);
quat ori(1,0,0,0);
jvec3 latPos(0,0,0);

//int		calibMode = 0;

void draw5Teapot(void) {
	static GLuint phongShader=0;
	static GLuint teapotList =0;

	if( FionaIsFirstOfCycle() )
	{
		pos+=jvec3(0,0,curJoy.z*0.01f);
		ori =exp(YAXIS*curJoy.x*0.01f)*ori;
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTranslate(-pos);
	glRotate(ori);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glLight(vec4(0,0,0,1),GL_LIGHT0,0xFF202020);
	glBindTexture(GL_TEXTURE_2D,0);
	jvec3 pos[5]={jvec3(0,0,-1.5),jvec3(-1.5,0,0),jvec3(1.5,0,0),jvec3(0,-1.5,0), jvec3(0,1.5,0)};
	if( teapotList <=0 )
	{
		teapotList = glGenLists(1);
		glNewList(teapotList,GL_COMPILE);
		//glutSolidTeapot(.65f);
		glEndList();
	}
	if( phongShader<=0 )
	{
		glewInit();
		std::string vshader = std::string(commonVShader());
		std::string fshader = coinFShader(PLASTICY,PHONG,false);
		phongShader = loadProgram(vshader,fshader,true);
	}
	glUseProgram(phongShader);
	glEnable(GL_DEPTH_TEST); glMat(0xFFFF8000,0xFFFFFFFF,0xFF404040);
	glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,10);
	for(int i=0;i<3; i++){ glPushMatrix(); glTranslate(pos[i]);
		//glutSolidTeapot(.65f);
		//glCallList(teapotList);
		glSphere(V3ID,.65);
		glPopMatrix();}
	glUseProgram(0);
}

void drawSkyTest(void)
{
	if( FionaIsFirstOfCycle() )
	{
		pos+=jvec3(0,0,curJoy.z*0.01f);
		ori =exp(YAXIS*curJoy.x*0.01f)*ori;
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTranslate(-pos);
	glRotate(ori);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glLight(vec4(0,0,0,1),GL_LIGHT0,0xFF202020);
	glBindTexture(GL_TEXTURE_2D,0);
	jvec3 pos[5]={jvec3(0,0,-1.5),jvec3(-1.5,0,0),jvec3(1.5,0,0),jvec3(0,-1.5,0), jvec3(0,1.5,0)};

	static GLuint phongShader=0;
	if( phongShader<=0 )
	{
		glewInit();
		std::string vshader = std::string("simple.vert");
		std::string fshader = std::string("red.frag");
		phongShader = loadProgramFiles(vshader,fshader,true);
	}
	
	GLint projLoc = glGetUniformLocation(phongShader, "projection");
	GLint viewLoc = glGetUniformLocation(phongShader, "modelview");
	
	glUseProgram(phongShader);
	
	glEnable(GL_DEPTH_TEST); glMat(0xFFFF8000,0xFFFFFFFF,0xFF404040);
	glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,10);
	for(int i=0;i<3; i++)
	{ 
		glPushMatrix(); 
		glTranslate(pos[i]);
		
		GLfloat view[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, view);
		glUniformMatrix4fv(viewLoc, 1, FALSE, view);
		GLfloat proj[16];
		glGetFloatv(GL_PROJECTION_MATRIX, proj);
		glUniformMatrix4fv(projLoc, 1, FALSE, proj);

		//glutSolidSphere(.25, 12, 12);
		//glutSolidTeapot(.65f);
		//glCallList(teapotList);
		glSphere(V3ID,.65);
		glPopMatrix();
	}
	glUseProgram(0);
}

void drawLatLines()
{
	 //make it zero disparity
	 fionaConf.lEyeOffset = jvec3(0,0,0);
	 fionaConf.rEyeOffset = jvec3(0,0,0);

	 float distToWall = -1.4478;
	 jvec3 head = fionaConf.headPos;
	 //now flip it around the tracker
	 head.z = (distToWall - head.z);
	 //try for the wand now too
	 jvec3 wand = fionaConf.wandPos;
	 wand.z = -1.4478;//(distToWall - wand.z);
 
	 jvec3 bcenter = jvec3(0,latPos.y,wand.z);

	 //lets draw this far away rect
	 float z = -1.4478;
	 float s = 10000;
	 glBegin(GL_QUADS);
 
	 glColor3f( 0.2, 0.2, 0.2 );
	 glVertex3f( -s, -s, z ); 
	 glVertex3f( s, -s, z ); 
	 glVertex3f( s, latPos.y, z ); 
	 glVertex3f( -s, latPos.y, z ); 

	 glColor3f( 0.8, 0.8, 0.8 );
	 glVertex3f( -s, latPos.y, z ); 
	 glVertex3f( s, latPos.y, z ); 
	 glVertex3f( s, s, z ); 
	 glVertex3f( -s, s, z ); 

	 if (wand.y > latPos.y)
	 {
		 glColor3f( 1,0,0 );
		 glVertex3f( -s, latPos.y, z ); 
		 glVertex3f( s, latPos.y, z ); 
		 glVertex3f( s, wand.y, z ); 
		 glVertex3f( -s, wand.y, z ); 
	 }
	 else
	 {
		 glColor3f( 0,0,1 );
		 glVertex3f( -s, wand.y, z ); 
		 glVertex3f( s, wand.y, z ); 
		 glVertex3f( s, latPos.y, z ); 
		 glVertex3f( -s, latPos.y, z ); 
	 }
 
	 glEnd(); 

	 if (wand.y > latPos.y)
		glColor3f(0,1,1);
	 else
		glColor3f(1,1,0); 

	 //now draw the line
	 glLineWidth(10);
	 glLine(wand, bcenter);
}

enum APP_TYPE
{
	APP_TAEPOT = 0,
	APP_OGRE_HEAD = 1,
	APP_OGRE_SCENE = 2,
	APP_GO_PRO = 3,
	APP_WORD_CAKE = 4,
	APP_LAT_TEST = 5,
	APP_OCULUS_PER_CAL = 6
} type = APP_TAEPOT;

void wandBtns(int button, int state, int idx)
{
	if(type == APP_LAT_TEST)
	{
		printf("set config point\n");
		latPos = fionaConf.wandPos;
	}
	else
	{
		if(scene)
		{
			scene->buttons(button, state);
		}
	}
}

void keyboard(unsigned int key, int x, int y)
{
	if(type == APP_LAT_TEST)
	{
		printf("set config point\n");
		latPos = fionaConf.wandPos;
	}
	else
	{
		if(scene)
		{
			scene->keyboard(key, x, y);
		}
	}
}

void joystick(int w, const jvec3& v)
{
	if(scene) 
	{
		scene->updateJoystick(v);
	}
	curJoy = v;
}

void mouseBtns(int button, int state, int x, int y) {}
void mouseMove(int x, int y) {}


void tracker(int s,const jvec3& p, const quat& q)
{ 
	if(s==1)
	{
		fionaConf.wandPos = p;
		fionaConf.wandRot = q;
	}

	if(s==1 && scene)
	{
		scene->updateWand(p,q); 
	}
}

void preDisplay(float value)
{
	if(scene != 0)
	{
		scene->preRender(value);
	}
}

void postDisplay(void)
{
	if(scene != 0)
	{
		scene->postRender();
	}
}

void render(void)
{
	if(scene!=NULL)
	{
		scene->render();
	}
	else 
	{
		if(type == APP_LAT_TEST)
		{
			drawLatLines();			
		}
		else
		{
			//drawSkyTest();
			draw5Teapot();
		}
	}
}

/*void wiiFitCheck(void)
{
	if(fionaNetMaster || fionaConf.appType == FionaConfig::WINDOWED)
	{
		if(!balance_board.IsConnected())
		{
			bool bConnected = balance_board.Connect(wiimote::FIRST_AVAILABLE);
			if(bConnected)
			{
				balance_board.SetLEDs(0x0f);
			}
		}
		else
		{
			static int reCalibrated = 0;
			if(reCalibrated < 5)
			{
				balance_board.CalibrateAtRest();
				reCalibrated++;
			}

			balance_board.RefreshState();

			float fourWeights[4];
			fourWeights[0] = balance_board.BalanceBoard.Lb.BottomL;
			fourWeights[1] = balance_board.BalanceBoard.Lb.BottomR;
			fourWeights[2] = balance_board.BalanceBoard.Lb.TopL;
			fourWeights[3] = balance_board.BalanceBoard.Lb.TopR;
			
			//let's threshold the values since they always seem to have some value to them...
			for(int i = 0; i < 4; ++i)
			{
				if(fabs(fourWeights[i]) < 5.f)
				{
					fourWeights[i] = 0.f;
				}
			}

			//printf("Bottom Left: %f, Bottom Right: %f, Top Left: %f, Top Right: %f\n", fourWeights[0], fourWeights[1], fourWeights[2], fourWeights[3]);
			if(fionaConf.appType == FionaConfig::HEADNODE)
			{
				//pack this wii-fit data into a network packet..
				_FionaUTSyncSendWiiFit(fourWeights);
			}
		}
	}
}*/

int main(int argc, char *argv[])
{
	glutInit(&argc,argv);

	float measuredIPD=63.5;
	int userID=0;
	bool writeFull=false;
	bool drawAxis = false;
	bool drawWand = true;
	bool isRISE = false;
	std::string fn;
	const char *sPlaybackFN = 0;

	for(int i=1; i<argc; i++)
	{
		if(cmp(argv[i],"--ogreScene"))	
		{	
			type=APP_OGRE_SCENE; 
			i++;
			fn=argv[i];//getst(argv,i,argc); 
			printf("%s\n",fn.c_str());
		}
		else if(cmp(argv[i],"--ogreHead"))	
		{
			type=APP_OGRE_HEAD; 
		}
		else if(cmp(argv[i], "--gopro"))
		{
			type=APP_GO_PRO;
		}
		else if(cmp(argv[i], "--wordcake"))
		{
			type = APP_WORD_CAKE;
			fn=getst(argv,i,argc); 
			printf("%s\n",fn.c_str());
		}
		else if(cmp(argv[i], "--oculusPerCal"))
		{
			type = APP_OCULUS_PER_CAL;
			fn=getst(argv,i,argc); 
			printf("%s\n",fn.c_str());
		}
		else if(cmp(argv[i], "--latency"))
		{
			type = APP_LAT_TEST;
		}

		else if(cmp(argv[i], "-drawAxis"))
		{
			drawAxis = true;
		}
		else if(cmp(argv[i], "-camPlayback"))
		{
			i++;
			sPlaybackFN = argv[i];
		}
		else if(cmp(argv[i], "-RISE"))
		{
			isRISE = true;
		}
		else if(cmp(argv[i], "-hideWand"))
		{
			drawWand = false;
		}
	}

	switch( type )
	{
		case APP_OGRE_HEAD:	//ogre head is just a test scene...
			scene = new FionaOgre();
			scene->navMode=WAND_WORLD;
			static_cast<FionaOgre*>(scene)->setDrawAxis(drawAxis);
			if(sPlaybackFN)
			{
				static_cast<FionaOgre*>(scene)->setCameraPath(sPlaybackFN);
			}
			static_cast<FionaOgre*>(scene)->initOgre();
			break;
		case APP_OGRE_SCENE:
			scene = new FionaOgre();
			scene->navMode=WAND_WORLD;
			static_cast<FionaOgre*>(scene)->setDrawAxis(drawAxis);
			static_cast<FionaOgre*>(scene)->setDrawWand(drawWand);
			if(sPlaybackFN)
			{
				static_cast<FionaOgre*>(scene)->setCameraPath(sPlaybackFN);
			}
			static_cast<FionaOgre*>(scene)->setRISE(isRISE);
			static_cast<FionaOgre*>(scene)->initOgre(fn);
			break;
#ifndef LINUX_BUILD
		case APP_GO_PRO:
			scene = new FionaGoPro();
			scene->navMode=WAND_WORLD;
			break;
#endif
		case APP_WORD_CAKE:
			scene = new WordCake();
			scene->navMode=WAND_WORLD;
			static_cast<WordCake*>(scene)->initOgre(fn);
			break;

		case APP_OCULUS_PER_CAL:
			scene = new OculusPerCal();
			scene->navMode= WAND_NONE;
			static_cast<OculusPerCal*>(scene)->initOgre(fn);
			break;

		case APP_LAT_TEST:

			break;
	}
	
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
	
	glutCreateWindow	("Window");
	glutDisplayFunc		(render);
	glutJoystickFunc	(joystick);
	glutMouseFunc		(mouseBtns);
	glutMotionFunc		(mouseMove);
	glutWandButtonFunc	(wandBtns);
	glutTrackerFunc		(tracker);
	glutKeyboardFunc	(keyboard);
	glutFrameFunc		(preDisplay);
	glutPostRender		(postDisplay);
	
	glutMainLoop();

	delete scene;

	return 0;
}
