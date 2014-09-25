
// Jak's stripification class source file
#include "JStrip.h"

using namespace CoreStructures;
using namespace std;

// Constructor
JStrip::JStrip(ID3D11Device *device, ID3DBlob *vsBytecode, wchar_t* filename)
{
	// Load model
	CGModel* testModel = new CGModel();
	importGSF(filename, testModel);

	// 1. Create copy
	meshCopy = new CGPolyMesh(testModel->getMeshAtIndex(0));

	// 2. Get access to data by obtaining the mesh's CGBaseMeshDefStruct
	meshCopy->createMeshDef(&meshData);

	// 3. Setup buffer data
	setupBuffers(device, vsBytecode);
}

JStrip::~JStrip()
{

}

void JStrip::setupBuffers(ID3D11Device *device, ID3DBlob *vsBytecode)
{
	// Local variables
	CGVertexExt* vertices = nullptr;
	DWORD* indices = nullptr;
	
	vertices =  (CGVertexExt*) malloc (sizeof(CGVertexExt) * meshData.N);
	indices = (DWORD*) malloc (sizeof(DWORD) * (meshData.n * 3));

	if (!vertices || !indices)
		throw("Cannot create stripification model buffers");

	int count = 0;

	// Setup vertex data
	for(int i = 0; i < meshData.N; ++i)
	{
		vertices[i].pos.x = meshData.V[i].x;
		vertices[i].pos.y = meshData.V[i].y;
		vertices[i].pos.z = meshData.V[i].z;

		vertices[i].texCoord.x = meshData.Vt[i].q;
		vertices[i].texCoord.y = meshData.Vt[i].s;

		vertices[i].matDiffuse = XMCOLOR(0.0f, 1.0f, 0.0f, 1.0f);
		vertices[i].matSpecular = XMCOLOR(0.0f, 0.0f, 0.0f, 0.0f);

		if(count == 0)
		{
			vertices[i].normal.x = meshData.Vn[count].x;
			vertices[i].normal.y = meshData.Vn[count].y;
			vertices[i].normal.z = meshData.Vn[count].z;
		}

		if(count != 2)
			++count;
		else
			count = 0;
	}

	// Reset count
	count = 0;

	// Setup indices
	for(int i = 0; i < meshData.n; ++i)
	{
		indices[count] = meshData.Fv[i].v1;
		indices[count + 1] = meshData.Fv[i].v2;
		indices[count + 2] = meshData.Fv[i].v3;

		count += 3;
	}

	D3D11_BUFFER_DESC vertexDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

	vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexDesc.ByteWidth = sizeof(CGVertexExt) * meshData.N;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexData.pSysMem = vertices;

	HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

	// Check for errors
	if (!SUCCEEDED(hr))
		throw("Vertex buffer cannot be created");

	// 4. Setup index data
	D3D11_BUFFER_DESC indexDesc;
	D3D11_SUBRESOURCE_DATA indexData;

	ZeroMemory(&indexDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));

	indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexDesc.ByteWidth = sizeof(DWORD) * (meshData.n * 3);
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexData.pSysMem = indices;

	hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

	if (!SUCCEEDED(hr))
		throw("Index buffer cannot be created");

	free(vertices);
	free(indices);

	hr = CGVertexExt::createInputLayout(device, vsBytecode, &inputLayout);

	if (!SUCCEEDED(hr))
		throw("Cannot create input layout interface");
}

void JStrip::render(ID3D11DeviceContext *context)
{
	// Set vertex layout
	context->IASetInputLayout(inputLayout);

	// Set stripification model vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = {vertexBuffer};
	UINT vertexStrides[] = {sizeof(CGVertexExt)};
	UINT vertexOffsets[] = {0};

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw stripification model
	context->DrawIndexed((meshData.n * 3), 0, 0);
}