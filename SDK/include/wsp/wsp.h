/*
    This file is part of VHMsg written by Thomas Amundsen and Edward Fast at 
    University of Southern California's Institute for Creative Technologies.
    http://www.ict.usc.edu
    Copyright 2008 Thomas Amundsen, Edward Fast, University of Southern California

    VHMsg is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VHMsg is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with VHMsg.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef WSP_H
#define WSP_H


#include "wsp_vector.h"


namespace WSP
{
   ///////////////////////////////////////////////////////
   // WSPN Data Types and Function Prototypes

   enum data_type
   {
      INT_TYPE,
      DOUBLE_TYPE,
      STRING_TYPE,
      VECTOR_3D_TYPE,
      VECTOR_4D_TYPE,
      RAYCAST_TYPE
   };

   enum error_code
   {
      NO_ERRORS,
      NOT_FOUND,
      MISMATCH_ID,
      MISMATCH_ATTRIBUTE,
      TIMEOUT,
      PARSE_ERROR,
      UNKNOWN
   };

   typedef void * WSP_ERROR;  // This is a WSP_Error class pointer, but clients don't need to know that.

   // Accessor Function Prototypes
   typedef WSP_ERROR ( *int_accessor_function )( const std::string id, const std::string attribute_name, long & value, void * data );
   typedef WSP_ERROR ( *double_accessor_function )( const std::string id, const std::string attribute_name, double & value, void * data );
   typedef WSP_ERROR ( *string_accessor_function )( const std::string id, const std::string attribute_name, std::string & value, void * data );
   typedef WSP_ERROR ( *vector_3d_accessor_function )( const std::string id, const std::string attribute_name, wsp_vector & value, void * data );
   typedef WSP_ERROR ( *vector_4d_accessor_function )( const std::string id, const std::string attribute_name, wsp_vector & value, void * data );
   typedef WSP_ERROR ( *raycast_provider_function )( const wsp_vector & position, const wsp_vector & direction, std::string & object_name, void * data );

   // Data Callback Function Prototypes
   typedef WSP::WSP_ERROR ( *int_callback )( const std::string id, const std::string attribute_name, long & value, void * data, const std::string & data_provider );
   typedef WSP::WSP_ERROR ( *double_callback )( const std::string id, const std::string attribute_name, double & value, void * data, const std::string & data_provider );
   typedef WSP::WSP_ERROR ( *string_callback )( const std::string id, const std::string attribute_name, std::string  & value, void * data , const std::string & data_provider);
   typedef WSP::WSP_ERROR ( *vector_3d_callback )( const std::string id, const std::string attribute_name, wsp_vector& value, void * data, const std::string & data_provider );
   typedef WSP::WSP_ERROR ( *vector_4d_callback )( const std::string id, const std::string attribute_name, wsp_vector & value, void * data, const std::string & data_provider );
   typedef WSP::WSP_ERROR ( *raycast_callback )( const wsp_vector& position, const wsp_vector & direction, std::string & object_name, void * data, const std::string & data_provider );

   // Error handler
   typedef void ( *error_callback )( const std::string id, const std::string attribute_name, int error, std::string reason, void * data );


   class Manager;  // Pre-declare for factory functions

   ///////////////////////////////////////////////////////
   // Factory Functions
   Manager * create_manager();

   // the following generate the errors used by accessor functions.
   WSP_ERROR not_found_error( const std::string reason );
   WSP_ERROR mismatch_id_error( const std::string reason );
   WSP_ERROR mismatch_attribute_error( const std::string reason );
   WSP_ERROR no_error();

   ///////////////////////////////////////////////////////
   // Core Class
   class Manager
   {
   public:
      virtual ~Manager();

      virtual int init( const std::string process_name ) = 0;
      virtual int shutdown() = 0;
      virtual void broadcast_update() = 0;
      virtual void process_command( const char * message ) = 0;

      virtual int register_int_source( const std::string id, const std::string attribute_name, const int_accessor_function, void * data ) = 0;
      virtual int register_double_source( const std::string id, const std::string attribute_name, const double_accessor_function, void * data ) = 0;
      virtual int register_string_source( const std::string id, const std::string attribute_name, const string_accessor_function, void * data ) = 0;
      virtual int register_vector_3d_source( const std::string id, const std::string attribute_name, const vector_3d_accessor_function, void * data ) = 0;
      virtual int register_vector_4d_source( const std::string id, const std::string attribute_name, const vector_4d_accessor_function, void * data ) = 0;
      virtual int register_raycast_provider( const raycast_provider_function, void * data ) = 0;

      virtual void unregister_data_source( const std::string id, const std::string attribute_name ) = 0;

      virtual void query_int_data( const std::string id, const std::string attribute_name, const error_callback, const int_callback, void * data ) = 0;
      virtual void query_double_data( const std::string id, const std::string attribute_name, const error_callback, const double_callback, void * data ) = 0;
      virtual void query_string_data( const std::string id, const std::string attribute_name, const error_callback, const string_callback, void * data ) = 0;
      virtual void query_vector_3d_data( const std::string id, const std::string attribute_name, const error_callback, const vector_3d_callback, void * data ) = 0;
      virtual void query_vector_4d_data( const std::string id, const std::string attribute_name, const error_callback, const vector_4d_callback, void * data ) = 0;
      virtual void query_raycast( const wsp_vector & position, const wsp_vector & direction, const error_callback, const raycast_callback, void * data ) = 0;

      virtual int subscribe_int_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const int_callback, void * data ) = 0;
      virtual int subscribe_double_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const double_callback, void * data ) = 0;
      virtual int subscribe_string_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const string_callback, void * data ) = 0;
      virtual int subscribe_vector_3d_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const vector_3d_callback, void * data ) = 0;
      virtual int subscribe_vector_4d_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const vector_4d_callback, void * data ) = 0;

      virtual int unsubscribe( const std::string id, const std::string attribute_name, const int interval ) = 0;
   };
}

#endif  // WSP_H
