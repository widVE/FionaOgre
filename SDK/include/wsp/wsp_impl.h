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

#ifndef WSP_IMPL_H
#define WSP_IMPL_H


#if !defined(WIN_BUILD)
#include <sys/time.h>
#endif

#include "wsp.h"
#include "wsp_subscriptions.h"
#include "wsp_error.h"
#include "wsp_data_sources.h"


namespace WSP
{
   // used for the tokenize function
   typedef std::vector<std::string> string_vector;

   class WSP_Impl : public Manager
   {
   private:
      // your application's time, gets assigned by broadcast_update
      int64_t time;
#if !defined(WIN_BUILD)
      timeval start;
#endif

      std::string process_name;

      // data sources we've registered to have accessor funtions
      data_sources data_registration_map;

      // subscriptions we must respond to ( producers )
      subscriptions sub_prod_list;

      // subscriptions we want to receive data for ( consumers )
      subscriptions sub_cons_list;

   public:
      int init( const std::string process_name );
      int shutdown();
      void broadcast_update();
      void process_command( const char* message );

      int register_int_source( const std::string id, const std::string attribute_name, const int_accessor_function, void * data );
      int register_double_source( const std::string id, const std::string attribute_name, const double_accessor_function, void * data );
      int register_string_source( const std::string id, const std::string attribute_name, const string_accessor_function, void * data );
      int register_vector_3d_source( const std::string id, const std::string attribute_name, const vector_3d_accessor_function, void * data );
      int register_vector_4d_source( const std::string id, const std::string attribute_name, const vector_4d_accessor_function, void * data );
      int register_raycast_provider( const raycast_provider_function, void * data );

      void unregister_data_source( const std::string id, const std::string attribute_name );

      void query_int_data( const std::string id, const std::string attribute_name, const error_callback, const int_callback, void * data );
      void query_double_data( const std::string id, const std::string attribute_name, const error_callback, const double_callback, void * data );
      void query_string_data( const std::string id, const std::string attribute_name, const error_callback, const string_callback, void * data );
      void query_vector_3d_data( const std::string id, const std::string attribute_name, const error_callback, const vector_3d_callback, void * data );
      void query_vector_4d_data( const std::string id, const std::string attribute_name, const error_callback, const vector_4d_callback, void * data );
      void query_raycast( const wsp_vector & position, const wsp_vector & direction, const error_callback, const raycast_callback, void * data );

      int subscribe_int_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const int_callback, void * data );
      int subscribe_double_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const double_callback, void * data );
      int subscribe_string_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const string_callback, void * data );
      int subscribe_vector_3d_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const vector_3d_callback, void * data );
      int subscribe_vector_4d_interval( const std::string id, const std::string attribute_name, const int interval, const error_callback, const vector_4d_callback, void * data );

      int unsubscribe( const std::string id, const std::string attribute_name, const int interval );

   protected:
      // helper functions for broadcast_update
      void server_broadcast();
      void client_broadcast();

      void send_data( subscription * current_job, long & int_value, double & double_value, std::string & string_value, const wsp_vector & vector_value );
      void access_error( subscription * current_job, WSP_Error * accessor_error );

      wsp_vector get_vector_from_string( std::string vector_string );
      bool b_parse_error( std::string id, std::string attribute_name, std::string message, std::string & parse_error_message );

      // functions to determine what data type the result is in an Elvin answer message
      bool b_is_int_message( std::string message );
      bool b_is_double_message( std::string message );
      bool b_is_vector_message( std::string message, unsigned int num_elements );

      // template funciton that all the subscribe_x_interval functions call
      template< typename T >
      int subscribe( const std::string id, const std::string attribute_name, const int interval, const error_callback, const T, data_type type, void * data );

      // template funciton that all the query_x_data functions call
      template< typename T >
      void query( const std::string id, const std::string attribute_name, const error_callback error_handler, const T cb_func, data_type type, void * data );

      // functions that actually send Elvin answer messages
      int send_int_response( const std::string id, const std::string attribute_name, const int result );
      int send_double_response( const std::string id, const std::string attribute_name, const double result );
      int send_string_response( const std::string id, const std::string attribute_name, const std::string result );
      int send_vector_response( const std::string id, const std::string attribute_name, const wsp_vector & result );
      int send_raycast_response( subscription * current_job, const wsp_vector & position, const wsp_vector & direction, const std::string raycast_intersection_object );

      // function that actually sends Elvin error messages
      int send_error( const std::string id, const std::string attribute_name, const error_code, const std::string message );

      // helper functions for process_command
      int handle_message_query_attribute_value( string_vector message_tokens );
      int handle_message_query_raycast( string_vector message_tokens );
      void handle_message_answer_attribute_value( string_vector message_tokens );
      int handle_message_answer_raycast( string_vector message_tokens );
      void handle_message_subscribe( string_vector message_tokens );
      void handle_message_unsubscribe( string_vector message_tokens );

      void add_query_subscription( subscription * new_subscription );
   };
}


#endif  // WSP_IMPL_H
