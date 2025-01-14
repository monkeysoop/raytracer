#pragma once

#include <filesystem>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

/* 

Based on http://www.opengl-tutorial.org/

*/

// Helper classes

struct VertexPosColor
{
    glm::vec3 position;
    glm::vec3 color;
};

struct VertexPosTex
{
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

// Helper functions

void loadShader( const GLuint loadedShader, const std::filesystem::path& _fileName );
void compileShaderFromSource( const GLuint loadedShader, std::string_view shaderCode );

void AssembleProgram( const GLuint programID, const std::filesystem::path& vs_filename, const std::filesystem::path& fs_filename );

void TextureFromFile( const GLuint tex, const std::filesystem::path& fileName, GLenum Type, GLenum Role );

inline void TextureFromFile( const GLuint tex, const std::filesystem::path& fileName, GLenum Type = GL_TEXTURE_2D ) { TextureFromFile( tex, fileName, Type, Type ); }

void SetupTextureSampling( GLenum Target, GLuint textureID, bool generateMipMap = true );

template<typename VertexT>
struct MeshObject
{
    std::vector<VertexT> vertexArray;
    std::vector<GLuint>  indexArray;
};

struct OGLObject
{
    GLuint  vaoID = 0; // Vertex array object resource ID
    GLuint  vboID = 0; // Vertex buffer object resource ID
    GLuint  iboID = 0; // Index buffer object resource ID
    GLsizei count = 0; // How many indeces/vertices do we draw?
};


struct VertexAttributeDescriptor
{
	GLuint         index = -1;
	std::uintptr_t strideInBytes = 0;
	GLint          numberOfComponents = 0;
	GLenum         glType = GL_NONE;
};

template <typename VertexT>
[[nodiscard]] OGLObject CreateGLObjectFromMesh( const MeshObject<VertexT>& mesh, std::initializer_list<VertexAttributeDescriptor> vertexAttrDescList )
{
	OGLObject meshGPU = { 0 };

	glGenVertexArrays(1, &meshGPU.vaoID);	// Generate 1 Vertex Array Object (VAO)
	glBindVertexArray(meshGPU.vaoID);		// Make the freshly generated VAO active (bind it)

	// Generate 1 Vertex Buffer Object (VBO)
	glGenBuffers(1, &meshGPU.vboID);
	glBindBuffer(GL_ARRAY_BUFFER, meshGPU.vboID);	// Make it active (bind it)

	// Transfer data to the buffer bound to GL_ARRAY_BUFFER
	glBufferData(GL_ARRAY_BUFFER,							// Where is the buffer bound
				  mesh.vertexArray.size() * sizeof(VertexT),// Number of BYTES
				  mesh.vertexArray.data(),					// Pointer to the data
				  GL_STATIC_DRAW);	// We do not intend to modify the data later, but we will use the buffer in a LOT of draw calls

	// Generate 1 Index Buffer Object (IBO)
	glGenBuffers(1, &meshGPU.iboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshGPU.iboID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indexArray.size() * sizeof(GLuint), mesh.indexArray.data(), GL_STATIC_DRAW);

	meshGPU.count = static_cast<GLsizei>(mesh.indexArray.size());

	for ( const auto& vertexAttrDesc: vertexAttrDescList )
	{
		glEnableVertexAttribArray(vertexAttrDesc.index); // Enable generic vertex attribute array
		glVertexAttribPointer(
			vertexAttrDesc.index,				// Index of the generic vertex attribute
			vertexAttrDesc.numberOfComponents,	// Component count
			vertexAttrDesc.glType,				// Component type
			GL_FALSE,							// Normalize or not
			sizeof(VertexT),					// Stride (0 would mean tightly packed)
			reinterpret_cast<const void*>(vertexAttrDesc.strideInBytes) // Specifies the offset of the first component
		);
	}

	// We are done with these, unbind them
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return meshGPU;
}

void CleanOGLObject( OGLObject& ObjectGPU );

