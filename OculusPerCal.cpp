#include "OculusPerCal.h"

#include <time.h>
	
void OculusPerCal::preRender( float value )
{

	FionaOgre::preRender(value);

	// update scene before render
	setIPD( current_ipd );
	updateByJoystick( joystick );

}

void OculusPerCal::render(void)
{
	if( shouldRenderScene )
		FionaOgre::render();
	else
	// otherwise, just throw out the render (TODO: find way to stop it?)
		glClear( GL_COLOR || GL_DEPTH );
}

// for extra rendering; I'm also hijacking it to clear the screen for the "blindfold"
void OculusPerCal::openGLRender(void)
{
	//glEnable( GL_MULTISAMPLE_ARB );

	//if( shouldRenderScene )
		//FionaOgre::openGLRender();

	if( !shouldRenderScene )
	// otherwise, just throw out the render (TODO: find way to stop it?)
		glClear( GL_COLOR || GL_DEPTH );


	// extra drawing here

}
	
	
void OculusPerCal::setIPD(float ipd)
{ 
	// Can't do it the fiona way if using oculus; suspect the prewarp shader won't be informed
		
	// limit update (or oculus never renders)
	//time_t now = time( NULL );
	//if( now - last_ipd_update < 1 )		{	return;	}
	//else								{	last_ipd_update = now;	}

	
	//printf( "setIPD: currentIPD: %f\n", fionaConf.stereoConfig.GetIPD( ) );
	//#ifdef ENABLE_OCULUS
	//	fionaConf.stereoConfig.SetIPD( ipd );
	//	printf("It was called.  Really.");
	//#else
		fionaConf.lEyeOffset.x = -ipd/2.f; 
		fionaConf.rEyeOffset.x =  ipd/2.f; 
	//#endif
	
	return;
}

void OculusPerCal::keyboard( unsigned int key, int x, int y )
{

	key = toupper( key );
		 
	printf("key = %c : %d\n", (char)key, key);

	//if(key == 'b' || key == 'B')
	//	shouldRenderScene = !shouldRenderScene;

	if(key == ' ')
	{	
		shouldRenderScene = !shouldRenderScene;
		printf("Should Render: %d\n", shouldRenderScene);
	}

	do_IPD_onKeys( key );

	/*
	#ifdef ENABLE_OCULUS
		float eye_to_screen = fionaConf.stereoConfig.GetEyeToScreenDistance(); //TODO: Move to init?
			//		fionaConf.kevinOffset.z += 0.01;
			//		fionaConf.kevinOffset.z -= 0.01;
		if(key == 'Z')	fionaConf.stereoConfig.SetEyeToScreenDistance( eye_to_screen += 0.025 );
		if(key == 'X')	fionaConf.stereoConfig.SetEyeToScreenDistance( eye_to_screen -= 0.025 );
	#endif
		
	if(key == 'W')		fionaConf.kevinOffset.y += 0.001;
	if(key == 'S')		fionaConf.kevinOffset.y -= 0.001;
	if(key == 'A')		fionaConf.kevinOffset.x += 0.001;
	if(key == 'D')		fionaConf.kevinOffset.x -= 0.001;

	//if(key == 'Z')		fionaConf.kevinOffset.z += 0.01;
	//if(key == 'X')		fionaConf.kevinOffset.z -= 0.01;

	if(key == 'O')		current_ipd -= 0.025;
	if(key == 'P')		current_ipd += 0.025;

				
	if(key == 'T')		isTestMode = !isTestMode;
	*/
}

void OculusPerCal::do_IPD_onKeys( unsigned int key )
{
	//printf( "Is alpha check: %d < %d > %d?\n", (int)'A', key, (int)'Z' ); //DEBUG//
	if( key < (int)'A' || key > (int)'Z' )
		return;

	int mod_key = key - (int)'A';

	current_ipd = (50 + mod_key) * 0.001;

	setIPD( current_ipd );
	printf( "New vIPD: %f\n", current_ipd );

}
	
void OculusPerCal::updateByJoystick(const jvec3& joy)
{
	//FionaScene::updateJoystick( joy );

	//if( isTestMode == true )
	//{
		//fionaConf.kevinOffset.z += joy.z * 0.005f;


		//current_ipd += joy.x * 0.005f;
		//if( joy.x != 0 )	printf( "Joy mod; Current vIPD: %f\n", current_ipd );


		//do_eye_depth( joy );


	//}

		//TODO: suddenly doesn't seem to be happening.

}

void OculusPerCal::do_eye_depth( const jvec3& joy ) // not currently working
{
		
		
	//DEBUG//
	//if( joy.z != 0 )
	//{
	//	printf( "joy.z: %f\n", joy.z );
	//}
	//DEBUG//

	//DEBUG//
	//if( joy.z != 0 || joy.x != 0 )
	//{
	//	printf( "\nJoy.z: %f, %f, %f", joy.x, joy.y, joy.z );
		// Looks like these values are always negative?  I assume it's a bad printf, somehow.
	//}
	//DEBUG//
		
	#ifdef ENABLE_OCULUS

		if( joy.z != 0 )
		{

			//fionaConf.stereoConfig.SetStereoMode( OVR::Util::Render::StereoMode::Stereo_LeftRight_Multipass ); // Stereo_LeftRight_Multipass

			float eye_to_screen = 0.f;//fionaConf.stereoConfig.GetEyeToScreenDistance(); //TODO: Move to init?
			//printf( "\nEye to screen: %f + %f\n", eye_to_screen, joy.z * 0.005f ); //DEBUG//

			/*
			//DEBUG//
				OVR::Matrix4f p1;
				p1 = fionaConf.stereoConfig.GetEyeRenderParams(OVR::Util::Render::StereoEye_Left).Projection;
				printf( "Before:\n%f, %f, %f, %f \n %f, %f, %f, %f \n %f, %f, %f, %f \n %f, %f, %f, %f \n", 
						p1.M[0][0], p1.M[0][1], p1.M[0][2], p1.M[0][3],
						p1.M[1][0], p1.M[1][1], p1.M[1][2], p1.M[1][3],
						p1.M[2][0], p1.M[2][1], p1.M[2][2], p1.M[2][3],
						p1.M[3][0], p1.M[3][1], p1.M[3][2], p1.M[3][3]
						);
			//DEBUG//
			*/

			//fionaConf.stereoConfig.SetEyeToScreenDistance( eye_to_screen + joy.z * 0.005f );
				
			// DEBUG //
				//fionaConf.stereoConfig.SetEyeToScreenDistance( 1000000 ); 
				//float eye_to_screen_after = fionaConf.stereoConfig.GetEyeToScreenDistance(); 
				//printf( "Eye to screen After: %f\n", eye_to_screen_after );
			// DEBUG //

			/*
			//DEBUG//
				OVR::Matrix4f p2;
				p2 = fionaConf.stereoConfig.GetEyeRenderParams(OVR::Util::Render::StereoEye_Left).Projection;
				printf( "After:\n%f, %f, %f, %f \n %f, %f, %f, %f \n %f, %f, %f, %f \n %f, %f, %f, %f \n", 
						p2.M[0][0], p2.M[0][1], p2.M[0][2], p2.M[0][3],
						p2.M[1][0], p2.M[1][1], p2.M[1][2], p2.M[1][3],
						p2.M[2][0], p2.M[2][1], p2.M[2][2], p2.M[2][3],
						p2.M[3][0], p2.M[3][1], p2.M[3][2], p2.M[3][3]
						);

				int mode = fionaConf.stereoConfig.GetStereoMode();
				printf( "mode: %d", mode );
			//DEBUG//
			*/
		}

	#endif
}

//misc reference code:
		//switch(isCalibratingWhat)
	//{
		/*
		case CALIB_KO_XY:
			fionaConf.kevinOffset.y += joy.z;// * 0.001;
			fionaConf.kevinOffset.x += joy.x;// * 0.001;
		break;
		case CALIB_KO_Z:
			fionaConf.kevinOffset.z += joy.z * 0.005f;
		break;
		case CALIB_IPD:
			current_ipd += joy.x;
			setIPD( current_ipd );
		break;
		*/

		//case CALIB_TRI:
		//case CALIB_PP:
		/*
		case CALIB_SWIM:
			CUR_KOZ+=joy.z*0.001f;
			setIPD(part->koz2ipd(CUR_KOZ));
			break;
		case SWIM:
		case SWIM2:
			if (joy.z)
			{
				OgreOggSound::OgreOggSoundManager *mSoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();
				if(OgreOggSound::OgreOggISound * pSound = mSoundManager->getSound("SoundMove"))
				{
					pSound->play();		
				}
				//if(moveSound) moveSound->play(true);
			}

			boxPos-=jvec3(0,0,joy.z*0.0025f);
			updateScene();
			break;
		case TILT:
			if (joy.z)
			{
				OgreOggSound::OgreOggSoundManager *mSoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();
				if(OgreOggSound::OgreOggISound * pSound = mSoundManager->getSound("SoundMove"))
				{
					pSound->play();		
				}
			}

			tiltAngle-=joy.z*0.0025f;
			updateScene();
			break;
		case SHAPE:
		case SHAPE2:
			if (fabs(joy.x) > fabs(joy.z))
				boxScale.x+=joystick.x*0.01;
			else if (fabs(joy.z) > fabs(joy.x))
			boxScale.y+=joystick.z*0.01;

			if (joy.x)
			{
				OgreOggSound::OgreOggSoundManager *mSoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();
				if(OgreOggSound::OgreOggISound * pSound = mSoundManager->getSound("SoundMove"))
				{
					pSound->play();		
				}
			}
				
			if (joy.z)
			{
				OgreOggSound::OgreOggSoundManager *mSoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();
				if(OgreOggSound::OgreOggISound * pSound = mSoundManager->getSound("SoundMove"))
				{
					pSound->play();		
				}
			}
			updateScene();
		break;
		*/
