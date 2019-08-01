#include "OgreDotScene.h"
#include <OGRE/Ogre.h>
#include <OGRE/Terrain/OgreTerrain.h>
#include <OGRE/Terrain/OgreTerrainGroup.h>
#include <OGRE/Terrain/OgreTerrainMaterialGeneratorA.h>

#ifndef LINUX_BUILD
#include "OGRE/PagedGeometry/PagedGeometry.h"
#include "OGRE/PagedGeometry/GrassLoader.h" 
#include "OGRE/PagedGeometry/BatchPage.h"
#include "OGRE/PagedGeometry/ImpostorPage.h"
#include "OGRE/PagedGeometry/TreeLoader3D.h"
#endif

#define USE_BULLET 0

#pragma warning(disable:4390)
#pragma warning(disable:4305)

#include "rapidxml-1.13/rapidxml_print.hpp"

using namespace Forests;


Ogre::TerrainGroup *StaticGroupPtr = 0;

Ogre::Real OgitorTerrainGroupHeightFunction(Ogre::Real x, Ogre::Real z, void *userData)
{
    return StaticGroupPtr->getHeightAtWorldPosition(x,0,z);
}


DotSceneLoader::DotSceneLoader() : mSceneMgr(0), mTerrainGroup(0)
#ifndef LINUX_BUILD
, critterRender(0), mSkyX(0)
#endif
{
    mTerrainGlobalOptions = OGRE_NEW Ogre::TerrainGlobalOptions();
	mPGPageSize=0;
    mPGDetailDistance=0;
	mGrassLoaderHandle=0;
}
 
DotSceneLoader::~DotSceneLoader()
{

}

void ParseStringVector(Ogre::String &str, Ogre::StringVector &list)
{
    list.clear();
    Ogre::StringUtil::trim(str,true,true);
    if(str == "") 
        return;

    int pos = str.find(";");
    while(pos != -1)
    {
        list.push_back(str.substr(0,pos));
        str.erase(0,pos + 1);
        pos = str.find(";");
    }
  
    if(str != "") 
        list.push_back(str);
}


void DotSceneLoader::parseDotScene(const Ogre::String &SceneName, const Ogre::String &groupName, Ogre::SceneManager *yourSceneMgr, Ogre::SceneNode *pAttachNode, const Ogre::String &sPrependNode)
{
    // set up shared object values
    m_sGroupName = groupName;
    mSceneMgr = yourSceneMgr;
    m_sPrependNode = sPrependNode;
    staticObjects.clear();
    dynamicObjects.clear();
 
    rapidxml::xml_document<> XMLDoc;    // character type defaults to char
 
    rapidxml::xml_node<>* XMLRoot;
	Ogre::DataStreamPtr stream;
	
	try {
		stream = Ogre::ResourceGroupManager::getSingleton().openResource(SceneName, groupName );
	} catch (Ogre::FileNotFoundException e)
	{
		printf("Scene not found: %s\n", SceneName.c_str());
		return;
	}

    char* scene = strdup(stream->getAsString().c_str());
    XMLDoc.parse<0>(scene);
 
    // Grab the scene node
    XMLRoot = XMLDoc.first_node("scene");
 
    // Validate the File
    if( getAttrib(XMLRoot, "formatVersion", "") == "")
    {
        Ogre::LogManager::getSingleton().logMessage( "[DotSceneLoader] Error: Invalid .scene File. Missing <scene>" );
        delete scene;
        return;
    }
 
    // figure out where to attach any nodes we create
    mAttachNode = pAttachNode;
    if(!mAttachNode)
        mAttachNode = mSceneMgr->getRootSceneNode();
 
    // Process the scene
    processScene(XMLRoot);

   // delete scene;
}
 
void DotSceneLoader::processScene(rapidxml::xml_node<>* XMLRoot)
{
    // Process the scene parameters
    Ogre::String version = getAttrib(XMLRoot, "formatVersion", "unknown");
 
    Ogre::String message = "[DotSceneLoader] Parsing dotScene file with version " + version;
    if(XMLRoot->first_attribute("ID"))
        message += ", id " + Ogre::String(XMLRoot->first_attribute("ID")->value());
    if(XMLRoot->first_attribute("sceneManager"))
        message += ", scene manager " + Ogre::String(XMLRoot->first_attribute("sceneManager")->value());
    if(XMLRoot->first_attribute("minOgreVersion"))
        message += ", min. Ogre version " + Ogre::String(XMLRoot->first_attribute("minOgreVersion")->value());
    if(XMLRoot->first_attribute("author"))
        message += ", author " + Ogre::String(XMLRoot->first_attribute("author")->value());
 
    Ogre::LogManager::getSingleton().logMessage(message);
 
    rapidxml::xml_node<>* pElement;
 
	pElement = XMLRoot->first_node("resourceLocations");
	if(pElement)
		processResourceLocations(pElement);

    // Process environment (?)
    pElement = XMLRoot->first_node("environment");
    if(pElement)
        processEnvironment(pElement);

 	pElement = XMLRoot->first_node("skyx");
	if(pElement)
		processSkyX(pElement);

	pElement = XMLRoot->first_node("wordCake");
	if(pElement)
	{
		if(pElement->first_attribute("dir"))
		{
			m_sWordCake = pElement->first_attribute("dir")->value();
		}
		if(pElement->first_attribute("width"))
		{
			m_sWCWidth = pElement->first_attribute("width")->value();
		}
		if(pElement->first_attribute("height"))
		{
			m_sWCHeight = pElement->first_attribute("height")->value();
		}
		if(pElement->first_attribute("numFiles"))
		{
			m_sWCNumFiles = pElement->first_attribute("numFiles")->value();
		}
	}
		

    // Process nodes (?)
    pElement = XMLRoot->first_node("nodes");
    if(pElement)
        processNodes(pElement);
 
    // Process externals (?)
    pElement = XMLRoot->first_node("externals");
    if(pElement)
        processExternals(pElement);
 
    // Process userDataReference (?)
    pElement = XMLRoot->first_node("userDataReference");
    if(pElement)
        processUserDataReference(pElement);
 
    // Process octree (?)
    pElement = XMLRoot->first_node("octree");
    if(pElement)
        processOctree(pElement);
 
    // Process light (?)
    pElement = XMLRoot->first_node("light");
    if(pElement)
        processLight(pElement);
 
    // Process camera (?)
    pElement = XMLRoot->first_node("camera");
    if(pElement)
        processCamera(pElement);
 
    // Process terrain (?)
    pElement = XMLRoot->first_node("terrain");
    if(pElement)
        processTerrain(pElement);
}
 
void DotSceneLoader::processNodes(rapidxml::xml_node<>* XMLNode)
{
    rapidxml::xml_node<>* pElement;
 
    // Process node (*)
    pElement = XMLNode->first_node("node");
    while(pElement)
    {
        processNode(pElement);
        pElement = pElement->next_sibling("node");
    }
 
    // Process position (?)
    pElement = XMLNode->first_node("position");
    if(pElement)
    {
        mAttachNode->setPosition(parseVector3(pElement));
        mAttachNode->setInitialState();
    }
 
    // Process rotation (?)
    pElement = XMLNode->first_node("rotation");
    if(pElement)
    {
        mAttachNode->setOrientation(parseQuaternion(pElement));
        mAttachNode->setInitialState();
    }
 
    // Process scale (?)
    pElement = XMLNode->first_node("scale");
    if(pElement)
    {
        mAttachNode->setScale(parseVector3(pElement));
        mAttachNode->setInitialState();
    }
}
 
void DotSceneLoader::processExternals(rapidxml::xml_node<>* XMLNode)
{
    //! @todo Implement this
}
 
void DotSceneLoader::processResourceLocations(rapidxml::xml_node<>* XMLNode)
{
	rapidxml::xml_node<>* pResourceElement = XMLNode->first_node("resourceLocation");
    while(pResourceElement)
    {
        Ogre::String sType = getAttrib(pResourceElement, "type");
		Ogre::String sName = getAttrib(pResourceElement, "name");
		Ogre::ResourceGroupManager& rman=Ogre::ResourceGroupManager::getSingleton();
		Ogre::String toDir("..\\ext\\Media\\levels\\");
		toDir = toDir + sName;
		rman.addResourceLocation(toDir, sType);
        pResourceElement = pResourceElement->next_sibling("resourceLocation");
    }

	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void DotSceneLoader::processEnvironment(rapidxml::xml_node<>* XMLNode)
{
    rapidxml::xml_node<>* pElement;
 
    // Process camera (?)
    pElement = XMLNode->first_node("camera");
    if(pElement)
        processCamera(pElement);
 
    // Process fog (?)
    pElement = XMLNode->first_node("fog");
    if(pElement)
        processFog(pElement);
 
    // Process skyBox (?)
    pElement = XMLNode->first_node("skyBox");
    if(pElement)
        processSkyBox(pElement);
 
    // Process skyDome (?)
    pElement = XMLNode->first_node("skyDome");
    if(pElement)
        processSkyDome(pElement);
 
    // Process skyPlane (?)
    pElement = XMLNode->first_node("skyPlane");
    if(pElement)
        processSkyPlane(pElement);
 
    // Process clipping (?)
    pElement = XMLNode->first_node("clipping");
    if(pElement)
        processClipping(pElement);
 
    // Process colourAmbient (?)
    pElement = XMLNode->first_node("colourAmbient");
    if(pElement)
        mSceneMgr->setAmbientLight(parseColour(pElement));
 
    // Process colourBackground (?)
    //! @todo Set the background colour of all viewports (RenderWindow has to be provided then)
    pElement = XMLNode->first_node("colourBackground");
    if(pElement)
        ;//mSceneMgr->set(parseColour(pElement));
 
    // Process userDataReference (?)
    pElement = XMLNode->first_node("userDataReference");
    if(pElement)
        processUserDataReference(pElement);
}
 
void DotSceneLoader::processSkyX(rapidxml::xml_node<>* XMLNode)
{
#ifndef LINUX_BUILD
	rapidxml::xml_node<>* pElement;

	// Create SkyX
    SkyX::BasicController * mBasicController = new SkyX::BasicController(true);
	mSkyX = new SkyX::SkyX(mSceneMgr, mBasicController);
	
    SkyX::AtmosphereManager::Options opt = mSkyX->getAtmosphereManager()->getOptions();    
    opt.RayleighMultiplier  = getAttribReal(XMLNode, "rayleighMultiplier", 0);
    opt.MieMultiplier       = getAttribReal(XMLNode, "mieMultiplier", -1);
    opt.NumberOfSamples     = getAttribReal(XMLNode, "sampleCount", 0);
    opt.HeightPosition      = getAttribReal(XMLNode, "height", 10);
    opt.Exposure            = getAttribReal(XMLNode, "exposure", 0);
    opt.InnerRadius         = getAttribReal(XMLNode, "innerRadius", 5000);
    opt.OuterRadius         = getAttribReal(XMLNode, "outerRadius", 1000);
    opt.SunIntensity        = getAttribReal(XMLNode, "sunIntensity", 10);
    opt.G                   = getAttribReal(XMLNode, "G", 10);
	
	pElement = XMLNode->first_node("time");
	if(pElement)
	{
		Ogre::Vector3 vTime;
		Ogre::Real tm = getAttribReal(pElement, "multiplier");
		vTime.x = getAttribReal(pElement, "current");
		vTime.y = getAttribReal(pElement, "sunRise");
		vTime.z = getAttribReal(pElement, "sunSet");
		mBasicController->setTime(vTime);
		mSkyX->setTimeMultiplier(tm);
	}

	pElement = XMLNode->first_node("eastPosition");
	if(pElement)
	{
		Ogre::Real eastX = getAttribReal(pElement, "X");
		Ogre::Real eastY = getAttribReal(pElement, "Y");
		mBasicController->setEastDirection(Ogre::Vector2(eastX, eastY));
	}

	pElement = XMLNode->first_node("waveLength");
	if(pElement)
	{
		Ogre::Real wR = getAttribReal(pElement, "R");
		Ogre::Real wG = getAttribReal(pElement, "G");
		Ogre::Real wB = getAttribReal(pElement, "B");
		Ogre::Vector3 colour(wR, wG, wB);
		opt.WaveLength = colour;
	}

	mSkyX->getAtmosphereManager()->setOptions(opt);

	pElement = XMLNode->first_node("moon");
	if(pElement)
	{
		Ogre::Real moonPhase = getAttribReal(pElement, "phase");
		Ogre::Real moonSize = getAttribReal(pElement, "size");
		Ogre::Real haloIntensity = getAttribReal(pElement, "haloIntensity");
		Ogre::Real haloStrength = getAttribReal(pElement, "haloStrength");
		mBasicController->setMoonPhase(moonPhase);
		if(!mSkyX->getMoonManager()->isCreated())
		{
			mSkyX->getMoonManager()->create();
			mSkyX->getMoonManager()->setMoonHaloIntensity(haloIntensity);
			mSkyX->getMoonManager()->setMoonHaloStrength(haloStrength);
			mSkyX->getMoonManager()->setMoonSize(moonSize);
		}
	}

	pElement = XMLNode->first_node("vClouds");
	if(pElement)
	{
		if(getAttribBool(pElement, "enable", true))
		{
			if (!mSkyX->getVCloudsManager()->isCreated())
			{
				mSkyX->getVCloudsManager()->create(mSkyX->getMeshManager()->getSkydomeRadius(mSceneMgr->getCamera("OgreGLUTDefaultCamera")));
				mSkyX->getVCloudsManager()->getVClouds()->setDistanceFallingParams(Ogre::Vector2(2, -1));
				mSkyX->getVCloudsManager()->getVClouds()->registerCamera(mSceneMgr->getCamera("OgreGLUTDefaultCamera"));
			}

			mSkyX->getVCloudsManager()->setAutoupdate(getAttribBool(pElement, "autoUpdate", true));
			mSkyX->getVCloudsManager()->getVClouds()->setWindSpeed(getAttribReal(pElement, "windSpeed"));
			mSkyX->getVCloudsManager()->getVClouds()->setWindDirection(Ogre::Radian(Ogre::Degree(getAttribReal(pElement, "windDirection"))));
			mSkyX->getVCloudsManager()->getVClouds()->setCloudFieldScale(getAttribReal(pElement, "cloudScale"));
			mSkyX->getVCloudsManager()->getVClouds()->setNoiseScale(getAttribReal(pElement, "noiseScale"));

			rapidxml::xml_node<>* pElementChild;
			pElementChild = pElement->first_node("ambientColor");
			if(pElementChild)
			{
				Ogre::Real aR = getAttribReal(pElementChild, "R");
				Ogre::Real aG = getAttribReal(pElementChild, "G");
				Ogre::Real aB = getAttribReal(pElementChild, "B");
				mSkyX->getVCloudsManager()->getVClouds()->setAmbientColor(Ogre::Vector3(aR, aG, aB));
			}
			pElementChild = pElement->first_node("lightReponse");
			if(pElementChild)
			{
				Ogre::Real lX = getAttribReal(pElementChild, "X");
				Ogre::Real lY = getAttribReal(pElementChild, "Y");
				Ogre::Real lZ = getAttribReal(pElementChild, "Z");
				Ogre::Real lW = getAttribReal(pElementChild, "W");
				mSkyX->getVCloudsManager()->getVClouds()->setLightResponse(Ogre::Vector4(lX, lY, lZ, lW));
			}
			pElementChild = pElement->first_node("lightReponse");
			if(pElementChild)
			{
				Ogre::Real lX = getAttribReal(pElementChild, "X");
				Ogre::Real lY = getAttribReal(pElementChild, "Y");
				Ogre::Real lZ = getAttribReal(pElementChild, "Z");
				Ogre::Real lW = getAttribReal(pElementChild, "W");
				mSkyX->getVCloudsManager()->getVClouds()->setAmbientFactors(Ogre::Vector4(lX, lY, lZ, lW));
			}
			pElementChild = pElement->first_node("weather");
			if(pElementChild)
			{
				Ogre::Real lX = getAttribReal(pElementChild, "X");
				Ogre::Real lY = getAttribReal(pElementChild, "Y");
				mSkyX->getVCloudsManager()->getVClouds()->setWheater(lX, lY, false);
			}
			pElementChild = pElement->first_node("lightning");
			if(pElementChild)
			{
				if(getAttribBool(pElementChild, "enable", false))
				{
					if(!mSkyX->getVCloudsManager()->getVClouds()->getLightningManager()->isCreated())
					{
						mSkyX->getVCloudsManager()->getVClouds()->getLightningManager()->create();
						mSkyX->getVCloudsManager()->getVClouds()->getLightningManager()->setEnabled(true);
					}
					mSkyX->getVCloudsManager()->getVClouds()->getLightningManager()->setAverageLightningApparitionTime(getAttribReal(pElementChild, "lightingAT"));
					mSkyX->getVCloudsManager()->getVClouds()->getLightningManager()->setLightningTimeMultiplier(getAttribReal(pElementChild, "lightingTM"));
					rapidxml::xml_node<>* pElementSubChild;
					pElementSubChild = pElementChild->first_node("lightningColor");
					if(pElementSubChild)
					{
						Ogre::Real lX = getAttribReal(pElementSubChild, "R");
						Ogre::Real lY = getAttribReal(pElementSubChild, "G");
						Ogre::Real lZ = getAttribReal(pElementSubChild, "B");
						mSkyX->getVCloudsManager()->getVClouds()->getLightningManager()->setLightningColor(Ogre::Vector3(lX, lY, lZ));
					}
				}
			}
		}
	}

	//mSkyX->setStarfieldEnabled(false);
	mSkyX->create();
#endif
}

void DotSceneLoader::processTerrain(rapidxml::xml_node<>* XMLNode)
{
    Ogre::Real worldSize = getAttribReal(XMLNode, "worldSize");
    int mapSize = Ogre::StringConverter::parseInt(Ogre::String(XMLNode->first_attribute("mapSize")->value()));
    bool colourmapEnabled = getAttribBool(XMLNode, "colourmapEnabled");
    int colourMapTextureSize = Ogre::StringConverter::parseInt(Ogre::String(XMLNode->first_attribute("colourMapTextureSize")->value()));
    int compositeMapDistance = Ogre::StringConverter::parseInt(Ogre::String(XMLNode->first_attribute("tuningCompositeMapDistance")->value()));
    int maxPixelError = Ogre::StringConverter::parseInt(Ogre::String(XMLNode->first_attribute("tuningMaxPixelError")->value()));
 
    Ogre::Vector3 lightdir(0, -0.3, 0.75);
    lightdir.normalise();
    Ogre::Light* l = mSceneMgr->createLight("tstLight");
    l->setType(Ogre::Light::LT_DIRECTIONAL);
    l->setDirection(lightdir);
    l->setDiffuseColour(Ogre::ColourValue(1.0, 1.0, 1.0));
    l->setSpecularColour(Ogre::ColourValue(0.4, 0.4, 0.4));
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.6, 0.6, 0.6));
 
    mTerrainGlobalOptions->setMaxPixelError((Ogre::Real)maxPixelError);
    mTerrainGlobalOptions->setCompositeMapDistance((Ogre::Real)compositeMapDistance);
    mTerrainGlobalOptions->setLightMapDirection(lightdir);
    mTerrainGlobalOptions->setCompositeMapAmbient(mSceneMgr->getAmbientLight());
    mTerrainGlobalOptions->setCompositeMapDiffuse(l->getDiffuseColour());
 
    mTerrainGroup = OGRE_NEW Ogre::TerrainGroup(mSceneMgr, Ogre::Terrain::ALIGN_X_Z, mapSize, worldSize);
    mTerrainGroup->setOrigin(Ogre::Vector3::ZERO);
 
    mTerrainGroup->setResourceGroup("General");
 
    rapidxml::xml_node<>* pElement;
    rapidxml::xml_node<>* pPageElement;
 
    // Process terrain pages (*)
    pElement = XMLNode->first_node("terrainPages");
    if(pElement)
    {
        pPageElement = pElement->first_node("terrainPage");
        while(pPageElement)
        {
            processTerrainPage(pPageElement);
            pPageElement = pPageElement->next_sibling("terrainPage");
        }
    }
    mTerrainGroup->loadAllTerrains(true);
 
    mTerrainGroup->freeTemporaryResources();
    //mTerrain->setPosition(mTerrainPosition);
}
 
void DotSceneLoader::processTerrainPage(rapidxml::xml_node<>* XMLNode)
{
    Ogre::String name = getAttrib(XMLNode, "name");
    int pageX = Ogre::StringConverter::parseInt(Ogre::String(XMLNode->first_attribute("pageX")->value()));
    int pageY = Ogre::StringConverter::parseInt(Ogre::String(XMLNode->first_attribute("pageY")->value()));
 
    mPGPageSize       = Ogre::StringConverter::parseInt(XMLNode->first_attribute("pagedGeometryPageSize")->value());
    mPGDetailDistance = Ogre::StringConverter::parseInt(XMLNode->first_attribute("pagedGeometryDetailDistance")->value());
    // error checking
    if(mPGPageSize < 10){
        Ogre::LogManager::getSingleton().logMessage("[DotSceneLoader] pagedGeometryPageSize value error!", Ogre::LML_CRITICAL);
        mPGPageSize = 10;
    }
    if(mPGDetailDistance < 100){
        Ogre::LogManager::getSingleton().logMessage("[DotSceneLoader] pagedGeometryDetailDistance value error!", Ogre::LML_CRITICAL);
        mPGDetailDistance = 100;
    }

    if (Ogre::ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), name))
    {
        mTerrainGroup->defineTerrain(pageX, pageY, name);
    }
    
    // grass layers
    rapidxml::xml_node<>* pElement = XMLNode->first_node("grassLayers");

    if(pElement)
    {
        processGrassLayers(pElement);
    }
}

void DotSceneLoader::processGrassLayers(rapidxml::xml_node<>* XMLNode)
{
#ifndef LINUX_BUILD
    Ogre::String dMapName = getAttrib(XMLNode, "densityMap");
    mTerrainGlobalOptions->setVisibilityFlags(Ogre::StringConverter::parseUnsignedInt(XMLNode->first_attribute("visibilityFlags")->value()));

    // create a temporary camera
    Ogre::Camera* tempCam = mSceneMgr->createCamera("ThIsNamEShoUlDnOtExisT");

    // create paged geometry what the grass will use
    Forests::PagedGeometry * mPGHandle = new PagedGeometry(tempCam, mPGPageSize);
    mPGHandle->addDetailLevel<GrassPage>(mPGDetailDistance);

    //Create a GrassLoader object
    mGrassLoaderHandle = new GrassLoader(mPGHandle);
    mGrassLoaderHandle->setVisibilityFlags(mTerrainGlobalOptions->getVisibilityFlags());

    //Assign the "grassLoader" to be used to load geometry for the PagedGrass instance
    mPGHandle->setPageLoader(mGrassLoaderHandle);    
    
    // set the terrain group pointer
    StaticGroupPtr = mTerrainGroup;
    
    //Supply a height function to GrassLoader so it can calculate grass Y values
    mGrassLoaderHandle->setHeightFunction(OgitorTerrainGroupHeightFunction);
    
    // push the page geometry handle into the PGHandles array
    mPGHandles.push_back(mPGHandle);

    // create the layers and load the options for them
    rapidxml::xml_node<>* pElement = XMLNode->first_node("grassLayer");
    rapidxml::xml_node<>* pSubElement;
    Forests::GrassLayer* gLayer;
    Ogre::String tempStr;
    while(pElement)
    {
        // grassLayer
        gLayer = mGrassLoaderHandle->addLayer(pElement->first_attribute("material")->value());
        gLayer->setId(Ogre::StringConverter::parseInt(pElement->first_attribute("id")->value()));
        gLayer->setEnabled(Ogre::StringConverter::parseBool(pElement->first_attribute("enabled")->value()));
        gLayer->setMaxSlope(Ogre::StringConverter::parseReal(pElement->first_attribute("maxSlope")->value()));
        gLayer->setLightingEnabled(Ogre::StringConverter::parseBool(pElement->first_attribute("lighting")->value()));

        // densityMapProps
        pSubElement = pElement->first_node("densityMapProps");
        tempStr = pSubElement->first_attribute("channel")->value();
        MapChannel mapCh;
        if(!tempStr.compare("ALPHA")) mapCh = CHANNEL_ALPHA; else
        if(!tempStr.compare("BLUE"))  mapCh = CHANNEL_BLUE;  else
        if(!tempStr.compare("COLOR")) mapCh = CHANNEL_COLOR; else
        if(!tempStr.compare("GREEN")) mapCh = CHANNEL_GREEN; else
        if(!tempStr.compare("RED"))   mapCh = CHANNEL_RED;
        
        gLayer->setDensityMap(dMapName, mapCh);
        gLayer->setDensity(Ogre::StringConverter::parseReal(pSubElement->first_attribute("density")->value()));

        // mapBounds
        pSubElement = pElement->first_node("mapBounds");
        gLayer->setMapBounds( TBounds( 
                        Ogre::StringConverter::parseReal(pSubElement->first_attribute("left")->value()),  // left
                        Ogre::StringConverter::parseReal(pSubElement->first_attribute("top")->value()),   // top
                        Ogre::StringConverter::parseReal(pSubElement->first_attribute("right")->value()), // right
                        Ogre::StringConverter::parseReal(pSubElement->first_attribute("bottom")->value()) // bottom
                                )
                            );

        // grassSizes
        pSubElement = pElement->first_node("grassSizes");
        gLayer->setMinimumSize( Ogre::StringConverter::parseReal(pSubElement->first_attribute("minWidth")->value()),   // width
                                Ogre::StringConverter::parseReal(pSubElement->first_attribute("minHeight")->value()) );// height
        gLayer->setMaximumSize( Ogre::StringConverter::parseReal(pSubElement->first_attribute("maxWidth")->value()),   // width
                                Ogre::StringConverter::parseReal(pSubElement->first_attribute("maxHeight")->value()) );// height

        // techniques
        pSubElement = pElement->first_node("techniques");
        tempStr = pSubElement->first_attribute("renderTechnique")->value();
        GrassTechnique rendTech;
        
        if(!tempStr.compare("QUAD"))       rendTech = GRASSTECH_QUAD;       else
        if(!tempStr.compare("CROSSQUADS")) rendTech = GRASSTECH_CROSSQUADS; else
        if(!tempStr.compare("SPRITE"))     rendTech = GRASSTECH_SPRITE;
        gLayer->setRenderTechnique( rendTech,
                                    Ogre::StringConverter::parseBool(pSubElement->first_attribute("blend")->value()) );

        tempStr = pSubElement->first_attribute("fadeTechnique")->value();
        FadeTechnique fadeTech;
        if(!tempStr.compare("ALPHA")) fadeTech = FADETECH_ALPHA; else
        if(!tempStr.compare("GROW"))  fadeTech = FADETECH_GROW; else
        if(!tempStr.compare("ALPHAGROW")) fadeTech = FADETECH_ALPHAGROW;
        gLayer->setFadeTechnique(fadeTech);
        
        // animation
        pSubElement = pElement->first_node("animation");
        gLayer->setAnimationEnabled(Ogre::StringConverter::parseBool(pSubElement->first_attribute("animate")->value()));
        gLayer->setSwayLength(Ogre::StringConverter::parseReal(pSubElement->first_attribute("swayLength")->value()));
        gLayer->setSwaySpeed(Ogre::StringConverter::parseReal(pSubElement->first_attribute("swaySpeed")->value()));
        gLayer->setSwayDistribution(Ogre::StringConverter::parseReal(pSubElement->first_attribute("swayDistribution")->value()));

        // next layer
        pElement = pElement->next_sibling("grassLayer");
    }	

    mSceneMgr->destroyCamera(tempCam);
#endif
}
 
void DotSceneLoader::processUserDataReference(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    //! @todo Implement this
}
 
void DotSceneLoader::processOctree(rapidxml::xml_node<>* XMLNode)
{
    //! @todo Implement this
}
 
void DotSceneLoader::processLight(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    // Process attributes
    Ogre::String name = getAttrib(XMLNode, "name");
    Ogre::String id = getAttrib(XMLNode, "id");
 
    // Create the light
    Ogre::Light *pLight = mSceneMgr->createLight(name);
    if(pParent)
        pParent->attachObject(pLight);
 
    Ogre::String sValue = getAttrib(XMLNode, "type");
    if(sValue == "point")
        pLight->setType(Ogre::Light::LT_POINT);
    else if(sValue == "directional")
        pLight->setType(Ogre::Light::LT_DIRECTIONAL);
    else if(sValue == "spot")
        pLight->setType(Ogre::Light::LT_SPOTLIGHT);
    else if(sValue == "radPoint")
        pLight->setType(Ogre::Light::LT_POINT);
 
    pLight->setVisible(getAttribBool(XMLNode, "visible", true));
    pLight->setCastShadows(getAttribBool(XMLNode, "castShadows", true));
 
    rapidxml::xml_node<>* pElement;
 
    // Process position (?)
    pElement = XMLNode->first_node("position");
    if(pElement)
        pLight->setPosition(parseVector3(pElement));
 
    // Process normal (?)
    pElement = XMLNode->first_node("normal");
    if(pElement)
        pLight->setDirection(parseVector3(pElement));
 
    pElement = XMLNode->first_node("directionVector");
    if(pElement)
    {
        pLight->setDirection(parseVector3(pElement));
        mLightDirection = parseVector3(pElement);
    }
 
    // Process colourDiffuse (?)
    pElement = XMLNode->first_node("colourDiffuse");
    if(pElement)
        pLight->setDiffuseColour(parseColour(pElement));
 
    // Process colourSpecular (?)
    pElement = XMLNode->first_node("colourSpecular");
    if(pElement)
        pLight->setSpecularColour(parseColour(pElement));
 
    if(sValue != "directional")
    {
        // Process lightRange (?)
        pElement = XMLNode->first_node("lightRange");
        if(pElement)
            processLightRange(pElement, pLight);
 
        // Process lightAttenuation (?)
        pElement = XMLNode->first_node("lightAttenuation");
        if(pElement)
            processLightAttenuation(pElement, pLight);
    }
    // Process userDataReference (?)
    pElement = XMLNode->first_node("userDataReference");
    if(pElement)
        ;//processUserDataReference(pElement, pLight);
}
 
void DotSceneLoader::processCamera(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    // Process attributes
    Ogre::String name = getAttrib(XMLNode, "name");
    Ogre::String id = getAttrib(XMLNode, "id");
    Ogre::Real fov = getAttribReal(XMLNode, "fov", 45);
    Ogre::Real aspectRatio = getAttribReal(XMLNode, "aspectRatio", 1.3333);
    Ogre::String projectionType = getAttrib(XMLNode, "projectionType", "perspective");
 
    // Create the camera
    Ogre::Camera *pCamera = mSceneMgr->createCamera(name);
 
    //TODO: make a flag or attribute indicating whether or not the camera should be attached to any parent node.
    //if(pParent)
    //    pParent->attachObject(pCamera);
 
    // Set the field-of-view
    //! @todo Is this always in degrees?
    //pCamera->setFOVy(Ogre::Degree(fov));
 
    // Set the aspect ratio
    //pCamera->setAspectRatio(aspectRatio);
 
    // Set the projection type
    if(projectionType == "perspective")
        pCamera->setProjectionType(Ogre::PT_PERSPECTIVE);
    else if(projectionType == "orthographic")
        pCamera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
 
    rapidxml::xml_node<>* pElement;
 
    // Process clipping (?)
    pElement = XMLNode->first_node("clipping");
    if(pElement)
    {
        Ogre::Real nearDist = getAttribReal(pElement, "near");
        pCamera->setNearClipDistance(nearDist);
 
        Ogre::Real farDist =  getAttribReal(pElement, "far");
        pCamera->setFarClipDistance(farDist);
    }
 
    // Process position (?)
    pElement = XMLNode->first_node("position");
    if(pElement)
        pCamera->setPosition(parseVector3(pElement));
 
    // Process rotation (?)
    pElement = XMLNode->first_node("rotation");
    if(pElement)
        pCamera->setOrientation(parseQuaternion(pElement));
 
    // Process normal (?)
    pElement = XMLNode->first_node("normal");
    if(pElement)
        ;//!< @todo What to do with this element?
 
    // Process lookTarget (?)
    pElement = XMLNode->first_node("lookTarget");
    if(pElement)
        ;//!< @todo Implement the camera look target
 
    // Process trackTarget (?)
    pElement = XMLNode->first_node("trackTarget");
    if(pElement)
        ;//!< @todo Implement the camera track target
 
    // Process userDataReference (?)
    pElement = XMLNode->first_node("userDataReference");
    if(pElement)
        ;//!< @todo Implement the camera user data reference
 
    // construct a scenenode is no parent
    if(!pParent)
    {
        Ogre::SceneNode* pNode = mAttachNode->createChildSceneNode(name);
        pNode->setPosition(pCamera->getPosition());
        pNode->setOrientation(pCamera->getOrientation());
        pNode->scale(1,1,1);
    }
}
 
void DotSceneLoader::processNode(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    // Construct the node's name
    Ogre::String name = m_sPrependNode + getAttrib(XMLNode, "name");
 
    // Create the scene node
    Ogre::SceneNode *pNode;
    if(name.empty())
    {
        // Let Ogre choose the name
        if(pParent)
            pNode = pParent->createChildSceneNode();
        else
            pNode = mAttachNode->createChildSceneNode();
    }
    else
    {
        // Provide the name
        if(pParent)
            pNode = pParent->createChildSceneNode(name);
        else
            pNode = mAttachNode->createChildSceneNode(name);
    }
 
    // Process other attributes
    Ogre::String id = getAttrib(XMLNode, "id");
    bool isTarget = getAttribBool(XMLNode, "isTarget");
 
    rapidxml::xml_node<>* pElement;
 
    // Process position (?)
    pElement = XMLNode->first_node("position");
    if(pElement)
    {
        pNode->setPosition(parseVector3(pElement));
        pNode->setInitialState();
    }
 
    // Process rotation (?)
    pElement = XMLNode->first_node("rotation");
    if(pElement)
    {
        pNode->setOrientation(parseQuaternion(pElement));
        pNode->setInitialState();
    }
 
    // Process scale (?)
    pElement = XMLNode->first_node("scale");
    if(pElement)
    {
        pNode->setScale(parseVector3(pElement));
        pNode->setInitialState();
    }
 
    // Process lookTarget (?)
    pElement = XMLNode->first_node("lookTarget");
    if(pElement)
        processLookTarget(pElement, pNode);
 
    // Process trackTarget (?)
    pElement = XMLNode->first_node("trackTarget");
    if(pElement)
        processTrackTarget(pElement, pNode);
 
    // Process node (*)
    pElement = XMLNode->first_node("node");
    while(pElement)
    {
        processNode(pElement, pNode);
        pElement = pElement->next_sibling("node");
    }
 
    // Process entity (*)
    pElement = XMLNode->first_node("entity");
    while(pElement)
    {
        processEntity(pElement, pNode);
        pElement = pElement->next_sibling("entity");
    }
 
    // Process light (*)
    //pElement = XMLNode->first_node("light");
    //while(pElement)
    //{
    //    processLight(pElement, pNode);
    //    pElement = pElement->next_sibling("light");
    //}
 
    // Process camera (*)
    pElement = XMLNode->first_node("camera");
    while(pElement)
    {
        processCamera(pElement, pNode);
        pElement = pElement->next_sibling("camera");
    }
 
    // Process particleSystem (*)
    pElement = XMLNode->first_node("particleSystem");
    while(pElement)
    {
        processParticleSystem(pElement, pNode);
        pElement = pElement->next_sibling("particleSystem");
    }
 
    // Process billboardSet (*)
    pElement = XMLNode->first_node("billboardSet");
    while(pElement)
    {
        processBillboardSet(pElement, pNode);
        pElement = pElement->next_sibling("billboardSet");
    }
 
    // Process plane (*)
    pElement = XMLNode->first_node("plane");
    while(pElement)
    {
        processPlane(pElement, pNode);
        pElement = pElement->next_sibling("plane");
    }
 
    // Process userDataReference (?)
    pElement = XMLNode->first_node("userDataReference");
    if(pElement)
        processUserDataReference(pElement, pNode);

    pElement = XMLNode->first_node("pagedgeometry");
    while(pElement)
    {
        processPagedGeometry(pElement, pNode);
        pElement = pElement->next_sibling("pagedgeometry");
    }
}
 
void DotSceneLoader::processLookTarget(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    //! @todo Is this correct? Cause I don't have a clue actually
 
    // Process attributes
    Ogre::String nodeName = getAttrib(XMLNode, "nodeName");
 
    Ogre::Node::TransformSpace relativeTo = Ogre::Node::TS_PARENT;
    Ogre::String sValue = getAttrib(XMLNode, "relativeTo");
    if(sValue == "local")
        relativeTo = Ogre::Node::TS_LOCAL;
    else if(sValue == "parent")
        relativeTo = Ogre::Node::TS_PARENT;
    else if(sValue == "world")
        relativeTo = Ogre::Node::TS_WORLD;
 
    rapidxml::xml_node<>* pElement;
 
    // Process position (?)
    Ogre::Vector3 position;
    pElement = XMLNode->first_node("position");
    if(pElement)
        position = parseVector3(pElement);
 
    // Process localDirection (?)
    Ogre::Vector3 localDirection = Ogre::Vector3::NEGATIVE_UNIT_Z;
    pElement = XMLNode->first_node("localDirection");
    if(pElement)
        localDirection = parseVector3(pElement);
 
    // Setup the look target
    try
    {
        if(!nodeName.empty())
        {
            Ogre::SceneNode *pLookNode = mSceneMgr->getSceneNode(nodeName);
            position = pLookNode->_getDerivedPosition();
        }
 
        pParent->lookAt(position, relativeTo, localDirection);
    }
    catch(Ogre::Exception &/*e*/)
    {
        Ogre::LogManager::getSingleton().logMessage("[DotSceneLoader] Error processing a look target!");
    }
}
 
void DotSceneLoader::processTrackTarget(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    // Process attributes
    Ogre::String nodeName = getAttrib(XMLNode, "nodeName");
 
    rapidxml::xml_node<>* pElement;
 
    // Process localDirection (?)
    Ogre::Vector3 localDirection = Ogre::Vector3::NEGATIVE_UNIT_Z;
    pElement = XMLNode->first_node("localDirection");
    if(pElement)
        localDirection = parseVector3(pElement);
 
    // Process offset (?)
    Ogre::Vector3 offset = Ogre::Vector3::ZERO;
    pElement = XMLNode->first_node("offset");
    if(pElement)
        offset = parseVector3(pElement);
 
    // Setup the track target
    try
    {
        Ogre::SceneNode *pTrackNode = mSceneMgr->getSceneNode(nodeName);
        pParent->setAutoTracking(true, pTrackNode, localDirection, offset);
    }
    catch(Ogre::Exception &/*e*/)
    {
        Ogre::LogManager::getSingleton().logMessage("[DotSceneLoader] Error processing a track target!");
    }
}
 
void DotSceneLoader::processEntity(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    // Process attributes
    Ogre::String name = getAttrib(XMLNode, "name");
    Ogre::String id = getAttrib(XMLNode, "id");
    Ogre::String meshFile = getAttrib(XMLNode, "meshFile");
    Ogre::String materialFile = getAttrib(XMLNode, "materialFile");
    bool isStatic = getAttribBool(XMLNode, "static", false);
    bool castShadows = getAttribBool(XMLNode, "castShadows", true);
	Ogre::String physicsShape = getAttrib(XMLNode, "physicsShape");
	bool isHidden = getAttribBool(XMLNode, "hidden", false);


    // TEMP: Maintain a list of static and dynamic objects
    if(isStatic)
        staticObjects.push_back(name);
    //else
       // dynamicObjects.push_back(name);
 
    rapidxml::xml_node<>* pElement;
 
    // Process vertexBuffer (?)
    pElement = XMLNode->first_node("vertexBuffer");
    if(pElement)
        ;//processVertexBuffer(pElement);
 
    // Process indexBuffer (?)
    pElement = XMLNode->first_node("indexBuffer");
    if(pElement)
        ;//processIndexBuffer(pElement);
 
    // Create the entity
    Ogre::Entity *pEntity = 0;
    try
    {
        Ogre::MeshManager::getSingleton().load(meshFile, m_sGroupName);
        pEntity = mSceneMgr->createEntity(name, meshFile);
        pEntity->setCastShadows(castShadows);
        
		if(isHidden)
		{
			pEntity->setVisible(false);
		}

        if(!materialFile.empty())
            pEntity->setMaterialName(materialFile);

#ifndef LINUX_BUILD
		if(Ogre::StringUtil::match(physicsShape, "None", false)==false)
		{
			if(Ogre::StringUtil::match(physicsShape, "Box", false))
			{

#if USE_BULLET
             OgreBulletCollisions::BoxCollisionShape *sceneBoxShape = new OgreBulletCollisions::BoxCollisionShape(size);
             // and the Bullet rigid body
             OgreBulletDynamics::RigidBody *defaultBody = new OgreBulletDynamics::RigidBody(
                   "defaultBoxRigid" + StringConverter::toString(mNumEntitiesInstanced),
                   mWorld);
             defaultBody->setShape(   node,
                               sceneBoxShape,
                               0.6f,         // dynamic body restitution
                               0.6f,         // dynamic body friction
                               1.0f,          // dynamic bodymass
                               position,      // starting position of the box
                               Quaternion(0,0,0,1));
#else
				//load physics...
				Critter::BodyDescription bodyDescription;
				bodyDescription.mMass = 40.0f;
				bodyDescription.mLinearVelocity = NxOgre::Vec3(0.f, 0.f, 0.f);//initialVelocity;
				Ogre::Vector3 vPos = pParent->_getDerivedPosition();//getPosition();
				/*vPos.x *= pParent->getScale().x;
				vPos.y *= pParent->getScale().y;
				vPos.z *= pParent->getScale().z;*/
				const Ogre::Quaternion &vRot = pParent->_getDerivedOrientation();//getOrientation();
				NxOgre::Matrix44 globalPose(vPos, vRot);
				const Ogre::AxisAlignedBox & bbox = pEntity->getBoundingBox();
				Critter::Body* b = critterRender->createBody(NxOgre::BoxDescription(bbox.getSize().x, bbox.getSize().y, bbox.getSize().z), globalPose, meshFile, bodyDescription);
				if(b)
				{
					dynamicObjects.push_back(b);
				}
				//dynamicObjects.push_back(name);
				//pParent->attachObject(pEntity);
#endif
			}
			else if(Ogre::StringUtil::match(physicsShape, "Sphere", false))
			{
				Critter::BodyDescription bodyDescription;
				bodyDescription.mMass = 40.0f;
				bodyDescription.mLinearVelocity = NxOgre::Vec3(0.f, 0.f, 0.f);//initialVelocity;
				const Ogre::Vector3 &vPos = pParent->getPosition();
				const Ogre::Quaternion &vRot = pParent->getOrientation();
				NxOgre::Matrix44 globalPose(vPos, vRot);
				Critter::Body* b = critterRender->createBody(NxOgre::SphereDescription(0.5), globalPose, meshFile, bodyDescription);
				if(b)
				{
					dynamicObjects.push_back(b);
				}
				//pParent->attachObject(pEntity);
			}
			else if(Ogre::StringUtil::match(physicsShape, "Capsule", false) || Ogre::StringUtil::match(physicsShape, "Cylinder", false))
			{
				Critter::BodyDescription bodyDescription;
				bodyDescription.mMass = 40.0f;
				bodyDescription.mLinearVelocity = NxOgre::Vec3(0.f, 0.f, 0.f);//initialVelocity;
				const Ogre::Vector3 &vPos = pParent->getPosition();
				const Ogre::Quaternion &vRot = pParent->getOrientation();
				NxOgre::Matrix44 globalPose(vPos, vRot);
				Critter::Body *b = critterRender->createBody(NxOgre::CapsuleDescription(), globalPose, meshFile, bodyDescription);
				if(b)
				{
					dynamicObjects.push_back(b);
				}
				//dynamicObjects.push_back(name);
				//pParent->attachObject(pEntity);
			}
			else if(Ogre::StringUtil::match(physicsShape, "Convex", false))
			{
				Critter::BodyDescription bodyDescription;
				bodyDescription.mMass = 40.0f;
				bodyDescription.mLinearVelocity = NxOgre::Vec3(0.f, 0.f, 0.f);//initialVelocity;
				const Ogre::Vector3 &vPos = pParent->getPosition();
				const Ogre::Quaternion &vRot = pParent->getOrientation();
				NxOgre::Matrix44 globalPose(vPos, vRot);
				std::string nxs = "ogre://General/"+name+".nxs";
				NxOgre::Mesh *mesh = NxOgre::MeshManager::getSingleton()->load(nxs.c_str(), name.c_str());
				if(mesh)
				{
					critterRender->createBody(NxOgre::ConvexDescription(mesh), globalPose, meshFile, bodyDescription);
					//dynamicObjects.push_back(name);
				}
			}
			else if(Ogre::StringUtil::match(physicsShape, "Triangle", false))
			{
				Critter::BodyDescription bodyDescription;
				bodyDescription.mMass = 40.0f;
				bodyDescription.mLinearVelocity = NxOgre::Vec3(0.f, 0.f, 0.f);//initialVelocity;
				const Ogre::Vector3 &vPos = pParent->getPosition();
				const Ogre::Quaternion &vRot = pParent->getOrientation();
				NxOgre::Matrix44 globalPose(vPos, vRot);
				std::string nxs = "ogre://General/"+name+".nxs";
				NxOgre::Mesh *triangleMesh = NxOgre::MeshManager::getSingleton()->load(nxs.c_str(), name.c_str());
				if(triangleMesh)
				{
					// Remember TriangleMeshes can only be given into SceneGeometries, and NOT actors or bodies.
					// - We create the SceneGeometry through the Scene, and not Critter.
					// - We don't need a RigidBodyDescription for this, the defaults are fine.
					// - The visualisation is setup in the next few lines.
					critterRender->getScene()->createSceneGeometry(NxOgre::TriangleGeometryDescription(triangleMesh), NxOgre::Vec3::ZERO);
					// The visualisation.
					//Ogre::SceneNode* triNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(Ogre::Vector3(0,0,0));
					//triNode->attachObject(mSceneMgr->createEntity(name.c_str(), meshFile.c_str()));
					pParent->attachObject(pEntity);
				}
			}
			else
			{
				pParent->attachObject(pEntity);
			}
		}
		else
		{
#endif
			//other-wise still show plain mesh w/o physics
			pParent->attachObject(pEntity);
#ifndef LINUX_BUILD
		}
#endif

    }
    catch(Ogre::Exception &/*e*/)
    {
        Ogre::LogManager::getSingleton().logMessage("[DotSceneLoader] Error loading an entity!");
    }
 
    // Process userDataReference (?)
    pElement = XMLNode->first_node("userDataReference");
    if(pElement)
        processUserDataReference(pElement, pEntity);
}
 
void DotSceneLoader::processParticleSystem(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    // Process attributes
    Ogre::String name = getAttrib(XMLNode, "name");
    Ogre::String id = getAttrib(XMLNode, "id");
    Ogre::String file = getAttrib(XMLNode, "file");
 
    // Create the particle system
    try
    {
        Ogre::ParticleSystem *pParticles = mSceneMgr->createParticleSystem(name, file);
        pParent->attachObject(pParticles);
    }
    catch(Ogre::Exception &/*e*/)
    {
        Ogre::LogManager::getSingleton().logMessage("[DotSceneLoader] Error creating a particle system!");
    }
}
 
void DotSceneLoader::processBillboardSet(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    //! @todo Implement this
}
 
void DotSceneLoader::processPlane(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
    Ogre::String name = getAttrib(XMLNode, "name");
    Ogre::Real distance = getAttribReal(XMLNode, "distance");
    Ogre::Real width = getAttribReal(XMLNode, "width");
    Ogre::Real height = getAttribReal(XMLNode, "height");
    int xSegments = Ogre::StringConverter::parseInt(Ogre::String(getAttrib(XMLNode, "xSegments")));
    int ySegments = Ogre::StringConverter::parseInt(Ogre::String(getAttrib(XMLNode, "ySegments")));
    int numTexCoordSets = Ogre::StringConverter::parseInt(Ogre::String(getAttrib(XMLNode, "numTexCoordSets")));
    Ogre::Real uTile = getAttribReal(XMLNode, "uTile");
    Ogre::Real vTile = getAttribReal(XMLNode, "vTile");
    Ogre::String material = getAttrib(XMLNode, "material");
    bool hasNormals = getAttribBool(XMLNode, "hasNormals");
    Ogre::Vector3 normal = parseVector3(XMLNode->first_node("normal"));
    Ogre::Vector3 up = parseVector3(XMLNode->first_node("upVector"));
 
    Ogre::Plane plane(normal, distance);
    Ogre::MeshPtr res = Ogre::MeshManager::getSingletonPtr()->createPlane(
                        name + "mesh", "General", plane, width, height, xSegments, ySegments, hasNormals,
    numTexCoordSets, uTile, vTile, up);
    Ogre::Entity* ent = mSceneMgr->createEntity(name, name + "mesh");
 
    ent->setMaterialName(material);
 
    pParent->attachObject(ent);
}
 

struct PGInstanceInfo
{
    Ogre::Vector3 pos;
    Ogre::Real    scale;
    Ogre::Real    yaw;
};


typedef std::vector<PGInstanceInfo> PGInstanceList;

void DotSceneLoader::processPagedGeometry(rapidxml::xml_node<>* XMLNode, Ogre::SceneNode *pParent)
{
#ifndef LINUX_BUILD
    Ogre::String filename = "../Projects/SampleScene3/" + getAttrib(XMLNode, "fileName");
    Ogre::String model = getAttrib(XMLNode, "model");
    Ogre::Real pagesize = getAttribReal(XMLNode, "pageSize");
    Ogre::Real batchdistance = getAttribReal(XMLNode, "batchDistance");
    Ogre::Real impostordistance = getAttribReal(XMLNode, "impostorDistance");
    Ogre::Vector4 bounds = Ogre::StringConverter::parseVector4(getAttrib(XMLNode, "bounds"));

    PagedGeometry *mPGHandle = new PagedGeometry();
    mPGHandle->setCamera(mSceneMgr->getCameraIterator().begin()->second);
    mPGHandle->setPageSize(pagesize);
    mPGHandle->setInfinite();

    mPGHandle->addDetailLevel<BatchPage>(batchdistance,0);
    mPGHandle->addDetailLevel<ImpostorPage>(impostordistance,0);

    TreeLoader3D *mHandle = new TreeLoader3D(mPGHandle, Forests::TBounds(bounds.x, bounds.y, bounds.z, bounds.w));
    mPGHandle->setPageLoader(mHandle);

    mPGHandles.push_back(mPGHandle);
    mTreeHandles.push_back(mHandle);

    std::ifstream stream(filename.c_str());

    if(!stream.is_open())
        return;

    Ogre::StringVector list;

    char res[128];

    PGInstanceList mInstanceList;

    while(!stream.eof())
    {
        stream.getline(res, 128);
        Ogre::String resStr(res);

        ParseStringVector(resStr, list);

        if(list.size() == 3)
        {
            PGInstanceInfo info;
            
            info.pos = Ogre::StringConverter::parseVector3(list[0]);
            info.scale = Ogre::StringConverter::parseReal(list[1]);
            info.yaw = Ogre::StringConverter::parseReal(list[2]);

            mInstanceList.push_back(info);
        }
        else if(list.size() == 4)
        {
            PGInstanceInfo info;
            
            info.pos = Ogre::StringConverter::parseVector3(list[1]);
            info.scale = Ogre::StringConverter::parseReal(list[2]);
            info.yaw = Ogre::StringConverter::parseReal(list[3]);

            mInstanceList.push_back(info);
        }
    }

    stream.close();

    if(model != "")
    {
        Ogre::Entity *mEntityHandle = mSceneMgr->createEntity(model + ".mesh");

        PGInstanceList::iterator it = mInstanceList.begin();

        while(it != mInstanceList.end())
        {
            mHandle->addTree(mEntityHandle, it->pos, Ogre::Degree(it->yaw), it->scale);

            it++;
        }
    }
#endif
}


void DotSceneLoader::processFog(rapidxml::xml_node<>* XMLNode)
{
    // Process attributes
    Ogre::Real expDensity = getAttribReal(XMLNode, "density", 0.001);
    Ogre::Real linearStart = getAttribReal(XMLNode, "start", 0.0);
    Ogre::Real linearEnd = getAttribReal(XMLNode, "end", 1.0);
 
    Ogre::FogMode mode = Ogre::FOG_NONE;
    Ogre::String sMode = getAttrib(XMLNode, "mode");
    if(sMode == "none")
        mode = Ogre::FOG_NONE;
    else if(sMode == "exp")
        mode = Ogre::FOG_EXP;
    else if(sMode == "exp2")
        mode = Ogre::FOG_EXP2;
    else if(sMode == "linear")
        mode = Ogre::FOG_LINEAR;
    else
        mode = (Ogre::FogMode)Ogre::StringConverter::parseInt(sMode);
 
    rapidxml::xml_node<>* pElement;
 
    // Process colourDiffuse (?)
    Ogre::ColourValue colourDiffuse = Ogre::ColourValue::White;
    pElement = XMLNode->first_node("colour");
    if(pElement)
        colourDiffuse = parseColour(pElement);
 
    // Setup the fog
    mSceneMgr->setFog(mode, colourDiffuse, expDensity, linearStart, linearEnd);
}
 
void DotSceneLoader::processSkyBox(rapidxml::xml_node<>* XMLNode)
{
    // Process attributes
    Ogre::String material = getAttrib(XMLNode, "material", "BaseWhite");
    Ogre::Real distance = getAttribReal(XMLNode, "distance", 5000);
    bool drawFirst = getAttribBool(XMLNode, "drawFirst", true);
    bool active = getAttribBool(XMLNode, "active", false);
    if(!active)
        return;
 
    rapidxml::xml_node<>* pElement;
 
    // Process rotation (?)
    Ogre::Quaternion rotation = Ogre::Quaternion::IDENTITY;
    pElement = XMLNode->first_node("rotation");
    if(pElement)
        rotation = parseQuaternion(pElement);
 
    // Setup the sky box
    mSceneMgr->setSkyBox(true, material, distance, drawFirst, rotation, m_sGroupName);
}
 
void DotSceneLoader::processSkyDome(rapidxml::xml_node<>* XMLNode)
{
    // Process attributes
    Ogre::String material = XMLNode->first_attribute("material")->value();
    Ogre::Real curvature = getAttribReal(XMLNode, "curvature", 10);
    Ogre::Real tiling = getAttribReal(XMLNode, "tiling", 8);
    Ogre::Real distance = getAttribReal(XMLNode, "distance", 4000);
    bool drawFirst = getAttribBool(XMLNode, "drawFirst", true);
    bool active = getAttribBool(XMLNode, "active", false);
    if(!active)
        return;
 
    rapidxml::xml_node<>* pElement;
 
    // Process rotation (?)
    Ogre::Quaternion rotation = Ogre::Quaternion::IDENTITY;
    pElement = XMLNode->first_node("rotation");
    if(pElement)
        rotation = parseQuaternion(pElement);
 
    // Setup the sky dome
    mSceneMgr->setSkyDome(true, material, curvature, tiling, distance, drawFirst, rotation, 16, 16, -1, m_sGroupName);
}
 
void DotSceneLoader::processSkyPlane(rapidxml::xml_node<>* XMLNode)
{
    // Process attributes
    Ogre::String material = getAttrib(XMLNode, "material");
    Ogre::Real planeX = getAttribReal(XMLNode, "planeX", 0);
    Ogre::Real planeY = getAttribReal(XMLNode, "planeY", -1);
    Ogre::Real planeZ = getAttribReal(XMLNode, "planeX", 0);
    Ogre::Real planeD = getAttribReal(XMLNode, "planeD", 5000);
    Ogre::Real scale = getAttribReal(XMLNode, "scale", 1000);
    Ogre::Real bow = getAttribReal(XMLNode, "bow", 0);
    Ogre::Real tiling = getAttribReal(XMLNode, "tiling", 10);
    bool drawFirst = getAttribBool(XMLNode, "drawFirst", true);
 
    // Setup the sky plane
    Ogre::Plane plane;
    plane.normal = Ogre::Vector3(planeX, planeY, planeZ);
    plane.d = planeD;
    mSceneMgr->setSkyPlane(true, plane, material, scale, tiling, drawFirst, bow, 1, 1, m_sGroupName);
}
 
void DotSceneLoader::processClipping(rapidxml::xml_node<>* XMLNode)
{
    //! @todo Implement this
 
    // Process attributes
    Ogre::Real fNear = getAttribReal(XMLNode, "near", 0);
    Ogre::Real fFar = getAttribReal(XMLNode, "far", 1);
}
 
void DotSceneLoader::processLightRange(rapidxml::xml_node<>* XMLNode, Ogre::Light *pLight)
{
    // Process attributes
    Ogre::Real inner = getAttribReal(XMLNode, "inner");
    Ogre::Real outer = getAttribReal(XMLNode, "outer");
    Ogre::Real falloff = getAttribReal(XMLNode, "falloff", 1.0);
 
    // Setup the light range
    pLight->setSpotlightRange(Ogre::Angle(inner), Ogre::Angle(outer), falloff);
}
 
void DotSceneLoader::processLightAttenuation(rapidxml::xml_node<>* XMLNode, Ogre::Light *pLight)
{
    // Process attributes
    Ogre::Real range = getAttribReal(XMLNode, "range");
    Ogre::Real constant = getAttribReal(XMLNode, "constant");
    Ogre::Real linear = getAttribReal(XMLNode, "linear");
    Ogre::Real quadratic = getAttribReal(XMLNode, "quadratic");
 
    // Setup the light attenuation
    pLight->setAttenuation(range, constant, linear, quadratic);
}
 
 
Ogre::String DotSceneLoader::getAttrib(rapidxml::xml_node<>* XMLNode, const Ogre::String &attrib, const Ogre::String &defaultValue)
{
    if(XMLNode->first_attribute(attrib.c_str()))
        return XMLNode->first_attribute(attrib.c_str())->value();
    else
        return defaultValue;
}
 
Ogre::Real DotSceneLoader::getAttribReal(rapidxml::xml_node<>* XMLNode, const Ogre::String &attrib, Ogre::Real defaultValue)
{
    if(XMLNode->first_attribute(attrib.c_str()))
        return Ogre::StringConverter::parseReal(XMLNode->first_attribute(attrib.c_str())->value());
    else
        return defaultValue;
}
 
bool DotSceneLoader::getAttribBool(rapidxml::xml_node<>* XMLNode, const Ogre::String &attrib, bool defaultValue)
{
    if(!XMLNode->first_attribute(attrib.c_str()))
        return defaultValue;
 
    if(Ogre::String(XMLNode->first_attribute(attrib.c_str())->value()) == "true")
        return true;
 
    return false;
}
 
Ogre::Vector3 DotSceneLoader::parseVector3(rapidxml::xml_node<>* XMLNode)
{
    return Ogre::Vector3(
        Ogre::StringConverter::parseReal(XMLNode->first_attribute("x")->value()),
        Ogre::StringConverter::parseReal(XMLNode->first_attribute("y")->value()),
        Ogre::StringConverter::parseReal(XMLNode->first_attribute("z")->value())
    );
}
 
Ogre::Quaternion DotSceneLoader::parseQuaternion(rapidxml::xml_node<>* XMLNode)
{
    //! @todo Fix this crap!
 
    Ogre::Quaternion orientation;
 
    if(XMLNode->first_attribute("qx"))
    {
        orientation.x = Ogre::StringConverter::parseReal(XMLNode->first_attribute("qx")->value());
        orientation.y = Ogre::StringConverter::parseReal(XMLNode->first_attribute("qy")->value());
        orientation.z = Ogre::StringConverter::parseReal(XMLNode->first_attribute("qz")->value());
        orientation.w = Ogre::StringConverter::parseReal(XMLNode->first_attribute("qw")->value());
    }
    if(XMLNode->first_attribute("qw"))
    {
        orientation.w = Ogre::StringConverter::parseReal(XMLNode->first_attribute("qw")->value());
        orientation.x = Ogre::StringConverter::parseReal(XMLNode->first_attribute("qx")->value());
        orientation.y = Ogre::StringConverter::parseReal(XMLNode->first_attribute("qy")->value());
        orientation.z = Ogre::StringConverter::parseReal(XMLNode->first_attribute("qz")->value());
    }
    else if(XMLNode->first_attribute("axisX"))
    {
        Ogre::Vector3 axis;
        axis.x = Ogre::StringConverter::parseReal(XMLNode->first_attribute("axisX")->value());
        axis.y = Ogre::StringConverter::parseReal(XMLNode->first_attribute("axisY")->value());
        axis.z = Ogre::StringConverter::parseReal(XMLNode->first_attribute("axisZ")->value());
        Ogre::Real angle = Ogre::StringConverter::parseReal(XMLNode->first_attribute("angle")->value());;
        orientation.FromAngleAxis(Ogre::Angle(angle), axis);
    }
    else if(XMLNode->first_attribute("angleX"))
    {
        Ogre::Vector3 axis;
        axis.x = Ogre::StringConverter::parseReal(XMLNode->first_attribute("angleX")->value());
        axis.y = Ogre::StringConverter::parseReal(XMLNode->first_attribute("angleY")->value());
        axis.z = Ogre::StringConverter::parseReal(XMLNode->first_attribute("angleZ")->value());
        //orientation.FromAxes(&axis);
        //orientation.F
    }
    else if(XMLNode->first_attribute("x"))
    {
        orientation.x = Ogre::StringConverter::parseReal(XMLNode->first_attribute("x")->value());
        orientation.y = Ogre::StringConverter::parseReal(XMLNode->first_attribute("y")->value());
        orientation.z = Ogre::StringConverter::parseReal(XMLNode->first_attribute("z")->value());
        orientation.w = Ogre::StringConverter::parseReal(XMLNode->first_attribute("w")->value());
    }
    else if(XMLNode->first_attribute("w"))
    {
        orientation.w = Ogre::StringConverter::parseReal(XMLNode->first_attribute("w")->value());
        orientation.x = Ogre::StringConverter::parseReal(XMLNode->first_attribute("x")->value());
        orientation.y = Ogre::StringConverter::parseReal(XMLNode->first_attribute("y")->value());
        orientation.z = Ogre::StringConverter::parseReal(XMLNode->first_attribute("z")->value());
    }
 
    return orientation;
}
 
Ogre::ColourValue DotSceneLoader::parseColour(rapidxml::xml_node<>* XMLNode)
{
    return Ogre::ColourValue(
        Ogre::StringConverter::parseReal(XMLNode->first_attribute("r")->value()),
        Ogre::StringConverter::parseReal(XMLNode->first_attribute("g")->value()),
        Ogre::StringConverter::parseReal(XMLNode->first_attribute("b")->value()),
        XMLNode->first_attribute("a") != NULL ? Ogre::StringConverter::parseReal(XMLNode->first_attribute("a")->value()) : 1
    );
}
 
Ogre::String DotSceneLoader::getProperty(const Ogre::String &ndNm, const Ogre::String &prop)
{
    for ( unsigned int i = 0 ; i < nodeProperties.size(); i++ )
    {
        if ( nodeProperties[i].nodeName == ndNm && nodeProperties[i].propertyNm == prop )
        {
            return nodeProperties[i].valueName;
        }
    }
 
    return "";
}
 
void DotSceneLoader::processUserDataReference(rapidxml::xml_node<>* XMLNode, Ogre::Entity *pEntity)
{
    Ogre::String str = XMLNode->first_attribute("id")->value();
    pEntity->setUserAny(Ogre::Any(str));
}

void DotSceneLoader::sceneExplore(Ogre::SceneManager* mSceneMgr, const char *saveName, const char *meshDir, const char *sceneName)
{
   //The CSaveSceneView::SceneExplore() function simply creates the XML root
   //scene node and then calls CSaveSceneView::SceneNodeExplore() with the
   //RootSceneNode.

	Ogre::SceneNode *RootSceneNode = mSceneMgr->getRootSceneNode();
	rapidxml::xml_document<> doc;
	rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "scene");
	doc.append_node(root);
	root->append_attribute(doc.allocate_attribute("formatVersion", "1.0"));
	Ogre::String *strSaveName = new Ogre::String(saveName);
	strSaveName->append(Ogre::String("mesh"));
	rapidxml::xml_node<> * resourceLocations = doc.allocate_node(rapidxml::node_element, "resourceLocations");
	root->append_node(resourceLocations);
	rapidxml::xml_node<>* resourceMeshLocations = doc.allocate_node(rapidxml::node_element, "resourceLocation");
	resourceMeshLocations->append_attribute(doc.allocate_attribute("type", "FileSystem"));
	resourceMeshLocations->append_attribute(doc.allocate_attribute("name", strSaveName->c_str()));	//put save location path here
	resourceMeshLocations->append_attribute(doc.allocate_attribute("recursive", "false"));
	resourceLocations->append_node(resourceMeshLocations);

	rapidxml::xml_node<>* rootNodes = doc.allocate_node(rapidxml::node_element, "nodes");

	sceneNodeExplore(RootSceneNode, &doc, rootNodes, meshDir);

	root->append_node(rootNodes);
	
	std::string s;
	print(std::back_inserter(s), doc, 0);
	std::ofstream myfile;
    	myfile.open((saveName+std::string(sceneName)+std::string(".scene")).c_str(), std::ios_base::out);
	myfile << s;
	myfile.close();
}
void DotSceneLoader::sceneNodeExplore(Ogre::SceneNode* node, rapidxml::xml_document<> * xmlDoc, rapidxml::xml_node<> * xmlNodes, const char *meshDir)
{   /*The CSaveSceneView::SceneNodeExplore function writes information about the
   current scene node to memory, including all the entities attached to the node, then it
   iterates over each scene node child and calls CSaveSceneView::SceneNodeExpl
   ore with that child, as the current scene node.*/

	//note - this is currently leaking memory due to how the xml stuff has to work (needs dynamically allocated strings)
   Ogre::Entity *mEntity = NULL;
   Ogre::Camera *mCamera = NULL;
   Ogre::Light *mLight = NULL;
   Ogre::ParticleSystem *mParticleSystem = NULL;
   Ogre::ManualObject *mManualObject = NULL;
   Ogre::BillboardSet *mBillboardSet = NULL;
   Ogre::SceneNode::ObjectIterator obji = node->getAttachedObjectIterator();

   while(obji.hasMoreElements())
   {
      Ogre::MovableObject* mobj = obji.getNext();
      Ogre::String type = mobj->getMovableType();

	  const Ogre::String &SceneNodeName = node->getName();

      if(type == "Entity")
      {
		 mEntity = (Ogre::Entity*)(mobj);
		 const Ogre::String &EntityName = mEntity->getName();
		 if(strcmp(EntityName.c_str(), "Floor") != 0)
		 {
			 Ogre::String *pBetterName = new Ogre::String();
			 rapidxml::xml_node<> * xmlNode = xmlDoc->allocate_node(rapidxml::node_element, "node");
			 xmlNodes->append_node(xmlNode);
         
			 pBetterName->append(mEntity->getName());
			 pBetterName->append("_parent");
			 xmlNode->append_attribute(xmlDoc->allocate_attribute("name", pBetterName->c_str()));
			 //write out position, scale, rotation and then sub-entities...
			 rapidxml::xml_node<> * xmlPos = xmlDoc->allocate_node(rapidxml::node_element, "position");
			 xmlNode->append_node(xmlPos);
			 const Ogre::Vector3 &vPos = mEntity->getParentSceneNode()->getPosition();
			 const Ogre::Vector3 &vScale = mEntity->getParentSceneNode()->getScale();
			 const Ogre::Quaternion &mRot = mEntity->getParentSceneNode()->getOrientation();
			 char *xPos = new char[32];
			 memset(xPos, 0, 32);
			 sprintf(xPos, "%.3f", vPos.x);
			 char *yPos = new char[32];
			 memset(yPos, 0, 32);
			 sprintf(yPos, "%.3f", vPos.y);
			 char *zPos = new char[32];
			 memset(zPos, 0, 32);
			 sprintf(zPos, "%.3f", vPos.z);
			 xmlPos->append_attribute(xmlDoc->allocate_attribute("x", xPos));
			 xmlPos->append_attribute(xmlDoc->allocate_attribute("y", yPos));
			 xmlPos->append_attribute(xmlDoc->allocate_attribute("z", zPos));
			 rapidxml::xml_node<> * xmlScale = xmlDoc->allocate_node(rapidxml::node_element, "scale");
			 xmlNode->append_node(xmlScale);
			 char *xScale = new char[32];
			 memset(xScale, 0, 32);
			 sprintf(xScale, "%.3f", vScale.x);
			 char *yScale = new char[32];
			 memset(yScale, 0, 32);
			 sprintf(yScale, "%.3f", vScale.y);
			 char *zScale = new char[32];
			 memset(zScale, 0, 32);
			 sprintf(zScale, "%.3f", vScale.z);
			 xmlScale->append_attribute(xmlDoc->allocate_attribute("x", xScale));
			 xmlScale->append_attribute(xmlDoc->allocate_attribute("y", yScale));
			 xmlScale->append_attribute(xmlDoc->allocate_attribute("z", zScale));
			 rapidxml::xml_node<> * xmlRot = xmlDoc->allocate_node(rapidxml::node_element, "rotation");
			 xmlNode->append_node(xmlRot);
			 char *xRot = new char[32];
			 memset(xRot, 0, 32);
			 sprintf(xRot, "%.3f", mRot.x);
			 char *yRot = new char[32];
			 memset(yRot, 0, 32);
			 sprintf(yRot, "%.3f", mRot.y);
			 char *zRot = new char[32];
			 memset(zRot, 0, 32);
			 sprintf(zRot, "%.3f", mRot.z);
			 char *wRot = new char[32];
			 memset(wRot, 0, 32);
			 sprintf(wRot, "%.3f", mRot.w);
			 xmlRot->append_attribute(xmlDoc->allocate_attribute("qw", wRot));
			 xmlRot->append_attribute(xmlDoc->allocate_attribute("qx", xRot));
			 xmlRot->append_attribute(xmlDoc->allocate_attribute("qy", yRot));
			 xmlRot->append_attribute(xmlDoc->allocate_attribute("qz", zRot));
		 
			 Ogre::MeshPtr mesh = mEntity->getMesh();

			 Ogre::MeshSerializer *serializer = new Ogre::MeshSerializer();
			 Ogre::String MeshName = mesh->getName();
			 //MeshName.append(".mesh");
			 Ogre::String *newName = new Ogre::String(MeshName);
			 newName->append(".mesh");
			 std::string meshExport(meshDir);
			 meshExport.append("\\");
			 meshExport.append(MeshName.c_str());
			 meshExport.append(".mesh");
			 serializer->exportMesh(mesh.get(), meshExport.c_str());
         
			 rapidxml::xml_node<> * xmlEntity = xmlDoc->allocate_node(rapidxml::node_element, "entity");
			 xmlNode->append_node(xmlEntity);
			 xmlEntity->append_attribute(xmlDoc->allocate_attribute("name", EntityName.c_str()));
			 xmlEntity->append_attribute(xmlDoc->allocate_attribute("castShadows", "true"));
			 xmlEntity->append_attribute(xmlDoc->allocate_attribute("receiveShadows", "true"));
			 xmlEntity->append_attribute(xmlDoc->allocate_attribute("meshFile", newName->c_str()));
			 xmlEntity->append_attribute(xmlDoc->allocate_attribute("physicsShape", "None"));

			 rapidxml::xml_node<> * xmlSubEnts = xmlDoc->allocate_node(rapidxml::node_element, "subentities");
			 xmlEntity->append_node(xmlSubEnts);
			 rapidxml::xml_node<> * xmlSubEnt = xmlDoc->allocate_node(rapidxml::node_element, "subentity");
			 xmlSubEnts->append_node(xmlSubEnt);
			 xmlSubEnt->append_attribute(xmlDoc->allocate_attribute("index", "0"));
			 xmlSubEnt->append_attribute(xmlDoc->allocate_attribute("materialName", "Simple_Perpixel"));
		 }
      }
      if(type == "Camera")
      {
         mCamera = (Ogre::Camera *)(mobj);
         Ogre::String CameraName = mCamera->getName();
         //xmlTextWriterStartElement(m_XmlWriter, BAD_CAST "mCamera");
         //xmlTextWriterWriteAttribute(m_XmlWriter,BAD_CAST "CameraName",BAD_CAST CameraName.c_str());
         Ogre::Vector3 CameraPosition = mCamera->getPosition();
         //xmlTextWriterWriteFormatAttribute(m_XmlWriter,BAD_CAST "XPosition","%f",CameraPosition.x);
         //xmlTextWriterWriteFormatAttribute(m_XmlWriter,BAD_CAST "YPosition","%f",CameraPosition.y);
         //xmlTextWriterWriteFormatAttribute(m_XmlWriter,BAD_CAST "ZPosition","%f",CameraPosition.z);
         Ogre::Vector3 CameraDirection = mCamera->getDirection();
         //xmlTextWriterWriteFormatAttribute(m_XmlWriter,BAD_CAST "XDirection","%f",CameraDirection.x);
         //xmlTextWriterWriteFormatAttribute(m_XmlWriter,BAD_CAST "YDirection","%f",CameraDirection.y);
         //xmlTextWriterWriteFormatAttribute(m_XmlWriter,BAD_CAST "ZDirection","%f",CameraDirection.z);
         //xmlTextWriterEndElement(m_XmlWriter);
      }
      if (type == "Light") 
      {
         mLight = (Ogre::Light *)(mobj);
      }
      if (type == "ParticleSystem") 
      {
         mParticleSystem = (Ogre::ParticleSystem *)(mobj);
      }
      if (type == "ManualObject") 
      {
         mManualObject = (Ogre::ManualObject *)(mobj);
      }
      if (type == "BillboardSet") 
      {
         mBillboardSet = (Ogre::BillboardSet *)(mobj);
      }
   }

   Ogre::Node::ChildNodeIterator nodei = node->getChildIterator();
   while(nodei.hasMoreElements())
   {
      Ogre::SceneNode* mNode = (Ogre::SceneNode*)(nodei.getNext());
      // Add this subnode and its children...
      sceneNodeExplore(mNode, xmlDoc, xmlNodes, meshDir);
   }
}
