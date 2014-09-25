// ----------------------------------------
// Class: DCEL Mesh Data Source
// ----------------------------------------

// Include header
#include "DCMeshData.h"

// ----------------------------------------
// Vertex Data
// ----------------------------------------

// Constructor
VertexData::VertexData()
{
	// Set initial values
	this->position.x = 0;
	this->position.y = 0;
	this->position.z = 0;

	set = 0;
}

// Destructor
VertexData::~VertexData()
{
	// Empty
}

// Constuctor Overloads
// Position - floats x, y, z
VertexData::VertexData(float x, float y, float z)
{
	// Set initial values
	this->position.x = x;
	this->position.y = y;
	this->position.z = z;
}

// Position Setter
// Position - GUVector4
void VertexData::setPosition(CoreStructures::GUVector4 position)
{
	this->position = position;
}

// Normal Setter
void VertexData::setNormal(CoreStructures::GUVector4 normal)
{
	this->normal = normal;
}

// ----------------------------------------
// Half Edge Data
// ----------------------------------------

// Constructor
HalfEdgeData::HalfEdgeData()
{
	// Set initial state
	modified = false;
}

// Destructor
HalfEdgeData::~HalfEdgeData()
{
	// Empty
}

// Getters & Setters
void HalfEdgeData::modify()
{
	modified = true;
}

bool HalfEdgeData::isModified()
{
	return modified;
}

// ----------------------------------------
// Face Data
// ----------------------------------------

// Constructor
FaceData::FaceData()
{
	// Set initial values
	this->r = 0;
	this->g = 0;
	this->b = 0;

	for(int i = 0; i < 3; ++i)
	{
		v[i] = 0; // Vertex
		t[i] = 0; // Texture
	}

	// Initial free setting
	free = 1;
	degree = 0; // No free neighbours
}

// Destructor
FaceData::~FaceData()
{
	// Empty
}

// Constructor Overloads
// Colour - floats r, g, b
FaceData::FaceData(float r, float g, float b)
{
	this->r = r;
	this->g = g;
	this->b = b;
}

// Colour setter
// Colour - floats r, g, b
void FaceData::setColour(float r, float g, float b)
{
	this->r = r;
	this->g = g;
	this->b = b;
}

// Index setter
// Vertex Indices - unsigned int v1, v2, v3
void FaceData::setIndices(unsigned int v1, unsigned int v2, unsigned int v3)
{
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
}

// Texture Coordinate setter
// Texture Coordinate indices - unsigned int t1, t2, t3
void FaceData::setTexIndices(unsigned int t1, unsigned int t2, unsigned int t3)
{
	t[0] = t1;
	t[1] = t2;
	t[2] = t3;
}