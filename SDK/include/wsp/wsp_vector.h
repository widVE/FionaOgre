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

#ifndef WSP_VECTOR_H
#define WSP_VECTOR_H


class wsp_vector
{
public:
   double x, y, z, q;
   int num_dimensions;

   std::string to_string() const;

   wsp_vector( const std::string vector_string );
   wsp_vector( double x, double y, double z );
   wsp_vector( double x, double y, double z, double q );
};


#endif // WSP_VECTOR_H
