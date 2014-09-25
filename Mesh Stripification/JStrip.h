
#pragma once
#ifndef JSTRIP
#define JSTRIP

// Includes
#include <D3D11.h>
#include <xnamath.h>

#include <Importers\CGImporters.h>
#include <CoreStructures\CoreStructures.h>
#include "Source\CGVertexExt.h"
#include "Source\CGBaseModel.h"
#include "CGModel\CGPolyMesh.h"
#include "CGModel\CGModel.h"

// --------------------------------
// Jak's Stripification class
// --------------------------------
class JStrip : public CGBaseModel
{
private:
	CGPolyMesh*						meshCopy;
	CGBaseMeshDefStruct				meshData;

	ID3D11ShaderResourceView		*textureResourceView;
	ID3D11SamplerState				*sampler;

public:
	// Constructor & destructor
	JStrip(ID3D11Device *device, ID3DBlob *vsBytecode, wchar_t* filename);
	~JStrip();

	// Buffer Setup
	void setupBuffers(ID3D11Device *device, ID3DBlob *vsBytecode);

	// Stripification algorithm
	void stripify();

	// Render
	void render(ID3D11DeviceContext *context);
};

#endif