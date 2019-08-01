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

#ifndef WSP_DATA_SOURCES_H
#define WSP_DATA_SOURCES_H


#include <string>
#include <list>

#include "wsp_error.h"


// this class stores a data accessor function that will give us information
// for a specific (id,attribute_name) value
class data_source
{
public:
   std::string id;
   std::string attribute_name;

   WSP::data_type type;

   WSP::int_accessor_function acc_int;
   WSP::double_accessor_function acc_double;
   WSP::string_accessor_function acc_string;
   WSP::vector_3d_accessor_function acc_vector_3d;
   WSP::vector_4d_accessor_function acc_vector_4d;
   WSP::raycast_provider_function perform_raycast;

   void * data;

   data_source();
   data_source( std::string id, std::string attribute_name );
};


typedef std::list< data_source * > data_source_list;
typedef data_source_list::iterator data_source_list_it;


// this class is basically a wrapper for a list< data_source* >
class data_sources
{
private:
   data_source_list the_list;
public:
   void remove_element( data_source * the_source );
   void push_front( data_source * the_source);
   void clear();
   bool empty();
   data_source * front();
   data_source_list_it begin();
   data_source_list_it end();
};


#endif // WSP_DATA_SOURCES_H
