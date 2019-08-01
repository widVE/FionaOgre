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

#ifndef WSP_SUBSCRIPTIONS_H
#define WSP_SUBSCRIPTIONS_H


#include <list>

#include "wsp_error.h"
#include "wsp.h"


typedef std::list< int > int_list;


// this class holds any data we need about a subscription
class subscription
{
public:
   std::string id;
   std::string attribute_name;

   WSP::data_type type;

   const wsp_vector * position;
   const wsp_vector * direction;

   // we use this union because we will only ever need one of the callbacks
   union
   {
      WSP::int_callback int_cb_func;
      WSP::double_callback double_cb_func;
      WSP::string_callback string_cb_func;
      WSP::vector_3d_callback vector_3d_cb_func;
      WSP::vector_4d_callback vector_4d_cb_func;
      WSP::raycast_callback raycast_cb_func;

      void * cb_func;
   };

   void * data;

   WSP::error_callback error_handler;

   // rate at which this subscription should receive updates
   int interval;

   // list of intervals for other subscriptions matching this id/attribute-name
   int_list intervals;

   // time the next update should be received for this subscription
   int64_t time;

   bool operator<( const subscription &rhs ) const;
   bool operator>( const subscription &rhs ) const;
   int operator==( const subscription &rhs ) const;

   subscription( const wsp_vector& position, const wsp_vector& direction );
   subscription( std::string id, std::string attribute_name );
};


typedef std::list< subscription* > subscription_list;
typedef subscription_list::iterator subscription_list_it;


// this is basically a wrapper class for list< subscription* >
// so that we can remove elements from the list without
// creating memory/resource leaks
class subscriptions
{
private:
   subscription_list the_list;

public:
   // provide functionality of list<>
   void remove_elements( subscriptions remove_list );
   void remove_element( subscription * the_subscription );
   void push_front( subscription * the_subscription );
   void clear();
   bool empty();
   subscription * front();
   subscription_list_it begin();
   subscription_list_it end();
   void sort();
   void pop_front();
   void push_back( subscription * the_subscription );
   int size();
};


#endif  // WSP_SUBSCRIPTIONS_H
