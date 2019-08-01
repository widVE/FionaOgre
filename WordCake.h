#ifndef _WORD_CAKE_H_
#define _WORD_CAKE_H_

#include "FionaOgre.h"

class WordCake : public FionaOgre
{
public:

	WordCake() : FionaOgre(), m_numFiles(33), m_cakeWidth(DEFAULT_CAKE_WIDTH), m_cakeHeight(DEFAULT_CAKE_HEIGHT), m_bIsSmart(false), m_bShowingSMART(true), 
		m_bShowingEvents(true), m_bShowArcs(true), m_bSideView(false), m_arcDisplayList(0) {}
	virtual ~WordCake() {}

	void			addSegmentPoint(std::vector<Ogre::Vector3> vPointList) { m_segmentPoints.push_back(vPointList); }
	void			clearSegmentPoints();
	bool			isSideView(void) const { return m_bSideView; }
	bool			isSmart(void) const { return m_bIsSmart; }
	bool			isShowingEvents(void) const { return m_bShowingEvents; }
	bool			isShowingSMARTConcepts(void) const { return m_bShowingSMART; }
	virtual void	setupScene(Ogre::SceneManager* scene);
	void			showEvents(bool bShow);
	void			switchSMARTMaterials(void);
	void			toggleSideView(void);
	void			toggleShowArcs(void) { m_bShowArcs = !m_bShowArcs; }
	
protected:
	virtual void	openGLRender(void);

private:

	std::vector< std::vector<Ogre::Vector3> > m_segmentPoints;
	std::vector< unsigned int > m_keyConceptColors;
	static const unsigned int NUM_CONCEPTS = 5;
	static const jvec3 conceptColors[NUM_CONCEPTS];
	static const float EVENT_HEIGHT;
	static const float EVENT_WIDTH;
	std::vector<Ogre::SceneNode*>			m_events;
	std::vector<float>						m_arcDepths;

	bool			m_bIsSmart;
	bool			m_bShowingSMART;
	bool			m_bShowingEvents;
	bool			m_bShowArcs;
	bool			m_bSideView;

	std::string		m_sWordCakeDir;
	unsigned int	m_numFiles;
	float			m_cakeWidth;
	float			m_cakeHeight;

	GLuint			m_arcDisplayList;

	static const float DEFAULT_CAKE_WIDTH;
	static const float DEFAULT_CAKE_HEIGHT;
};

#endif
