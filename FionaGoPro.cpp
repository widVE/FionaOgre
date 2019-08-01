#include "FionaGoPro.h"

#ifdef LINUX_BUILD
#include <stdlib.h>
#endif

#include "soil/SOIL.h"

#include <Kit3D/glslUtils.h>
#include <Kit3D/glUtils.h>
#include "GLUT/glut.h"
#include "FionaUT.h"

FionaGoPro::FionaGoPro() : FionaScene(), init(false), anaglyph(false), rotateIn(true),
	textureR(0), textureL(0), program(0), angSize(0), rOff(0), lOff(0), rTex(0), lTex(0), mode(0), rotateUniform(0), gDist(4.f), 
	gRotX(90.f), gRotY(0.f), gImageAngleX(120.f), gImageAngleY(90.f), rx(0.f), ry(0.f), lx(0.f), ly(0.f), changeSpeed(0.001f)
{
	//for right now lets just hard code this
	loadString = "../../Media/GoProTest/GOPR%04d.JPG";
	loadStartNumber = 310;
	loadMaxNumber=50;
	loadSkipNumber=10;

	//from our OpenCV calcs, lets set this to what we think it is
	gImageAngleX=117;
	gImageAngleY=93;
}

FionaGoPro::~FionaGoPro()
{
	if(textureR != 0)
	{
		glDeleteTextures(1, &textureR);
	}

	if(textureL != 0)
	{
		glDeleteTextures(1, &textureL);
	}

	if(program != 0)
	{
		glDeleteProgram(program);
	}
}

void FionaGoPro::setRotateIn(bool rotate)
{ 
	 rotateIn = rotate; 
	 if(rotate)
	 {
		printf("rotating in\n"); 
	 }
	 else
	 {
		printf("not rotating in\n"); 
	 }
}

void FionaGoPro::loadImageSequence()
{
	char fname[1024];
	
	//lets do a for loop to cap this 
	for (int i = loadStartNumber; i < loadStartNumber+loadMaxNumber; i+=loadSkipNumber)
	{
		sprintf(fname, loadString.c_str(), i); 
		printf("attempting to load %s\n", fname);
		GLuint texture =  SOIL_load_OGL_texture
			(fname,
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y
			);

		if (texture)
		{
			//we found the file
			imageSequence.push_back(new GoProImage(texture, jvec3(i*.01, 0, 0)));
		}
		else
		{
			printf("file not found\n");
			//we didn't find a file
			return;
		}

	}
}

void FionaGoPro::render(void)
{
	FionaScene::render();

	if(init == false)
	{

		std::string vshader = std::string("spheremap.vert");
		std::string fshader = std::string("spheremap.frag");
		program = loadProgramFiles(vshader,fshader,true);

		loadImageSequence();

		//lets set it super wide
		textureL=imageSequence.front()->texture;
		textureR=imageSequence.back()->texture;

		/*
		
		textureL =  SOIL_load_OGL_texture
			("LEFT.JPG",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y
			);

		textureR =  SOIL_load_OGL_texture
			(
			"RIGHT.JPG",
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y
			);
		*/
		

		lTex = glGetUniformLocation(program, "leftTexture");
		rTex = glGetUniformLocation(program, "rightTexture");
		mode = glGetUniformLocation(program, "mode");
		angSize = glGetUniformLocation(program, "ImageAngleSize");
		rOff = glGetUniformLocation(program, "roffset");
		lOff = glGetUniformLocation(program, "loffset");
		rotateUniform = glGetUniformLocation(program, "rotateOn");
		init = true;
	}

	fionaConf.lEyeOffset.set(0.f, 0.f, 0.f);
	fionaConf.rEyeOffset.set(0.f ,0.f, 0.f);

	glColor3f(1,1,1);
	float s = 20;
	
	float d, d0=-60;

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
		// Typical Texture Generation Using Data From The Bitmap
	glBindTexture(GL_TEXTURE_2D, textureR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
		// Typical Texture Generation Using Data From The Bitmap
	glBindTexture(GL_TEXTURE_2D, textureL);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	//glTranslatef(0,0,-gDist);
	glRotatef(gRotX,0,1,0);
	glRotatef(gRotY,0,0,1);
	
	if(rotateIn)
	{
		/*jvec3 head = fionaConf.headPos;
		jvec3 ep;
		if(FionaRenderCycleLeft())
		{
			ep = fionaConf.kevinOffset+fionaConf.lEyeOffset;		
		}
		else
		{
			ep = fionaConf.kevinOffset+fionaConf.rEyeOffset;		
		}

		FionaWall wall = fionaWinConf[0].walls[0];
		quat preRot = wall.preRot;
		head = preRot.rot(head)-wall.cntr;
		ep   = preRot.rot(fionaConf.headRot.rot(ep))+head;
		float dw = ep.z - wall.cntr.z;*/
		static const float h = 0.05f * 0.5f;	//11 cm
		float dw = fionaConf.headPos.z + 1.4478;
		float ang = atan2(h, dw)*(180./3.1415);
		if(FionaRenderCycleLeft())
		{
			glRotatef(ang, 0.f, 1.f, 0.f);
		}
		else
		{
			glRotatef(-ang, 0.f, 1.f, 0.f);
		}
	}

	glUseProgram(program);
		glUniform2f(angSize, gImageAngleX, gImageAngleY);
		glUniform2f(rOff, rx, ry);
		glUniform2f(lOff, lx, ly);
		glUniform1i(lTex, 0);
		glUniform1i(rTex, 1);
		if(rotateIn)
		{
			glUniform1i(rotateUniform, 1);
		}
		else	
		{
			glUniform1i(rotateUniform, 0);
		}

		if(!anaglyph)
		{
			if(FionaRenderCycleLeft())
			{
				glUniform1i(mode, 0);
			}
			else if(FionaRenderCycleRight())
			{
				glUniform1i(mode, 1);
			}
			else
			{
				glUniform1i(mode, 2);
			}
		}
		else
		{
			glUniform1i(mode, 2);
		}

		glutSolidSphere(20.0,64,64);

	glUseProgram(0);

	glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
}

void FionaGoPro::buttons(int button, int state)
{
	FionaScene::buttons(button, state);

	if(button == 5 && state == 0)
	{
		setRotateIn(!rotateIn);
	}
}

void FionaGoPro::keyboard(unsigned int key, int x, int y)
{
	float camSpeed=.01;
	if(key == 82)	//r key
	{
		setRotateIn(!rotateIn);
	}
	else if(key == 'w')	//r key
	{
		fionaConf.kevinOffset.z +=camSpeed;
		printf("KO: %04f %04f %04f\n", fionaConf.kevinOffset.x, fionaConf.kevinOffset.y, fionaConf.kevinOffset.z);
	}
	else if(key == 's')	//r key
	{
		fionaConf.kevinOffset.z -=camSpeed;
		printf("KO: %04f %04f %04f\n", fionaConf.kevinOffset.x, fionaConf.kevinOffset.y, fionaConf.kevinOffset.z);
	}
}

void FionaGoPro::onExit(void)
{

}

void FionaGoPro::updateJoystick(const jvec3& v)
{
	fionaConf.kevinOffset.z += .001*v.z;
	printf("KO: %04f %04f %04f\n", fionaConf.kevinOffset.x, fionaConf.kevinOffset.y, fionaConf.kevinOffset.z);
	//FionaScene::updateJoystick(v);
	
}

void FionaGoPro::updateWand(const jvec3& p, const quat& q)
{
	FionaScene::updateWand(p, q);

}
