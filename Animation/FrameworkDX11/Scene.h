// MIT License
// Copyright (c) 2025 David White
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// Scene class for encapsulating the responsibily for storing / initialising the world and whatever is in it

#pragma once

#include "constants.h"
#include "Camera.h"
#include <d3d11_1.h>
#include <vector>
#include "wrl.h"
#include "structures.h"
#include "scenegraph.h"

class DX11Renderer;

class Scene
{
public:

	Scene() {}

	~Scene() {}

	HRESULT		init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, DX11Renderer* renderer);
	void		cleanUp();
	Camera*		getCamera() { return m_pCamera; }

	void		update(const float deltaTime);
	
	const LightPropertiesConstantBuffer& getLightProperties() { return m_lightProperties; }

	void CreateWaveAnimationSampler(int nodeIndex, Animation* anim);
	void CreateWaveAnimationSamplerForPreSkin(int nodeIndex, Animation* anim, Skeleton* skeleton);
	Animation CreateWaveAnimation(Skeleton* s);
	Animation CreateWaveArmAnimation();

private:
	void setupLightProperties();

public:
	Camera* m_pCamera;
	
	Microsoft::WRL::ComPtr <ID3D11Device>			m_pd3dDevice;
	Microsoft::WRL::ComPtr <ID3D11DeviceContext>	m_pImmediateContext;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pConstantBuffer;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pLightConstantBuffer;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pCustomConstantBuffer;


	LightPropertiesConstantBuffer m_lightProperties;
	IRenderingContext m_ctx;
	SceneGraph m_sceneobject;
	SceneGraph m_armobject;
	SceneGraph m_foxobject;

	Skeleton m_robotArmSkeleton;
	std::vector<SceneNode*> m_armSegmentNodes;
	std::vector<Animation*> m_robotArmAnimations;

	//ImGui controllable parameters
	//fox
	int m_numFoxes = 3;
	int m_foxAnimIndex = 1;
	float m_foxAnimationSpeed = 0.5f;
	float m_moveSpeed = 0.5f;

	int m_blendAnimA = 2; //to Walk
	int m_blendAnimB = 0; //to Run
	float m_blendRatio = 0.5f;



private:
	ID3D11ShaderResourceView* m_pTextureDiffuse;
	ID3D11ShaderResourceView* m_pTextureNormal;
	ID3D11ShaderResourceView* m_pTextureMetallic;
	ID3D11ShaderResourceView* m_pTextureRoughness;
	ID3D11ShaderResourceView* m_pTextureAmbientOcclusion;
	ID3D11ShaderResourceView* m_pTextureSpecularIBL;
	ID3D11ShaderResourceView* m_pTextureDiffuseIBL;


	ID3D11SamplerState* m_pSamplerLinear;
};

