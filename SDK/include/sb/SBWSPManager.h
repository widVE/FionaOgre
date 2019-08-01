#ifndef _SBWSPMANAGER_H_
#define _SBWSPMANAGER_H_

#include <sb/SBTypes.h>
#include <sb/SBService.h>
#include <string>

#if USE_WSP
namespace WSP {
	class Manager;
}
#endif

namespace SmartBody {

class SBWSPManager : public SBService
{
	public:
		SBAPI SBWSPManager();
		SBAPI ~SBWSPManager();

		SBAPI void setEnable(bool val);
		SBAPI bool isEnable();

		SBAPI virtual int init( const std::string process_name );
		SBAPI virtual int shutdown();
		SBAPI virtual void broadcastUpdate();
		SBAPI virtual void processCommand( const char * message );

	protected:
#if USE_WSP
		WSP::Manager* _wsp;
#endif
};

}


#endif