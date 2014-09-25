// ----------------------------------------
// Class:		DCEL Mesh Data
// Description:	Data Structures for mesh
// ----------------------------------------

#pragma once
#ifndef DCMESH
#define DCMESH

// ----------------------------------------
// INCLUDES
// ----------------------------------------
#include <CoreStructures\GUVector4.h>

// ----------------------------------------

// ----------------------------------------
// Vertex Data Interface
// ----------------------------------------
class VertexData
{
// ----------------------------------------
public:
	// Attributes -------------------------
	// Position x, y, z, w
	CoreStructures::GUVector4 position;

	// Normals
	CoreStructures::GUVector4 normal;

	// Duplication Checker variables
	bool set;
	unsigned int texIndex;

	// Methods ----------------------------
	// Constructor & Destructor
	VertexData();
	~VertexData();

	// Constructor Overloads
	VertexData(float x, float y, float z);

	// Position Setter
	void setPosition(CoreStructures::GUVector4 position);

	// Normal Setter
	void setNormal(CoreStructures::GUVector4 normal);

};

// ----------------------------------------
// Half Edge Data Interface
// ----------------------------------------
class HalfEdgeData
{
// ----------------------------------------
private:
	// Attributes -------------------------
	bool modified;

// ----------------------------------------
public:
	// Methods ----------------------------
	// Constructor & Destructor
	HalfEdgeData();
	~HalfEdgeData();

	// Getters & Setters
	void modify();
	
	bool isModified();
};

// ----------------------------------------
// Face Data Interface
// ----------------------------------------
class FaceData
{
// ----------------------------------------
public:
	// Attributes -------------------------
	// Colour
	float r, g, b;

	// Indices - vertex IDs for face
	unsigned int v[3];

	// Texture Coordinate Indices
	unsigned int t[3];

	// Search variables
	bool free; // Has been added to a strip
	int degree; // Number of free trianges adjacent

	// Methods ----------------------------
	// Constructor & Destructor
	FaceData();
	~FaceData();

	// Constructor Overloads
	FaceData(float r, float g, float b);

	// Colour setter
	void setColour(float r, float g, float b);

	// Index setter
	void setIndices(unsigned int v1, unsigned int v2, unsigned int v3);

	// Texture Coordinate Setter
	void setTexIndices(unsigned int v1, unsigned int v2, unsigned int v3);
};

#endif