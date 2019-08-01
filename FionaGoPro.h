#ifndef _FIONA_GOPRO_H__
#define _FIONA_GOPRO_H__

#include "FionaScene.h"

struct GoProImage
{
	GLuint texture;
	jvec3 position;

	GoProImage(GLuint texture, jvec3 position)
	{
		this->texture=texture;
		this->position=position;
	}
};

class FionaGoPro : public FionaScene
{
public:
	FionaGoPro();
	virtual ~FionaGoPro();

	virtual void	render(void);

	virtual void	buttons(int button, int state);
	virtual void	keyboard(unsigned int key, int x, int y);
	virtual void	onExit(void);
	virtual void	updateJoystick(const jvec3& v);
	virtual void	updateWand(const jvec3& p, const quat& q);

	void			setRotateIn(bool rotate);

	void	loadImageSequence();

private:

	bool init;
	bool anaglyph;
	bool rotateIn;

	GLuint textureR;
	GLuint textureL;
	GLuint program;
	GLuint angSize;	//uniform location
	GLuint rOff;	//uniform location
	GLuint lOff;	//uniform location
	GLuint lTex;	//uniform location
	GLuint rTex;	//uniform location
	GLuint mode;	//uniform location (0 = left, 1 = right, 2 = anaglyph)
	GLuint rotateUniform;	//uniform location

	float gDist;
	float gRotX;
	float gRotY;
	float gImageAngleX;
	float gImageAngleY;

	float rx;
	float ry;
	float lx;
	float ly;

	float changeSpeed;


	std::string loadString;
	int loadStartNumber;
	int loadMaxNumber;
	int loadSkipNumber;
	std::vector<GoProImage *> imageSequence; 
};

#endif