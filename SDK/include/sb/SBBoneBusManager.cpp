#include "SBBoneBusManager.h"
#include "vhcl.h"
#include <sb/SBAttribute.h>
#include <sb/SBSkeleton.h>
#include <sb/SBSimulationManager.h>
#include <sb/SBScene.h>

namespace SmartBody {

SBBoneBusManager::SBBoneBusManager()
{
	setName("BoneBus");
	_host = "";

	createStringAttribute("host", "", true, "", 10, false, false, false, "Host where the BoneBus data will be sent. If not set, will use 'localhost'");
}

SBBoneBusManager::~SBBoneBusManager()
{
}

#ifndef SB_NO_BONEBUS
bonebus::BoneBusClient& SBBoneBusManager::getBoneBus()
{
	return _boneBus;
}
#endif

void SBBoneBusManager::setEnable(bool val)
{
#ifdef SB_NO_BONEBUS
	LOG("Bonebus has been disabled and is not available.");
	return;
#else
	SBService::setEnable(val);

	if (val)
	{
		if (_boneBus.IsOpen())
		{
			LOG("Closing Bone Bus connection.");
			_boneBus.CloseConnection();
		}

		std::string host = this->getStringAttribute("host");
		if (host == "")
			host = "localhost";
		bool success = _boneBus.OpenConnection(host.c_str());
		if (!success)
		{
			LOG("Could not open Bone Bus connection to %s", host.c_str());
			SmartBody::BoolAttribute* enableAttribute = dynamic_cast<SmartBody::BoolAttribute*>(getAttribute("enable"));
			if (enableAttribute)
				enableAttribute->setValueFast(false);
			return;
		}
		else
		{
			LOG("Connected Bone Bus to %s", host.c_str());
		}

	}
	else
	{
		if (_boneBus.IsOpen())
		{
			LOG("Closing Bone Bus connection.");
			_boneBus.CloseConnection();
		}
	}

	SmartBody::BoolAttribute* enableAttribute = dynamic_cast<SmartBody::BoolAttribute*>(getAttribute("enable"));
	if (enableAttribute)
		enableAttribute->setValueFast(val);
#endif
}

void SBBoneBusManager::setHost(const std::string& host)
{
#ifdef SB_NO_BONEBUS
	LOG("Bonebus has been disabled and can not set the bonebus host.");
	return;
#endif
	_host = host;
	SmartBody::StringAttribute* hostAttribute = dynamic_cast<SmartBody::StringAttribute*>(getAttribute("host"));
	if (hostAttribute)
		hostAttribute->setValueFast(host);
}

const std::string& SBBoneBusManager::getHost()
{
	return _host;
}

void SBBoneBusManager::start()
{
}

void SBBoneBusManager::beforeUpdate(double time)
{
#ifdef SB_NO_BONEBUS
	return;
#else
	// process commands received over BoneBus protocol
	std::vector<std::string> commands = _boneBus.GetCommand();
	for ( size_t i = 0; i < commands.size(); i++ )
	{
		SmartBody::SBScene::getScene()->command( (char *)commands[i].c_str() );
	}
#endif
}

void SBBoneBusManager::update(double time)
{
}

void SBBoneBusManager::afterUpdate(double time)
{
}

void SBBoneBusManager::stop()
{
}

void SBBoneBusManager::notify(SBSubject* subject)
{
#ifdef SB_NO_BONEBUS
	return;
#endif

	SBService::notify(subject);

	SmartBody::SBAttribute* attribute = dynamic_cast<SmartBody::SBAttribute*>(subject);
	if (!attribute)
	{
		return;
	}

	const std::string& name = attribute->getName();
	if (name == "enable")
	{
		bool val = getBoolAttribute("enable");
		setEnable(val);
		return;
	}
	else if (name == "host")
	{
		setHost(getStringAttribute("host"));
		return;
	}

}

#ifndef SB_NO_BONEBUS
void SBBoneBusManager::NetworkSendSkeleton( bonebus::BoneBusCharacter * character, SmartBody::SBSkeleton* skeleton, GeneralParamMap * param_map )
{

	if ( character == NULL )
	{
		return;
	}

	// Send the bone rotation for each joint in the skeleton
	const std::vector<SkJoint *> & joints  = skeleton->joints();

	character->IncrementTime();
	character->StartSendBoneRotations();

	std::vector<int> otherJoints;

	for ( size_t i = 0; i < joints.size(); i++ )
	{
		SkJoint * j = joints[ i ];
		if (j->getJointType() != SkJoint::TypeJoint)
		{
			if (j->getJointType() == SkJoint::TypeOther)
				otherJoints.push_back(i); // collect the 'other' joins
			continue;
		}

		const SrQuat& q = j->quat()->value();

		character->AddBoneRotation( j->extName().c_str(), q.w, q.x, q.y, q.z, SmartBody::SBScene::getScene()->getSimulationManager()->getTime() );

		//printf( "%s %f %f %f %f\n", (const char *)j->name(), q.w, q.x, q.y, q.z );
	}

	character->EndSendBoneRotations();


	character->StartSendBonePositions();

	for ( size_t i = 0; i < joints.size(); i++ )
	{
		SkJoint * j = joints[ i ];
		if (j->getJointType() != SkJoint::TypeJoint)
			continue;

		float posx = j->pos()->value( 0 );
		float posy = j->pos()->value( 1 );
		float posz = j->pos()->value( 2 );
		if ( false )
		{
			posx += j->offset().x;
			posy += j->offset().y;
			posz += j->offset().z;
		}

		//these coordinates are meant to mimic the setpositionbyname coordinates you give to move the character
		//so if you wanted to move a joint on the face in the x direction you'd do whatever you did to move the actor
		//itself further in the x position.
		character->AddBonePosition( j->extName().c_str(), posx, posy, posz, SmartBody::SBScene::getScene()->getSimulationManager()->getTime() );
	}

	character->EndSendBonePositions();

	if (otherJoints.size() > 0)
	{
		character->StartSendGeneralParameters();
		for (size_t i = 0; i < otherJoints.size(); i++)
		{
			SkJoint* joint = joints[otherJoints[i]];
			character->AddGeneralParameters(i, 1, joint->pos()->value( 0 ), i, SmartBody::SBScene::getScene()->getSimulationManager()->getTime());

		}
		character->EndSendGeneralParameters();
	}
	

/*
	// Passing General Parameters
	character->StartSendGeneralParameters();
<<<<<<< .mine
	for (size_t i = 0; i < joints.size(); i++)
=======
	int numFound = 0;
	for (int i = 0; i < joints.size(); i++)
>>>>>>> .r2317
	{
		SkJoint* j = joints[ i ];
		if (j->getJointType() != SkJoint::TypeOther)
			continue;

		// judge whether it is joint for general parameters, usually should have a prefix as "param"
		string j_name = j->name();
		int name_end_pos = j_name.find_first_of("_");
		string test_prefix = j_name.substr( 0, name_end_pos );
		if( test_prefix == character->m_name )	
		{
			// if is, prepare adding data
			int index = 0;
			GeneralParamMap::iterator pos;
			for(pos = param_map->begin(); pos != param_map->end(); pos++)
			{
				for(int n = 0; n < (int)pos->second->char_names.size(); n++)
				{
					if( character->m_name == pos->second->char_names[n] )
					{
						index ++;
						for(int m = 0 ; m < pos->second->size; m++)
						{
							std::stringstream joint_name;
							joint_name << character->m_name << "_" << index << "_" << ( m + 1 );
							if(_stricmp( j->name().c_str(), joint_name.str().c_str()) == 0)
								character->AddGeneralParameters(index, pos->second->size, j->pos()->value(0), m, time);
						}
					}
				}
			}
		}
	}
	character->EndSendGeneralParameters();
*/
	
}
#endif


}


