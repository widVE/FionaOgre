
#include "vhcl.h"

#include "smartbody-c-dll.h"


#include <fstream>
#include <ios>
#include <string.h>
#include <map>
#include "smartbody-dll.h"


using std::string;


struct SBM_CallbackInfo
{
    string name;
    string objectClass;
    string visemeName;
    float weight;
    float blendTime;
    string logMessage;
    int logMessageType;

    SBM_CallbackInfo() : weight(0), blendTime(0) {}	
    void operator = ( const SBM_CallbackInfo &l )
    { name = l.name;
      objectClass = l.objectClass;
      visemeName = l.visemeName;
      weight = l.weight;
      blendTime = l.blendTime;
      logMessage = l.logMessage;
      logMessageType = l.logMessageType;
    }
};


std::map< int, std::vector<SBM_CallbackInfo> > g_CreateCallbackInfo;
std::map< int, std::vector<SBM_CallbackInfo> > g_DeleteCallbackInfo;
std::map< int, std::vector<SBM_CallbackInfo> > g_ChangeCallbackInfo;
std::map< int, std::vector<SBM_CallbackInfo> > g_VisemeCallbackInfo;
std::map< int, std::vector<SBM_CallbackInfo> > g_ChannelCallbackInfo;
std::map< int, std::vector<SBM_CallbackInfo> > g_LogCallbackInfo;

class LogMessageListener : public vhcl::Log::Listener
{
public:
   LogMessageListener() {}
   ~LogMessageListener() {}

   virtual void OnMessage( const std::string & message )
   {
      int messageType = 0;
      if (message.find("WARNING") != std::string::npos)
      {
         messageType = 2;
      }
      else if (message.find("ERROR") != std::string::npos)
      {
         messageType = 1;
      }
      SBM_LogMessage(message.c_str(), messageType);

#if defined(IPHONE_BUILD)
      SBM_CallbackInfo info;
      info.logMessage = message;
      info.logMessageType = messageType;

      //g_LogCallbackInfo[m_sbmHandle].push_back(info);
       g_LogCallbackInfo[0].push_back(info);
#endif
   }
};

LogMessageListener* g_pLogMessageListener = NULL;
LogMessageCallback g_LogMessageFunc = NULL;


SBAPI bool SBM_SetLogMessageCallback(LogMessageCallback cb)
{
   g_LogMessageFunc = cb;

   if (g_pLogMessageListener == NULL)
   {
      g_pLogMessageListener = new LogMessageListener();
      vhcl::Log::g_log.AddListener(g_pLogMessageListener);
      return true;
   }

   return false;
}


SBAPI void SBM_LogMessage(const char* message, int messageType)
{
   // 0 = normal, 1 = error, 2 = warning
   if (g_LogMessageFunc)
   {
      g_LogMessageFunc(message, messageType);
   }
}

class SBM_SmartbodyListener : public SmartbodyListener
{
private:
   SBMHANDLE m_sbmHandle;
   SBM_OnCreateCharacterCallback m_createCharacterCallback;
   SBM_OnCharacterDeleteCallback m_deleteCharacterCallback;
   SBM_OnCharacterChangeCallback m_changeCharacterCallback;
   SBM_OnVisemeCallback m_viseme;
   SBM_OnChannelCallback m_channel;

public:
   SBM_SmartbodyListener( SBMHANDLE sbmHandle, SBM_OnCreateCharacterCallback createCharCallback, SBM_OnCharacterDeleteCallback deleteCharCallback, SBM_OnCharacterChangeCallback changeCharCallback, SBM_OnVisemeCallback visemeCallback, SBM_OnChannelCallback channelCallback )
   {
      m_sbmHandle = sbmHandle;
      m_createCharacterCallback = createCharCallback;
      m_deleteCharacterCallback = deleteCharCallback;
      m_changeCharacterCallback = changeCharCallback;
      m_viseme = visemeCallback;
      m_channel = channelCallback;
   }

   virtual void OnCharacterCreate( const std::string & name, const std::string & objectClass )
   {
#if !defined(IPHONE_BUILD) && !defined(ANDROID_BUILD)
      m_createCharacterCallback( m_sbmHandle, name.c_str(), objectClass.c_str() );
#else
      SBM_CallbackInfo info;
      info.name = name;
      info.objectClass = objectClass;
      g_CreateCallbackInfo[m_sbmHandle].push_back(info);
      //LOG("smartbody-c-dll : OnCharacterCreate, name = %s, objectClass = %s, number of callback info = %d",name.c_str(), objectClass.c_str(), g_CreateCallbackInfo[m_sbmHandle].size());
#endif
   }

   virtual void OnCharacterDelete( const std::string & name )
   {
#if !defined(IPHONE_BUILD) && !defined(ANDROID_BUILD)
      m_deleteCharacterCallback( m_sbmHandle, name.c_str() );
#else
      SBM_CallbackInfo info;
      info.name = name;
      g_DeleteCallbackInfo[m_sbmHandle].push_back(info);
#endif
   }

   virtual void OnViseme( const std::string & name, const std::string & visemeName, const float weight, const float blendTime )
   {
#if !defined(IPHONE_BUILD) && !defined(ANDROID_BUILD)
      m_viseme( m_sbmHandle, name.c_str(), visemeName.c_str(), weight, blendTime );
#else
      SBM_CallbackInfo info;
      info.name = name;
      info.visemeName = visemeName;
      info.weight = weight;
      info.blendTime = blendTime;
      g_VisemeCallbackInfo[m_sbmHandle].push_back(info);
#endif
   }

   virtual void OnChannel( const std::string & name, const std::string & channelName, const float value )
   {
#if !defined(IPHONE_BUILD) && !defined(ANDROID_BUILD)
      m_channel( m_sbmHandle, name.c_str(), channelName.c_str(), value );
#else
      SBM_CallbackInfo info;
      info.name = name;
      info.visemeName = channelName;
      info.weight = value;
      g_ChannelCallbackInfo[m_sbmHandle].push_back(info);
#endif
   }
};


bool SBM_ReleaseCharacterJoints( SBM_SmartbodyCharacter * character );
bool SBM_HandleExists( SBMHANDLE sbmHandle );
void SBM_CharToCSbmChar( const SmartbodyCharacter * sbmChar, SBM_SmartbodyCharacter * sbmCChar );


std::map< int, Smartbody_dll * > g_smartbodyInstances;
int g_handleId_DLL = 0;


SBAPI SBMHANDLE SBM_CreateSBM()
{
   g_handleId_DLL++;
   g_smartbodyInstances[ g_handleId_DLL ] = new Smartbody_dll();
   return g_handleId_DLL;
}


SBAPI bool SBM_SetSpeechAudiofileBasePath( SBMHANDLE sbmHandle, const char * basePath )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   g_smartbodyInstances[ sbmHandle ]->SetSpeechAudiofileBasePath( basePath );
   return true;
}

SBAPI bool SBM_SetProcessId( SBMHANDLE sbmHandle, const char * processId )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   g_smartbodyInstances[ sbmHandle ]->SetProcessId( processId );
   return true;
}


SBAPI bool SBM_SetMediaPath( SBMHANDLE sbmHandle, const char * path )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   g_smartbodyInstances[ sbmHandle ]->SetMediaPath( path );
   return true;
}


SBAPI bool SBM_Init( SBMHANDLE sbmHandle, const char* pythonLibPath, bool logToFile )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->Init(pythonLibPath, logToFile);
}


SBAPI bool SBM_Shutdown( SBMHANDLE sbmHandle )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   std::map< int, Smartbody_dll * >::iterator it = g_smartbodyInstances.find( sbmHandle );
   Smartbody_dll * sbm = g_smartbodyInstances[ sbmHandle ];
   g_smartbodyInstances.erase( it );
   bool retVal = sbm->Shutdown();
   delete sbm;

   // release the logger
   if (g_pLogMessageListener)
   {
      vhcl::Log::g_log.RemoveListener(g_pLogMessageListener);
      delete g_pLogMessageListener;
      g_pLogMessageListener = NULL;
   }

   return retVal;
}


SBAPI bool SBM_LoadSkeleton( SBMHANDLE sbmHandle, const void * data, int sizeBytes, const char * skeletonName )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->LoadSkeleton(data, sizeBytes, skeletonName);
}


SBAPI bool SBM_LoadMotion( SBMHANDLE sbmHandle, const void * data, int sizeBytes, const char * motionName )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->LoadMotion(data, sizeBytes, motionName);
}


SBAPI bool SBM_MapSkeleton( SBMHANDLE sbmHandle, const char * mapName, const char * skeletonName )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->MapSkeleton(mapName, skeletonName);
}

SBAPI bool SBM_MapMotion( SBMHANDLE sbmHandle, const char * mapName, const char * motionName )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->MapMotion(mapName, motionName);
}


SBAPI bool SBM_SetListener( SBMHANDLE sbmHandle, SBM_OnCreateCharacterCallback createCB, SBM_OnCharacterDeleteCallback deleteCB, SBM_OnCharacterChangeCallback changedCB, SBM_OnVisemeCallback visemeCB, SBM_OnChannelCallback channelCB )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   SBM_SmartbodyListener * listener = new SBM_SmartbodyListener( sbmHandle, createCB, deleteCB, changedCB, visemeCB, channelCB );
   g_smartbodyInstances[ sbmHandle ]->SetListener( listener );
   return true;
}


SBAPI bool SBM_Update( SBMHANDLE sbmHandle, double timeInSeconds )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->Update( timeInSeconds );
}


SBAPI void SBM_SetDebuggerId( SBMHANDLE sbmHandle, const char * id )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return;
   }

   g_smartbodyInstances[ sbmHandle ]->SetDebuggerId( id );
}


SBAPI void SBM_SetDebuggerCameraValues( SBMHANDLE sbmHandle, double x, double y, double z, double rx, double ry, double rz, double rw, double fov, double aspect, double zNear, double zFar )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return;
   }

   g_smartbodyInstances[ sbmHandle ]->SetDebuggerCameraValues( x, y, z, rx, ry, rz, rw, fov, aspect, zNear, zFar );
}


SBAPI void SBM_SetDebuggerRendererRightHanded( SBMHANDLE sbmHandle, bool enabled )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return;
   }

   g_smartbodyInstances[ sbmHandle ]->SetDebuggerRendererRightHanded( enabled );
}


SBAPI bool SBM_ProcessVHMsgs( SBMHANDLE sbmHandle, const char * op, const char * args )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->ProcessVHMsgs( op, args );
}

SBAPI bool SBM_ExecutePython( SBMHANDLE sbmHandle, const char * command )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->ExecutePython( command );
}


SBAPI int SBM_GetNumberOfCharacters( SBMHANDLE sbmHandle )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return -1;
   }

   return g_smartbodyInstances[ sbmHandle ]->GetNumberOfCharacters();
}


SBAPI bool SBM_InitCharacter( SBMHANDLE sbmHandle, const char * name, SBM_SmartbodyCharacter * character )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   character->m_name = new char[ strlen(name) + 1 ];
   strcpy( character->m_name, name );

   character->x  = 0;
   character->y  = 0;
   character->z  = 0;
   character->rw = 0;
   character->rx = 0;
   character->ry = 0;
   character->rz = 0;
   character->m_numJoints = 0;

   character->jname = NULL;
   character->jx  = NULL;
   character->jy  = NULL;
   character->jz  = NULL;
   character->jrw = NULL;
   character->jrx = NULL;
   character->jry = NULL;
   character->jrz = NULL;

   return true;
}


SBAPI bool SBM_GetCharacter( SBMHANDLE sbmHandle, const char * name, SBM_SmartbodyCharacter * character )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      LOG("SBM_GetCharcter : Handle %d does not exist", sbmHandle);
      return false;
   }

   SmartbodyCharacter& dllChar = g_smartbodyInstances[ sbmHandle ]->GetCharacter( (string)name );

   SBM_CharToCSbmChar( &dllChar, character );
   //LOG("dll char pos = %f %f %f, unity character pos = %f %f %f",dllChar.x, dllChar.y, dllChar.z , character->x, character->y, character->z);

   return true;
}


SBAPI bool SBM_ReleaseCharacter( SBM_SmartbodyCharacter * character )
{
   if ( !character )
   {
      return false;
   }

   SBM_ReleaseCharacterJoints(character);

   delete [] character->m_name;
   character->m_name = NULL;

   return true;
}


bool SBM_ReleaseCharacterJoints( SBM_SmartbodyCharacter * character )
{
   if ( !character )
   {
      return false;
   }

   for ( size_t i = 0; i < character->m_numJoints; i++ )
   {
      delete [] character->jname[ i ];
   }

   character->x  = 0;
   character->y  = 0;
   character->z  = 0;
   character->rw = 0;
   character->rx = 0;
   character->ry = 0;
   character->rz = 0;
   character->m_numJoints = 0;

   delete [] character->jname;
   delete [] character->jx;
   delete [] character->jy;
   delete [] character->jz;
   delete [] character->jrw;
   delete [] character->jrx;
   delete [] character->jry;
   delete [] character->jrz;

   character->jname = NULL;
   character->jx  = NULL;
   character->jy  = NULL;
   character->jz  = NULL;
   character->jrw = NULL;
   character->jrx = NULL;
   character->jry = NULL;
   character->jrz = NULL;

   return true;
}


bool SBM_HandleExists( SBMHANDLE sbmHandle )
{
   return g_smartbodyInstances.find( sbmHandle ) != g_smartbodyInstances.end();
}


void SBM_CharToCSbmChar( const SmartbodyCharacter * sbmChar, SBM_SmartbodyCharacter * sbmCChar )
{
   // copy transformation data
   sbmCChar->x = sbmChar->x;
   sbmCChar->y = sbmChar->y;
   sbmCChar->z = sbmChar->z;
   sbmCChar->rw = sbmChar->rw;
   sbmCChar->rx = sbmChar->rx;
   sbmCChar->ry = sbmChar->ry;
   sbmCChar->rz = sbmChar->rz;


   if ( sbmChar->m_joints.size() > 0 )
   {
      bool initJoints = false;
      if (sbmCChar->m_numJoints == 0)
      {
         //SBM_LogMessage("CREATING JOINTS!", 2);
         sbmCChar->m_numJoints = sbmChar->m_joints.size();
         sbmCChar->jname = new char * [ sbmCChar->m_numJoints ];
         sbmCChar->jx = new float [ sbmCChar->m_numJoints ];
         sbmCChar->jy = new float [ sbmCChar->m_numJoints ];
         sbmCChar->jz = new float [ sbmCChar->m_numJoints ];
         sbmCChar->jrw = new float [ sbmCChar->m_numJoints ];
         sbmCChar->jrx = new float [ sbmCChar->m_numJoints ];
         sbmCChar->jry = new float [ sbmCChar->m_numJoints ];
         sbmCChar->jrz = new float [ sbmCChar->m_numJoints ];
         initJoints = true;
      }

      for ( size_t i = 0; i < sbmCChar->m_numJoints; i++ )
      {
         // copy transformation data
         sbmCChar->jx[ i ] = sbmChar->m_joints[ i ].x;
         sbmCChar->jy[ i ] = sbmChar->m_joints[ i ].y;
         sbmCChar->jz[ i ] = sbmChar->m_joints[ i ].z;
         sbmCChar->jrw[ i ] = sbmChar->m_joints[ i ].rw;
         sbmCChar->jrx[ i ] = sbmChar->m_joints[ i ].rx;
         sbmCChar->jry[ i ] = sbmChar->m_joints[ i ].ry;
         sbmCChar->jrz[ i ] = sbmChar->m_joints[ i ].rz;

         // copy name
         if (initJoints)
         {
            // only initialize joints if this is the first time
            sbmCChar->jname[ i ] = new char[ sbmChar->m_joints[ i ].m_name.length() + 1 ];
            strcpy( sbmCChar->jname[ i ], sbmChar->m_joints[ i ].m_name.c_str() );
         }
      }
   }
}

SBAPI bool SBM_IsCharacterCreated( SBMHANDLE sbmHandle, char * name, int maxNameLen, char * objectClass, int maxObjectClassLen )
{
    if ( !SBM_HandleExists( sbmHandle ) || g_CreateCallbackInfo[sbmHandle].size() == 0)
    {
        return false;
    }

    SBM_CallbackInfo info = g_CreateCallbackInfo[sbmHandle].back();
    g_CreateCallbackInfo[sbmHandle].pop_back();
    //LOG("SBM_IsCharacterCreated, info.name = %s, info.objectClass = %s",info.name.c_str(), info.objectClass.c_str());
    strncpy(name, info.name.c_str(), maxNameLen);
    strncpy(objectClass, info.objectClass.c_str(), maxObjectClassLen);    
    return true;
}

SBAPI bool SBM_IsLogMessageWaiting( SBMHANDLE sbmHandle, char *logMessage, int maxLogMessageLen, int* logMessageType)
{
    if (g_LogCallbackInfo[0].size() == 0)
    {
        return false;
    }

    SBM_CallbackInfo info = g_LogCallbackInfo[0].back();
    g_LogCallbackInfo[0].pop_back();
    strncpy(logMessage, info.logMessage.c_str(), maxLogMessageLen);
    *logMessageType = info.logMessageType;

    return true;
}

SBAPI bool SBM_IsCharacterDeleted( SBMHANDLE sbmHandle, char * name, int maxNameLen )
{
    if ( !SBM_HandleExists( sbmHandle ) || g_DeleteCallbackInfo[sbmHandle].size() == 0)
    {
        return false;
    }

    SBM_CallbackInfo info = g_DeleteCallbackInfo[sbmHandle].back();
    g_DeleteCallbackInfo[sbmHandle].pop_back();

    strncpy(name, info.name.c_str(), maxNameLen);
    return true;
}

SBAPI bool SBM_IsCharacterChanged( SBMHANDLE sbmHandle, char * name, int maxNameLen )
{
    if ( !SBM_HandleExists( sbmHandle ) || g_ChangeCallbackInfo[sbmHandle].size() == 0)
    {
        return false;
    }

    SBM_CallbackInfo info = g_ChangeCallbackInfo[sbmHandle].back();
    g_ChangeCallbackInfo[sbmHandle].pop_back();    
    strncpy(name, info.name.c_str(), maxNameLen);   
 
    return true;
}

SBAPI bool SBM_IsVisemeSet( SBMHANDLE sbmHandle, char * name, int maxNameLen, char * visemeName, int maxVisemeNameLen, float * weight, float * blendTime )
{
    if ( !SBM_HandleExists( sbmHandle ) || g_VisemeCallbackInfo[sbmHandle].size() == 0)
    {
        return false;
    }

    SBM_CallbackInfo info = g_VisemeCallbackInfo[sbmHandle].back();
    g_VisemeCallbackInfo[sbmHandle].pop_back();
   
    strncpy(name, info.name.c_str(), maxNameLen);
    strncpy(visemeName, info.visemeName.c_str(), maxNameLen);
    
    *weight = info.weight;
    *blendTime = info.blendTime;
    return true;
}

SBAPI bool SBM_IsChannelSet( SBMHANDLE sbmHandle, char * name, int maxNameLen, char * channelName, int maxChannelNameLen, float * value )
{
    if ( !SBM_HandleExists( sbmHandle ) || g_ChannelCallbackInfo[sbmHandle].size() == 0)
    {
        return false;
    }

    SBM_CallbackInfo info = g_ChannelCallbackInfo[sbmHandle].back();
    g_ChannelCallbackInfo[sbmHandle].pop_back();

    strncpy(name, info.name.c_str(), maxNameLen);
    strncpy(channelName, info.visemeName.c_str(), maxNameLen);
    
    *value = info.weight;
    return true;
}


SBAPI bool SBM_PythonCommandVoid( SBMHANDLE sbmHandle, const char * command)
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->PythonCommandVoid( command );
}

SBAPI bool SBM_PythonCommandBool( SBMHANDLE sbmHandle,  const char * command )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return false;
   }

   return g_smartbodyInstances[ sbmHandle ]->PythonCommandBool( command );
}

SBAPI int SBM_PythonCommandInt( SBMHANDLE sbmHandle,  const char * command )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return 0;
   }

   return g_smartbodyInstances[ sbmHandle ]->PythonCommandInt( command );
}

SBAPI float SBM_PythonCommandFloat( SBMHANDLE sbmHandle,  const char * command )
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return 0;
   }

   return g_smartbodyInstances[ sbmHandle ]->PythonCommandFloat( command );
}

SBAPI char * SBM_PythonCommandString( SBMHANDLE sbmHandle,  const char * command, char * output, int maxLen)
{
   if ( !SBM_HandleExists( sbmHandle ) )
   {
      return 0;
   }

   std::string temp = g_smartbodyInstances[ sbmHandle ]->PythonCommandString( command );
   strncpy(output, temp.c_str(), maxLen);
   return output;
}






#if 0
// stubs used for testing dll usage on other platforms
int unused = 0;
SBAPI SBMHANDLE SBM_CreateSBM() { unused++; return unused; }

SBAPI bool SBM_SetSpeechAudiofileBasePath( SBMHANDLE sbmHandle, const char * basePath ) { return true; }
SBAPI bool SBM_SetProcessId( SBMHANDLE sbmHandle, const char * processId ) { return true; }
SBAPI bool SBM_SetMediaPath( SBMHANDLE sbmHandle, const char * path ) { return true; }

SBAPI bool SBM_Init( SBMHANDLE sbmHandle, const char* pythonLibPath, bool logToFile ) { return true; }
SBAPI bool SBM_Shutdown( SBMHANDLE sbmHandle ) { return true; }

SBAPI bool SBM_SetListener( SBMHANDLE sbmHandle, SBM_OnCreateCharacterCallback createCB, SBM_OnCharacterDeleteCallback deleteCB, SBM_OnCharacterChangeCallback changedCB, SBM_OnVisemeCallback visemeCB, SBM_OnChannelCallback channelCB ) { return true; }

SBAPI bool SBM_Update( SBMHANDLE sbmHandle, double timeInSeconds ) { return true; }

SBAPI void SBM_SetDebuggerId( SBMHANDLE sbmHandle, const char * id ) { return; }
SBAPI void SBM_SetDebuggerCameraValues( SBMHANDLE sbmHandle, double x, double y, double z, double rx, double ry, double rz, double rw, double fov, double aspect, double zNear, double zFar ) { return; }
SBAPI void SBM_SetDebuggerRendererRightHanded( SBMHANDLE sbmHandle, bool enabled ) { return; }

SBAPI bool SBM_ProcessVHMsgs( SBMHANDLE sbmHandle, const char * op, const char * args ) { return true; }
SBAPI bool SBM_ExecutePython( SBMHANDLE sbmHandle, const char * command ) { return true; }

SBAPI int  SBM_GetNumberOfCharacters( SBMHANDLE sbmHandle ) { return 42; }
SBAPI bool SBM_InitCharacter( SBMHANDLE sbmHandle, const char * name, SBM_SmartbodyCharacter * character ) { return true; }
SBAPI bool SBM_GetCharacter( SBMHANDLE sbmHandle, const char * name, SBM_SmartbodyCharacter * character ) { return true; }
SBAPI bool SBM_ReleaseCharacter( SBM_SmartbodyCharacter * character ) { return true; }
SBAPI bool SBM_SetLogMessageCallback(LogMessageCallback cb) { return true; }
SBAPI void SBM_LogMessage(const char * message, int messageType) { return; }

// used for polling on iOS since callbacks aren't allowed
SBAPI bool SBM_IsCharacterCreated( SBMHANDLE sbmHandle, char * name, int maxNameLen, char * objectClass, int maxObjectClassLen ) { return false; }
SBAPI bool SBM_IsCharacterDeleted( SBMHANDLE sbmHandle, char * name, int maxNameLen ) { return false; }
SBAPI bool SBM_IsCharacterChanged( SBMHANDLE sbmHandle, char * name, int maxNameLen ) { return false; }
SBAPI bool SBM_IsVisemeSet( SBMHANDLE sbmHandle, char * name, int maxNameLen, char * visemeName, int maxVisemeNameLen, float * weight, float * blendTime ) { return false; }
SBAPI bool SBM_IsChannelSet( SBMHANDLE sbmHandle, char * name, int maxNameLen, char * channelName, int maxChannelNameLen, float * value ) { return false; }
SBAPI bool SBM_IsLogMessageWaiting( SBMHANDLE sbmHandle, char *logMessage, int maxLogMessageLen, int* logMessageType ) { return false; }

// python usage functions
// functions can't be distinguished by return type alone so they are named differently
SBAPI bool SBM_PythonCommandVoid( SBMHANDLE sbmHandle,  const char * command ) { return true; }
SBAPI bool SBM_PythonCommandBool( SBMHANDLE sbmHandle,  const char * command ) { return true; }
SBAPI int SBM_PythonCommandInt( SBMHANDLE sbmHandle,  const char * command ) { return 42; }
SBAPI float SBM_PythonCommandFloat( SBMHANDLE sbmHandle,  const char * command )  { return 42; }
SBAPI char * SBM_PythonCommandString( SBMHANDLE sbmHandle, const char * command, char * output, int maxLen) { return "test"; }

#endif
