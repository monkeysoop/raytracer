#include "GLUtils.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <SDL2/SDL_image.h>

/* 

Based on http://www.opengl-tutorial.org/

*/
void loadShader( const GLuint loadedShader, const std::filesystem::path& _fileName )
{
	// Upon failure, log the error message and return with -1
	if ( loadedShader == 0 )
	{
		SDL_LogMessage( SDL_LOG_CATEGORY_ERROR,
						SDL_LOG_PRIORITY_ERROR,
						"Shader needs to be inited before loading %s !", _fileName.string().c_str());
		return;
	}

	// Loading a shader from disk
	std::string shaderCode = "";

	// Open '_fileName'
	std::ifstream shaderStream(_fileName);
	if (!shaderStream.is_open())
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			SDL_LOG_PRIORITY_ERROR,
			"Error while loading shader %s!", _fileName.string().c_str());
		return;
}

	// Load the contents of the file into the 'shaderCode' variable.
	std::string line = "";
	while (std::getline(shaderStream, line))
	{
		shaderCode += line + "\n";
	}

	shaderStream.close();

	compileShaderFromSource(loadedShader, shaderCode);
}

void compileShaderFromSource( const GLuint loadedShader, std::string_view shaderCode )
{
	// Assign the loaded source code to the shader object
	const char* sourcePointer = shaderCode.data();
	GLint sourceLength = static_cast<GLint>(shaderCode.length());

	glShaderSource(loadedShader, 1, &sourcePointer, &sourceLength);

	// Let's compile the shader
	glCompileShader(loadedShader);

	// Check whether the compilation was successful
	GLint result = GL_FALSE;
	int infoLogLength;

	// For this, retrieve the status
	glGetShaderiv(loadedShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(loadedShader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (GL_FALSE == result || infoLogLength != 0)
	{
		// Get and log the error message.
		std::string ErrorMessage(infoLogLength, '\0');
		glGetShaderInfoLog(loadedShader, infoLogLength, NULL, ErrorMessage.data());

		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			(result) ? SDL_LOG_PRIORITY_WARN : SDL_LOG_PRIORITY_ERROR,
			"[glLinkProgram] Shader compile error: %s", ErrorMessage.data());
	}
}


void AssembleProgram( const GLuint programID, const std::filesystem::path& vs_filename, const std::filesystem::path& fs_filename )
{
	//
	// Loading shaders
	//

	if (programID == 0) return;

	GLuint vs_ID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs_ID = glCreateShader(GL_FRAGMENT_SHADER);

	if (vs_ID == 0 || fs_ID == 0)
	{
		SDL_SetError("Error while initing shaders (glCreateShader)!");
	}

	loadShader(vs_ID, vs_filename);
	loadShader(fs_ID, fs_filename);

	// Add shaders to the program
	glAttachShader(programID, vs_ID);
	glAttachShader(programID, fs_ID);

	// We link the shaders (connecting outgoing-incoming variables etc.)
	glLinkProgram(programID);

	// Check for linking errors
	GLint infoLogLength = 0, result = 0;

	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (GL_FALSE == result || infoLogLength != 0)
	{
		std::string ErrorMessage(infoLogLength, '\0');
		glGetProgramInfoLog(programID, infoLogLength, nullptr, ErrorMessage.data());
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			(result) ? SDL_LOG_PRIORITY_WARN : SDL_LOG_PRIORITY_ERROR,
			"[glLinkProgram] Shader linking error: %s", ErrorMessage.data());
	}

	// No need for these
	glDeleteShader(vs_ID);
	glDeleteShader(fs_ID);
}

static void invert_image_RGBA(int pitchInPixels, int height, Uint32* image_pixels)
{
	int height_div_2 = height / 2;
	Uint32* lower_data  =image_pixels;
	Uint32* higher_data =image_pixels + ( height - 1 ) * pitchInPixels;

	for ( int index = 0; index < height_div_2; index++ )
	{
		for ( int rowIndex = 0; rowIndex < pitchInPixels; rowIndex++ )
		{
			*lower_data ^= higher_data[ rowIndex ];
			higher_data[ rowIndex ] ^= *lower_data;
			*lower_data ^= higher_data[ rowIndex ];

			lower_data++;
		}
		higher_data -= pitchInPixels;
	}
}

void TextureFromFile(const GLuint tex, const std::filesystem::path& fileName, GLenum Type, GLenum Role)
{
	if (tex == 0)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			SDL_LOG_PRIORITY_ERROR,
			"Texture object needs to be inited before loading %s !", fileName.string().c_str());
		return;
	}

	// Load the image
	SDL_Surface* loaded_img = IMG_Load(fileName.string().c_str());

	if (loaded_img == nullptr)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			SDL_LOG_PRIORITY_ERROR,
			"[TextureFromFile] Error while loading texture: %s", fileName.string().c_str());
		return;
	}

	// SDL stores the colors in Uint32, hence we need to account for the byte
	// order here
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	Uint32 format = SDL_PIXELFORMAT_ABGR8888;
#else
	Uint32 format = SDL_PIXELFORMAT_RGBA8888;
#endif

	// Convert the image format to 32bit RGBA if it wasn't that already
	SDL_Surface* formattedSurf = SDL_ConvertSurfaceFormat(loaded_img, format, 0);
	if (formattedSurf == nullptr)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			SDL_LOG_PRIORITY_ERROR,
			"[TextureFromFile] Error while processing texture");
		return;
	}

	// While (0,0) in SDL means top-left, in OpenGL it means bottom-left, so we
	// need to convert it from one to another
	if (Type != GL_TEXTURE_CUBE_MAP && Type != GL_TEXTURE_CUBE_MAP_ARRAY)
		invert_image_RGBA(formattedSurf->pitch / sizeof(Uint32), formattedSurf->h, reinterpret_cast<Uint32*>(formattedSurf->pixels));

	glBindTexture(Type, tex);
	glTexImage2D(
		Role, 						// The binding point that holds the texture
		0, 							// Level-of-detail
		GL_RGBA, 					// Texture's internal format (GPU side)
		formattedSurf->w, 			// Width
		formattedSurf->h, 			// Height
		0, 							// Must be 0 ( https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml )
		GL_RGBA, 					// Source (CPU side) format
		GL_UNSIGNED_BYTE, 			// Data type of the pixel data (CPU side)
		formattedSurf->pixels);		// Pointer to the data

	glBindTexture(Type, 0);

	// Free the SDL_Surfaces
	SDL_FreeSurface(formattedSurf);
	SDL_FreeSurface(loaded_img);
}

void SetupTextureSampling(GLenum Target, GLuint textureID, bool generateMipMap)
{
	// Sampling parameters
	glBindTexture(Target, textureID);
	if (generateMipMap) glGenerateMipmap(Target); // Generate mipmap
	// Bilinear filtering for magnification (default value)
	glTexParameteri(Target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Trilinear filtering from the mipmaps while minifying (default value)
	glTexParameteri(Target, GL_TEXTURE_MIN_FILTER, generateMipMap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

	// How should sampling from outside of the texture behave?
	glTexParameteri(Target, GL_TEXTURE_WRAP_S, GL_REPEAT); // Vertically
	glTexParameteri(Target, GL_TEXTURE_WRAP_T, GL_REPEAT); // Horizontally
	glBindTexture(Target, 0);
}

void CleanOGLObject(OGLObject& ObjectGPU)
{
	glDeleteBuffers(1, &ObjectGPU.vboID);
	ObjectGPU.vboID = 0;
	glDeleteBuffers(1, &ObjectGPU.iboID);
	ObjectGPU.iboID = 0;
	glDeleteVertexArrays(1, &ObjectGPU.vaoID);
	ObjectGPU.vaoID = 0;
}