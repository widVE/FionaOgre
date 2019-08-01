#include "SBWSPManager.h"
#if USE_WSP
#include "wsp.h"
#endif

namespace SmartBody {

SBWSPManager::SBWSPManager()
{
	_wsp = NULL;
	setName("World State Protocol");
}

SBWSPManager::~SBWSPManager()
{
#if USE_WSP
	if (_wsp)
		_wsp->shutdown();
#endif
}

void SBWSPManager::setEnable(bool val)
{
	SBService::setEnable(val);

#if USE_WSP
	if (val)
	{
		if (_wsp)
		{
			_wsp->shutdown();
			_wsp = NULL;
		}
		_wsp = WSP::create_manager();
		_wsp->init( "SMARTBODY" );
	}
	else
	{
		if (_wsp)
		{
			_wsp->shutdown();
			delete _wsp;
			_wsp = NULL;
		}
	}
#endif
}

bool SBWSPManager::isEnable()
{
	return SBService::isEnable();
}

int SBWSPManager::init( const std::string process_name )
{
#if USE_WSP
	if (_wsp)
		return _wsp->init(process_name);
#endif
	return 0;
}

int SBWSPManager::shutdown()
{
#if USE_WSP
	if (_wsp)
		return _wsp->shutdown();
#endif
	return 0;
}

void SBWSPManager::broadcastUpdate()
{
#if USE_WSP
	if (_wsp)
		_wsp->broadcast_update();
#endif
}

void SBWSPManager::processCommand( const char * message )
{
#if USE_WSP
#endif
}

}
