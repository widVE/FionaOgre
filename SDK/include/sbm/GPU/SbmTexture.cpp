#include "SbmTexture.h"
#include "SbmShader.h"
#include "external/SOIL/SOIL.h"
#include <sb/SBTypes.h>
#include <vhcl_log.h>
//#include "external/imdebug/imdebug.h"

/************************************************************************/
/* Sbm Texture Manager                                                  */
/************************************************************************/
SbmTextureManager* SbmTextureManager::_singleton = NULL;

SbmTextureManager::SbmTextureManager( void )
{

}

SbmTextureManager::~SbmTextureManager( void )
{
	releaseAllTextures();
}

void SbmTextureManager::releaseAllTextures()
{
	StrTextureMap::iterator vi;
	for ( vi  = textureMap.begin();
		  vi != textureMap.end();
		  vi++)
	{
		SbmTexture* tex = vi->second;
		delete tex;		
	}

	for ( vi  = normalTexMap.begin();
		  vi != normalTexMap.end();
		  vi++)
	{
		SbmTexture* tex = vi->second;
		delete tex;		
	}

	for ( vi  = specularTexMap.begin();
		vi != specularTexMap.end();
		vi++)
	{
		SbmTexture* tex = vi->second;
		delete tex;
	}
	textureMap.clear();
	normalTexMap.clear();
	specularTexMap.clear();	
}

StrTextureMap& SbmTextureManager::findMap( int type )
{
	if (type == TEXTURE_DIFFUSE)
		return textureMap;
	else if (type == TEXTURE_NORMALMAP)
		return normalTexMap;
	else if (type == TEXTURE_SPECULARMAP)
		return specularTexMap;

	return textureMap;
}


std::vector<std::string> SbmTextureManager::getTextureNames( int type )
{
	StrTextureMap& texMap = findMap(type);
	
	std::vector<std::string> texNames;
	StrTextureMap::iterator vi;
	for ( vi  = texMap.begin();
		vi != texMap.end();
		vi++)
	{
		texNames.push_back(vi->first);		
	}
	return texNames;
}



void SbmTextureManager::loadTexture(int iType, const char* textureName, const char* fileName )
{
	std::string strTex = textureName;
	StrTextureMap& texMap = findMap(iType);
	if (texMap.find(strTex) == texMap.end()) // the texture does not exist in the texture map, create a new one
	{
		SbmTexture* texture = new SbmTexture(textureName);
		texture->loadImage(fileName);
		texMap[strTex] = texture;
	}	
}

void SbmTextureManager::updateTexture()
{
	StrTextureMap::iterator vi;
	for ( vi  = textureMap.begin();
		  vi != textureMap.end();
		  vi++)
	{
		SbmTexture* tex = vi->second;
		if (!tex->hasBuild())
			tex->buildTexture();
	}

	for ( vi  = normalTexMap.begin();
		  vi != normalTexMap.end();
		  vi++)
	{
		SbmTexture* tex = vi->second;
		if (!tex->hasBuild())
			tex->buildTexture();
		}

	for ( vi  = specularTexMap.begin();
		vi != specularTexMap.end();
		vi++)
	{
		SbmTexture* tex = vi->second;
		if (!tex->hasBuild())
			tex->buildTexture();
	}
}


SBAPI void SbmTextureManager::reloadTexture()
{
	StrTextureMap::iterator vi;
	for ( vi  = textureMap.begin();
		vi != textureMap.end();
		vi++)
	{
		SbmTexture* tex = vi->second;
		tex->buildTexture();
	}

	for ( vi  = normalTexMap.begin();
		vi != normalTexMap.end();
		vi++)
	{
		SbmTexture* tex = vi->second;
		tex->buildTexture();
	}

	for ( vi  = specularTexMap.begin();
		vi != specularTexMap.end();
		vi++)
	{
		SbmTexture* tex = vi->second;
		tex->buildTexture();
	}
}


SbmTexture* SbmTextureManager::findTexture(int type, const char* textureName )
{
	std::string strTex = textureName;
	StrTextureMap& texMap = findMap(type);
	if (texMap.find(strTex) != texMap.end())
		return texMap[strTex];
	return NULL;
}
/************************************************************************/
/* Sbm Texture                                                          */
/************************************************************************/

SbmTexture::SbmTexture( const char* texName )
{
	textureName = texName;
	texID = 0;
	buffer = NULL;
	finishBuild = false;
	transparentTexture = false;
	width = -1;
	height = -1;
	channels = -1;
}

SbmTexture::~SbmTexture(void)
{
	if (buffer)
      	   delete [] buffer;
	if (texID != 0)
	   glDeleteTextures(1,&texID);	
}

bool SbmTexture::loadImage( const char* fileName )
{
	buffer = SOIL_load_image(fileName,&width,&height,&channels,SOIL_LOAD_AUTO);	
	if (width < 0 || height < 0 || channels < 0)
		return false;
	std::string testOutFileName = fileName;
	//testOutFileName += ".bmp";
	//SOIL_save_image(testOutFileName.c_str(),SOIL_SAVE_TYPE_BMP,width,height,channels,buffer);
	int transparentPixel = 0;
	// invert the image in y-axis
	for(int j = 0; j*2 < height; ++j )
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;
		for(int i = width * channels ; i > 0; --i )
		{
			unsigned char temp = buffer[index1];
			buffer[index1] = buffer[index2];
			buffer[index2] = temp;
			++index1;
			++index2;			
		}
	}

	if (channels == 4)
	{
		for (int j=0;j<height;j++)
		{
			for (int i=0;i<width;i++)
			{
				unsigned char alphaVal = buffer[j*width*channels+i*channels+3];
				if (alphaVal < 250)
					transparentPixel++;
			}
		}
	}
	

	if (transparentPixel*20 > height*width)
		transparentTexture = true;
	imgBuffer.resize(width*height*channels);
	for (int i=0;i<width*height*channels;i++)
	{
		imgBuffer[i] = buffer[i];
	}
	// set the texture file name
	textureFileName = fileName;	
	SOIL_free_image_data(buffer);
	buffer = NULL;
	return true;
}

void SbmTexture::buildTexture(bool buildMipMap)
{	
	//LOG("Start Build Texture");
	if (!getBuffer()) return;
#if !defined(__native_client__)
	//SbmShaderProgram::printOglError("SbmTexture.cpp:10");		
	GLuint iType = GL_TEXTURE_2D;
	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glEnable(iType);	
	glGenTextures(1,&texID);
	glBindTexture(iType,texID);
#if !defined(__ANDROID__) && !defined(SB_IPHONE)
	if (!glIsTexture(texID))
	{
		SbmShaderProgram::printOglError("SbmTexture.cpp:100");
	}
#endif
	//LOG("After Initialize and bind texture");
	//SbmShaderProgram::printOglError("SbmTexture.cpp:50");	
#if defined(__ANDROID__) || defined(SB_IPHONE)
#define GL_CLAMP GL_CLAMP_TO_EDGE
#define GL_RGB8 GL_RGB
#define GL_RGBA8 GL_RGBA
#endif
	
	glTexParameteri(iType,GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(iType,GL_TEXTURE_WRAP_T, GL_REPEAT);

	//LOG("After Texture parameters : GL_TEXTURE_WRAP_S");

#if !defined (__FLASHPLAYER__) && !defined(__ANDROID__) && !defined(SB_IPHONE)
	if (buildMipMap)
		glTexParameteri(iType, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(iType, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
#else
	glTexParameteri(iType, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
#endif
	//LOG("After Texture parameters : GL_TEXTURE_MIN_FILTER");
	glTexParameteri(iType, GL_TEXTURE_MAG_FILTER,GL_LINEAR); 
	//LOG("After Texture parameters : GL_TEXTURE_MAX_FILTER");
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//LOG("After glTexEnvf");
	//SbmShaderProgram::printOglError("SbmTexture.cpp:100");	
	if (channels == 3)
	{
		internal_format = GL_RGB8;
		texture_format = GL_RGB;		
	}
	else if (channels == 4)
	{
		internal_format = GL_RGBA8;
		texture_format = GL_RGBA;				
	}
	//glTexImage2D(iType,0,texture_format,width,height,0,texture_format,GL_UNSIGNED_BYTE,buffer);	
#if !defined (__FLASHPLAYER__) && !defined(__ANDROID__) && !defined(SB_IPHONE)
	if (buildMipMap)
		gluBuild2DMipmaps(iType, channels, width, height, texture_format, GL_UNSIGNED_BYTE, &imgBuffer[0] );
	else
		glTexImage2D(iType,0,texture_format,width,height,0,texture_format,GL_UNSIGNED_BYTE, &imgBuffer[0]);
#else
	glTexImage2D(iType,0,texture_format,width,height,0,texture_format,GL_UNSIGNED_BYTE, &imgBuffer[0]);
	//glGenerateMipmap(GL_TEXTURE_2D);
#endif

	//LOG("texture id = %u, texture name = %s, width = %d, height = %d, channel = %d",texID, textureName.c_str(), width, height, channels);

	//glGenerateMipmap(iType);
	//SbmShaderProgram::printOglError("Sb!defined(SB_IPHONE)mTexture.cpp:200");
#if !defined(__ANDROID__) && !defined(SB_IPHONE)
	GLclampf iPrority = 1.0;
	glPrioritizeTextures(1,&texID,&iPrority);
#endif
	//TextureDebug();	
	glBindTexture(iType,0);	
	finishBuild = true;
	//LOG("Finish build texture");
	//SbmShaderProgram::printOglError("SbmTexture.cpp:300");
	//printf("Texture name = %s, texture ID = %d\n",textureName.c_str(),texID);	
	//imdebug("rgb w=%d h=%d %p", width, height, buffer);
#endif
}

unsigned char* SbmTexture::getBuffer()
{
	if (imgBuffer.size() == 0) return NULL;
	
	return &imgBuffer[0];
}

int SbmTexture::getBufferSize()
{
	return imgBuffer.size();
}

void SbmTexture::setBuffer(unsigned char* buffer, int size)
{
	imgBuffer.clear();
	for (int i = 0; i < size; ++i)
	{
		imgBuffer.push_back(buffer[i]);
	}
}

void SbmTexture::setTextureSize(int w, int h, int numChannels)
{
	width = w;
	height = h;
	channels = numChannels;
}