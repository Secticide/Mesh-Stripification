// ----------------------------------------
// Class: DCEL Stripification source
// ----------------------------------------

// Include header
#include "DCStripification.h"

#include <iostream>

// Ensure correct namespace use
using namespace std;

// Constructor
DCStripification::DCStripification(ID3D11Device *device, ID3DBlob *vsBytecode, wchar_t* modelFilename, wchar_t* textureFilename, bool stripify)
{
	// Default buffer settings
	vertexBuffer = NULL;
	indexBuffer = NULL;
	inputLayout = NULL;
	texCoords = NULL;
	textured = true;

	// Set stripification settings
	this->stripify = stripify;
	maxLength = 500;

	// Ensure cleared vector
	duplications.clear();

	// Load model
	loadModel(modelFilename);

	// Load texture
	loadResources(device, textureFilename);

	// Setup buffers
	setupBuffers(device, vsBytecode);
}

// Destructor
DCStripification::~DCStripification()
{
	// Clean memory
	duplications.clear();

	if(texCoords)
		delete texCoords;

	if(textureResourceView)
		delete textureResourceView;

	if(sampler)
		delete sampler;
}

// Buffer setup
void DCStripification::setupBuffers(ID3D11Device *device, ID3DBlob *vsBytecode)
{
	// Fills the Direct X buffers
	CGVertexExt* vertices = NULL;
	DWORD* indices = NULL;

	// Vertex counter
	vertices = (CGVertexExt*) malloc (sizeof(CGVertexExt) * (model.getNumVertices() + duplications.size()));

	// Setup vertex array
	for(unsigned int i = 0; i < model.getNumVertices(); ++i)
	{
		// Get vertex
		DCMesh::Vertex* vert = model.getVertex(i);

		// Set position data
		vertices[i].pos.x = vert->getData().position.x;
		vertices[i].pos.y = vert->getData().position.y;
		vertices[i].pos.z = vert->getData().position.z;

		// Set normal data
		vertices[i].normal.x += vert->getData().normal.x;
		vertices[i].normal.y += vert->getData().normal.y;
		vertices[i].normal.z += vert->getData().normal.z;
	}

	// Index counter
	unsigned int count = 0;

	// Create vertex duplications
	for(std::list<vDuplication>::iterator dupeIt = duplications.begin();
		dupeIt != duplications.end(); ++dupeIt)
	{
		// Get vertex
		DCMesh::Vertex* vert = model.getVertex(dupeIt->vertIndex);

		// Set position data
		vertices[model.getNumVertices() + count].pos.x = vert->getData().position.x;
		vertices[model.getNumVertices() + count].pos.y = vert->getData().position.y;
		vertices[model.getNumVertices() + count].pos.z = vert->getData().position.z;

		// Set normal data
		vertices[model.getNumVertices() + count].normal.x += vert->getData().normal.x;
		vertices[model.getNumVertices() + count].normal.y += vert->getData().normal.y;
		vertices[model.getNumVertices() + count].normal.z += vert->getData().normal.z;

		// Increment counter
		count += 1;
	}

	// Setup indexes based on stripification setting
	if(stripify)
		indices = setupStripification(device); // Stripification
	else
		indices = setupIndexBuffer(device); // Normal setup

	// Reset counter
	count = 0;

	// Setup texture index array
	if(textured)
	{
		for(unsigned int i = 0; i < model.getNumFaces(); ++i)
		{
			for(unsigned int j = 0; j < 3; ++j)
			{
				// Get face
				DCMesh::Face* face = model.getFace(i);

				// Setup texture coordinates
				vertices[face->getData().v[j]].texCoord.x = texCoords[face->getData().t[j]].s;
				vertices[face->getData().v[j]].texCoord.y = 1 - texCoords[face->getData().t[j]].t;

				// iterate count
				count += 1;
			}
		}
	}

	// Buffer setup
	D3D11_BUFFER_DESC vertexDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

	vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexDesc.ByteWidth = sizeof(CGVertexExt) * (model.getNumVertices() + duplications.size());
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexData.pSysMem = vertices;

	HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

	// Check for errors
	if (!SUCCEEDED(hr))
		throw("Vertex buffer cannot be created");

	if(vertices)
		free(vertices);

	if(indices)
		free(indices);

	hr = CGVertexExt::createInputLayout(device, vsBytecode, &inputLayout);

	if (!SUCCEEDED(hr))
		throw("Cannot create input layout interface");
}

// Setup index buffer - Standard: No stripification
DWORD* DCStripification::setupIndexBuffer(ID3D11Device *device)
{
	// Normal index setup
	DWORD* indices = NULL;

	indices = (DWORD*) malloc (sizeof(DWORD) * (model.getNumFaces() * 3));

	// Index counter
	unsigned int count = 0;

	// Setup index array
	for(unsigned int i = 0; i < model.getNumFaces(); ++i)
	{
		for(unsigned int j = 0; j < 3; ++j)
		{
			// Setup vertex indices
			indices[count] = model.getFace(i)->getData().v[j];

			// iterate count;
			count += 1;
		}
	}

	// Setup index data
	D3D11_BUFFER_DESC indexDesc;
	D3D11_SUBRESOURCE_DATA indexData;

	ZeroMemory(&indexDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));

	indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexDesc.ByteWidth = sizeof(DWORD) * (model.getNumFaces() * 3);
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexData.pSysMem = indices;

	HRESULT hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

	if (!SUCCEEDED(hr))
		throw("Index buffer cannot be created");

	return indices;
}

// Setup index buffer - Stripified
DWORD* DCStripification::setupStripification(ID3D11Device *device)
{
	// Index buffer for stripification
	DWORD* indices = NULL;
	std::vector<unsigned int> indexVec;

	cerr << "Stripifiying Mesh..." << endl;

	// 1. Select face with the most neighbours (free)
	DCMesh::Face* face = getFreeFace();

	// Loop while there is a free face
	while(face)
	{
		// 2. Create strip from face
		createStrip(face);

		// Get the next free face
		face = getFreeFace();
	}

	cerr << strips.size() << " strips created." << endl;

	// 3. Setup strip index buffer
	for(stripsIt = strips.begin(); stripsIt != strips.end(); ++stripsIt)
	{
		// Pointer to strip vector
		std::vector<unsigned int>* strip = *stripsIt;
		int swapCount = 0;

		// Loop through each of the faces in the strip and add the indices
		for(unsigned int i = 0; i < strip->size(); ++i)
		{
			// 1. Add verticies to the strip - from stand alone vertex
			if(i == 0)
			{
				// If it is the first face in the strip
				// Find the standalone vertex
				bool found = false;
				int j = 0;

				// Check for single triangle strip
				if(strip->size() > 1)
				{
					while(j < 3 && !found)
					{
						for(int k = 0; k < 3; ++k)
						{
							if(model.getFace((*strip)[i])->getData().v[j] == model.getFace((*strip)[i + 1])->getData().v[k])
							{
								found = true;
							}
						}

						// The standalone has been found
						if(!found)
						{
							indexVec.push_back(model.getFace((*strip)[i])->getData().v[j]);

							if(j + 1 < 3)
								indexVec.push_back(model.getFace((*strip)[i])->getData().v[j + 1]);
							else
								indexVec.push_back(model.getFace((*strip)[i])->getData().v[0]);

							if(j + 2 < 3)
								indexVec.push_back(model.getFace((*strip)[i])->getData().v[j + 2]);
							else
								indexVec.push_back(model.getFace((*strip)[i])->getData().v[(j + 2) - 3]);
						}
						else
							found = false;

						// Increment j
						++j;
					}
				}
				else
				{
					for(int k = 0; k < 3; ++k)
						indexVec.push_back(model.getFace((*strip)[i])->getData().v[k]);
				}
			}
			// 2. Increment i, adding the next triangle
			else
			{
				bool found = false;
				int j = 0;

				// 3. Consider the existance of i + 1
				if(i == strip->size() - 1)
					// If no, add the other vertex of i
					indexVec.push_back(otherVertex(model.getFace((*strip)[i]), model.getFace((*strip)[i - 1])));
				// If yes, find the common vertex between i - 1 and i + 1
				else
				{
					// Find common vertex in i - 1 and i + 1
					unsigned int common = commonVertex(model.getFace((*strip)[i - 1]), model.getFace((*strip)[i + 1]));

					// If the common vertex is the 'tail' of the strip, add the 'other vertex'
					if(indexVec.back() == common)
						indexVec.push_back(otherVertex(model.getFace((*strip)[i]), model.getFace((*strip)[i - 1])));
					// If not, add the common vertex to the strip (swap), then add the 'other vertex'
					else
					{
						indexVec.push_back(common);
						indexVec.push_back(otherVertex(model.getFace((*strip)[i]), model.getFace((*strip)[i - 1])));

						// A swap has occured - increment count
						++swapCount;
					}
				}
			}
		}

		// DIRT.lib
		(*strip).resize((*strip).size() + swapCount);
	}

	// Allocate index buffer memory
	indices = (DWORD*) malloc (sizeof(DWORD) * indexVec.size());

	// Copy index values
	for(unsigned int i = 0; i < indexVec.size(); ++i)
	{
		indices[i] = indexVec[i];
		//cerr << indices[i] << ", ";
	}

	// Setup index data
	D3D11_BUFFER_DESC indexDesc;
	D3D11_SUBRESOURCE_DATA indexData;

	ZeroMemory(&indexDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));

	indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexDesc.ByteWidth = sizeof(DWORD) * indexVec.size();
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexData.pSysMem = indices;

	HRESULT hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

	if (!SUCCEEDED(hr))
		throw("Index buffer cannot be created");

	return indices;
}

// Create strip
void DCStripification::createStrip(DCMesh::Face* face)
{
	// Strip - list of face indexes
	std::vector<unsigned int>* strip;

	// Get the first face to stripify and create new strip
	strip = new vector<unsigned int>;

	// Loop exit conditions
	unsigned int i = 0;
	bool exit = false; // Used for if there are no neighbours

	// Faces added to strip iteratively
	while(i < maxLength && !exit)
	{
		// Add the first triangle to the strip
		strip->push_back(model.getFaceId(face));
		
		// Update face free status
		face->getData().free = 0;

		// Search face neighbours - update free and select next face (based on degree)
		EdgeIteratorT<VertexData, HalfEdgeData, FaceData> edgeIt(face); // Create edge iterator

		// Update neighbours
		for(int j = 0; j < 3; ++j)
		{
			DCMesh::Face* testFace = edgeIt.getNext()->getTwin()->getFace();

			// Check if the face exists
			if(testFace)
				testFace->getData().degree -= 1;
		}

		// Get next face to add to strip and iterate
		face = getNextFace(face);

		// GetNextFace will return NULL if no neighbours are found
		if(!face)
			exit = true;

		// Iterate
		++i;
	}

	// Once strip has been completed - or reached maxLength; add to list
	strips.push_back(strip);
}

// Get free face - with lowest degree
DCMesh::Face* DCStripification::getFreeFace()
{
	DCMesh::Face* selected = NULL;

	for(unsigned int i = 0; i < model.getNumFaces(); ++i)
	{
		if(model.getFace(i)->getData().free)
		{
			// If selected is not
			if(!selected)
				selected = model.getFace(i);
			else if(model.getFace(i)->getData().degree < selected->getData().degree)
				selected = model.getFace(i);
		}
	}

	return selected;
}

// Get Next Face - Free/Low degree/Not texture seam
DCMesh::Face* DCStripification::getNextFace(DCMesh::Face* face)
{
	// Check if current face has no neighbours
	if(face->getData().degree != 0)
	{
		// Variable for selected face
		DCMesh::Face* selected = NULL;

		// Edge iterator to get neighbours
		EdgeIteratorT<VertexData, HalfEdgeData, FaceData> edgeIt(face);

		// Loop through neighbours
		for(unsigned int i = 0; i < 3; ++i)
		{
			// Get neighbour
			DCMesh::Face* neighbour = edgeIt.getNext()->getTwin()->getFace();

			// Check if the face is on a texture seam - not considered
			if(neighbour) // Check if neighbour exists
			{
				if(!isTextureSeam(face, neighbour) && neighbour->getData().free)
				{
					// Check if the neighbour has a degree of 0
					if(neighbour->getData().degree == 0)
						return neighbour;

					// If it is not on a texture seam - check degree against others
					if(!selected) // If the selected face has not been set
						selected = neighbour;
					// If selected has been set, check against neighbour
					else if(neighbour->getData().degree < selected->getData().degree)
						selected = neighbour; // Reassign selected to lowest degree
				}
			}
		}

		// Once each neighbour has been checked - return best face to add to strip
		return selected;
	}
	else
		return NULL;
}

// Check texture seam
bool DCStripification::isTextureSeam(DCMesh::Face* face1, DCMesh::Face* face2)
{
	if(textured)
	{
		// A count for the number of different indices
		int difference = 0;

		// If the index is found
		bool found = false;

		// If there is more than a single different index - there is a texture seam
		for(int i = 0; i < 3; ++i)
		{
			for(int j = 0; j < 3; ++j)
				if(face1->getData().v[i] == face2->getData().v[j])
					found = true;

			if(!found)
				difference += 1;

			// Reset found
			found = false;
		}

		// Returns 1 if there is more than a single difference
		if(difference > 1)
			return 1;
		else
			return 0;
	}
	else
		return 0;
}

// Other vertex - returns index of face1's other vertex
unsigned int DCStripification::otherVertex(DCMesh::Face* face1, DCMesh::Face* face2)
{
	int count = 0;
	bool found = false;

	// Returns the other vertex in [FACE 1] compared to face2
	while(count < 3 && !found)
	{
		for(int i = 0; i < 3; ++i)
		{
			if(face1->getData().v[count] == face2->getData().v[i])
				found = true;
		}

		// If the vertex is found - it is not 'other'
		// If it isn't - it is the other
		if(!found)
			return face1->getData().v[count];
		else
			found = false;

		// Increment count
		++count;
	}
}

// Common vertex
unsigned int DCStripification::commonVertex(DCMesh::Face* face1, DCMesh::Face* face2)
{
	int count = 0;
	bool found = false;

	// Returns the common vertex in face1 and face2
	while(count < 3 && !found)
	{
		for(int i = 0; i < 3; ++i)
		{
			if(face1->getData().v[count] == face2->getData().v[i])
				found = true;
		}

		// If the vertex is found - it is 'common'
		if(found)
			return face1->getData().v[count];
		else
			found = false;

		// Increment count
		++count;
	}
}

// Load Model
void DCStripification::loadModel(wchar_t* filename)
{
	// 1. Create new model file
	CGModel* import = new CGModel();

	// DEBUG
	cerr << "Loading model data:" << endl;

	// 2. Load model using CGImport3's obj loader
	importOBJ(filename, import);

	// 3. Make a copy to access private attributes
	CGPolyMesh* meshCopy = new CGPolyMesh(import->getMeshAtIndex(0));
	
	// Aquire private attributes
	CGBaseMeshDefStruct* meshData = new CGBaseMeshDefStruct();
	meshCopy->createMeshDef(meshData);

	// DEBUG
	cerr << "Reserving memory for: " << endl;
	cerr << meshData->N << " vertices;" << endl;
	cerr << meshData->n << " faces;" << endl;
	cerr << (2 * (3 * meshData->n)) << " half-edges." << endl;

	// Clear model data and reserve space
	model.clear();
    model.getVertices().reserve(meshData->N);
    model.getFaces().reserve(meshData->n);
    model.getHalfEdges().reserve(2 * (3 * meshData->n));

	// DEBUG
	cerr << "Populating DCEL structure..." << endl;
	cerr << "Populating vertices..." << endl;

	// 4. Fill the DCEL with data from the imported obj
	// Create vertices
	for(int i = 0; i < meshData->N; ++i)
	{
		// Create new vertex
		DCMesh::Vertex* vert = model.createGetVertex();

		// Set data - position, normals and texture coordinates
		vert->getData().setPosition(meshData->V[i]);
		vert->getData().setNormal(meshData->Vn[i].unitVector());
	}

	// Check for texture coordinates
	if(meshData->VtSize)
	{
		// Copy texture coordinates
		// Allocate memory
		texCoords = (CoreStructures::CGTextureCoord*) malloc (sizeof(CoreStructures::CGTextureCoord) * meshData->VtSize);

		for(int i = 0; i <  meshData->VtSize; ++i)
			texCoords[i] = (*meshData).Vt[i];
	}
	else
		// There are no texture coordinates - dont load them
		textured = false;

	// DEBUG
	cerr << "Populating faces..." << endl;

	// Create faces - triangular
	for(int i = 0; i < meshData->n; ++i)
		model.createTriangularFace(meshData->Fv[i].v1, meshData->Fv[i].v2, meshData->Fv[i].v3);

	// Check faces & Manage unhandled
	model.checkAllFaces();
	model.manageUnhandledTriangles();

	// Setup faces
	for(unsigned int i = 0; i < model.getNumFaces(); ++i)
	{
		// Create new face
		DCMesh::Face* face = model.getFace(i);

		// Per-face indices
		face->getData().setIndices(meshData->Fv[i].v1, meshData->Fv[i].v2, meshData->Fv[i].v3);

		// Duplicate faces are not needed if there are no texture coordinates
		if(textured)
		{
			// Per-face texture coordinates
			face->getData().setTexIndices(meshData->Fvt[i].t1, meshData->Fvt[i].t2, meshData->Fvt[i].t3);

			// Check for vertex duplications - texture coordinate seams
			checkDuplication(model.getVertex(meshData->Fv[i].v1), face, meshData->Fvt[i].t1, 0);
			checkDuplication(model.getVertex(meshData->Fv[i].v2), face, meshData->Fvt[i].t2, 1);
			checkDuplication(model.getVertex(meshData->Fv[i].v3), face, meshData->Fvt[i].t3, 2);
		}
	}

	// DEBUG
	cerr << duplications.size() << " duplicated vertices." << endl;
	cerr << model.getNumHalfEdges() << " half-edges created." << endl;

	// Resize half-edge vector to save memory
	model.getHalfEdges().resize(model.getNumHalfEdges());

	cerr << "Linking faces... " << endl;

	// Loop through faces - alter degree
	for(unsigned int i = 0; i < model.getNumFaces(); ++i)
	{
		// Create edge iterator
		EdgeIteratorT<VertexData, HalfEdgeData, FaceData> edgeIt(model.getFace(i));

		// Check neighbours
		for(int j = 0; j < 3; ++j)
			edgeIt.getNext()->getFace()->getData().degree += 1; // Add to number of neighbours
	}

	cerr << "Done!" << endl;

	// 5. Clean up imported data - we have copied the data we need
	if(meshData)
		delete meshData;

	if(meshCopy)
		delete meshCopy;

	if(import)
		delete import;
}

// Loads shader and rexture resources
void DCStripification::loadResources(ID3D11Device* device, wchar_t* filename)
{
	// 1. Load texture
	D3DX11CreateShaderResourceViewFromFile(device, filename, 0, 0, &textureResourceView, 0);

	// 2. Setup linear sampler object
	D3D11_SAMPLER_DESC linearDesc;

	ZeroMemory(&linearDesc, sizeof(D3D11_SAMPLER_DESC));

	linearDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.MinLOD = 0.0f;
	linearDesc.MaxLOD = 0.0f;
	linearDesc.MipLODBias = 0.0f;
	linearDesc.MaxAnisotropy = 0; // unused for non-anisotropic filtering
	linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateSamplerState(&linearDesc, &sampler);
}

// Duplication check
void DCStripification::checkDuplication(DCMesh::Vertex* vert, DCMesh::Face* face, unsigned int texIndex, int faceVertIndex)
{
	if(vert->getData().set) // Texture coord has been set
	{
		if(vert->getData().texIndex != texIndex)
		{
			bool found = false;
			unsigned int index = 0;

			// Check if the duplication has been recorded before
			std::list<vDuplication>::iterator dupeIt = duplications.begin();

			while(dupeIt != duplications.end() && !found)
			{
				// If the duplication has happened before
				if(dupeIt->vertIndex == model.getVertexId(vert) && dupeIt->texIndex == texIndex)
				{
					// Set the vertex to the same index
					face->getData().v[faceVertIndex] = (model.getNumVertices() + index);
					found = true; // Exit condition
				}

				// Increment
				++dupeIt;
				++index;
			}

			// If the duplication is new
			if(!found)
			{
				// Record duplication
				vDuplication duplication;

				duplication.vertIndex = model.getVertexId(vert);
				duplication.texIndex = texIndex;

				duplications.push_back(duplication);

				// Alter face index
				face->getData().v[faceVertIndex] = (model.getNumVertices() + duplications.size()) - 1;
			}
		}
	}
	else
	{
		vert->getData().set = 1;
		vert->getData().texIndex = texIndex;
	}
}

// Render
void DCStripification::render(ID3D11DeviceContext *context)
{
	// Link texture to shader
	// Link resource views and sampler with variables within the shader
	if(textured)
	{
		context->PSSetShaderResources(0, 1, &textureResourceView);
		context->PSSetSamplers(0, 1, &sampler);
	}

	// Set vertex layout
	context->IASetInputLayout(inputLayout);

	// Set stripification model vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = {vertexBuffer};
	UINT vertexStrides[] = {sizeof(CGVertexExt)};
	UINT vertexOffsets[] = {0};

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	if(stripify)
	{
		// Set primitive topology for IA
		context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// Drawn index
		unsigned int index = 0;

		// Loop through and draw all strips
		for(stripsIt = strips.begin(); stripsIt != strips.end(); ++stripsIt)
		{
			//stripsIt = strips.begin();
			context->DrawIndexed((*stripsIt)->size() + 2, index, 0);
			index += (*stripsIt)->size() + 2;
		}
	}
	else
	{
		// Set primitive topology for IA
		context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP

		// Draw stripification model
		context->DrawIndexed(model.getNumFaces() * 3, 0, 0);
	}
}