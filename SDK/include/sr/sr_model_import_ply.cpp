/*  sr_model_import_obj.cpp - part of Motion Engine and SmartBody-lib
 *  Copyright (C) 2008  University of Southern California
 *
 *  SmartBody-lib is free software: you can redistribute it and/or
 *  modify it under the terms of the Lesser GNU General Public License
 *  as published by the Free Software Foundation, version 3 of the
 *  license.
 *
 *  SmartBody-lib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  Lesser GNU General Public License for more details.
 *
 *  You should have received a copy of the Lesser GNU General Public
 *  License along with SmarBody-lib.  If not, see:
 *      http://www.gnu.org/licenses/lgpl-3.0.txt
 *
 *  CONTRIBUTORS:
 *      Marcelo Kallmann, USC (currently UC Merced)
 */

#include <sr/sr_string_array.h>
#include <sr/sr_model.h>
#include <sb/SBTypes.h>
#include <external/rply/rply.h>

#if !defined (__ANDROID__) && !defined(SB_IPHONE)
#include <sbm/GPU/SbmTexture.h>
#endif


static int vertex_cb(p_ply_argument argument) {
	static int count = 0;
	long idx;	
	SrModel* model;
	ply_get_argument_user_data(argument, (void**)&model, &idx);
	if (idx == 0)
		model->V.push();
	double argumentValue = ply_get_argument_value(argument);
	model->V.top()[idx] = (float) argumentValue;	
	return 1;
}

static int vertex_color_cb(p_ply_argument argument) {
	static int count = 0;
	long idx;	
	SrModel* model;
	ply_get_argument_user_data(argument, (void**)&model, &idx);
	if (idx == 0)
		model->Vc.push();
	double argumentValue = ply_get_argument_value(argument);
	model->Vc.top()[idx] = (float) argumentValue/255.0;	
	return 1;
}

static int face_cb(p_ply_argument argument) {	
	long length, value_index;
	long idx;
	SrModel* model;
	ply_get_argument_user_data(argument, (void**)&model, &idx);
	ply_get_argument_property(argument, NULL, &length, &value_index);
	
	if (value_index == -1) // first entry in the list
	{
		model->F.push();
		model->Fm.push(0);
	}
	else if (value_index >= 0 && value_index <= 2) // a triangle face
	{
		double argumentValue = ply_get_argument_value(argument);
		model->F.top()[value_index] = (float) argumentValue;		
	}		
	return 1;
}

#if 1
/************************************************************************/
/* Import Ply mesh                                                      */
/************************************************************************/
bool SrModel::import_ply( const char* file )
{

	long nvertices, ntriangles;
	M.push();
	mtlnames.push("noname");
	p_ply ply = ply_open(file, NULL, 0, NULL);
	if (!ply) return false;
	if (!ply_read_header(ply)) return false;
	SrString path=file;
	SrString filename;
	path.extract_file_name(filename);
	name = filename;
	name.remove_file_extension();
	nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, this, 0);
	ply_set_read_cb(ply, "vertex", "y", vertex_cb, this, 1);
	ply_set_read_cb(ply, "vertex", "z", vertex_cb, this, 2);
	ply_set_read_cb(ply, "vertex", "red", vertex_color_cb, this, 0);
	ply_set_read_cb(ply, "vertex", "green", vertex_color_cb, this, 1);
	ply_set_read_cb(ply, "vertex", "blue", vertex_color_cb, this, 2);
	ntriangles = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, this, 0);
	printf("%ld\n%ld\n", nvertices, ntriangles);
	if (!ply_read(ply)) return false;
	ply_close(ply);

	validate ();
	remove_redundant_materials ();
	//   remove_redundant_normals ();
	compress ();
	return true;
}
#endif


//============================ EOF ===============================
