// ----------------------------------------
// Class:		DCEL stripification
// Description:	Loads and stripifies a model
// ----------------------------------------

#pragma once
#ifndef DCSTRIPIFICATION
#define DCSTRIPIFICATION

// ----------------------------------------
// INCLUDES
// ----------------------------------------
// STL
#include <List>
#include <vector>

// Direct X
#include <D3D11.h>

// DCEL - Half-Edge
#include <DCEL\Mesh.h>
#include "DCMeshData.h" // Personalised mesh data

// CoreStructures
#include <CoreStructures\CGTextureCoord.h>
#include <importers\CGImporters.h>
#include <CGModel\CGPolyMesh.h>
#include <CGModel\CGModel.h>
#include "Source\CGBaseModel.h"
#include "Source\CGVertexExt.h"

// ----------------------------------------

// Structure for vertex duplication
struct vDuplication
{
	unsigned int vertIndex;
	unsigned int texIndex;
};

// Typedef for ease of use
typedef Mesh<VertexData, HalfEdgeData, FaceData> DCMesh;

// ----------------------------------------
// CLASS INTERFACE DESIGN
// ----------------------------------------
class DCStripification : public CGBaseModel
{
// ----------------------------------------
private:
	// Attributes -------------------------
	// Model (DCEL)
	DCMesh model;

	// Texture Coordinates
	bool textured;
	CoreStructures::CGTextureCoord* texCoords;

	// Direct X and Shader variables
	ID3D11ShaderResourceView	*textureResourceView;
	ID3D11SamplerState			*sampler;

	// List for vertex duplications
	std::list<vDuplication> duplications;

	// List of strips - stored as face indexes
	std::list< std::vector<unsigned int>* > strips;
	std::list< std::vector<unsigned int>* >::iterator stripsIt; // Iterator

	// Stripification
	bool stripify;

	// Stripification settings
	unsigned int maxLength;

	// Methods ----------------------------
	// Used for buffer setup
	void setupBuffers(ID3D11Device *device, ID3DBlob *vsBytecode);

	// Setup Index buffer
	DWORD* setupIndexBuffer(ID3D11Device *device);
	DWORD* setupStripification(ID3D11Device *device);

	// Strip creation functions
	void createStrip(DCMesh::Face* face);
	DCMesh::Face* getFreeFace(); // Gets a free face with the lowest degree
	DCMesh::Face* getNextFace(DCMesh::Face* face);
	bool isTextureSeam(DCMesh::Face* face1, DCMesh::Face* face2);

	// Index searching/creation
	unsigned int otherVertex(DCMesh::Face* face1, DCMesh::Face* face2);
	unsigned int commonVertex(DCMesh::Face* face1, DCMesh::Face* face2);

	// Loads model data into DCMesh
	void loadModel(wchar_t* filename);

	// Loads texture resource
	void loadResources(ID3D11Device *device, wchar_t* filename);

	// Duplication check
	void checkDuplication(DCMesh::Vertex* vert, DCMesh::Face* face, unsigned int texIndex, int faceVertIndex);

// ----------------------------------------
public:

	// Constructor / Destructor
	DCStripification(ID3D11Device *device, ID3DBlob *vsBytecode, wchar_t* modelFilename, wchar_t* textureFilename, bool stripify = 1);
	~DCStripification();

	// Render
	void render(ID3D11DeviceContext *context);

};
// ----------------------------------------

#endif