#include "WordCake.h"

#include "VROgreAction.h"
#include "MovableText.h"
#ifndef LINUX_BUILD
#include "FionaUtil.h"
#endif
#include "OgreDotScene.h"

const float WordCake::DEFAULT_CAKE_WIDTH=2.29749;
const float WordCake::DEFAULT_CAKE_HEIGHT=14.478;
const jvec3 WordCake::conceptColors[WordCake::NUM_CONCEPTS] = { jvec3(253.f/255.f,191.f/255.f,111.f/255.f),	//interoperable platform
																jvec3(166.f/255.f, 206.f/255.f, 227.f/255.f),	//flexible health information infrastructure
																jvec3(202.f/255.f, 178.f/255.f, 214.f/255.f),	//substitutable app
																jvec3(178.f/255.f, 223.f/255.f, 138.f/255.f),	//mobile medical applications
																jvec3(251.f/255.f, 154.f/255.f, 153.f/255.f) };	//modular health technology
const float WordCake::EVENT_HEIGHT = 150.f;
const float WordCake::EVENT_WIDTH = 160.f;

void WordCake::clearSegmentPoints()
{
	for(int i = 0; i < m_segmentPoints.size(); ++i)
	{
		m_segmentPoints[i].clear();
	}

	m_segmentPoints.clear();
}

void WordCake::openGLRender(void)
{
	FionaOgre::openGLRender();

	//perform extra word cake drawing here..

	glDisable(GL_LIGHTING);

	if(m_bShowArcs)
	{
		glPushMatrix();
		glColor4f(0.4f, 0.4f, 0.4f, 1.f);
		int numArcs = m_arcDepths.size();
		for(int i = 0; i < numArcs; ++i)
		{
			if(m_keyConceptColors.size() > 0)
			{
				glColor3f(conceptColors[m_keyConceptColors[i]].x, conceptColors[m_keyConceptColors[i]].y, conceptColors[m_keyConceptColors[i]].z);
			}
			glTranslatef(0.f, 0.f, m_arcDepths[i]);
			glCallList(m_arcDisplayList);
			glTranslatef(0.f, 0.f, -m_arcDepths[i]);
		}
		glPopMatrix();
	}

	if(m_segmentPoints.size() > 0)
	{ 
		glColor4f(0.f, 1.f, 0.f, 1.f);
		
		int s = m_segmentPoints.size();
		for(int j = 0; j < s; ++j)
		{
			int n = m_segmentPoints[j].size();
			glBegin(GL_LINES);
			for(int i = 1; i < n; ++i)
			{
				glVertex3f(m_segmentPoints[j][i-1].x, m_segmentPoints[j][i-1].y, m_segmentPoints[j][i-1].z);
				glVertex3f(m_segmentPoints[j][i].x, m_segmentPoints[j][i].y, m_segmentPoints[j][i].z);
			}
			glEnd();
		}
		
		glColor4f(1.f, 1.f, 1.f, 1.f);
	}

	glEnable(GL_LIGHTING);
}

bool sortPred(int a, int b)
{
	return a > b;
}

void WordCake::setupScene(Ogre::SceneManager* scene)
{
	///NxOgre::ResourceSystem::getSingleton()->openProtocol(new Critter::OgreResourceProtocol());
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

	DotSceneLoader loader;
#ifndef LINUX_BUILD
#if !USE_BULLET
	loader.setCritterRender(critterRender);
#endif
#endif
	loader.parseDotScene(fionaConf.OgreMediaBasePath+sceneName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, scene);
		
	if(loader.HasWordCake())
	{
		m_sWordCakeDir = loader.GetWordCakeDir();
		sscanf(loader.GetWordCakeHeight().c_str(), "%f", &m_cakeHeight);
		m_cakeHeight = m_cakeHeight * 0.0254f;
		sscanf(loader.GetWordCakeWidth().c_str(), "%f", &m_cakeWidth);
		m_cakeWidth = m_cakeWidth * 0.0254f;
		sscanf(loader.GetWordCakeNumFiles().c_str(), "%d", &m_numFiles);
	}

	//scene->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
	scene->setSkyBox(true, "Examples/WordCake", 35.f);  // set a skybox - eventually get this from the dot scene
	//scene->setFog(Ogre::FOG_LINEAR, Ogre::Vector3(1.f, 1.f, 1.f), 0.01, 15.f, 35.f);
	defaultSetup();

	//position the user in the center of the CAVE...!
	//camPos.set(CAVE_CENTER.x, CAVE_CENTER.y, CAVE_CENTER.z);
	//add custom word cake actions..
	VROWCVizSelect *pSelect = new VROWCVizSelect();
	//pSelect->SetButton(0);
	pSelect->SetButton(5);
	pSelect->SetOnRelease(true);
	m_actions.GetCurrentSet()->AddAction(pSelect);

	VROSwitchVizMat * pSwitch = new VROSwitchVizMat();
	pSwitch->SetButton(4);
	pSwitch->SetOnRelease(true);
	m_actions.GetCurrentSet()->AddAction(pSwitch);

	VROWCToggleEvents * pToggleEvents = new VROWCToggleEvents();
	pToggleEvents->SetButton(3);
	pToggleEvents->SetOnRelease(true);
	m_actions.GetCurrentSet()->AddAction(pToggleEvents);

	VROWCToggleArcs * pToggleArcs = new VROWCToggleArcs();
	pToggleArcs->SetButton(2);
	pToggleArcs->SetOnRelease(true);
	m_actions.GetCurrentSet()->AddAction(pToggleArcs);

	VROWCSideView * pSideView = new VROWCSideView();
	pSideView->SetButton(1);
	pSideView->SetOnRelease(true);
	m_actions.GetCurrentSet()->AddAction(pSideView);

	VROWCFollowPath * pFollowPath = new VROWCFollowPath();
	pFollowPath->SetButton(0);
	pFollowPath->SetOnRelease(true);
	m_actions.GetCurrentSet()->AddAction(pFollowPath);

#ifndef LINUX_BUILD
	if(m_sWordCakeDir.length() > 0)
	{
		//load in word cake information..
		std::vector<std::string> files;
		FionaUtil::ListFilesInDirectory(m_sWordCakeDir.c_str(), files, "\\*.txt");
		if(files.size() > 0)
		{
			//m_numFiles = files.size();

			Ogre::String events(m_sWordCakeDir);
			events.append("wc_pilot_events.csv");
			std::ifstream f(events.c_str());
			//f.open(events.c_str());

			if(f.is_open())
			{
				int count = 0;
				std::time_t firstDate = 0;
				std::time_t lastDate = 0;
				int lineCount[4] = {0, 0, 0, 0};
				int lastNumLines[4] = {0,0,0,0};

				static const float DATE_TO_DISTANCE = 12.f * 0.0254f;
				while(!f.eof())
				{
					char l[512];
					memset(l, 0, 512);
					f.getline(l, 512);
					if(count != 0)
					{
						Ogre::String sLine(l);
						if(sLine.length() > 0)
						{
							Ogre::vector<Ogre::String>::type strs = Ogre::StringUtil::tokenise(sLine, ",");
							Ogre::String date(strs[0]);
							Ogre::vector<Ogre::String>::type dateStrs = Ogre::StringUtil::tokenise(date, "_");
							//dateStrs[0] is year
							//dateStrs[1] is month
							//dateStrs[2] is day
							std::tm t;
							t.tm_year = Ogre::StringConverter::parseInt(dateStrs[0])-1900;
							t.tm_mon = Ogre::StringConverter::parseInt(dateStrs[1])-1;
							t.tm_mday = Ogre::StringConverter::parseInt(dateStrs[2]);
							t.tm_hour = 0;
							t.tm_isdst = -1;
							t.tm_min = 0;
							t.tm_sec = 0;

							std::time_t tVal = std::mktime(&t);
							tVal = tVal / 86400.0;
							if(firstDate == 0)
							{
								firstDate = tVal;
							}

							float zCoord = (tVal - firstDate) * DATE_TO_DISTANCE;
							zCoord = -zCoord;
							//strs[0] is the date
							//strs[1..size-2] is the event
							//strs[size-1] is the event type
							Ogre::String sEvent;
							for(unsigned int i = 1; i < strs.size()-1; ++i)
							{
								sEvent.append(strs[i]);
								if(i != strs.size()-2)
								{
									sEvent.append(",");
								}
							}

							int cIndex = 0;
							Ogre::Vector3 vec;
							Ogre::ColourValue c;
							if(strs[strs.size()-1].compare("Interaction") == 0)
							{
								vec.x = -EVENT_WIDTH * 0.0254f;
								vec.y = EVENT_HEIGHT * 0.0254f;
								vec.z = zCoord;
								c.a = 1.f;
								c.r = 1.f;
								c.g = 127.f/255.f;
								c.b = 0.f;
								cIndex = 3;
							}
							else if(strs[strs.size()-1].compare("Internal") == 0)
							{
								vec.x = -EVENT_WIDTH * 0.0254f;
								vec.y = -EVENT_HEIGHT * 0.0254f;
								vec.z = zCoord;	//this must correspond to the date.
								c.a = 1.f;
								c.r = 227.f/255.f;
								c.g = 26.f/255.f;
								c.b = 28.f/255.f;
								cIndex = 1;
							}
							else if(strs[strs.size()-1].compare("External") == 0)
							{
								vec.x = EVENT_WIDTH * 0.0254f;
								vec.y = EVENT_HEIGHT * 0.0254f;
								vec.z = zCoord;	//this must correspond to the date.
								c.a = 1.f;
								c.r = 51.f/255.f;
								c.g = 160.f/255.f;
								c.b = 44.f/255.f;
							}
							else if(strs[strs.size()-1].compare("Evaluation") == 0)
							{
								vec.x = EVENT_WIDTH * 0.0254f;
								vec.y = -EVENT_HEIGHT * 0.0254f;
								vec.z = zCoord;
								c.a = 1.f;
								c.r = 31.f/255.f;
								c.g = 120.f/255.f;
								c.b = 180.f/255.f;
								cIndex = 2;
							}
			
							
							if(lastDate == tVal)
							{
								lineCount[cIndex] = lineCount[cIndex] + lastNumLines[cIndex];
							}
							else
							{
								for(int k = 0; k < 4; ++k)
								{
									lineCount[k]=0;
									lastNumLines[k]=0;
								}
							}

							int lineLen = 30;
							int numLines = sEvent.length() / lineLen;
							lastNumLines[cIndex] = numLines+1;

							std::string sNewEvent;
							if(lineCount[cIndex] != 0)
							{
								for(int p = 0; p < lineCount[cIndex]; ++p)
								{
									sNewEvent.append("\n");
								}
							}
							//we need to put new line characters in sEvent..
							
							int charcount=0;
							
							sNewEvent.append(date);
							sNewEvent.append(": ");
							for(unsigned int i = 0; i < sEvent.length(); ++i)
							{
								if(sEvent[i] == ' ' && charcount >= lineLen)
								{
									sNewEvent.append("\n");
									charcount=0;
								}
								else
								{
									sNewEvent += sEvent[i];
								}
								charcount++;
							}

							char sName[256];
							memset(sName, 0, 256);
							sprintf(sName, "%s_%d", "event_", count);
							Ogre::String eventName(sName);
							Ogre::SceneNode* textNode = getScene()->getRootSceneNode()->createChildSceneNode();
							textNode->setPosition(vec);
							m_events.push_back(textNode);
							Ogre::MovableText *pText = new Ogre::MovableText(eventName.c_str(), sNewEvent.c_str(), "BlueHighway-8", 0.1f, c);
							//pText->setTextAlignment(Ogre::MovableText::H_CENTER, Ogre::MovableText::V_ABOVE);
							textNode->attachObject(pText);
							lastDate = tVal;
						}
					}
					count++;
				}

				f.close();
			}

			Ogre::String keyconcepts(m_sWordCakeDir);
			keyconcepts.append("wc_pilot_keyconcepts.csv");
			std::ifstream fk(keyconcepts.c_str());
			if(fk.is_open())
			{
				while(!fk.eof())
				{
					char l[512];
					memset(l, 0, 512);
					fk.getline(l, 512);

					Ogre::String sLine(l);
					if(sLine.length() > 0)
					{
						Ogre::vector<Ogre::String>::type strs = Ogre::StringUtil::tokenise(sLine, ",");
						Ogre::String concept(strs[1]);	//this is the concept
						if(concept == "interoperable platform")
						{
							m_keyConceptColors.push_back(0);
						}
						else if(concept == "flexible health information infrastructure")
						{
							m_keyConceptColors.push_back(1);
						}
						else if(concept == "substitutable apps")
						{
							m_keyConceptColors.push_back(2);
						}
						else if(concept == "mobile medical applications")
						{
							m_keyConceptColors.push_back(3);
						}
						else if(concept == "modular health technology")
						{
							m_keyConceptColors.push_back(4);
						}
						else
						{
							int k = 0;
						}
					}
				}
				fk.close();
			}
		}
	}
#endif
	Ogre::SceneManager::MovableObjectIterator iterator = getScene()->getMovableObjectIterator("Entity");
	while(iterator.hasMoreElements())
	{
		Ogre::Entity* e = static_cast<Ogre::Entity*>(iterator.getNext());
		const Ogre::String &sName = e->getName();
		
		if(sName.find("_twc") != std::string::npos)
		{
			const Ogre::Vector3 & vPos = e->getParentSceneNode()->getPosition();
			m_arcDepths.push_back(vPos.z);
		}
	}

	//we need to sort m_arcDepths at this point..
	//otherwise the color matching isn't going to work out..
	std::sort(m_arcDepths.begin(), m_arcDepths.end(), sortPred);

	float radius = m_cakeWidth*1.3f;
	m_arcDisplayList = glGenLists(1);
	glNewList(m_arcDisplayList, GL_COMPILE);
	glBegin(GL_LINE_LOOP);
	for (int i=0; i <= 360; i++)
	{
		float degInRad = (float)i * PI / 180.f;
		glVertex3f(cos(degInRad)*radius, sin(degInRad)*radius, 0.f);
	}
	glEnd();
	glEndList();
}

void WordCake::showEvents(bool bShow)
{
	m_bShowingEvents = !m_bShowingEvents;
	for(unsigned int i = 0; i < m_events.size(); ++i)
	{
		m_events[i]->setVisible(bShow);
	}
}

void WordCake::switchSMARTMaterials(void)
{
	m_bShowingSMART = !m_bShowingSMART;

	Ogre::SceneManager::MovableObjectIterator iterator = getScene()->getMovableObjectIterator("Entity");
	while(iterator.hasMoreElements())
	{
		Ogre::Entity* e = static_cast<Ogre::Entity*>(iterator.getNext());
		const Ogre::String &sName = e->getName();
		Ogre::vector<Ogre::String>::type strs = Ogre::StringUtil::tokenise(sName, ":");
		
		if(strs.size() > 0)
		{
			std::string datePost = strs[0];
			std::string sMore = strs[strs.size()-1];
			Ogre::vector<Ogre::String>::type sMatName = Ogre::StringUtil::tokenise(sMore, "_");	

			Ogre::String noSel("noselect");

			size_t foundTWC = sName.find("_twc");
			size_t foundCOF = sName.find("_cof");
			size_t foundTCN = sName.find("_tcn");

			//don't assign to any of the arc entity's
			if(foundTWC == std::string::npos && foundCOF == std::string::npos  && foundTCN == std::string::npos)
			{
				Ogre::Any any;
				const Ogre::String & sMat = e->getSubEntity(0)->getMaterialName();
				if(sMat != noSel && !m_bShowingSMART)
				{
					any = sMat;
					e->setUserAny(any);
				}

				if(!m_bShowingSMART)
				{
					e->setMaterialName(sMatName[0]);
				}
				else
				{
					const Ogre::Any& a = e->getUserAny();
					if(!a.isEmpty())
					{
						static_cast<Ogre::Entity*>(e)->setMaterialName(Ogre::any_cast<Ogre::String>(a));
					}
				}
			}
		}
	}
}

void WordCake::toggleSideView(void)
{
	m_bSideView = !m_bSideView;
}