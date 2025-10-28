#include "Scene.h"
#include "DDSTextureLoader.h"
#include <algorithm>


HRESULT Scene::init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, DX11Renderer* renderer)
{
	m_pd3dDevice = device;
	m_pImmediateContext = context;

    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;
    HRESULT hr;

    m_ctx.Init(device.Get(), context.Get(), renderer);
    //bool ok = m_sceneobject.LoadSphere(m_ctx);
    //bool ok = m_sceneobject.LoadGLTF(m_ctx, L"Resources\\sphere.gltf");
    bool ok = m_sceneobject.LoadGLTF(m_ctx, L"Resources\\Box.gltf");
    bool okPlanet = m_scenePlanet.LoadGLTF(m_ctx, L"Resources\\Box.gltf");
    //bool ok = m_sceneobject.LoadGLTF(m_ctx, L"Resources\\FlightHelmet.gltf");
    //bool ok = m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\Fox.gltf");

    m_pCamera = new Camera(XMFLOAT3(0, 0, -6), XMFLOAT3(0, 0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), width, height);
    

    // Create the constant buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer);
    if (FAILED(hr))
        return hr;

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBufferLighting);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBufferLighting);
    if (FAILED(hr))
        return hr;

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBufferAlbedo);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBufferAlbedo);
    if (FAILED(hr))
        return hr;

    setupLightProperties();
    SetupPBRProperties();

    // Create the light constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pLightConstantBuffer);
    if (FAILED(hr))
        return hr;

    // load and setup textures
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_diff.dds", nullptr, &m_pTextureDiffuse);
    if (FAILED(hr))
        return hr;

    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\SpecularCM.dds", nullptr, &m_pTextureSpecularIBL);
    if (FAILED(hr))
        return hr;

    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\DiffuseCM.dds", nullptr, &m_pTextureDiffuseIBL);
    if (FAILED(hr))
        return hr;

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);

	//animation setup
    //---------------------------------------------------------------------------------rotation
    DirectX::XMStoreFloat4(&m_startRot, DirectX::XMQuaternionIdentity()); //No rotation
    DirectX::XMStoreFloat4(&m_endRot, DirectX::XMQuaternionRotationAxis(
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), // Y-axis
        DirectX::XM_PI // 180 degrees
    ));


    return S_OK;
}

void Scene::cleanUp()
{
    delete m_pCamera;
}

void Scene::SetupPBRProperties() {
	m_propertiesAlbedo.AlbedoColour = XMFLOAT4(1.0f, 0.78f, 0.34f, 1.0f);
	m_propertiesAlbedo.SkyColour = XMFLOAT4(0.11f, 0.11f, 0.94f, 1.0f);
	m_propertiesAlbedo.GroundColour = XMFLOAT4(0.0f, 0.33f, 0.0f, 1.0f);
    m_propertiesLight.metallicness = 1.0f;
    m_propertiesLight.rough = 0.01f;
    m_propertiesLight.IBLType = 1;
}

void Scene::setupLightProperties()
{
    Light light;
    light.Enabled = static_cast<int>(true);
    light.LightType = PointLight;
    light.Color = XMFLOAT4(1,1,1,1);
    light.SpotAngle = XMConvertToRadians(45.0f);
    light.ConstantAttenuation = 1.0f;
    light.LinearAttenuation = 1;
    light.QuadraticAttenuation = 1;

    // set up the light
    // XMFLOAT4 LightPosition(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);
    XMFLOAT4 LightPosition(5, 5, -6, 1);
    light.Position = LightPosition;
    
    m_lightProperties.EyePosition = LightPosition;
    m_lightProperties.Lights[0] = light;
}

void Scene::update(const float deltaTime)
{
    // note the pixel shader and the vertex shader have been set by the renderer class calling this method
    m_pImmediateContext->PSSetShaderResources(0, 1, &m_pTextureDiffuse);
    m_pImmediateContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

    m_pImmediateContext->PSSetShaderResources(1, 1, &m_pTextureSpecularIBL);
    m_pImmediateContext->PSSetShaderResources(2, 1, &m_pTextureDiffuseIBL);


    ConstantBuffer cb1;
    cb1.mView = XMMatrixTranspose(getCamera()->getViewMatrix());
    cb1.mProjection = XMMatrixTranspose(getCamera()->getProjectionMatrix());
    cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);

    ConstantBufferLighting cbL;

    m_lightProperties.EyePosition = XMFLOAT4(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);

    m_pImmediateContext->UpdateSubresource(m_pLightConstantBuffer.Get(), 0, nullptr, &m_lightProperties, 0, 0);
    ID3D11Buffer* buf = m_pLightConstantBuffer.Get();
    m_pImmediateContext->PSSetConstantBuffers(1, 1, &buf);

    m_pImmediateContext->UpdateSubresource(m_pConstantBufferLighting.Get(), 0, nullptr, &m_propertiesLight, 0, 0);
    ID3D11Buffer* buf_lighting = m_pConstantBufferLighting.Get();
    m_pImmediateContext->PSSetConstantBuffers(2, 1, &buf_lighting);

    m_pImmediateContext->UpdateSubresource(m_pConstantBufferAlbedo.Get(), 0, nullptr, &m_propertiesAlbedo, 0, 0);
    ID3D11Buffer* buf_albedo = m_pConstantBufferAlbedo.Get();
    m_pImmediateContext->PSSetConstantBuffers(3, 1, &buf_albedo);


	//animation update transformation heirarchy
    m_t += deltaTime;
    DirectX::XMMATRIX finalMatrix = DirectX::XMMatrixIdentity();
    m_sceneobject.GetRootNode(0)->SetMatrix(finalMatrix);

    // --- CHILD (Planet) ---
    // 1. Create the planet's LOCAL transformation relative to the sun.
    // It rotates on its own axis and is translated away from the sun.
    DirectX::XMMATRIX planetRotation = DirectX::XMMatrixRotationY(m_t);
    DirectX::XMMATRIX planetTranslation = DirectX::XMMatrixTranslation(2.0f, 0.0f, 0.0f); // Orbit distance
    DirectX::XMMATRIX planetLocal = planetTranslation * planetRotation;
    // 2. To get the planet's FINAL world matrix, multiply its local
    // transform by its parent's (the sun's) world matrix.
    DirectX::XMMATRIX worldPlanet = planetLocal * finalMatrix;

    m_scenePlanet.GetRootNode(0)->SetMatrix(worldPlanet);

    // scene object 1 
    m_sceneobject.AnimateFrame(m_ctx); // this updates the transform matrix for the object - this should be called after all transforms have been made
    m_sceneobject.RenderFrame(m_ctx, deltaTime); // renders the object

    //scene object 2 (planet)
    m_scenePlanet.AnimateFrame(m_ctx); // this updates the transform matrix for the object - this should be called after all transforms have been made
    m_scenePlanet.RenderFrame(m_ctx, deltaTime); // renders the object
}