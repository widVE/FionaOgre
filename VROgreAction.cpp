#include "VROgreAction.h"

#include "FionaOgre.h"
#include "MovableText.h"
#include "WordCake.h"

void VROChangeTSR::ButtonUp(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
	wbScene->changeTSR();
}

void VRODelete::ButtonUp(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
	if(wbScene->numSelected() > 0)
	{
		Ogre::MovableObject *pObj = wbScene->getSelected(0);
		if(pObj)
		{
			//remove object from selection..
			wbScene->clearSelection();
			Ogre::SceneNode *pParent = pObj->getParentSceneNode();
			pParent->detachObject(pObj);
			wbScene->getScene()->destroyEntity(pObj->getName());
			wbScene->getScene()->destroySceneNode(pParent->getName());
            // Play a sound!
            /*if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
                wbScene->mSoundManager->getSound("delete")->play();
            }*/
		}
	}
}

void VRODuplicate::ButtonUp(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
	if(wbScene->numSelected() > 0)
	{
		Ogre::MovableObject *pObj = wbScene->getSelected(0);
		if(pObj)
		{
			Ogre::MeshPtr p = static_cast<Ogre::Entity*>(pObj)->getMesh();
			static int dCount = 0;
			Ogre::String sName = pObj->getName();
			char newName[256];
			memset(newName, 0, 256);
			//todo - fix up this naming..
			sprintf(newName, "%s%d", sName.c_str(), dCount);
			dCount++;
			
			const Ogre::Vector3 & vPos = pObj->getParentSceneNode()->getPosition();
			float fXMove = pObj->getBoundingBox().getSize().x;
			Ogre::SceneNode* headNode = wbScene->getScene()->getRootSceneNode()->createChildSceneNode();
			headNode->setPosition(vPos.x + fXMove, vPos.y, vPos.z);
			const Ogre::Vector3 &vScale = pObj->getParentSceneNode()->getScale();
			const Ogre::Quaternion &qRot = pObj->getParentSceneNode()->getOrientation();
			headNode->setScale(vScale.x, vScale.y, vScale.z);
			headNode->setOrientation(qRot.w, qRot.x, qRot.y, qRot.z);
			headNode->attachObject(wbScene->getScene()->createEntity(newName, p));
            // Play a sound!
            /*if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
                wbScene->mSoundManager->getSound("duplicate")->play();
            }*/
		}
	}
}

void VRORotate::ButtonDown(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	m_bDoRotate = false;
	
	if(wbScene->numSelected() > 0)
	{
		float fDistance = 0.f;

		Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDistance);
		if(pObj)
		{
			jvec3 vPos;
			wbScene->getWandWorldSpace(vPos, true);
			//this correctly orients the direction of fire..
			jvec3 vWandDir;
			wbScene->getWandDirWorldSpace(vWandDir, true);
			jvec3 vNewPos = vPos + vWandDir * fDistance;

			m_fSelectionDistance = fDistance;
			Ogre::Vector3 vIntersectPos(vNewPos.x, vNewPos.y, vNewPos.z);
			Ogre::Vector3 vec = pObj->getParentSceneNode()->getPosition();
			Ogre::Vector3 vOffset = vec - vIntersectPos;
			m_vOffset.set(vOffset.x, vOffset.y, vOffset.z);
			m_bDoRotate = true;
		}
	}
}

void VRORotate::ButtonUp(void)
{
	m_bDoRotate = false;
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f ,0.f, 0.f);
}

void VRORotate::WandMove(void)
{
	if(m_bDoRotate)
	{
		//todo - eventually handle multiple selection..
		FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				jvec3 vPos;
				wbScene->getWandWorldSpace(vPos, true);
				//this correctly orients the direction of fire..
				jvec3 vWandDir;
				wbScene->getWandDirWorldSpace(vWandDir, true);

				jvec3 vNewPos = m_vOffset + vPos + vWandDir * m_fSelectionDistance;
				pObj->getParentSceneNode()->setPosition(vNewPos.x, vNewPos.y, vNewPos.z);
			}
		}
	}
}

void VRORotate::JoystickMove(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	if(wbScene->numSelected() > 0)
	{
		Ogre::MovableObject *pObj = wbScene->getSelected(0);
		if(pObj)
		{
			const Ogre::Quaternion &qRot = pObj->getParentSceneNode()->getOrientation();
			Ogre::Quaternion qMult = Ogre::Quaternion::IDENTITY;
			if(fionaConf.currentJoystick.x < 0.f)
			{
				qMult.FromAngleAxis(Ogre::Radian::Radian(-0.05f), Ogre::Vector3::UNIT_Y);
			}
			else if(fionaConf.currentJoystick.x > 0.f)
			{
				qMult.FromAngleAxis(Ogre::Radian::Radian(0.05f), Ogre::Vector3::UNIT_Y);
			}
			else if(fionaConf.currentJoystick.z < 0.f)
			{
				qMult.FromAngleAxis(Ogre::Radian::Radian(-0.05f), Ogre::Vector3::UNIT_X);
			}
			else if(fionaConf.currentJoystick.z > 0.f)
			{
				qMult.FromAngleAxis(Ogre::Radian::Radian(0.05f), Ogre::Vector3::UNIT_X);
			}
			Ogre::Quaternion qNewRot = qRot * qMult;
			pObj->getParentSceneNode()->setOrientation(qNewRot);
		}
	}
}

void VROSelect::ButtonUp(void)
{
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	m_bDoTranslate = false;
}

void VROSelect::ButtonDown(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	m_bDoTranslate = false;
	
	float fDistance = 0.f;

	Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDistance);
	if(pObj)
	{
		jvec3 vPos;
		wbScene->getWandWorldSpace(vPos, true);
		//this correctly orients the direction of fire..
		jvec3 vWandDir;
		wbScene->getWandDirWorldSpace(vWandDir, true);
		jvec3 vNewPos = vPos + vWandDir * fDistance;

		m_fSelectionDistance = fDistance;
		Ogre::Vector3 vIntersectPos(vNewPos.x, vNewPos.y, vNewPos.z);
		Ogre::Vector3 vec = pObj->getParentSceneNode()->getPosition();
		Ogre::Vector3 vOffset = vec - vIntersectPos;
		m_vOffset.set(vOffset.x, vOffset.y, vOffset.z);
		m_bDoTranslate = true;
	}
}

void VROSelect::WandMove(void)
{
	if(m_bDoTranslate)
	{
		//todo - eventually handle multiple selection..
		FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				jvec3 vPos;
				wbScene->getWandWorldSpace(vPos, true);
				//this correctly orients the direction of fire..
				jvec3 vWandDir;
				wbScene->getWandDirWorldSpace(vWandDir, true);
				vWandDir = vWandDir.normalize();

				jvec3 vWandUp;
				wbScene->getWandDirWorldSpace(vWandUp, false);
				vWandUp = vWandUp.normalize();

				jvec3 vWandLeft = vWandDir * vWandUp;
				vWandLeft = vWandLeft.normalize();

				/*Ogre::Matrix3 mat;
				mat.SetColumn(0, Ogre::Vector3(vWandDir.x, vWandDir.y, vWandDir.z));
				mat.SetColumn(1, Ogre::Vector3(vWandUp.x, vWandUp.y, vWandUp.z));
				mat.SetColumn(2, Ogre::Vector3(vWandLeft.x, vWandLeft.y, vWandLeft.z));*/

				jvec3 vNewPos = m_vOffset + (vPos + vWandDir * m_fSelectionDistance);
				pObj->getParentSceneNode()->setPosition(vNewPos.x, vNewPos.y, vNewPos.z);
				//pObj->getParentSceneNode()->setOrientation(mat);
			}
		}
	}
}

void VROSelect::JoystickMove(void)
{
	if(m_bDoTranslate)
	{
		FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				if(wbScene->getTSR() == FionaOgre::TRANSLATE)
				{
					m_fSelectionDistance += (fionaConf.currentJoystick.z * 0.1f);
				}
				else if(wbScene->getTSR() == FionaOgre::SCALE)
				{
					Ogre::MovableObject *pObj = wbScene->getSelected(0);
					if(pObj)
					{
						Ogre::Vector3 vScale = pObj->getParentSceneNode()->getScale();
						//printf("curr joystick: %f, %f\n", fionaConf.currentJoystick.x, fionaConf.currentJoystick.z);
						if(fionaConf.currentJoystick.x > 0.f)
						{
							vScale.x += 0.005f;
							vScale.y += 0.005f;
							vScale.z += 0.005f;
							pObj->getParentSceneNode()->setScale(vScale);
						}
						else if(fionaConf.currentJoystick.x < 0.f)
						{
							vScale.x -= 0.005f;
							vScale.y -= 0.005f;
							vScale.z -= 0.005f;
							pObj->getParentSceneNode()->setScale(vScale);
						}
					}
				}
				else if(wbScene->getTSR() == FionaOgre::ROTATE)
				{
					Ogre::MovableObject *pObj = wbScene->getSelected(0);
					if(pObj)
					{
						const Ogre::Quaternion &qRot = pObj->getParentSceneNode()->getOrientation();
						Ogre::Quaternion qMult = Ogre::Quaternion::IDENTITY;
						if(fionaConf.currentJoystick.x < 0.f)
						{
							qMult.FromAngleAxis(Ogre::Radian::Radian(-0.05f), Ogre::Vector3::UNIT_Y);
						}
						else if(fionaConf.currentJoystick.x > 0.f)
						{
							qMult.FromAngleAxis(Ogre::Radian::Radian(0.05f), Ogre::Vector3::UNIT_Y);
						}
						Ogre::Quaternion qNewRot = qRot * qMult;
						pObj->getParentSceneNode()->setOrientation(qNewRot);
					}
				}
			}
		}
	}
}

void VROSave::ButtonUp(void)
{
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) 
	{
		FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
		wbScene->saveScene();
	}
}

void VROScale::ButtonDown(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	m_bDoScale = false;
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	
	float fDist = 0.f;
	Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDist);
	if(pObj)
	{
		jvec3 vPos;
		wbScene->getWandWorldSpace(vPos, true);
		//this correctly orients the direction of fire..
		jvec3 vWandDir;
		wbScene->getWandDirWorldSpace(vWandDir, true);

		m_fSelectionDistance = fDist;
		jvec3 vNewPos = vPos + vWandDir * fDist;
		Ogre::Vector3 vIntersectPos(vNewPos.x, vNewPos.y, vNewPos.z);
		Ogre::Vector3 vec = pObj->getParentSceneNode()->getPosition();
		Ogre::Vector3 vOffset = vec - vIntersectPos;
		m_vOffset.set(vOffset.x, vOffset.y, vOffset.z);
		m_bDoScale = true;
	}
}

void VROScale::ButtonUp(void)
{
	m_bDoScale = false;
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
}

void VROScale::WandMove(void)
{
	if(m_bDoScale)
	{
		//todo - eventually handle multiple selection..
		FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				jvec3 vPos;
				wbScene->getWandWorldSpace(vPos, true);
				//this correctly orients the direction of fire..
				jvec3 vWandDir;
				wbScene->getWandDirWorldSpace(vWandDir, true);

				jvec3 vNewPos = m_vOffset + vPos + vWandDir * m_fSelectionDistance;
				pObj->getParentSceneNode()->setPosition(vNewPos.x, vNewPos.y, vNewPos.z);
			}
		}
	}
}

void VROScale::JoystickMove(void)
{
	if(m_bDoScale)
	{
		FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
		Ogre::MovableObject *pObj = wbScene->getSelected(0);
		if(pObj)
		{
			Ogre::Vector3 vScale = pObj->getParentSceneNode()->getScale();
			//printf("curr joystick: %f, %f\n", fionaConf.currentJoystick.x, fionaConf.currentJoystick.z);
			if(fionaConf.currentJoystick.x > 0.f)
			{
				vScale.x += 0.005f;
				vScale.y += 0.005f;
				vScale.z += 0.005f;
				pObj->getParentSceneNode()->setScale(vScale);
			}
			else if(fionaConf.currentJoystick.x < 0.f)
			{
				vScale.x -= 0.005f;
				vScale.y -= 0.005f;
				vScale.z -= 0.005f;
				pObj->getParentSceneNode()->setScale(vScale);
			}
		}
	}
}

void VROTranslate::ButtonDown(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	m_bDoTranslate = false;
	
	if(wbScene->numSelected() > 0)
	{
		float fDistance = 0.f;

		Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDistance);
		if(pObj)
		{
			jvec3 vPos;
			wbScene->getWandWorldSpace(vPos, true);
			//this correctly orients the direction of fire..
			jvec3 vWandDir;
			wbScene->getWandDirWorldSpace(vWandDir, true);
			jvec3 vNewPos = vPos + vWandDir * fDistance;

			m_fSelectionDistance = fDistance;
			Ogre::Vector3 vIntersectPos(vNewPos.x, vNewPos.y, vNewPos.z);
			Ogre::Vector3 vec = pObj->getParentSceneNode()->getPosition();
			Ogre::Vector3 vOffset = vec - vIntersectPos;
			m_vOffset.set(vOffset.x, vOffset.y, vOffset.z);
			m_bDoTranslate = true;
		}
	}
}

void VROTranslate::WandMove(void)
{
	if(m_bDoTranslate)
	{
		//todo - eventually handle multiple selection..
		FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				jvec3 vPos;
				wbScene->getWandWorldSpace(vPos, true);
				//this correctly orients the direction of fire..
				jvec3 vWandDir;
				wbScene->getWandDirWorldSpace(vWandDir, true);

				jvec3 vNewPos = m_vOffset + (vPos + vWandDir * m_fSelectionDistance);
				pObj->getParentSceneNode()->setPosition(vNewPos.x, vNewPos.y, vNewPos.z);
			}
		}
	}
}

void VROTranslate::JoystickMove(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
	if(wbScene->numSelected() > 0)
	{
		m_fSelectionDistance += (fionaConf.currentJoystick.z * 0.1f);
	}
}

void VROTranslate::ButtonUp(void)
{
	m_bDoTranslate = false;
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
}


void VROTSR::ButtonDown(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	if(wbScene->getTSR() == FionaOgre::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.ButtonDown();
	}
	else if(wbScene->getTSR() == FionaOgre::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.ButtonDown();
	}
	else if(wbScene->getTSR() == FionaOgre::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.ButtonDown();
	}
}

void VROTSR::WandMove(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	if(wbScene->getTSR() == FionaOgre::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.WandMove();
	}
	else if(wbScene->getTSR() == FionaOgre::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.WandMove();
	}
	else if(wbScene->getTSR() == FionaOgre::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.WandMove();
	}
}

void VROTSR::ButtonUp(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	if(wbScene->getTSR() == FionaOgre::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.ButtonUp();
	}
	else if(wbScene->getTSR() == FionaOgre::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.ButtonUp();
	}
	else if(wbScene->getTSR() == FionaOgre::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.ButtonUp();
	}
}

void VROTSR::JoystickMove(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	if(wbScene->getTSR() == FionaOgre::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.JoystickMove();
	}
	else if(wbScene->getTSR() == FionaOgre::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.JoystickMove();
	}
	else if(wbScene->getTSR() == FionaOgre::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.JoystickMove();
	}
}

void VROTSR::DrawCallback(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	if(wbScene->getTSR() == FionaOgre::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.DrawCallback();
	}
	else if(wbScene->getTSR() == FionaOgre::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.DrawCallback();
	}
	else if(wbScene->getTSR() == FionaOgre::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.DrawCallback();
	}
}


void VROVizSelect::ButtonUp(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	float fDistance = 0.f;

	Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDistance);

	if(pObj)
	{
		DeselectAll();

		//run through the list of objects and check what other ogre names begin with the name of the selected object
		//for those that don't start with it, change their material to a transparent grey material...
		const Ogre::String &sName = pObj->getName();
		Ogre::vector<Ogre::String>::type strs = Ogre::StringUtil::tokenise(sName, "_");
		
		if(strs.size() > 0)
		{
			Ogre::SceneManager::MovableObjectIterator iterator = wbScene->getScene()->getMovableObjectIterator("Entity");
			while(iterator.hasMoreElements())
			{
				  Ogre::Entity* e = static_cast<Ogre::Entity*>(iterator.getNext());
				  if(!Ogre::StringUtil::startsWith(e->getName(), strs[0]))
				  {
					e->setMaterialName("noselect");
				  }
				  else
				  {
					  e->getParentSceneNode()->showBoundingBox(true);
					  wbScene->addSelection(e);
				  }
			}
		}
	}
	else
	{
		//change all materials back..
		DeselectAll();
	}
}

void VROVizSelect::DeselectAll(void)
{
	//this function shouldn't always assume word cake..
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);
	
	int numSelected = wbScene->numSelected();
	if(numSelected > 0)
	{
		for(int i = 0; i < numSelected; ++i)
		{
			Ogre::MovableObject * e= wbScene->getSelected(i);
			const Ogre::String &sName = e->getName();
			Ogre::vector<Ogre::String>::type strs = Ogre::StringUtil::tokenise(sName, "_");
			static_cast<Ogre::Entity*>(e)->setMaterialName(strs[0]);
			e->getParentSceneNode()->showBoundingBox(false);

		}
	}
}

void VROWCVizSelect::ButtonUp(void)
{
	FionaOgre *wbScene = static_cast<FionaOgre*>(m_scene);

	float fDistance = 0.f;

	Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDistance, false);

	if(pObj)
	{
		//run through the list of objects and check what other ogre names begin with the name of the selected object
		//for those that don't start with it, change their material to a transparent grey material...
		const Ogre::String &sSelName = pObj->getName();
		size_t foundTWC = sSelName.find("_twc");
		size_t foundCOF = sSelName.find("_cof");
		size_t foundTCN = sSelName.find("_tcn");
		if(foundTWC == std::string::npos && foundCOF == std::string::npos  && foundTCN == std::string::npos)
		{
			Ogre::vector<Ogre::String>::type strs = Ogre::StringUtil::tokenise(sSelName, ":");

			if(strs.size() > 0)
			{
				std::string datePost = strs[0];
				std::string sMore = strs[strs.size()-1];
				Ogre::vector<Ogre::String>::type sMatName = Ogre::StringUtil::tokenise(sMore, "_");
				Ogre::String noSel("noselect");
				Ogre::SceneManager::MovableObjectIterator iterator = wbScene->getScene()->getMovableObjectIterator("Entity");
				std::vector<Ogre::Vector3> vPointList;
				while(iterator.hasMoreElements())
				{
					Ogre::Entity* e = static_cast<Ogre::Entity*>(iterator.getNext());
				
					const Ogre::String & sName = e->getName();

					if(sName.find("_textNode") == std::string::npos)
					{
						bool found = true;
						size_t result = sName.find(sMatName[0].c_str());
						found = ((result != std::string::npos));// && (result==1));
						//if the name is found, we want to ideally perform an exact match..
						if(found)
						{
							Ogre::vector<Ogre::String>::type exactMatch = Ogre::StringUtil::tokenise(sName, ":");
							Ogre::vector<Ogre::String>::type sExactName = Ogre::StringUtil::tokenise(exactMatch[exactMatch.size()-1], "_");
							if(sExactName[0]==sMatName[0])
							{
								found = true;
							}
							else
							{
								found=false;
							}
						}

						if(!found && (pObj != e))
						{
							if(!wbScene->isSelected(e))
							{
								size_t foundTWC = sName.find("_twc");
								size_t foundCOF = sName.find("_cof");
								size_t foundTCN = sName.find("_tcn");

								//don't assign to any of the arc entity's
								if(foundTWC == std::string::npos && foundCOF == std::string::npos  && foundTCN == std::string::npos)
								{
									Ogre::Any any;
									const Ogre::String & sMat = e->getSubEntity(0)->getMaterialName();
									if(sMat != noSel)
									{
										any = sMat;
										e->setUserAny(any);
									}
									e->setMaterialName(noSel);
								}
							}
						}
						else
						{
							//this fixes the crash, but then the selected object doesn't get it's data added..
							if(!wbScene->isSelected(e) || (pObj == e))
							{
								//printf("%d\n", result);
								e->getParentSceneNode()->showBoundingBox(true);
								Ogre::vector<Ogre::String>::type sep = Ogre::StringUtil::tokenise(e->getName(), ":");
								//also add a movable text node that has the number of entries for this item..
								std::string name(e->getName());
								name.append("_textNode");
					  
								Ogre::SceneNode* textNode = wbScene->getScene()->getRootSceneNode()->createChildSceneNode(name);
								Ogre::Vector3 vPos =  e->getParentSceneNode()->getPosition() + e->getBoundingBox().getCenter();
								vPointList.push_back(vPos);
								//static_cast<WordCake*>(wbScene)->addSegmentPoint(vPos);
								vPos.z = vPos.z + 0.7f;
								vPos.y = vPos.y - 0.3f;
								vPos.x = vPos.x + 0.35f;
								textNode->setPosition(vPos);
					 
								name.append("_movable");

								Ogre::MovableText *pText = new Ogre::MovableText(name.c_str(), sep[1].c_str(), "BlueHighway-8", 0.1f, Ogre::ColourValue::Green);
								textNode->attachObject(pText);

								const Ogre::String & sMat = e->getSubEntity(0)->getMaterialName();
								if(sMat == noSel)
								{
									const Ogre::Any& a = e->getUserAny();
									if(!a.isEmpty())
									{
										static_cast<Ogre::Entity*>(e)->setMaterialName(Ogre::any_cast<Ogre::String>(a));
									}
								}
					
								wbScene->addSelection(e);
							}
						} 
					}
				}

				if(vPointList.size() > 0)
				{
					static_cast<WordCake*>(wbScene)->addSegmentPoint(vPointList);
				}
			}
		}
	}
	else
	{
		if(fDistance == 0.f)
		{
			static_cast<WordCake*>(wbScene)->clearSegmentPoints();
			//change all materials back..
			DeselectAll();
			wbScene->clearSelection();
		}
	}
}

void VROWCVizSelect::DeselectAll(void)
{
	//this function shouldn't always assume word cake..
	WordCake *wbScene = static_cast<WordCake*>(m_scene);
	wbScene->clearSegmentPoints();
	int numSelected = wbScene->numSelected();
	if(numSelected > 0)
	{
		for(int i = 0; i < numSelected; ++i)
		{
			Ogre::MovableObject * e= wbScene->getSelected(i);
			Ogre::String sName = e->getName();
			Ogre::vector<Ogre::String>::type strs = Ogre::StringUtil::tokenise(sName, ":");
			std::string datePost = strs[0];
			std::string sMore = strs[strs.size()-1];
			Ogre::vector<Ogre::String>::type sMatName = Ogre::StringUtil::tokenise(sMore, "_");

			if(wbScene->isShowingSMARTConcepts())
			{
				const Ogre::Any& a = e->getUserAny();
				if(!a.isEmpty())
				{
					static_cast<Ogre::Entity*>(e)->setMaterialName(Ogre::any_cast<Ogre::String>(a));
				}
			}
			else
			{
				static_cast<Ogre::Entity*>(e)->setMaterialName(sMatName[0]);
			}

			e->getParentSceneNode()->showBoundingBox(false);

			Ogre::String movableText(sName);
			movableText.append("_textNode");
			Ogre::String sText(movableText);
			sText.append("_movable");
		
			try 
			{
				if(wbScene->getScene()->hasSceneNode(movableText))
				{
					Ogre::SceneNode *pNode = (Ogre::SceneNode*)wbScene->getScene()->getRootSceneNode()->getChild(movableText);
					pNode->detachObject(sText);
					wbScene->getScene()->destroyEntity(sText);
					wbScene->getScene()->getRootSceneNode()->removeChild(pNode);
					wbScene->getScene()->destroySceneNode(pNode);
				}
			} 
			catch (Ogre::ItemIdentityException entExcep)
			{
				printf("get child exception\n");
			}
		}
	}
	
	Ogre::SceneManager::MovableObjectIterator iterator = wbScene->getScene()->getMovableObjectIterator("Entity");
	while(iterator.hasMoreElements())
	{
		Ogre::Entity* e = static_cast<Ogre::Entity*>(iterator.getNext());
		Ogre::String sName = e->getName();
		Ogre::vector<Ogre::String>::type strs = Ogre::StringUtil::tokenise(sName, ":");
		std::string datePost = strs[0];
		std::string sMore = strs[strs.size()-1];
		Ogre::vector<Ogre::String>::type sMatName = Ogre::StringUtil::tokenise(sMore, "_");
		if(wbScene->isShowingSMARTConcepts())
		{
			const Ogre::Any& a = e->getUserAny();
			if(!a.isEmpty())
			{
				static_cast<Ogre::Entity*>(e)->setMaterialName(Ogre::any_cast<Ogre::String>(a));
			}
		}
		else
		{
			static_cast<Ogre::Entity*>(e)->setMaterialName(sMatName[0]);
		}
	}
}


void VROSwitchVizMat::ButtonUp(void)
{
	WordCake *wbScene = static_cast<WordCake*>(m_scene);
	wbScene->switchSMARTMaterials();
}

 void VROWCToggleEvents::ButtonUp(void)
 {
	 WordCake *wbScene = static_cast<WordCake*>(m_scene);
	 wbScene->showEvents(!wbScene->isShowingEvents());
 }

void VROWCToggleArcs::ButtonUp(void)
{
	 WordCake *wbScene = static_cast<WordCake*>(m_scene);
	 wbScene->toggleShowArcs();
}

void VROWCFollowPath::ButtonUp(void)
{
	 WordCake *wbScene = static_cast<WordCake*>(m_scene);

}

void VROWCSideView::ButtonUp(void)
{
	 WordCake *wbScene = static_cast<WordCake*>(m_scene);
	 wbScene->toggleSideView();
	 if(wbScene->isSideView())
	 {
		 previousPosition = fionaConf.camPos;
		 previousOrientation = fionaConf.camRot;
		 fionaConf.camPos = jvec3(50.f, fionaConf.camPos.y, -50.f);
		 fionaConf.camRot = r2q(90.f * PI / 180.f, jvec3(0.f, 1.f, 0.f));
	 }
	 else
	 {
		 fionaConf.camPos = previousPosition;
		 fionaConf.camRot = previousOrientation;
	 }
}