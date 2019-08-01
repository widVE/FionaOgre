//
//  FionaOgre.h
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/10/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef _FIONA_OGRE_H__
#define _FIONA_OGRE_H__

#define NOMINMAX
#define USE_BULLET 0

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#include "Oculus2/Src/CAPI/CAPI_HMDState.h"
#endif
#endif

#ifndef LINUX_BUILD
#include <Ogre/OgrePrerequisites.h>
#include <Ogre/Hydrax/Hydrax.h>
#include <sb/SBScene.h>
#else
#include "OgreMatrix4.h"
#include "OgreVector3.h"
#define TRUE 1
#endif

#ifdef __APPLE__
//#include <GLEW/glew.h>
#include <OpenGL/GL.h>
#elif defined WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <GL/glew.h>
#include <Windows.h>
#endif

#include <vector>

/*#if defined __JMATH_H__ && !defined JMATH_OGRE_WRAPPER_
#error JMATH should be included after Ogre. \
If you included "FionaUT" before "FionaOgre", \
Swap them or do not include either FionaUT or Kit3D.
#endif*/

#include "FionaUT.h"
#include "FionaScene.h"

#ifdef __APPLE__
#include <Ogre/OSX/macUtils.h>
#include <Cocoa/Cocoa.h>
#endif

#ifndef LINUX_BUILD
#include <Ogre/Sound/OgreOggSoundManager.h>
#include <Ogre/Physics/NxOgreWorld.h>
#include <Ogre/Physics/NxOgreScene.h>
#include "critter/Critter.h"
#include "Skyx/SkyX.h"
#include "Ogre/PagedGeometry/PagedGeometry.h"
#include "Ogre/PagedGeometry/GrassLoader.h" 
#include "Ogre/PagedGeometry/BatchPage.h"
#include "Ogre/PagedGeometry/ImpostorPage.h"
#include "Ogre/PagedGeometry/TreeLoader3D.h"
#include "OgreBullet/Dynamics/OgreBulletDynamicsWorld.h"
#endif

#ifdef LINUX_BUILD
#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>
#else
#include <Ogre/Terrain/OgreTerrain.h>
#include <Ogre/Terrain/OgreTerrainGroup.h>
#endif

struct FionaOgreWinInfo
{
	Ogre::RenderWindow*		owin;
	WIN						nwin;
	CTX						ctx;
	Ogre::Viewport*			vp;
	int						gwin;
	FionaOgreWinInfo(Ogre::RenderWindow* ow, WIN nw, int gw, CTX c, Ogre::Viewport* v)
	: owin(ow), nwin(nw), ctx(c), vp(v), gwin(gw){}
};

class FionaOgre : public FionaScene
{
public:

	enum {
		TRANSLATE=0,
		ROTATE=1,
		SCALE=2,
		NUM_TRANSFORM_MODES
	}TransformMode;

	FionaOgre(void);
	virtual ~FionaOgre(void);

	void initOgre(std::string scene=std::string(""));

	void resize(int gwin, int w, int h);

	inline Ogre::SceneManager*		getScene(void) { return scene; }
	inline Ogre::Light*				getHeadlight(void) { return headLight; }
	inline Ogre::Camera*			getCamera(void) { return camera; }
	unsigned int					getTSR(void) const { return m_tsr; }

	// Following function are to be overrod for applications.
	virtual void preRender(float t);
	virtual void render(void);
	virtual void setupScene(Ogre::SceneManager* scene);
	virtual void addResourcePaths(void);
	virtual void buttons(int button, int state);
	virtual void keyboard(unsigned int key, int x, int y);

	void					addSelection(Ogre::MovableObject *obj) { m_currentSelection.push_back(obj); }
	void					clearSelection(void);
	void					changeTSR(void);
	jvec3					getWandPos(float x_off = 0.0, float y_off = 0.0, float z_off = 0.0) const;
	void					getSecondTrackerPos(jvec3 &vOut, const jvec3 &vOffset=jvec3(0.f, 0.f, 0.f)) const;

	int						numSelected(void) const { return m_currentSelection.size(); }
	bool					isSelected(Ogre::MovableObject *obj) const;
	Ogre::MovableObject *	getSelected(int index) { return m_currentSelection[index]; }
	Ogre::MovableObject *	rayCastSelect(float & fDist, bool bClear=true);
	void					saveScene(void);
	void					setCameraPath(const char *sFN);
	void					setDrawAxis(bool bAxis) { drawAxis_ = bAxis; }
	void					setDrawWand(bool bWand) { drawWand_ = bWand; }
	void					setRISE(bool r) { isRISE_ = r; }
	void					playback(void);

protected:
	inline bool isInited(WIN win) { return findWin(win)>=0; }
	void					defaultSetup(void);
	void					ogreSetup(void);

#ifndef LINUX_BUILD
#if !USE_BULLET
	Critter::RenderSystem*  critterRender;
#endif
#endif
	std::string				sceneName;

private:
	std::vector<FionaOgreWinInfo>	wins;

	inline int findWin(WIN win)
	{ for(int i=0;i<(int)wins.size();i++) if(wins[i].nwin==win)return i; return -1;}
	inline int findWin(int gwin)
	{ for(int i=0;i<(int)wins.size();i++) if(wins[i].gwin==gwin)return i; return -1;}

	CTX						sharedContext;
	Ogre::Root*					root;
	Ogre::SceneManager*			scene;
	Ogre::Camera*				camera;
	Ogre::Light*				headLight;
	Ogre::RenderTexture*		renderTexture;
	Ogre::Viewport*				vp;
	Ogre::RenderWindow*			ogreWin;
	Ogre::TerrainGlobalOptions*	mTerrainGlobalOptions;
	Ogre::TerrainGroup*			mTerrainGroup;

#ifndef LINUX_BUILD
#if USE_BULLET
	OgreBulletDynamics::DynamicsWorld *physicsWorld;
	OgreBulletCollisions::DebugDrawer *debugDrawer;
 	std::deque<OgreBulletDynamics::RigidBody *>         mBodies;
 	std::deque<OgreBulletCollisions::CollisionShape *>  mShapes;
#else
	NxOgre::World*			physicsWorld;
	NxOgre::Scene*			physicsScene;
	NxOgre::Mesh*			clothMesh;
	NxOgre::Cloth*			cloth;

    std::vector<Critter::Body*> dynamicObjects;

#endif
	Hydrax::Hydrax*			mHydrax;
	SkyX::SkyX*				mSkyx;
	std::vector<Forests::PagedGeometry *> mPGHandles;
    std::vector<Forests::TreeLoader3D *> mTreeHandles;
	Forests::GrassLoader* mGrassLoaderHandle;                /** Handle to Forests::GrassLoader object */
	
#endif

	bool				physicsOn;
	bool				simTestOn;
	bool				embody;
	Ogre::StringVector kinectMapping;

	float				lastTime;
	Ogre::Matrix4			lastOgreMat;

	unsigned int			m_tsr;

	std::vector<Ogre::MovableObject*> m_currentSelection;

	const static Ogre::Vector3 CAVE_CENTER;

	void			initWin(WIN nwin, CTX ctx, int gwin, int w, int h);	

#ifndef LINUX_BUILD
	void			drawHand(const LeapData::HandData &hand);

	/*void			drawArm(const KinectData &data); //draw virtual avatar arms from kinect-tracked human body
	//Any that is butterfly related...
	Ogre::Vector3	initialPos;
	Ogre::Vector3	finalPos;
	bool			isNetOver;
	float			totalTime;
	float			timeSinceNetOver;
	void			butterflyAim(void);								//this is for task 1
	void			butterflyFollow(const KinectData &data);		//this is for task 2
	void			JNL(void);										//this is pilot testing to determine JNL
	void			keyboard(unsigned int key, int x, int y);

	//read/write files
	int				taskIndex;	// be global variable.*/

#if USE_BULLET

#else
	Critter::Body * 	makeBox(const NxOgre::Matrix44& globalPose, const NxOgre::Vec3 & initialVelocity = NxOgre::Vec3::ZERO);	//physics test
	void				makeCloth(const NxOgre::Vec3& barPosition);	//physics test
#endif
#endif

protected:
	virtual void		addResPath(const std::string& path);
	void				drawAxis(void);
	//the drawing function that renders all ogre-based stuff.
	void				ogreRender(void);
	//below is for rendering opengl on top of ogre render
	void				preOpenGLRender(void);
	virtual void		openGLRender(void);
	void				postOpenGLRender(void);

	void				displayTextureOnBackground(void);

	SmartBody::SBScene* m_pSB;

	//these two should be moved to FionaScene eventually..
	bool				drawAxis_;
	bool				drawWand_;		
	
	struct CamInfo
	{
		float pos[3];
		quat rot;
	};

	OgreOggSound::OgreOggSoundManager * m_SoundManager;

	std::vector<CamInfo> camPlayback_;
	bool	playingBack_;
	bool	recording_;
	bool	snowStarted_;
	bool	isRISE_;
	FILE *	recordFile_;
	float	lastCamTime_;
	float	lastFrameTime_;
	int		lastIndex_;
	static const float playbackTime_;
};

#include "sb/SBSceneListener.h"

class FionaOgreSmartBodyListener : public SmartBody::SBSceneListener
{
   public:
	   FionaOgreSmartBodyListener(FionaOgre* osb);
	   ~FionaOgreSmartBodyListener();

		virtual void OnCharacterCreate( const std::string & name, const std::string & objectClass );	
		virtual void OnCharacterDelete( const std::string & name );
		virtual void OnCharacterChanged( const std::string& name );		 
		virtual void OnLogMessage( const std::string & message );


	private:
		FionaOgre* ogreSB;
};

#endif
