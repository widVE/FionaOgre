#ifndef _OCULUS_PER_CAL_H_
#define _OCULUS_PER_CAL_H_

#include "FionaOgre.h"

#include <time.h>

class OculusPerCal : public FionaOgre
{
public:

	OculusPerCal() : FionaOgre(), 
					 shouldRenderScene(true), 
					 isTestMode(true),
					 current_ipd( 0.064 )
							{ }
	virtual ~OculusPerCal()	{ }

	bool shouldRenderScene;
	bool isTestMode;

	float current_ipd;
	float last_ipd;

	time_t last_ipd_update;
	time_t last_ed_update;

protected:

	virtual void preRender( float value );
	virtual void render(void);
	virtual void openGLRender(void);
	
	void setIPD(float ipd);

	virtual void keyboard( unsigned int key, int x, int y );
	void do_IPD_onKeys( unsigned int key );
	

	virtual void updateByJoystick(const jvec3& joy);
	void do_eye_depth( const jvec3& joy ); // not currently working


private:



};

#endif
