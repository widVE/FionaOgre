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

#ifndef WSP_ERROR_H
#define WSP_ERROR_H


#include "wsp.h"


// this class holds information for the errors we will handle within the WSP
// it holds an error code and a human-readable reason for the error
class WSP_Error
{
public:
   WSP::error_code error_code;
   std::string reason;

   WSP_Error( const WSP::error_code code, const std::string reason );

   // given an error code, this function will return its string representation
   static std::string get_error_string( const WSP::error_code error_code );

   // given a string representation of an error code, this function will return the actual error code
   static WSP::error_code get_error_enum( const std::string error_string );

   // given a string, this function will return a boolean indicating
   // whether it is a string representation of an error code
   static bool b_is_error_code( const std::string token );
};


#endif // WSP_ERROR_H
