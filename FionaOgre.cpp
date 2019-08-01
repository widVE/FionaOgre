#include "FionaOgre.h"


#include <Kit3D/jtrans.h>

#include <OGRE/OgreRoot.h>
#include <OGRE/OgreRenderWindow.h>
#include <OGRE/OgreSceneManager.h>
#include <OGRE/OgreLight.h>
#include <OGRE/OgreEntity.h>
#include <OGRE/OgreHardwarePixelBuffer.h>
#include <OGRE/OgreVector3.h>
#include <OGRE/Ogre.h>
#include <OGRE/Overlay/OgreFontManager.h>

#ifndef LINUX_BUILD
#include <OGRE/Sound/OgreOggSound.h>
#include <OGRE/Sound/OgreOggSoundManager.h>
#include <OGRE/Physics/NxOgrePlaneGeometry.h>
#include <OGRE/Physics/NxOgreRemoteDebugger.h>
#include <OGRE/Hydrax/Noise/Perlin/Perlin.h>
#include <OGRE/Hydrax/Modules/ProjectedGrid/ProjectedGrid.h>
#include <OGRE/Skyx/SkyX.h>
#include "OgreBullet/Dynamics/OgreBulletDynamics.h"
#include "OgreBullet/Dynamics/OgreBulletDynamicsRigidBody.h"
#include "OgreBullet/Collisions/Shapes/OgreBulletCollisionsStaticPlaneShape.h"
#include "OGRE/Plugins/ParticleUniverse/ParticleUniverseSystemManager.h"
#include "sb/SBSimulationManager.h"
#include "sb/SBCharacter.h"
#include "sb/SBSkeleton.h"
#include "sb/SBPython.h"
#include "sb/SBBmlProcessor.h"
#else
#include "OGRE/OgreMeshManager.h"
#endif

//ROSS TESTING
#include "OgreDotScene.h"
#include "VROgreAction.h"
#include "FionaNetwork.h"

const Ogre::Vector3 FionaOgre::CAVE_CENTER = Ogre::Vector3(-5.25529762045281, 1.4478, 5.17094851606241);

const float FionaOgre::playbackTime_ = 235.f;	//3 minutes 55 seconds

Ogre::Matrix4 toOgreMat(const tran& p)
{
	return Ogre::Matrix4(p.a00,p.a01,p.a02,p.a03,
						 p.a10,p.a11,p.a12,p.a13,
						 p.a20,p.a21,p.a22,p.a23,
						 p.a30,p.a31,p.a32,p.a33);
}

FionaOgre::FionaOgre(void): FionaScene(), sharedContext(NULL), root(NULL), scene(NULL), physicsOn(false), drawAxis_(false), playingBack_(false), simTestOn(false), embody(false),
	recording_(false), recordFile_(0), snowStarted_(false), isRISE_(false), drawWand_(true),
	camera(0), headLight(0), renderTexture(0), vp(0), ogreWin(0), mTerrainGroup(0), mTerrainGlobalOptions(0), m_pSB(0),
#ifndef LINUX_BUILD
#if USE_BULLET
	physicsWorld(0), debugDrawer(0),
#else
	physicsWorld(0), physicsScene(0), 
	clothMesh(0), cloth(0), critterRender(0), 
#endif
	mHydrax(0), mSkyx(0), mGrassLoaderHandle(0), m_SoundManager(0),
#endif
	lastTime(0.f), lastOgreMat(Ogre::Matrix4::IDENTITY), m_tsr(0), lastCamTime_(0.f), lastFrameTime_(0.f), lastIndex_(0)
	
{

//	fp = fopen("tasks.txt", "r");
	// oculus test.
	//OVR::System::Init();
	/*OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));
	pManager = DeviceManager::Create();
    DeviceEnumerator<SensorDevice> isensor = pManager->EnumerateDevices<SensorDevice>();
    DeviceEnumerator<SensorDevice> oculusSensor;  
    while(isensor)
    {
        DeviceInfo di;
        if (isensor.GetDeviceInfo(&di))
        {
            if (strstr(di.ProductName, "Tracker"))
            {
                if (!oculusSensor)
                    oculusSensor = isensor;
            }
        }
        isensor.Next();
    }

    if (oculusSensor)
    {
        pSensor = *oculusSensor.CreateDevice();

        if (pSensor)
            pSensor->SetRange(SensorRange(4 * 9.81f, 8 * OVR::Math<float>::Pi, 1.0f), true);

	}

    oculusSensor.Clear();

	if (pSensor)
        fionaConf.sFusion.AttachToSensor(pSensor);
	// end oculus test.
	initialPos = Ogre::Vector3(-60,50,5);
	finalPos = Ogre::Vector3(60,-1,5);
	isNetOver = false;
	totalTime = 5; //time in seconds
	timeSinceNetOver = 0;*/
}

FionaOgre::~FionaOgre()
{
#ifndef LINUX_BUILD
	NxOgre::World::destroyWorld();
	if(mHydrax != 0)
	{
		mHydrax->remove();
		mHydrax=0;
	}

	if(mSkyx != 0)
	{
		mSkyx->remove();
		mSkyx = 0;
	}
    
	if(mGrassLoaderHandle)
	{
        delete mGrassLoaderHandle;
		mGrassLoaderHandle=0;
	}

    std::vector<Forests::PagedGeometry *>::iterator it = mPGHandles.begin();
    while(it != mPGHandles.end())
    {
        delete it[0];
        it++;
    }
    mPGHandles.clear();

	ParticleUniverse::ParticleSystemManager::getSingletonPtr()->destroyAllParticleSystems(scene);

	if(m_SoundManager)
	{
		m_SoundManager->destroyAllSounds();
	}

	if(m_pSB)
	{
		//SmartBody::SBScene::remove
	}
#endif

    if(mTerrainGroup != 0)
    {
        OGRE_DELETE mTerrainGroup;
    }

	if(mTerrainGlobalOptions != 0)
	{
		OGRE_DELETE mTerrainGlobalOptions;
	}
}

void FionaOgre::addResPath(const std::string& path)
{
	Ogre::ResourceGroupManager& rman=Ogre::ResourceGroupManager::getSingleton();
	rman.addResourceLocation(path, "FileSystem");
}


void FionaOgre::addResourcePaths(void)
{
	Ogre::FontManager* fmgr = new Ogre::FontManager();

#ifdef __APPLE__
	printf("%s\n", (fionaConf.OgreMediaBasePath+std::string("/models")).c_str());
	addResPath(fionaConf.OgreMediaBasePath+std::string("/models"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("/materials/scripts"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("/materials/textures"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("/materials/programs"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("/fonts"));
#elif defined LINUX_BUILD
	printf("%s\n", (fionaConf.OgreMediaBasePath+std::string("/models")).c_str());
	//addResPath(fionaConf.OgreMediaBasePath+std::string("/models"));
	//addResPath(fionaConf.OgreMediaBasePath+std::string("/materials/scripts"));
	//addResPath(fionaConf.OgreMediaBasePath+std::string("/materials/textures"));
	//addResPath(fionaConf.OgreMediaBasePath+std::string("/materials/programs"));
	//addResPath(fionaConf.OgreMediaBasePath+std::string("/fonts"));
	//addResPath(fionaConf.OgreMediaBasePath+std::string("/NxOgre"));
	Ogre::ConfigFile cf;
	std::string workDir="/mnt/dscvr/apps/FionaOgre/SDK/linux/install/share/OGRE/";
	std::string ogreConfigPath = workDir + "resources.cfg";
	cf.load(ogreConfigPath.c_str());
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
		typeName = i->first;
		archName = i->second;
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
		    archName, typeName, secName);
		}
	}

	if(sceneName.length() > 0)
	{
		printf("Loading scene : %s\n", sceneName.c_str());
		//cluster launcher isn't liking this line - we're cutting off the name early...
		size_t s = sceneName.find_last_of('/');
		if(s != -1)
		{
			printf("Adding resource paths for scene file\n");
			std::string dir = sceneName.substr(0, s);
			//get directory from scene name and add sub-dirs
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir);
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/bitmap"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/programs"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/material"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/mesh"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/models"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/materials"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Terrain"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Terrain/plants"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Terrain/terrain"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Terrain/textures/diffusespecular"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Terrain/textures/normalheight"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Templates"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Temp"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Scripts"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Materials"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("/") + dir + std::string("/Models"));
		}
		printf("Done adding resource paths\n");
	}

#elif defined WIN32
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Materials\\textures"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Materials\\programs"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Materials\\scripts"));

	ParticleUniverse::ParticleSystemManager* pManager = ParticleUniverse::ParticleSystemManager::getSingletonPtr();

	if(sceneName.length() > 0)
	{
		//cluster launcher isn't liking this line - we're cutting off the name early...
		size_t s = sceneName.find_last_of('\\');
		if(s != -1)
		{
			std::string dir = sceneName.substr(0, s);
			//get directory from scene name and add sub-dirs
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir);
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\bitmap"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\programs"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\material"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\mesh"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\models"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\materials"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Terrain"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Terrain\\plants"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Terrain\\terrain"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Terrain\\textures\\diffusespecular"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Terrain\\textures\\normalheight"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Templates"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Temp"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Scripts"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Materials"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\Models"));
		}
	}
	
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Models"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\NxOgre"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Box"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\fonts"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\RTShaderLib"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\particle"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\DeferredShadingMedia"));

	Ogre::ResourceGroupManager& rman=Ogre::ResourceGroupManager::getSingleton();
	
	rman.addResourceLocation(fionaConf.OgreMediaBasePath+std::string("\\pu\\textures"), "FileSystem", "FileSystem");
	rman.addResourceLocation(fionaConf.OgreMediaBasePath+std::string("\\pu\\scripts"), "FileSystem", "FileSystem");
	rman.addResourceLocation(fionaConf.OgreMediaBasePath+std::string("\\pu\\core"), "FileSystem", "FileSystem");
	rman.addResourceLocation(fionaConf.OgreMediaBasePath+std::string("\\pu\\models"), "FileSystem", "FileSystem");
	rman.addResourceLocation(fionaConf.OgreMediaBasePath+std::string("\\pu\\materials"), "FileSystem", "FileSystem");

	rman.createResourceGroup("Hydrax");
	rman.addResourceLocation(fionaConf.OgreMediaBasePath+std::string("\\Hydrax"), "FileSystem", "Hydrax");
    
    rman.createResourceGroup("SkyX");
	rman.addResourceLocation(fionaConf.OgreMediaBasePath+std::string("\\SkyX"), "FileSystem",  "SkyX");
    //mngr->initialiseResourceGroup("SkyX");

	addResPath(fionaConf.OgreMediaBasePath + std::string("/data"));
	addResPath(fionaConf.OgreMediaBasePath + std::string("/data/mesh"));
	addResPath(fionaConf.OgreMediaBasePath + std::string("/data/mesh/Sinbad"));
	addResPath(fionaConf.OgreMediaBasePath + std::string("/data/Sinbad"));

#endif
}

void FionaOgre::buttons(int button, int state)
{
	FionaScene::buttons(button, state);

	//this is hardcoded for the RRCP project
	//but we could use it for other things later

	float xJump = 4.77679;
	float zJump = 6.2357;

	float xJumpMin = 0*xJump;
	float xJumpMax = 5*xJump;
	float zJumpMin = -4*zJump;
	float zJumpMax = 0*zJump;
	
	if(button == 2 && state == 1)
	{
		if(!isRISE_)
		{
			/*Ogre::SceneManager::MovableObjectIterator iterator = getScene()->getMovableObjectIterator("Entity");
			while(iterator.hasMoreElements())
			{
				Ogre::Entity* e = static_cast<Ogre::Entity*>(iterator.getNext());
				e->setMaterialName("testNewColor");
			}*/
		}
		//TESTING on machine: (or wand_model type)
		//jvec3 vWandDir = camOri.rot(-ZAXIS);	//this appears gets the camera's orientation correctly
		//jvec3 vPos = camOri.rot(camPos); //need to multiply position by the orientation to get that correct

		//TESTING in dev lab: (or wand_world type)
		
		//vPos = WAND POSITION
		//jvec3 vPos = camPos + wandPos;	//might need to orient the wandPos somehow still...?
		//this correctly orients the direction of fire..
		//jvec3 vWandDir = wandOri.rot(-ZAXIS);
		//vWandDir = camOri.rot(vWandDir);
		//vWandDir at this point is WAND ORIENTATION
#ifndef LINUX_BUILD
		//NxOgre::Matrix44 matLoc(NxOgre::Vec3(vPos.x, vPos.y, vPos.z), NxOgre::Quat(camOri.w, camOri.x, camOri.y, camOri.z));
		//makeBox(matLoc, NxOgre::Vec3(vWandDir.x, vWandDir.y, vWandDir.z)*10.f);
#endif
	}
	else if(button == 3 && state == 1)
	{
		//makeBarrel(NxOgre::Matrix44::IDENTITY, NxOgre::Vec3(0.f, 0.f, 2.5f));
	}
	else if(button == 2 && state == 1)
	{
		camPos.y += 0.1f;
	}
	else if(button == 0 && state == 1)
	{
		camPos.y -= 0.1f;
	}
	else if(button == 10 && state == 1) //right
	{
		//only if we can
		if (camPos.x -xJump >= xJumpMin)
			camPos.x -=xJump;
	
		printf("move to: %d, %d\n", int(camPos.x/xJump), int(camPos.z/zJump));
	}
	else if(button == 11 && state == 1) //left
	{
		//only if we can
		if (camPos.x +xJump <= xJumpMax)
			camPos.x +=xJump;
		printf("move to: %d, %d\n", int(camPos.x/xJump), int(camPos.z/zJump));
	}
	else if(button == 12 && state == 1) //up
	{
		//only if we can
		if (camPos.z +zJump <= zJumpMax)
			camPos.z +=zJump;
		printf("move to: %d, %d\n", int(camPos.x/xJump), int(camPos.z/zJump));
	}
	else if(button == 13 && state == 1) //down
	{
		//only if we can
		if (camPos.z -zJump >= zJumpMin)
			camPos.z -=zJump;
		printf("move to: %d, %d\n", int(camPos.x/xJump), int(camPos.z/zJump));
	}

	//KEVIN HACKING
	/*
	{
		printf("button = %d\n", button);
	}
	*/
	else if(button == 5 && state == 1)
	{	
		recording_ = !recording_;
		if(!recording_)
		{
			if(recordFile_)
			{
				fclose(recordFile_);
				recordFile_ = 0;
			}
		}
	}
	else if(button == 0 && state == 1)
	{
		if(!isRISE_)
		{

		}
	}
}

void FionaOgre::changeTSR(void)
{
	m_tsr++;
	m_tsr = m_tsr % FionaOgre::NUM_TRANSFORM_MODES;
	// Play a sound to tell the user which mode they've switched to.
	/*if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
		switch (m_tsr) {
		case ROTATE:
			mSoundManager->getSound("rotate")->play();
			break;
		case TRANSLATE:
			mSoundManager->getSound("position")->play();
			break;
		case SCALE:
			mSoundManager->getSound("scale")->play();
			break;
		}
	}*/
}

void FionaOgre::clearSelection(void)
{
	std::vector<Ogre::MovableObject*>::iterator iter = m_currentSelection.begin();
	while(iter != m_currentSelection.end())
	{
		(*iter)->getParentSceneNode()->showBoundingBox(false);
		iter++;
	}

	m_currentSelection.clear();
}

void FionaOgre::displayTextureOnBackground(void)
{
	Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create("Background", "General");
	material->getTechnique(0)->getPass(0)->createTextureUnitState("rockwall.tga");
	material->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
	material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
	material->getTechnique(0)->getPass(0)->setLightingEnabled(false);
 
	// Create background rectangle covering the whole screen
	Ogre::Rectangle2D* rect = new Ogre::Rectangle2D(true);
	rect->setCorners(-1.0, 1.0, 1.0, -1.0);
	rect->setMaterial("Background");
 
	// Render the background before everything else
	rect->setRenderQueueGroup(Ogre::RENDER_QUEUE_BACKGROUND);
 
	// Use infinite AAB to always stay visible
	Ogre::AxisAlignedBox aabInf;
	aabInf.setInfinite();
	rect->setBoundingBox(aabInf);
 
	// Attach background to the scene
	Ogre::SceneNode* node = scene->getRootSceneNode()->createChildSceneNode("Background");
	node->attachObject(rect);
 
	// Example of background scrolling
	material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setScrollAnimation(-0.25, 0.0);
 
	// Don't forget to delete the Rectangle2D in the destructor of your application:
	delete rect;

}

void FionaOgre::drawAxis(void)
{
	glPushMatrix();
	glTranslatef(0.f, -CAVE_CENTER.y + 0.05f, 0.f);
	glLineWidth(4.f);
	glDisable(GL_LIGHTING);
	glColor3f(0.f, 1.f, 1.f);
	glBegin(GL_LINES);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(1000.f, 0.f, 0.f);
	glEnd();
	glColor3f(0.f, 1.f, 0.f);
	glBegin(GL_LINES);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 1000.f, 0.f);
	glEnd();
	glColor3f(0.f, 0.f, 1.f);
	glBegin(GL_LINES);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 0.f, 1000.f);
	glEnd();
	glLineWidth(1.f);
	glColor3f(1.f, 1.f, 1.f);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}
#ifndef LINUX_BUILD
void FionaOgre::drawHand(const LeapData::HandData &hand)
{
	glLineWidth(5.f);
	glColor4f(1.f, 0.f, 1.f, 1.f);
	glBegin(GL_LINES);
	jvec3 vTracker;
	getTrackerWorldSpace(vTracker);
	vTracker.z -= 1.f;	//into screen 1 meter
	vTracker.y -= 0.5f;	//down 1 meter
	for(int i = 0; i < 5; ++i)
	{
		if(hand.fingers[i].valid)
		{
			float fLen = hand.fingers[i].length;
			glVertex3f(vTracker.x + hand.fingers[i].tipPosition[0], vTracker.y + hand.fingers[i].tipPosition[1], vTracker.z + hand.fingers[i].tipPosition[2]);
			glVertex3f(vTracker.x + (hand.fingers[i].tipPosition[0] + hand.fingers[i].tipDirection[0]*fLen), 
						vTracker.y + (hand.fingers[i].tipPosition[1] + hand.fingers[i].tipDirection[1]*fLen), 
						vTracker.z + (hand.fingers[i].tipPosition[2] + hand.fingers[i].tipDirection[2]*fLen));
		}
	}
	glEnd();
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glLineWidth(1.f);
}

/*void FionaOgre::drawArm(const KinectData &data)
{
	glLineWidth(5.f);
	glColor4f(1.f, 0.f, 1.f, 1.f);
	glBegin(GL_LINES);
	vec3 vTracker;
	getTrackerWorldSpace(vTracker);
	vTracker.z -= 0.5f;	//into screen 1 meter
	vTracker.y -= 1.f;	//down 1 meter

	vTracker.x = 0;
	vTracker.y = 0;
	vTracker.z = 0;
	
	//how far are we away from the camera?
	float fLen = 20;

	//number of joint position coordinates (3) + joint rotation orientation in terms of quaternions (4)
	int structSize=7;

	//throw the butterfly net in front of the camera
	Ogre::SceneNode* netAndCamera = scene->getSceneNode("netAndCamera1");
	//netAndCamera->attachObject(camera);
	//Ogre::SceneNode* net = netAndCamera->createChildSceneNode("net");
	//net->attachObject(scene->createEntity("objectNet", "net.mesh"));
	//Ogre::SceneNode* net = scene->getRootSceneNode()->createChildSceneNode("net");
	//net->attachObject(scene->createEntity("net", "net.mesh"));

	//where is the camera located? this is from the tracked person's head's perspective
	//x, y, z (left/right(+), bottom/up(+), in(-)/out(+))
	netAndCamera->setPosition( vTracker.x + data.values[3*structSize]*fLen*1.2, vTracker.y + data.values[3*structSize+1]*fLen*1.2, data.values[3*structSize+2]*fLen*1.2 );
	//camera->setOrientation( Ogre::Quaternion(fionaConf.camRot.w, fionaConf.camRot.x, fionaConf.camRot.y, fionaConf.camRot.z) );

	Quatf qf = fionaConf.sFusion.GetOrientation();
	//netAndCamera->roll( Ogre::Radian(1) );
	//printf("camera %f %f %f\n", camera->getDirection().x, camera->getDirection().y,camera->getDirection().z );
	//printf("%f %f %f %f\n", qf.w, qf.x, qf.y, qf.z);
	
	//where are we looking at? look at the left hand for now
	//camera->lookAt( vTracker.x + data.values[9*structSize]*fLen, vTracker.y + data.values[9*structSize+1]*fLen, data.values[9*structSize+2]*fLen );
	//camera->lookAt(Ogre::Vector3(0,0,2));

	//Now we want to get the bones (segments) to do proper translation and rotation (orientation)...
	Ogre::Vector3 lHand;
	Ogre::Vector3 lForearm;
	Ogre::Vector3 lArm;
	Ogre::Vector3 rHand;
	Ogre::Vector3 rForearm;
	Ogre::Vector3 rArm;
	Ogre::Vector3 vectorZ = Ogre::Vector3::UNIT_Z;
	// for joint.
	Ogre::Quaternion q;
	Ogre::Degree deg;
	Ogre::Vector3 vectorAxis;
	// ...
	float cdgain = 1.7;					//set control-display gain to the arm segments
	float headCDgain = 4;				//set control-display gain to the head
	//This is quaternion from Oculus
	Ogre::Quaternion qOculus = Ogre::Quaternion( qf.w, qf.x, qf.y, qf.z);
	//Ogre::Quaternion qOculus = netAndCamera->getOrientation();
	Ogre::Vector3 vectorAxisOculus;
	Ogre::Degree degOculus;
	qOculus.ToAngleAxis(degOculus, vectorAxisOculus);	// translate oculus's orientation to axis and angle degree.
	//qOculus.FromAngleAxis( Ogre::Radian( degOculus.valueRadians()*headCDgain), vectorAxisOculus );	// set control-display gain to the head 
	// reset camera
	netAndCamera->setOrientation( Ogre::Quaternion::IDENTITY );
	// set rotate camera
	netAndCamera->setOrientation( qOculus );

	// calculate headDirection using net position.
	Ogre::SceneNode* net = scene->getSceneNode("net");
	Ogre::Vector3 netPosition = netAndCamera->convertLocalToWorldPosition( Ogre::Vector3(0,10,-3) );	
	Ogre::Vector3 eyePosition = netAndCamera->convertLocalToWorldPosition( Ogre::Vector3(0,10, 0) );	
	Ogre::Vector3 headDirection = netPosition.operator-(eyePosition);
	headDirection = headDirection.normalisedCopy();
	//printf("net %f %f %f\n", netPosition.x,netPosition.y,netPosition.z );

	//
	//Ogre::Vector3 vectorNegativeZ = Ogre::Vector3(0,0,1);
	//Ogre::Vector3 karen = qOculus.operator*( Ogre::Vector3::NEGATIVE_UNIT_Z );//
	//printf("ddd - %f %f %f\n", karen.x, karen.y, karen.z);
	Ogre::SceneNode* butterfly_scn = scene->getSceneNode("butterfly_scn");
	Ogre::Vector3 butterflyDirection = butterfly_scn->getPosition().operator-(eyePosition);
	butterflyDirection = butterflyDirection.normalisedCopy();
	//butterflyDirection = (1, 0, 0);
	//headDirection = headDirection.normalisedCopy();
	Ogre::Real dot = butterflyDirection.dotProduct(headDirection);
	Ogre::Radian radianOfHeadButterfly =  Ogre::Math::ACos(dot);
	Ogre::Real degOfHeadButterfly =radianOfHeadButterfly.valueDegrees();
	printf("%f - %f %f %f\n", degOfHeadButterfly, headDirection.x, headDirection.y, headDirection.z);
	//printf("%f %f %f \n", butterflyDirection.x, butterflyDirection.y, butterflyDirection.z);

	float closeDegree = 10;
	if ( degOfHeadButterfly < closeDegree || degOfHeadButterfly > 360 - closeDegree )
	{
		if (timeSinceNetOver == 0 )
			isNetOver = true;
	}

	//Left upper arm
	//take the parent joint coordinate and subtract the child joint; this will give the bone between the 2 joints
	lArm.x = data.values[4*structSize] - data.values[5*structSize];
	lArm.y = data.values[4*structSize+1] - data.values[5*structSize+1];
	lArm.z = data.values[4*structSize+2] - data.values[5*structSize+2];
	float lArmLength = lArm.length();
	lArm.normalise();
	Ogre::Quaternion qlArm = vectorZ.getRotationTo( lArm );
	qlArm.ToAngleAxis( deg , vectorAxis);
	qlArm.FromAngleAxis( Ogre::Radian( deg.valueRadians()), vectorAxis ); //add a control-display gain to the arm orientation / rotation angle (by a factor)
	//grab the arm
	Ogre::SceneNode* arm1 = scene->getSceneNode("lArm");
	q.w = data.values[5*structSize+3];
	q.x = data.values[5*structSize+4];
	q.y = data.values[5*structSize+5];
	q.z = data.values[5*structSize+6];
	arm1->resetOrientation();
	arm1->convertWorldToLocalOrientation(qlArm);
	if (q.w != 0)
		arm1->rotate(qlArm, Ogre::Node::TS_WORLD);
	arm1->setPosition( vTracker.x + data.values[4*structSize]*fLen, vTracker.y + data.values[4*structSize+1]*fLen, data.values[4*structSize+2]*fLen);

	//Left forearm
	//take the parent joint coordinate and subtract the child joint; this will give the bone between the 2 joints
	lForearm.x = data.values[5*structSize] - data.values[6*structSize];
	lForearm.y = data.values[5*structSize+1] - data.values[6*structSize+1];
	lForearm.z = data.values[5*structSize+2] - data.values[6*structSize+2];
	float lForearmLength = lForearm.length(); //compute the length of the limb segment, which will allow its distal segment to link up properly
	lForearm.normalise();
	Ogre::Quaternion qlForearm = lArm.getRotationTo( lForearm ); //rotate the forearm about the upper arm's axis
	qlForearm.ToAngleAxis( deg , vectorAxis);
	qlForearm.FromAngleAxis( Ogre::Radian( deg.valueRadians() ), vectorAxis ); //add a control-display gain to the arm orientation / rotation angle (by a factor)
	//grab the forearm
	Ogre::SceneNode* forearm1 = scene->getSceneNode("lForearm");
	q.w = data.values[6*structSize+3];
	q.x = data.values[6*structSize+4];
	q.y = data.values[6*structSize+5];
	q.z = data.values[6*structSize+6];
	forearm1->resetOrientation();
	forearm1->convertWorldToLocalOrientation(qlForearm);
	if (q.w != 0)
		forearm1->rotate(qlForearm, Ogre::Node::TS_WORLD);
	forearm1->setPosition( 0, 0, lArmLength*fLen/(-10));
	//forearm1->setPosition( lForearm.x*forearmLength*fLen, lForearm.y*forearmLength*fLen, lForearm.z*forearmLength*fLen);
			
	//Left hand
	//take the parent joint coordinate and subtract the child joint; this will give the bone between the 2 joints
	lHand.x = data.values[6*structSize] - data.values[7*structSize];
	lHand.y = data.values[6*structSize+1] - data.values[7*structSize+1];
	lHand.z = data.values[6*structSize+2] - data.values[7*structSize+2];
	lHand.normalise();
	Ogre::Quaternion qlHand = lForearm.getRotationTo( lHand );  //rotate the hand about the forearm's axis
	//grab the hand 
	Ogre::SceneNode* hand1 = scene->getSceneNode("lHand");			
	q.w = data.values[6*structSize+3];
	q.x = data.values[6*structSize+4];
	q.y = data.values[6*structSize+5];
	q.z = data.values[6*structSize+6];
	hand1->resetOrientation();
	hand1->convertWorldToLocalOrientation(qlHand);
	if (q.w != 0)
		hand1->rotate(qlHand, Ogre::Node::TS_WORLD);
	hand1->setPosition( 0, 0, lForearmLength*fLen/(-10));
	//hand1->setPosition( vTracker.x + data.values[6*structSize]*fLen, vTracker.y + data.values[6*structSize+1]*fLen, data.values[6*structSize+2]*fLen);
		
	//Right upper arm
	//take the parent joint coordinate and subtract the child joint; this will give the bone between the 2 joints
	rArm.x = data.values[8*structSize] - data.values[9*structSize];
	rArm.y = data.values[8*structSize+1] - data.values[9*structSize+1];
	rArm.z = data.values[8*structSize+2] - data.values[9*structSize+2];
	float rArmLength = rArm.length(); //compute the length of the limb segment, which will allow its distal segment to link up properly
	rArm.normalise();
	Ogre::Quaternion qrArm = vectorZ.getRotationTo( rArm ); 
	qrArm.ToAngleAxis( deg , vectorAxis);
	qrArm.FromAngleAxis( Ogre::Radian( deg.valueRadians()*cdgain ), vectorAxis ); //add a control-display gain to the arm orientation / rotation angle (by a factor)
	//grab the upper arm
	Ogre::SceneNode* arm2 = scene->getSceneNode("rArm");
	q.w = data.values[9*structSize+3];
	q.x = data.values[9*structSize+4];
	q.y = data.values[9*structSize+5];
	q.z = data.values[9*structSize+6];
	arm2->resetOrientation();
	arm2->convertWorldToLocalOrientation(qrArm);
	if (q.w != 0)
		arm2->rotate(qrArm, Ogre::Node::TS_WORLD);
	arm2->setPosition( vTracker.x + data.values[8*structSize]*fLen, vTracker.y + data.values[8*structSize+1]*fLen, data.values[8*structSize+2]*fLen);
	
	//Right forearm
	//take the parent joint coordinate and subtract the child joint; this will give the bone between the 2 joints
	rForearm.x = data.values[9*structSize] - data.values[10*structSize];
	rForearm.y = data.values[9*structSize+1] - data.values[10*structSize+1];
	rForearm.z = data.values[9*structSize+2] - data.values[10*structSize+2];
	float rForearmLength = rForearm.length(); //compute the length of the limb segment, which will allow its distal segment to link up properly
	rForearm.normalise();
	Ogre::Quaternion qrForearm = rArm.getRotationTo( rForearm ); //rotate the forearm about the arm's axis
	qrForearm.ToAngleAxis( deg , vectorAxis);
	qrForearm.FromAngleAxis( Ogre::Radian( deg.valueRadians()*cdgain ), vectorAxis ); //add a control-display gain to the arm orientation / rotation angle (by a factor)
	//grab the forearm mesh
	Ogre::SceneNode* forearm2 = scene->getSceneNode("rForearm");
	q.w = data.values[10*structSize+3];
	q.x = data.values[10*structSize+4];
	q.y = data.values[10*structSize+5];
	q.z = data.values[10*structSize+6];
	forearm2->resetOrientation();
	forearm2->convertWorldToLocalOrientation(qrForearm);
	if (q.w != 0)
		forearm2->rotate(qrForearm, Ogre::Node::TS_WORLD);
	forearm2->setPosition( 0, 0, rArmLength*fLen/(-10));
	//forearm2->setPosition( vTracker.x + data.values[9*structSize]*fLen, vTracker.y + data.values[9*structSize+1]*fLen, data.values[9*structSize+2]*fLen);
	
	//Right hand
	//take the parent joint coordinate and subtract the child joint; this will give the bone between the 2 joints
	rHand.x = data.values[10*structSize] - data.values[11*structSize];
	rHand.y = data.values[10*structSize+1] - data.values[11*structSize+1];
	rHand.z = data.values[10*structSize+2] - data.values[11*structSize+2];
	rHand.normalise();
	Ogre::Quaternion qrHand = rForearm.getRotationTo( rHand ); //rotate the hand about the forearm's axis
	//grab the hand mesh
	Ogre::SceneNode* hand2 = scene->getSceneNode("rHand");
	q.w = data.values[10*structSize+3];
	q.x = data.values[10*structSize+4];
	q.y = data.values[10*structSize+5];
	q.z = data.values[10*structSize+6];
	hand2->resetOrientation();
	hand2->convertWorldToLocalOrientation(qrHand);
	if (q.w != 0)
		hand2->rotate(qrHand, Ogre::Node::TS_WORLD);
	hand2->setPosition( 0, 0, rForearmLength*fLen/(-10));
	//hand2->setPosition( vTracker.x + data.values[10*structSize]*fLen, vTracker.y + data.values[10*structSize+1]*fLen, data.values[10*structSize+2]*fLen);

	//data.valid = false;
	glEnd();
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glLineWidth(1.f);
}*/
#endif

void FionaOgre::getSecondTrackerPos(jvec3 &vOut, const jvec3 &vOffset) const
{
	FionaScene::getSecondTrackerWorldSpace(vOut);
	//vOut += vOffset;
	/*float toDeg = m_wiiFitRotation * PI/180;
    mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
    //vOut = m.transpose().inv()  * vOut;
	vOut = m  * vOut;*/
}

jvec3 FionaOgre::getWandPos(float x_off, float y_off, float z_off) const {
    jvec3 vPos;
    getWandWorldSpace(vPos);
    vPos.x += x_off; vPos.y += y_off; vPos.z += z_off;

    /*float toDeg = m_wiiFitRotation * PI/180;
    mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
    //vPos = m.transpose().inv()  * vPos;
	//jvec3 vOffset(x_off, y_off, z_off);
	//vOffset = m.transpose().inv() * vOffset;
	vPos = m * vPos;*/

    return vPos;
}

void FionaOgre::initOgre(std::string scene)
{
	sceneName = scene;
#ifndef LINUX_BUILD
	ogreSetup();
#endif
}

void FionaOgre::ogreSetup(void)
{
#ifdef WIN32
#ifdef _DEBUG
	root = new Ogre::Root("Plugins_d.cfg");
#else
	root = new Ogre::Root("Plugins.cfg");
#endif
	std::string workDir="";
#endif
		
#ifdef __APPLE__
		printf("******** Think Different! **********\n");
		std::string workDir=Ogre::macBundlePath() + "/Contents/Resources/";
		root=new Ogre::Root(Ogre::getOgrePluginCFGPath());
#endif
#ifdef LINUX_BUILD
#ifdef ROSS_TEST
		std::string workDir="/home/rtredinnick/dev/FionaOgre/SDK/linux/Samples/Media/";
		std::string pluginConfigPath = "/home/rtredinnick/dev/FionaOgre/SDK/linux/bin/plugins.cfg";
		std::string ogreConfigPath = "/home/rtredinnick/dev/FionaOgre/SDK/linux/bin/resources.cfg";
		printf("Plugin configuration file path: %s", pluginConfigPath.c_str());
		root = new Ogre::Root(pluginConfigPath);//, ogreConfigPath, "/tmp/ogre.log");
#else
		printf("*************** Linux forever ************\n");
		std::string workDir="/mnt/dscvr/apps/FionaOgre/SDK/linux/install/share/OGRE/";
		std::string pluginConfigPath = workDir + "plugins.cfg";
		std::string ogreConfigPath = workDir + "resources.cfg";
		printf("Plugin configuration file path: %s", pluginConfigPath.c_str());
		root = new Ogre::Root(pluginConfigPath);//, ogreConfigPath, "/tmp/ogre.log");
#endif
#endif
	
	Ogre::RenderSystem* _rs=root->getRenderSystemByName("OpenGL Rendering Subsystem");
   	//_rs->setConfigOption("Full Screen", "No");
   	_rs->setConfigOption("VSync", "true");
   	_rs->setConfigOption("FSAA", "8");
	_rs->setConfigOption(Ogre::String("RTT Preferred Mode"), Ogre::String("FBO"));	
_rs->setConfigOption("sRGB Gamma Conversion", "No");
	root->setRenderSystem(_rs);
	
	_rs->setConfigOption(Ogre::String("FSAA"), Ogre::String("8"));
	printf("%s\n", fionaConf.OgreMediaBasePath.c_str());
	root->initialise(false);
	addResPath(workDir);
	addResPath(".");
	addResPath(fionaConf.OgreMediaBasePath);

	// User resource path
	addResourcePaths();
}

void FionaOgre::initWin(WIN nwin, CTX ctx, int gwin, int w, int h)
{
	printf("Setting up OGRE\n");
#ifdef LINUX_BUILD
	ogreSetup();
#endif
	printf("******************************INITIALIZING OGRE WINDOW*******************************\n");
	Ogre::NameValuePairList misc;
#ifdef WIN32
	HWND hWnd = (HWND)nwin;
	HGLRC hRC = (HGLRC)ctx;
	misc["externalWindowHandle"] = Ogre::StringConverter::toString((int)hWnd);
	misc["externalGLContext"] = Ogre::StringConverter::toString((unsigned long)hRC);
	//misc["FSAA"] = Ogre::StringConverter::toString(4);
	misc["externalGLControl"] = "true";
#elif defined __APPLE__
	NSView* view = (NSView *)(nwin);
	NSOpenGLContext* context = (NSOpenGLContext *)(ctx);
	misc["macAPI"] = "cocoa";
	misc["macAPICocoaUseNSView"] = "true";
	misc["externalWindowHandle"] = Ogre::StringConverter::toString((size_t)view);
	misc["externalGLContext"] = Ogre::StringConverter::toString((unsigned long)context);
	misc["externalGLControl"] = "true";
#elif defined LINUX_BUILD	
	FionaUTWin *xwin = (FionaUTWin*)nwin;
	//we need to set currentGLContext to true to tell GLX that ogre holds the rendering context
	misc["currentGLContext"] = "true";
	misc["externalGLControl"] = "true";
#endif

	misc["FSAA"] = "8";
	misc["vsync"] = "true";
	
	ogreWin = root->createRenderWindow("Ogre"+Ogre::StringConverter::toString(wins.size()),1920,1920,false,&misc);
	//ogreWin = root->createRenderWindow("Ogre"+Ogre::StringConverter::toString(wins.size()),w,h,false,&misc);
	//in linux we also need the below 2 functions! active and visible	
	ogreWin->setActive(true);
	ogreWin->setVisible(true);
	bool firstWindow = (scene == NULL);
	// Creating scene manager and setup other managers
	if( firstWindow ) // Meaning it is the first window ever.
	{
		printf("*******in first window*******\n");
		scene = root->createSceneManager(Ogre::ST_GENERIC);
		scene->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));
		//Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
		Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(10);
			
		camera = scene->createCamera("OgreGLUTDefaultCamera");
		camera->setNearClipDistance(0.01f);
		camera->setFarClipDistance(100000);
		//camera->setAutoAspectRatio(true);
		camera->lookAt(Ogre::Vector3(0,0,-10.0));
		camera->setPosition(Ogre::Vector3(0,0,20.0));
			
		headLight=scene->createLight("OgreGLUTDefaultHeadLight");
		headLight->setType(Ogre::Light::LT_POINT);
		headLight->setDiffuseColour(1, 1, 1);
		headLight->setPosition(Ogre::Vector3(0,0,0));
	}

	vp = ogreWin->addViewport(camera);
	vp->setClearEveryFrame(false);
	//vp->setBackgroundColour(Ogre::ColourValue(0.86, 0.86, 0.86, 1));
	vp->setDimensions(.0f, .0f, 1.0f, 1.0f);

	// Add to management system
	wins.push_back(FionaOgreWinInfo(ogreWin,nwin,gwin,ctx,vp));
	if(firstWindow) 
	{
	    setupScene(scene);
            /*Ogre::SceneNode* headNode = scene->getRootSceneNode()->createChildSceneNode();
            Ogre::Entity* entity = scene->createEntity("robot" , "robot.mesh");
            headNode->attachObject(entity);
            Ogre::AnimationState* mAnimState = entity->getAnimationState("Walk");
            mAnimState->setEnabled(true);
            mAnimState->setLoop(true);
		 mAnimState->addTime(0.05);for animation call this before the renderOneFrame*/
	}
}

void FionaOgre::ogreRender(void)
{
	WIN win = FionaUTGetNativeWindow();
	if( !isInited(win) )
	{
		CTX ctx = FionaUTGetContext(); 
		int width = glutGet(GLUT_WINDOW_WIDTH);
		int height = glutGet(GLUT_WINDOW_HEIGHT);
		printf("Ogre window initialization: %u, %u\n", width, height);
		initWin(win,ctx,glutGetWindow(),width, height);
	}

	vp->setBackgroundColour(Ogre::ColourValue(fionaConf.backgroundColor.x,fionaConf.backgroundColor.y,fionaConf.backgroundColor.z));
		
	/* //DEBUG//
	const Ogre::RenderSystemList & list = root->getAvailableRenderers();
	for(unsigned int j = 0; j < list.size(); ++j)
	{
		Ogre::ConfigOptionMap &om = list[j]->getConfigOptions();
		Ogre::StringVector v = om["FSAA"].possibleValues;
		printf("%s\n", om["FSAA"].currentValue.c_str());
		for(unsigned int i = 0; i < v.size(); ++i)
		{
			printf("Possible FSAA: %s\n", v[i].c_str());
		}
	}
	//DEBUG// */

#ifdef LINUX_BUILD
	ogreWin->setActive(true);
#endif

	if( _FionaUTIsInFBO() )
	{
		int i = findWin(glutGetWindow());
#ifdef __APPLE__
		wins[i].owin->resize(_FionaUTGetFBOWidth(),_FionaUTGetFBOHeight());
#else
		/*if(fionaConf.appType == FionaConfig::OCULUS)
		{
			if(fionaRenderCycleLeft)
			{
				wins[i].vp->setDimensions(0,0,0.5f, 1.f);//_FionaUTGetFBOWidth()/(float)glutGet(GLUT_WINDOW_WIDTH),
					//_FionaUTGetFBOHeight()/(float)glutGet(GLUT_WINDOW_HEIGHT));
			}
			else
			{
				wins[i].vp->setDimensions(0.5f,0,0.5f, 1.f);//_FionaUTGetFBOWidth()/(float)glutGet(GLUT_WINDOW_WIDTH),
					//_FionaUTGetFBOHeight()/(float)glutGet(GLUT_WINDOW_HEIGHT));
			}
		}*/
#endif
	}
	else
	{
#ifndef LINUX_BUILD
		//this is screwing things up.. ogre 1.8..
		int wid = glutGet(GLUT_WINDOW_WIDTH);
		int hei = glutGet(GLUT_WINDOW_HEIGHT);
		//printf("%d, %d\n", wid, hei);
		resize(glutGetWindow(),wid,hei);
#endif
	}

	if(fionaRenderCycleCount == 0)
	{
		vp->clear();
	}

	tran m, p;
	glGetFloatv(GL_MODELVIEW_MATRIX, m.p);
	glGetFloatv(GL_PROJECTION_MATRIX, p.p);
	lastOgreMat = toOgreMat(m);
	camera->setCustomViewMatrix(TRUE, lastOgreMat);
	camera->setCustomProjectionMatrix(TRUE, toOgreMat(p));

	//the below three lines have been added for the visualization so billboarding works, but this actually might be the correct thing to do anyways.. in general for other features to correctly work within Ogre.
	/*camera->setPosition(lastOgreMat.getTrans());
	Ogre::Quaternion q = lastOgreMat.transpose().extractQuaternion();
	camera->setOrientation(q);*/
	
	if( FionaIsFirstOfCycle() )
	{
#ifndef LINUX_BUILD
		//check for leap-based navigation..todo - move this into some sort of event class..
		if(fionaConf.leapData.hand1.valid && fionaConf.leapData.hand2.valid == false)
		{
			if(fionaConf.leapData.hand1.fingers[0].valid
				&& (fionaConf.leapData.hand1.fingers[1].valid == false)
				&& (fionaConf.leapData.hand1.fingers[2].valid == false)
				&& (fionaConf.leapData.hand1.fingers[3].valid == false)
				&& (fionaConf.leapData.hand1.fingers[4].valid == false))
			{
				float fX = fionaConf.leapData.hand1.fingers[0].tipDirection[0];
				float fY = fionaConf.leapData.hand1.fingers[0].tipDirection[1];
				float fZ = fionaConf.leapData.hand1.fingers[0].tipDirection[2];
				//we want to rotate this around x axis about 30 degrees or so..
				jvec3 wandDir(fX, fY, fZ);
				quat xRot = r2q(-30.f * PI / 180.f, XAXIS);
				wandDir = xRot.rot(wandDir);
				camPos += camOri.rot(wandDir * fionaConf.navigationSpeed * 0.25f);
			}
		}
#endif
		//jvec3 pos = camOri.rot(camPos);
		if(headLight)
		{
			if(fionaConf.appType == FionaConfig::WINDOWED || fionaConf.appType == FionaConfig::OCULUS)
			{

				headLight->setPosition(Ogre::Vector3(camPos.x, camPos.y, camPos.z));
			}
			else
			{
				jvec3 vPos;
				getTrackerWorldSpace(vPos);
				headLight->setPosition(vPos.x, vPos.y, vPos.z);
			}
		}

		/*float currTime = FionaUTTime();
		if(lastTime == 0.f)
		{
			lastTime=currTime;
		}
		physicsWorld->advance(currTime-lastTime);
		lastTime = currTime;*/

		/*Ogre::SceneNode* butterfly_scn = scene->getSceneNode("butterfly_scn");

		if (isNetOver)
		{
			Ogre::Vector3 nowPos = butterfly_scn->getPosition();
			//printf("%f %f %f\n",nowPos.x, nowPos.y, nowPos.z);
			Ogre::Vector3 distanceRemain = finalPos.operator-(nowPos);
			float timeRemain = totalTime-timeSinceNetOver;
			Ogre::Vector3 moveDistance = distanceRemain*intervalTime/timeRemain;

			float maxNoise = 0.1;
			int randomNum_X = rand()%10;	// random number 0~9
			int randomNum_Y = rand()%10;	// random number 0~9
			float randf = Ogre::Math::Sin(timeRemain);
			Ogre::Vector3 noiseDistance = Ogre::Vector3( maxNoise*randf, maxNoise*randf, 0 );
			//Ogre::Vector3 noiseDistance = Ogre::Vector3( maxNoise*randomNum_X/10.0 - maxNoise/2, maxNoise*randomNum_Y/10.0 - maxNoise/2, 0 );

			printf("%f %f %f\n",noiseDistance.x, noiseDistance.y, noiseDistance.z);
			butterfly_scn->translate( moveDistance );
			butterfly_scn->translate( noiseDistance );
			timeSinceNetOver += intervalTime;
			if (timeSinceNetOver >= totalTime)
			{
				butterfly_scn->setPosition( finalPos );
				isNetOver = false;
			}
		}*/
#ifndef LINUX_BUILD
		if(physicsWorld)
		{
#if USE_BULLET
			physicsWorld->stepSimulation(fionaConf.physicsStep);
#else
			if(physicsOn)
			{
				physicsWorld->advance(fionaConf.physicsStep);
			}
#endif
		}
		
		//update any water if it exists..
		if(mHydrax)
		{
			mHydrax->update(fionaConf.physicsStep);
		}

		if(mSkyx && !isRISE_)
		{
			mSkyx->update(fionaConf.physicsStep);
			mSkyx->notifyCameraRender(scene->getCamera("OgreGLUTDefaultCamera"));
		}

		for(unsigned int i = 0; i < mPGHandles.size(); i++)
		{
			mPGHandles[i]->update();
		}
#endif
	}
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	root->renderOneFrame();
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}

void FionaOgre::preRender(float t)
{
	FionaScene::preRender(t);

	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED))
	{
		if(m_SoundManager)
		{
			m_SoundManager->update();
		}
	}

	if(playingBack_)
	{

		if(mSkyx && isRISE_)
		{
			mSkyx->update(fionaConf.physicsStep);
			mSkyx->notifyCameraRender(scene->getCamera("OgreGLUTDefaultCamera"));
		}

		if(camPlayback_.size() > 0)
		{
			/*static const float frameTime = playbackTime_ / camPlayback_.size();
			float currTime = FionaUTTime();
			if(currTime - lastFrameTime_ > frameTime)
			{*/
				lastIndex_++;
				if(lastIndex_ == camPlayback_.size())
				{
					lastIndex_ = 0;
				}
				//lastFrameTime_ = currTime;
			//}

			int currIndex = lastIndex_;
			int nextIndex = lastIndex_+1;
			if(nextIndex == camPlayback_.size())
			{
				nextIndex = 0;
				playingBack_ = false;
			}

			camPos = camPlayback_[nextIndex].pos;
			camOri = camPlayback_[nextIndex].rot;
			/*float t = (currTime - lastFrameTime_ ) / frameTime;
			jvec3 toNext = jvec3(camPlayback_[nextIndex].pos[0] - camPlayback_[currIndex].pos[0], 
								camPlayback_[nextIndex].pos[1] - camPlayback_[currIndex].pos[1],
								camPlayback_[nextIndex].pos[2] - camPlayback_[currIndex].pos[2]);

			if(toNext.len() > 0.f)
			{
				toNext = toNext.normalize();
				jvec3 currLoc = jvec3(camPlayback_[currIndex].pos[0], camPlayback_[currIndex].pos[1], camPlayback_[currIndex].pos[2]);
				quat n = camPlayback_[nextIndex].rot;
				//printf("%f\n", t);
				camPos = currLoc + toNext * t;
				camOri = SLERP(camPlayback_[currIndex].rot, n, t);
			}*/

			if(fionaConf.appType == FionaConfig::HEADNODE)
			{
				//_FionaUTSyncSendCamera(camPos, camOri);
			}
		}
	}

	if(recording_)
	{
		if(fionaConf.appType == FionaConfig::HEADNODE || fionaConf.appType == FionaConfig::WINDOWED || fionaConf.appType == FionaConfig::DEVLAB)
		{
			if(recordFile_ == 0)
			{
				recordFile_ = fopen("caveOut.txt", "w");
			}

			if(recordFile_)
			{
				char buf[256];
				memset(buf, 0, 256);
				sprintf(buf, "%f, %f, %f\n%f, %f, %f, %f\n", camPos[0], camPos[1], camPos[2], camOri[0], camOri[1], camOri[2], camOri[3]);
				std::string s(buf);
				fwrite(s.c_str(), s.length(), 1, recordFile_);
			}

			//printf("%f, %f, %f\n", camPos[0], camPos[1], camPos[2]);
		}
	}

	if(isRISE_)
	{
		if(playingBack_)
		{
			if((FionaUTTime() - lastCamTime_) > 180)
			{
#ifndef LINUX_BUILD
				ParticleUniverse::ParticleSystemManager* pManager = ParticleUniverse::ParticleSystemManager::getSingletonPtr();
				ParticleUniverse::ParticleSystem* pSys1 = pManager->createParticleSystem("pSys1", "snow", scene);
				ParticleUniverse::ParticleSystem* pSys2 = pManager->createParticleSystem("pSys2", "snow", scene);
				scene->getRootSceneNode()->attachObject(pSys1);
				scene->getRootSceneNode()->attachObject(pSys2);

				// Scale the particle systems, just because we can!
				pSys1->setScaleVelocity(10);
				pSys1->setScale(Ogre::Vector3(10, 10, 10));
				pSys2->setScaleVelocity(10);
				pSys2->setScale(Ogre::Vector3(10, 10, 10));

				// Adjust the position of the particle systems a bit by repositioning their ParticleTechnique (there is only one technique in mp_torch)
				// Normally you would do that by setting the position of the SceneNode to which the Particle System is attached, but in this
				// demo they are both attached to the same rootnode.
				pSys1->getTechnique(0)->position = Ogre::Vector3(5, 0, 0);
				pSys2->getTechnique(0)->position = Ogre::Vector3(-5, 0, 0);

				// Start!
				pSys1->start();
				pSys2->start();
				snowStarted_ = true;
#endif
				if(!physicsOn)
				{
					for(int i = 0; i < dynamicObjects.size(); ++i)
					{
						dynamicObjects[i]->putToSleep();
					}
					lastFrameTime_ = FionaUTTime();
				}

				physicsOn = true;
				
				if((FionaUTTime() - lastFrameTime_) > 0.25)
				{
					static unsigned int j = 0;
					if(j < dynamicObjects.size())
					{
						//dynamicObjects[j]->wakeUp(0.1f);
						dynamicObjects[j]->addLocalForce(NxOgre::Vec3(-1.f, 0.f, 0.f));
						j++;
					}
					lastFrameTime_ = FionaUTTime();
				}

				if(!snowStarted_)
				{
					scene->destroyAllParticleSystems();
					ParticleUniverse::ParticleSystemManager* pManager = ParticleUniverse::ParticleSystemManager::getSingletonPtr();
					ParticleUniverse::ParticleSystem* pSys1 = pManager->createParticleSystem("pSys1", "snow", scene);
					ParticleUniverse::ParticleSystem* pSys2 = pManager->createParticleSystem("pSys2", "snow", scene);
					ParticleUniverse::ParticleSystem* pSys3 = pManager->createParticleSystem("pSys3", "snow", scene);
					ParticleUniverse::ParticleSystem* pSys4 = pManager->createParticleSystem("pSys4", "snow", scene);
					ParticleUniverse::ParticleSystem* pSys5 = pManager->createParticleSystem("pSys5", "snow", scene);
					ParticleUniverse::ParticleSystem* pSys6 = pManager->createParticleSystem("pSys6", "snow", scene);

					scene->getRootSceneNode()->attachObject(pSys1);
					scene->getRootSceneNode()->attachObject(pSys2);
					scene->getRootSceneNode()->attachObject(pSys3);
					scene->getRootSceneNode()->attachObject(pSys4);
					scene->getRootSceneNode()->attachObject(pSys5);
					scene->getRootSceneNode()->attachObject(pSys6);

					// Scale the particle systems, just because we can!
					//pSys1->setScaleVelocity(10);
					//pSys1->setScale(Ogre::Vector3(10, 10, 10));
					//pSys2->setScaleVelocity(10);
					//pSys2->setScale(Ogre::Vector3(10, 10, 10));

					// Adjust the position of the particle systems a bit by repositioning their ParticleTechnique (there is only one technique in mp_torch)
					// Normally you would do that by setting the position of the SceneNode to which the Particle System is attached, but in this
					// demo they are both attached to the same rootnode.
					pSys1->getTechnique(0)->position = Ogre::Vector3(22.0032661175374,7.50230726974056, 8.60745499397158);
					pSys2->getTechnique(0)->position = Ogre::Vector3(-27.0636394408882, 21.2387186023048, 11.4033715614286);
					pSys3->getTechnique(0)->position = Ogre::Vector3(-17.1129825454822, 32.3896968888445, 12.3805692477324);
					pSys4->getTechnique(0)->position = Ogre::Vector3(29.6937177362789, -20.0098679047165, 7.21595379912007);
					pSys5->getTechnique(0)->position = Ogre::Vector3(-19.3731878221466, -6.27345657215224, 10.0118703665771);
					pSys6->getTechnique(0)->position = Ogre::Vector3(31.9539230129434, 18.6532855562802,  9.5846526802754);

					// Start!
					pSys1->start();
					pSys2->start();
					pSys3->start();
					pSys4->start();
					pSys5->start();
					pSys6->start();
					snowStarted_ = true;
				}
			}
		}
	}

	if(m_pSB)
	{
		SmartBody::SBSimulationManager* sim = m_pSB->getSimulationManager();
		//static float mStartTime = Ogre::Root::getSingleton().getTimer()->getMilliseconds() / 1000.0f;
		sim->setTime(fionaConf.physicsTime);//(Ogre::Root::getSingleton().getTimer()->getMilliseconds() / 1000.0f) - mStartTime);
		m_pSB->update();

		int numCharacters = m_pSB->getNumCharacters();
		if (numCharacters == 0)
			return;

		if(simTestOn)
		{
			//sim->updateTimer();
		}

		const std::vector<std::string>& characterNames = m_pSB->getCharacterNames();

		for (size_t n = 0; n < characterNames.size(); n++)
		{
			SmartBody::SBCharacter* character = m_pSB->getCharacter(characterNames[n]);
			if (!scene->hasEntity(characterNames[n]))
				continue;

			Ogre::Entity* entity = scene->getEntity(characterNames[n]);
			Ogre::Skeleton* meshSkel = entity->getSkeleton();
			Ogre::Node* node = entity->getParentNode();

			SrVec pos = character->getPosition();
			SrQuat ori = character->getOrientation();
			//std::cout << ori.w << ori.x << " " << ori.y << " " << ori.z << std::endl;
			node->setPosition(pos.x, pos.y, pos.z);
			node->setOrientation(ori.w, ori.x, ori.y, ori.z);
	
			// Update joints
			SmartBody::SBSkeleton* sbSkel = character->getSkeleton();
			
			int numJoints = sbSkel->getNumJoints();
			for (int j = 0; j < numJoints; j++)
			{
				SmartBody::SBJoint* joint = sbSkel->getJoint(j);
	
				if(embody)
				{
					if(n == 0)
					{
						//map kinect joints to character joints...!
						static const int structSize = 7;
						static const unsigned int NUI_SKELETON_POSITION_HIP_CENTER	= 0;
						static const unsigned int NUI_SKELETON_POSITION_SPINE	= ( NUI_SKELETON_POSITION_HIP_CENTER + 1 );
						static const unsigned int NUI_SKELETON_POSITION_SHOULDER_CENTER	= ( NUI_SKELETON_POSITION_SPINE + 1 );
						static const unsigned int NUI_SKELETON_POSITION_HEAD	= ( NUI_SKELETON_POSITION_SHOULDER_CENTER + 1 );
						static const unsigned int NUI_SKELETON_POSITION_SHOULDER_LEFT	= ( NUI_SKELETON_POSITION_HEAD + 1 );
						static const unsigned int NUI_SKELETON_POSITION_ELBOW_LEFT	= ( NUI_SKELETON_POSITION_SHOULDER_LEFT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_WRIST_LEFT	= ( NUI_SKELETON_POSITION_ELBOW_LEFT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_HAND_LEFT	= ( NUI_SKELETON_POSITION_WRIST_LEFT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_SHOULDER_RIGHT	= ( NUI_SKELETON_POSITION_HAND_LEFT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_ELBOW_RIGHT	= ( NUI_SKELETON_POSITION_SHOULDER_RIGHT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_WRIST_RIGHT	= ( NUI_SKELETON_POSITION_ELBOW_RIGHT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_HAND_RIGHT	= ( NUI_SKELETON_POSITION_WRIST_RIGHT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_HIP_LEFT	= ( NUI_SKELETON_POSITION_HAND_RIGHT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_KNEE_LEFT	= ( NUI_SKELETON_POSITION_HIP_LEFT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_ANKLE_LEFT	= ( NUI_SKELETON_POSITION_KNEE_LEFT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_FOOT_LEFT	= ( NUI_SKELETON_POSITION_ANKLE_LEFT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_HIP_RIGHT	= ( NUI_SKELETON_POSITION_FOOT_LEFT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_KNEE_RIGHT	= ( NUI_SKELETON_POSITION_HIP_RIGHT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_ANKLE_RIGHT	= ( NUI_SKELETON_POSITION_KNEE_RIGHT + 1 );
						static const unsigned int NUI_SKELETON_POSITION_FOOT_RIGHT	= ( NUI_SKELETON_POSITION_ANKLE_RIGHT + 1 );
						
						//indexing scheme joint # * structSize + 0-6 (0,1,2 are position, 3,4,5,6 are rotation)
						//fionaConf.kinectData[
						try
						{
							Ogre::Bone* bone = meshSkel->getBone(joint->getName());
							if (!bone)
								continue;

							unsigned int kinectIndex = 0;
							bool found = false;
							for(unsigned int i = 0; i < kinectMapping.size(); ++i)
							{
								if(!strcmp(joint->getName().c_str(), kinectMapping[i].c_str()))
								{
									kinectIndex = i;
									found=true;
									break;
								}
							}

							if(found)
							{
								//printf("%s: %f, %f, %f\n", kinectMapping[kinectIndex].c_str(), fionaConf.kinectData.values[kinectIndex * structSize + 0], fionaConf.kinectData.values[kinectIndex * structSize + 1], fionaConf.kinectData.values[kinectIndex * structSize + 2]);
								bone->setPosition(bone->getInitialPosition() + Ogre::Vector3(fionaConf.kinectData.values[kinectIndex * structSize + 0], fionaConf.kinectData.values[kinectIndex * structSize + 1], -fionaConf.kinectData.values[kinectIndex * structSize + 2]));
								Ogre::Quaternion q(fionaConf.kinectData.values[kinectIndex * structSize + 3], fionaConf.kinectData.values[kinectIndex * structSize + 4], fionaConf.kinectData.values[kinectIndex * structSize + 5], fionaConf.kinectData.values[kinectIndex * structSize + 6]);
								//Ogre::Quaternion rot(Ogre::Radian(Ogre::Degree(180.f)), Ogre::Vector3(0.f, 1.f, 0.f));
								//q = rot * q;
								bone->setOrientation(q);
							}
							else
							{
								SrQuat orientation = joint->quat()->value();
								Ogre::Vector3 posDelta(joint->getPosition().x, joint->getPosition().y, joint->getPosition().z);
								Ogre::Quaternion quatDelta(orientation.w, orientation.x, orientation.y, orientation.z);
								bone->setPosition(bone->getInitialPosition() + posDelta);
								bone->setOrientation(quatDelta);
							}
						}
						catch (Ogre::ItemIdentityException& ex)
						{
							// Should not happen as we filtered using m_mValidBones
						}
						//bone->setPosition(bone->getInitialPosition() + posDelta);
						//bone->setOrientation(quatDelta);
					}
				}

				if(!embody || n != 0)
				{
					try
					{
						SrQuat orientation = joint->quat()->value();
	
						Ogre::Vector3 posDelta(joint->getPosition().x, joint->getPosition().y, joint->getPosition().z);
						Ogre::Quaternion quatDelta(orientation.w, orientation.x, orientation.y, orientation.z);
						Ogre::Bone* bone = meshSkel->getBone(joint->getName());
						if (!bone)
							continue;
						bone->setPosition(bone->getInitialPosition() + posDelta);
						bone->setOrientation(quatDelta);
					}
					catch (Ogre::ItemIdentityException& ex)
					{
						// Should not happen as we filtered using m_mValidBones
					}
				}
			}
		}
	}
}

void FionaOgre::render(void)
{
	FionaScene::render();

	ogreRender();

	preOpenGLRender();

	if(drawAxis_)
	{
		drawAxis();
	}
	
	openGLRender();

	postOpenGLRender();
}

void FionaOgre::preOpenGLRender(void)
{
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity(); //Texture addressing should start out as direct.
	glMatrixMode(GL_MODELVIEW);

	static Ogre::Pass* clearPass = NULL;
	if (!clearPass)
	{
		Ogre::MaterialPtr clearMat = Ogre::MaterialManager::getSingleton().getByName("BaseWhite");
		clearPass = clearMat->getTechnique(0)->getPass(0);
	}
	//Set a clear pass to give the renderer a clear renderstate
	scene->_setPass(clearPass, true, false);

#ifdef LINUX_BUILD
	ogreWin->setActive(false);
#endif
}

void FionaOgre::openGLRender(void)
{
	//default openglrendering for a fionaOgre application..

	//NOTE: we should put this in a config
	bool bDrawWand = true;

	if(VRAction *pAction = m_actions.GetCurrentSet()->GetCurrentAction())
	{
		if(pAction->GetType() == VRAction::WAND)
		{
			bDrawWand = static_cast<VRWandAction*>(pAction)->IsCaptureWand();
		}
	}

	static const float fLineLength = 25.f;
	if(drawWand_ && bDrawWand)
	{
		jvec3 vPos;
		getWandWorldSpace(vPos, true);
		//this correctly orients the direction of fire..
		jvec3 vWandDir;
		getWandDirWorldSpace(vWandDir, true);
		//if in world mode, let's draw a wand "beam"
		glPushMatrix();
		glLineWidth(4.f);
		glDisable(GL_LIGHTING);
		//glDisable(GL_DEPTH_TEST);
		if(m_tsr == FionaOgre::TRANSLATE)
		{
			glColor3f(1.f, 0.f, 0.f);
		}
		else if(m_tsr == FionaOgre::ROTATE)
		{
			glColor3f(0.f, 1.f, 0.f);
		}
		else if(m_tsr == FionaOgre::SCALE)
		{
			glColor3f(0.f, 0.f, 1.f);
		}
		glBegin(GL_LINES);
		//these two lines are for keyboard testing..
		//glVertex3f(0.f, 0.f, 0.f);
		//glVertex3f(vWandDir.x*fLineLength, vWandDir.y*fLineLength, vWandDir.z*fLineLength);
		//these would be for the cave..
		glVertex3f(vPos.x, vPos.y, vPos.z);
		glVertex3f(vPos.x + vWandDir.x*fLineLength, vPos.y + vWandDir.y * fLineLength, vPos.z + vWandDir.z * fLineLength);
		glEnd();
		//glTranslatef(vPos.x + vWandDir.x*fLineLength, vPos.y + vWandDir.y * fLineLength, vPos.z + vWandDir.z * fLineLength);
		//glutWireCube(1.0);
		glLineWidth(1.f);
		glColor3f(1.f, 1.f, 1.f);
		glEnable(GL_LIGHTING);
		//glEnable(GL_DEPTH_TEST);
		glPopMatrix();
	}
#ifndef LINUX_BUILD
	glDisable(GL_LIGHTING);
	if(fionaConf.leapData.hand1.valid)
	{
		drawHand(fionaConf.leapData.hand1);
	}

	if(fionaConf.leapData.hand2.valid)
	{
		drawHand(fionaConf.leapData.hand2);
	}
#endif
	glEnable(GL_LIGHTING);
}

void FionaOgre::postOpenGLRender(void)
{
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void FionaOgre::resize(int gwin, int w, int h)
{
	int i = findWin(gwin); if(i<0 ) return;
	wins[i].owin->resize(w,h);
//#ifdef WIN32
	wins[i].owin->windowMovedOrResized ();
//#endif
}

#define IS_WHITE(X) ((X)=='\n'||(X)==' '||(X)=='\t')
#define GETCH(X,Y,Z) {X>>Y; if((X).eof()) return Z;}
std::string GetConfigString(std::istream& is)
{
	is >> std::noskipws;
	std::string buf;
	char ch = 0;
EAT_UP_AND_READ_AGAIN:
	while(1){ GETCH(is,ch,buf); if(!IS_WHITE(ch)) break; }
	if(ch=='\"') // Quoted
		while(1) { GETCH(is,ch,buf); if(ch=='\"') return buf; buf+=ch; }
	else if(ch=='/') // Maybe comment?
	{
		is>>ch;
		if( ch=='/' ) // Comment
			while(1) { GETCH(is,ch,buf); if(ch=='\n') goto EAT_UP_AND_READ_AGAIN; }
		else // Single slash
		{
			buf+="/"; buf+=ch;
			while(1) { GETCH(is,ch,buf); if(IS_WHITE(ch)) return buf; buf+=ch; }
		}
	}
	else // normal word
	{
		buf+=ch;
		while(1) { GETCH(is,ch,buf); if(IS_WHITE(ch)) return buf; buf+=ch; }
	}
	return buf;
}

void FionaOgre::setupScene(Ogre::SceneManager* scene)
{
	//THIS NEEDS TO BE CALLED BEFORE BEING ABLE TO LOAD RESOURCES
	
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	
	//scene->setShadowTechnique( Ogre::SHADOWTYPE_TEXTURE_MODULATIVE );
	//scene->setShadowTextureSize( 4048 );
	//scene->setShadowColour( Ogre::ColourValue( 0.3f, 0.3f, 0.3f ) );
	/*scene->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);
	//scene->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
	scene->setShadowColour(Ogre::ColourValue(1.0, 0.0, 0.0));
	scene->setShadowTextureSize(1024);
	scene->setShadowTextureCount(1);*/
	
	defaultSetup();

	//ROSS TESTING
	if(!sceneName.empty())
	{
		///NxOgre::ResourceSystem::getSingleton()->openProtocol(new Critter::OgreResourceProtocol());
  
		DotSceneLoader loader;
#ifndef LINUX_BUILD
#if USE_BULLET

#else
		loader.setCritterRender(critterRender);
#endif
#endif
		loader.parseDotScene(fionaConf.OgreMediaBasePath+sceneName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, scene);
#ifndef LINUX_BUILD
		mGrassLoaderHandle = loader.mGrassLoaderHandle;

		for(unsigned int i = 0; i < loader.mPGHandles.size(); i++)
		{
			mPGHandles.push_back(loader.mPGHandles[i]);
			mPGHandles[i]->setCamera(camera);
		}

		for(unsigned int i = 0; i < loader.mTreeHandles.size(); ++i)
		{
			mTreeHandles.push_back(loader.mTreeHandles[i]);
		}
#endif
		scene->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
		
		//this is the net attached to the camera moving together
		/*Ogre::SceneNode* netAndCamera = scene->getRootSceneNode()->createChildSceneNode("netAndCamera1");
		netAndCamera->attachObject(camera);
		Ogre::SceneNode* net = netAndCamera->createChildSceneNode("net");
		net->attachObject(scene->createEntity("objectNet", "net.mesh"));
		//net->setScale(15, 15, 15);
		//x, y, z (left/right(+), bottom/up(+), in(-)/out(+))
		net->setPosition(0, 10, -3 );	
		

		
		//for the left side of the body
		//This is the left upper arm
		Ogre::SceneNode* leftArm = scene->getRootSceneNode()->createChildSceneNode("lArm");
		leftArm->attachObject(scene->createEntity("upperarm1", "l_upperarm.mesh"));
		leftArm->scale( 75, 75, 75);
		//Make the left forearm the child of the left upper arm
		Ogre::SceneNode* leftForearm = leftArm->createChildSceneNode("lForearm");
		leftForearm->attachObject( scene->createEntity("forearm1","l_forearm.mesh") );
		//Make the left hand the child of the left forearm
		Ogre::SceneNode* leftHand = leftForearm->createChildSceneNode("lHand");
		leftHand->attachObject( scene->createEntity("hand1","l_hand.mesh") );
				
		//for the right side of the body
		//This is the right upper arm
		Ogre::SceneNode* rightArm = scene->getRootSceneNode()->createChildSceneNode("rArm");
		rightArm->attachObject(scene->createEntity("upperarm2", "r_upperarm.mesh"));
		rightArm->scale( 75, 75, 75);
		//Make the right forearm the child of the right upper arm
		Ogre::SceneNode* rightForearm = rightArm->createChildSceneNode("rForearm");
		rightForearm->attachObject( scene->createEntity("forearm2","r_forearm.mesh"));
		//Make the right hand the child of the right forearm
		Ogre::SceneNode* rightHand = rightForearm->createChildSceneNode("rHand");
		rightHand->attachObject( scene->createEntity("hand2","r_hand.mesh"));*/
		//scene->setSkyBox(true, "Examples/StormySkyBox", 75.f);  // set a skybox - eventually get this from the dot scene
		//scene->setFog(Ogre::FOG_LINEAR, Ogre::Vector3(1.f, 1.f, 1.f), 0.01, 15.f, 35.f);

		//position the user in the center of the CAVE...!
		//camPos.set(CAVE_CENTER.x, CAVE_CENTER.y, CAVE_CENTER.z);
		mTerrainGroup = loader.mTerrainGroup;
		mTerrainGlobalOptions = loader.mTerrainGlobalOptions;
#ifndef LINUX_BUILD
		if(loader.mSkyX != 0)
		{
			mSkyx = loader.mSkyX;
			mSkyx->setLightingMode(SkyX::SkyX::LM_HDR);	//1 for hdr..
			mSkyx->update(0);
			// Add the atmospheric scattering pass to our terrain material
			// The ground atmospheric scattering pass must be added to all materials that are going to be used to 
			// render object which we want to have atmospheric scattering effects on. 
			/*mSkyx->getGPUManager()->addGroundPass(
				static_cast<Ogre::MaterialPtr>(Ogre::MaterialManager::getSingleton().
				getByName("Terrain"))->getTechnique(0)->createPass(), 5000, Ogre::SBT_TRANSPARENT_COLOUR);

			// Create the terrain
			mSceneMgr->setWorldGeometry("Terrain.cfg");*/

			// Add a basic cloud layer
			mSkyx->getCloudsManager()->add(SkyX::CloudLayer::Options(/* Default options */));
		}
		
		dynamicObjects = loader.dynamicObjects;

		//Ogre::SceneNode* smoke = scene->getRootSceneNode()->createChildSceneNode("smokeTest");
		//smoke->attachObject(scene->createParticleSystem("smokeExample", "Examples/Smoke"));
		scene->setSkyBox(true, "Examples/CloudyNoonSkyBox", 50.f);
		//scene->setFog(Ogre::FOG_LINEAR, Ogre::ColourValue(0.5f, 0.5f, 0.5f), 0.01, 0.f, 45.f);
		
		Ogre::Light* light = scene->createLight();
		light->setType(Ogre::Light::LT_DIRECTIONAL);
		light->setPosition(-10, 40, 20);
		light->setDirection(0.0, -0.66, 0.33);
		light->setSpecularColour(Ogre::ColourValue::White);

		if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED))
		{
			m_SoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();
		
			m_SoundManager->setSceneManager(scene);

			if (!m_SoundManager->init())
			{
				printf("Couldn't initialize sound manager..\n");
			}
			else
			{
				if(isRISE_)
				{
					// Create a streamed sound, no looping, no prebuffering
					bool bFoundSound = m_SoundManager->createSound("Sound1", fionaConf.OgreMediaBasePath+std::string("rise\\rise.ogg"), true, false, false);
					if(bFoundSound)
					{
						printf("Loaded RISE sound playback!\n");
					}
				}
			}
		}
#endif
	}
	else
	{
		//OGRE OpenAL Sound Example!
		/*OgreOggSound::OgreOggSoundManager *mSoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();

		if (mSoundManager->init())
		{
			// Create a streamed sound, no looping, no prebuffering
			if ( mSoundManager->createSound("Sound1", fionaConf.OgreMediaBasePath+std::string("Box\\hand.wav"), false, false, false) )
			{
				mSoundManager->getSound("Sound1")->play();
			}
		}*/
		
		Ogre::SceneNode* headNode = scene->getRootSceneNode()->createChildSceneNode();
		headNode->setScale(0.02f, 0.02f, 0.02f);
		Ogre::Entity *ogHead = scene->createEntity("Head", "ogrehead.mesh");
		//ogHead->setCastShadows(true);
		headNode->attachObject(ogHead);

		//load up default plane, etc..
		Ogre::ColourValue background = Ogre::ColourValue(fionaConf.backgroundColor.x, fionaConf.backgroundColor.y, fionaConf.backgroundColor.z);
		scene->setSkyBox(true, "Examples/CloudyNoonSkyBox", 50.f);  // set a skybox
		
		static const float floorPlaneHeight = -1.f;

		Ogre::MeshManager::getSingleton().createPlane("floor", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::Plane(Ogre::Vector3::UNIT_Y, floorPlaneHeight), 1000, 1000, 1,1 , true, 1, 1, 1, Ogre::Vector3::UNIT_Z);

		Ogre::Entity* floor = scene->createEntity("Floor", "floor");
		floor->setMaterialName("Examples/BumpyMetal");
		floor->setCastShadows(false);
		Ogre::SceneNode* planeNode = scene->getRootSceneNode()->createChildSceneNode();
		planeNode->setPosition(0.0,floorPlaneHeight, 0.0);
		planeNode->attachObject(floor);
		//scene->getRootSceneNode()->attachObject(floor);

		scene->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));

		Ogre::Light* light = scene->createLight();
		light->setType(Ogre::Light::LT_POINT);
		light->setPosition(-10, 40, 20);
		light->setSpecularColour(Ogre::ColourValue::White);

		physicsOn = true;

		//add the butterfly
		/*Ogre::SceneNode* butterfly = scene->getRootSceneNode()->createChildSceneNode("butterfly_scn");
		butterfly->attachObject(scene->createEntity("butterfly", "butterfly.mesh"));
		butterfly->setScale( 75, 75, 75);
		butterfly->setPosition(25, 45, -5);  //(left/right, up/down, in/out)*/

        // Create Hydrax object
		//mHydrax = new Hydrax::Hydrax(scene, camera, vp);

		// Create our projected grid module  
		//Hydrax::Module::ProjectedGrid *mModule 
		//	= new Hydrax::Module::ProjectedGrid(// Hydrax parent pointer
		//	                                    mHydrax,
		//										// Noise module
		//	                                    new Hydrax::Noise::Perlin(/*Generic one*/),
		//										// Base plane
		//	                                    Ogre::Plane(Ogre::Vector3(0,-1.4478,0), Ogre::Vector3(0,0,0)),
		//										// Normal mode
		//										Hydrax::MaterialManager::NM_VERTEX,
		//										// Projected grid options
		//								        Hydrax::Module::ProjectedGrid::Options(/*264 /*Generic one*/));

		// Set our module
		//mHydrax->setModule(static_cast<Hydrax::Module::Module*>(mModule));

		// Load all parameters from config file
		// Remarks: The config file must be in Hydrax resource group.
		// All parameters can be set/updated directly by code(Like previous versions),
		// but due to the high number of customizable parameters, since 0.4 version, Hydrax allows save/load config files.
		//mHydrax->loadCfg("HydraxDemo.hdx");

        // Create water
        //mHydrax->create();

		//camPos.set(CAVE_CENTER.x, CAVE_CENTER.y, CAVE_CENTER.z);
		//}
		//setup actions.. eventually these will be read in from file..

		// smartbody
		// the root path to SmartBody: change this to your own path
#ifdef SMART_BODY
		std::string smartbodyRoot = "../../Media";
		// set the following to the location of the Python libraries. 
		// if you downloaded SmartBody, it will be in core/smartbody/Python26/Lib
	#ifdef WIN32
		initPython("../../include/python27/lib");
	#else
		initPython("/usr/lib/python2.7");
	#endif
		m_pSB = SmartBody::SBScene::getScene();

		if(fionaConf.appType == FionaConfig::HEADNODE || fionaConf.appType == FionaConfig::WINDOWED || fionaConf.appType == FionaConfig::DEVLAB)
		{
			m_pSB->startFileLogging("smartbody.log");
		}

		FionaOgreSmartBodyListener* listener = new FionaOgreSmartBodyListener(this);
		m_pSB->addSceneListener(listener);
		m_pSB->start();

		// sets the media path, or root of all the data to be used
		// other paths will be relative to the media path
		m_pSB->setMediaPath(smartbodyRoot + "/data");
		m_pSB->addAssetPath("script", ".");
		//m_pSB->addAssetPath("motion", "ChrBrad");
		// the file 'OgreSmartBody.py' needs to be placed in the media path directory
		m_pSB->runScript("ogresmartbody.py");

		kinectMapping.push_back(std::string("Waist"));
		kinectMapping.push_back(std::string("Stomach"));
		kinectMapping.push_back(std::string("Chest"));
		kinectMapping.push_back(std::string("Head"));
		kinectMapping.push_back(std::string("Clavicle.L"));
		kinectMapping.push_back(std::string("Humerus.L"));
		kinectMapping.push_back(std::string("Ulna.L"));
		kinectMapping.push_back(std::string("Hand.L"));
		kinectMapping.push_back(std::string("Clavicle.R"));
		kinectMapping.push_back(std::string("Humerus.R"));
		kinectMapping.push_back(std::string("Ulna.R"));
		kinectMapping.push_back(std::string("Hand.R"));
		kinectMapping.push_back(std::string("Thigh.L"));
		kinectMapping.push_back(std::string("Calf.L"));
		kinectMapping.push_back(std::string("Foot.L"));
		kinectMapping.push_back(std::string("Toe.L"));
		kinectMapping.push_back(std::string("Thigh.R"));
		kinectMapping.push_back(std::string("Calf.R"));
		kinectMapping.push_back(std::string("Foot.R"));
		kinectMapping.push_back(std::string("Toe.R"));
		// create a character
		//LOG("Creating the character...");
		/*SmartBody::SBCharacter* character = m_pSB->createCharacter("sinbad01", "Sinbad");

		// load the skeleton from one of the available skeleton types
		SmartBody::SBSkeleton* skeleton = m_pSB->createSkeleton("ChrBrad.sk");

		// attach the skeleton to the character
		character->setSkeleton(skeleton);

		// create the standard set of controllers (idling, gesture, nodding, etc.)
		character->createStandardControllers();*/
#endif
	}
}

void FionaOgre::defaultSetup(void)
{
	//initialize default physics settings..
	//if(fionaConf.appType != FionaConfig::ZSPACE)
	{
#ifndef LINUX_BUILD

#if USE_BULLET
 		int mNumEntitiesInstanced = 0; // how many shapes are created
 		
 		// Start Bullet
		Ogre::AxisAlignedBox bounds(Ogre::Vector3 (-10000, -10000, -10000), Ogre::Vector3 (10000,  10000,  10000));
		Ogre::Vector3 gravityVector(0,-9.81,0);
 		physicsWorld = new OgreBulletDynamics::DynamicsWorld(scene, bounds, gravityVector);
 
 	        // add Debug info display tool
 		debugDrawer = new OgreBulletCollisions::DebugDrawer();
 		debugDrawer->setDrawWireframe(true);	// we want to see the Bullet containers
 
 		physicsWorld->setDebugDrawer(debugDrawer);
 		physicsWorld->setShowDebugShapes(true);		// enable it if you want to see the Bullet containers
 		Ogre::SceneNode *node = scene->getRootSceneNode()->createChildSceneNode("debugDrawer", Ogre::Vector3::ZERO);
 		node->attachObject(static_cast <Ogre::SimpleRenderable *> (debugDrawer));
 
        // Define a floor plane mesh
 		Ogre::Entity *ent;
        Ogre::Plane p;
        p.normal = Ogre::Vector3(0,1,0); p.d = 0;
        Ogre::MeshManager::getSingleton().createPlane("FloorPlane", 
                                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
                                                p, 200000, 200000, 20, 20, true, 1, 9000, 9000, 
                                                Ogre::Vector3::UNIT_Z);
        // Create an entity (the floor)
        ent = scene->createEntity("floor", "FloorPlane");
 		ent->setMaterialName("Examples/BumpyMetal");
        scene->getRootSceneNode()->createChildSceneNode()->attachObject(ent);
 
 		// add collision detection to it
 		OgreBulletCollisions::CollisionShape *Shape;
 		Shape = new OgreBulletCollisions::StaticPlaneCollisionShape(Ogre::Vector3(0,1,0), 0); // (normal vector, distance)
 		// a body is needed for the shape
 		OgreBulletDynamics::RigidBody *defaultPlaneBody = new OgreBulletDynamics::RigidBody("BasePlane", physicsWorld);
 		defaultPlaneBody->setStaticShape(Shape, 0.1, 0.8);// (shape, restitution, friction)
 		// push the created objects to the deques
 		mShapes.push_back(Shape);
 		mBodies.push_back(defaultPlaneBody);
#else
		physicsWorld = NxOgre::World::createWorld();
		physicsWorld->getRemoteDebugger()->connect();

		NxOgre::SceneDescription scene_description;
		scene_description.mGravity = NxOgre::Constants::MEAN_EARTH_GRAVITY;
		scene_description.mUseHardware = false;
  
		float floorPlaneHeight = -1.f;

		if(isRISE_)
		{
			floorPlaneHeight = -30.f;
			scene_description.mGravity = NxOgre::Constants::MEAN_EARTH_GRAVITY * 0.25f;
		}

		physicsScene = physicsWorld->createScene(scene_description);
		physicsScene->getMaterial(0)->setAll(0.1,0.9,0.5);
		NxOgre::PlaneGeometryDescription planeDesc(NxOgre::Vec3(0,1,0), floorPlaneHeight);
		physicsScene->createSceneGeometry(planeDesc);

		critterRender = new Critter::RenderSystem(physicsScene, scene);
			
		//below two lines will turn on physics debugging
		//critterRender->setVisualisationMode(NxOgre::Enums::VisualDebugger_ShowAll);
		//physicsWorld->getVisualDebugger()->enable();

		if(sceneName.empty())
		{
			//default physics scene.
			//makeBox(NxOgre::Matrix44(NxOgre::Vec3(0.0, 4.0, 0.0)), NxOgre::Vec3(0.0, 1.0, 0.0));
			makeBox(NxOgre::Matrix44(NxOgre::Vec3(2.0, 4.0, 0.0)), NxOgre::Vec3(0.0, 1.0, 0.0));
			makeBox(NxOgre::Matrix44(NxOgre::Vec3(-2.0, 4.0, 0.0)), NxOgre::Vec3(0.0, 1.0, 0.0));
			//makeCloth(NxOgre::Vec3(0,6,0));
		}
#endif
#endif
	}

	VRActionSet *pWorldMode = new VRActionSet();
	pWorldMode->SetName(std::string("world_mode"));
		
	/*VROSelect *pSelect = new VROSelect();
	//pSelect->SetButton(0);
	pSelect->SetButton(5);
	pSelect->SetOnRelease(true);
	pWorldMode->AddAction(pSelect);*/

	//VROTSR *pTrans = new VRTSR();
	//pTrans->SetNoMovement(true);
	//pTrans->SetButton(5);
	//pWorldMode->AddAction(pTrans);

	/*VROChangeTSR *pTSRChange = new VROChangeTSR();
	pTSRChange->SetButton(1);
	pTSRChange->SetOnRelease(true);
	pWorldMode->AddAction(pTSRChange);*/

	//VRExportPly *pSave = new VRExportPly();
	//pSave->SetButton(2);
	//pSave->SetOnRelease(true);
	//pWorldMode->AddAction(pSave);

	/*VRODelete *pDelete = new VRODelete();
	pDelete->SetButton(3);
	pDelete->SetOnRelease(true);
	pWorldMode->AddAction(pDelete);*/

	/*VRODuplicate *pDupe = new VRODuplicate();
	pDupe->SetButton(2);
	pDupe->SetOnRelease(true);
	pWorldMode->AddAction(pDupe);*/

	m_actions.AddSet(pWorldMode);
	m_actions.SetCurrentSet(pWorldMode);
}

#ifndef LINUX_BUILD
#if USE_BULLET

#else
Critter::Body* FionaOgre::makeBox(const NxOgre::Matrix44& globalPose, const NxOgre::Vec3 & initialVelocity)
{
	Critter::BodyDescription bodyDescription;
	bodyDescription.mMass = 40.0f;
	bodyDescription.mLinearVelocity = initialVelocity;
  
	Critter::Body* box = critterRender->createBody(NxOgre::BoxDescription(1,1,1), globalPose, "cube.1m.mesh", bodyDescription);

	return box;
}

void FionaOgre::makeCloth(const NxOgre::Vec3& barPosition)
{
	NxOgre::Vec3 pos = barPosition;
	NxOgre::Vec2 clothSize(8,4);

	pos.x -= clothSize.x * 0.5f;
  
	if (clothMesh == 0)
		clothMesh = NxOgre::MeshGenerator::makePlane(clothSize, 0.1, NxOgre::Enums::MeshType_Cloth, "file://rug.xcl");
  
	NxOgre::Vec3 holderPos = pos;
	holderPos.x += clothSize.x * 0.5f;;
	holderPos.y += 0.05f;
	holderPos.z -= 0.05f;
	NxOgre::SceneGeometry* geom = physicsScene->createSceneGeometry(NxOgre::BoxDescription(10, 0.1, 0.1), holderPos);
  
	Ogre::SceneNode* geom_node = critterRender->createSceneNodeEntityPair("cube.mesh", holderPos);
	geom_node->scale(0.1, 0.001, 0.001);
  
	NxOgre::ClothDescription desc;
	desc.mMesh = clothMesh;
	desc.mThickness = 0.2;
	desc.mFriction = 0.5;
	//desc.mFlags |= NxOgre::Enums::ClothFlags_BendingResistance;
	//desc.mFlags |= NxOgre::Enums::ClothFlags_TwoWayCollisions;
  
	desc.mGlobalPose.set(pos);
  
	cloth = critterRender->createCloth(desc, "wales");
	cloth->attachToShape((*geom->getShapes().begin()), NxOgre::Enums::ClothAttachmentFlags_Twoway);
}
#endif
#endif

bool FionaOgre::isSelected(Ogre::MovableObject *obj) const
{
	for(unsigned int i = 0; i < m_currentSelection.size(); ++i)
	{
		if(m_currentSelection[i] == obj)
		{
			return true;
		}
	}

	return false;
}

Ogre::MovableObject * FionaOgre::rayCastSelect(float & fDist, bool bClear)
{
	if(bClear)
	{
		clearSelection();
	}

	jvec3 vPos;
	getWandWorldSpace(vPos, true);
	//this correctly orients the direction of fire..
	jvec3 vWandDir;
	getWandDirWorldSpace(vWandDir, true);

	Ogre::Vector3 wandDir(vWandDir.x, vWandDir.y, vWandDir.z);

	//uncomment below line for keyboard-based testing..
	//wandDir = scene->getCamera("OgreGLUTDefaultCamera")->getDirection();

	//perform a selection in the scene...TODO - test this..
	Ogre::RaySceneQuery *selectionRay = scene->createRayQuery(Ogre::Ray(Ogre::Vector3(vPos.x, vPos.y, vPos.z), wandDir));
	Ogre::RaySceneQueryResult & results = selectionRay->execute();
		
	//todo - figure out how to sort this and just grab the closest object..
	Ogre::RaySceneQueryResult::iterator it = results.begin();
	float fClosestDist = 9999999.f;
	int index = 0;
	Ogre::RaySceneQueryResultEntry *closest=0;
	for(it; it != results.end(); it++)
	{
		//all results in this list were selected by the ray.. grab closest selection..
		if(it->distance < fClosestDist)
		{
			fClosestDist = it->distance;
			closest = &(*it);
		}
	}

	if(closest != 0)
	{
		Ogre::MovableObject *obj = closest->movable;
		if(obj != 0)
		{
			//no selecting the floor for now..
			if(strcmp(obj->getName().c_str(), "Floor")!=0)
			{
				//printf("intersected entity named: %s\n", obj->getName().c_str());
				//obj->setVisible(false);
				if(obj->getParentSceneNode())
				{
					fDist = fClosestDist;
					obj->getParentSceneNode()->showBoundingBox(true);
					scene->destroyQuery(selectionRay);
					if(!isSelected(obj))
					{
						addSelection(obj);
						return obj;
					}
					else
					{
						return 0;
					}

					// Play a sound to let the user we've selected something.
					/*if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
						mSoundManager->getSound("click")->play();
					}*/
				}
			}
		}
	} else {
		// Whatever else happened, we didn't get a selection.
		// Play a sound giving the user a hint: they should point at an object.
		// Temporarily disabled for now: this fires after deselecting an object too, which is annoying.
		// if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
		// 	mSoundManager->getSound("select_nothing_pointed_at")->play();
		// }
	}

	return 0;
}

void FionaOgre::saveScene(void)
{
	/*DotSceneLoader writer;
	std::string saveName = fionaConf.OgreMediaBasePath;
	saveName.append("levels\\");
	saveName.append(m_loggingDirectory);
	saveName.append("\\");
	
	//saveName.append(".scene");
	std::string meshDir = fionaConf.OgreMediaBasePath;
	meshDir.append("levels\\");
	meshDir.append(m_loggingDirectory);
	meshDir.append("\\");
	meshDir.append("mesh");

	if (!CreateDirectory (saveName.c_str(), NULL))
	{
		printf("couldn't create directory %s\n", saveName.c_str());
		exit(-1);
	}
	if (!CreateDirectory (meshDir.c_str(), NULL))
	{
		printf("couldn't create directory %s\n", meshDir.c_str());
		exit(-1);
	}

	//saveName.append(m_loggingDirectory.c_str());
	printf("creating directory %s\n", meshDir.c_str());
	writer.sceneExplore(scene, saveName.c_str(), meshDir.c_str(), m_loggingDirectory.c_str());*/
}

void FionaOgre::setCameraPath(const char *sFN)
{
	std::ifstream f(sFN);
	if(f.is_open())
	{
		CamInfo c;
		int i = 0;

		while(!f.eof())
		{
			std::string line;
			getline(f, line);
			Ogre::StringVector camVals = Ogre::StringUtil::split(line.c_str(), ",");
			
			if(camVals.size() == 3)
			{
				c.pos[0] = atof(camVals[0].c_str());
				c.pos[1] = atof(camVals[1].c_str());
				c.pos[2] = atof(camVals[2].c_str());
			}
			else if(camVals.size() == 4)
			{
				c.rot[0] = atof(camVals[0].c_str());
				c.rot[1] = atof(camVals[1].c_str());
				c.rot[2] = atof(camVals[2].c_str());
				c.rot[3] = atof(camVals[3].c_str());
			}

			i++;
			
			if(i % 2 == 0)
			{
				camPlayback_.push_back(c);
			}
			
		}

		f.close();
	}
}

void FionaOgre::keyboard(unsigned int key, int x, int y)
{
	//printf("%d", key);
	/*int taskIndex=0;	// be global variable.
	switch (taskIndex)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	}
	if (key == 'a' || key=='A')
	{
		initialPos = Ogre::Vector3(-5,5,-15);
		finalPos = Ogre::Vector3(10,5,-15);
		Ogre::SceneNode* butterfly_scn = scene->getSceneNode("butterfly_scn");
		butterfly_scn->setPosition( initialPos );
		isNetOver = false;
		totalTime = 15; //time in seconds
		timeSinceNetOver = 0; 
		printf("a\n");
	}
	else if (key == 'b' || key=='B')
	{
		printf("b\n");
		isNetOver = true; 
	}*/

	if(key == 'a' || key == 'A')
	{
		if(simTestOn)
		{
			simTestOn = false;
			m_pSB->getSimulationManager()->stop();
		}
		else
		{
			if(m_pSB)
			{
				simTestOn = true;
				m_pSB->getSimulationManager()->start();
				m_pSB->getBmlProcessor()->execBML("sinbad0", "<body posture=\"ChrUtah_Jog001\"/>");
				m_pSB->getSimulationManager()->resume();
			}
		}
	}

	if(key == 'e' || key == 'E')
	{
		embody = !embody;
	}

	if(key == 'b' || key == 'B')
	{
		physicsOn = !physicsOn;
	}

	if(key == 'p' || key == 'P')
	{
		playingBack_ = !playingBack_;
		lastCamTime_ = FionaUTTime();
		if(playingBack_)
		{
			printf("Playing back pre-made camera movement.\n");
			if(isRISE_)
			{
				if(m_SoundManager)
				{
					if(m_SoundManager->getSound("Sound1") != 0)
					{
						m_SoundManager->getSound("Sound1")->play();
					}
				}
			}
		}
		else
		{
			physicsOn = false;
			lastIndex_=0;
			printf("Stopping playback.\n");
			if(isRISE_)
			{
				if(m_SoundManager)
				{
					if(m_SoundManager->getSound("Sound1") != 0)
					{
						m_SoundManager->getSound("Sound1")->stop();
					}
				}
			}
		}
	}

	if(key == 'r' || key == 'R')
	{
		recording_ = !recording_;
		if(!recording_)
		{
			if(recordFile_)
			{
				fclose(recordFile_);
				recordFile_ = 0;
			}
		}
	}
}

FionaOgreSmartBodyListener::FionaOgreSmartBodyListener(FionaOgre* osb) : SBSceneListener()
{
	ogreSB = osb;
}

FionaOgreSmartBodyListener::~FionaOgreSmartBodyListener()
{

}

void FionaOgreSmartBodyListener::OnCharacterCreate( const std::string & name, const std::string & objectClass )
{
	if (ogreSB->getScene()->hasEntity(name))
	{
		std::cout << "An entity named '" << name << "' already exists, ignoring..." << std::endl;
		return;
	}

	Ogre::Entity* entity = ogreSB->getScene()->createEntity(name, objectClass + ".mesh");
	Ogre::SceneNode* node = ogreSB->getScene()->getRootSceneNode()->createChildSceneNode();
	node->attachObject(entity);

	Ogre::Skeleton* meshSkel = entity->getSkeleton();
	Ogre::Skeleton::BoneIterator it = meshSkel->getBoneIterator(); 
	while ( it.hasMoreElements() ) 
	{ 
		Ogre::Bone* bone = it.getNext();
		bone->setManuallyControlled(true);
	}
}

void FionaOgreSmartBodyListener::OnCharacterDelete( const std::string & name )
{
	if (!ogreSB->getScene()->hasEntity(name))
	{
		std::cout << "An entity named '" << name << "' does not exist, ignoring delete..." << std::endl;
		return;
	}

	Ogre::SceneNode * node = (Ogre::SceneNode *)ogreSB->getScene()->getRootSceneNode()->getChild(name);
	node->detachAllObjects();
	ogreSB->getScene()->destroyEntity(name);
	ogreSB->getScene()->getRootSceneNode()->removeAndDestroyChild(name);
}

void FionaOgreSmartBodyListener::OnCharacterChanged( const std::string& name )
{
	if (!ogreSB->getScene()->hasEntity(name))
	{
		std::cout << "An entity named '" << name << "' does not exist, ignoring update..." << std::endl;
		return;
	}

	Ogre::Entity* entity = ogreSB->getScene()->getEntity(name);
	Ogre::Skeleton* meshSkel = entity->getSkeleton();
	Ogre::Skeleton::BoneIterator it = meshSkel->getBoneIterator(); 
	while ( it.hasMoreElements() ) 
	{ 
		Ogre::Bone* bone = it.getNext();
		bone->setManuallyControlled(true);
	}
}

void FionaOgreSmartBodyListener::OnLogMessage( const std::string & message )
{
#ifdef WIN32
	//LOG(message.c_str());
#endif
}
