#include "Scene.h"
#include "DDSTextureLoader.h"

DirectX::XMFLOAT3 BakeTranslationOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& animTranslation);
DirectX::XMFLOAT4 BakeRotationOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& axis, float angleRadians);
DirectX::XMFLOAT3 BakeScaleOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& animScale);

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
    //bool ok = m_sceneobject.LoadGLTF(m_ctx, L"Resources\\FlightHelmet.gltf");
    //bool ok = m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\Fox.gltf");
    bool ok = m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\simplerig.gltf");

    Skeleton* s = m_sceneobject.GetRootNode(0)->GetSkeleton();
    CreateWaveAnimation(s);

    //skeleton Hierarchy
    //const float segmentLength = 2.0f;

    //DirectX::XMFLOAT4X4 shoulderTransform;

    //DirectX::XMStoreFloat4x4(&shoulderTransform, DirectX::XMMatrixIdentity());
    //int shoulderIndex = m_robotArmSkeleton.AddJoint(-1, shoulderTransform);

    //scale elbow and hand nodes
    //DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.75, 0.75, 0.75);

    //elbow is child of the shoulder
    //DirectX::XMFLOAT4X4 elbowTransform;
    //DirectX::XMStoreFloat4x4(&elbowTransform, scale * DirectX::XMMatrixTranslation(0.0f, segmentLength, 0.0f));
    //int elbowIndex = m_robotArmSkeleton.AddJoint(shoulderIndex, elbowTransform);

    //hand is child of the elbow
    //DirectX::XMFLOAT4X4 handTransform;
    //DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.0f,segmentLength, 0.0f);

    //DirectX::XMStoreFloat4x4(&handTransform, scale * translation);
    //int handIndex = m_robotArmSkeleton.AddJoint(elbowIndex, handTransform);

    //for (int i = 0; i < m_robotArmSkeleton.GetBoneCount(); ++i) {
    //    m_sceneobject.CreateRootNode();
    //}

    //for (int i = 0; i < m_robotArmSkeleton.GetBoneCount(); ++i) {
    //    SceneNode* segmentNode = m_sceneobject.GetRootNode(i);
    //    segmentNode->LoadCube(m_ctx);
    //    segmentNode->AddTranslation({ i * 2.0f, 0, 0 });
    //    m_armSegmentNodes.push_back(segmentNode);
    //}

    //Animation anim = CreateWaveAnimation();
    //m_robotArmAnimations.push_back(&anim);



    m_pCamera = new Camera(XMFLOAT3(0, 0, -10), XMFLOAT3(0, 0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), width, height);
    

    // Create the constant buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer);
    if (FAILED(hr))
        return hr;

    setupLightProperties();

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

    

    return S_OK;
}

void Scene::cleanUp()
{
    delete m_pCamera;
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
    XMFLOAT4 LightPosition(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);
    light.Position = LightPosition;
    
    m_lightProperties.EyePosition = LightPosition;
    m_lightProperties.Lights[0] = light;
}

void Scene::update(const float deltaTime)
{
    Skeleton* s = m_sceneobject.GetRootNode(0)->GetSkeleton();

    if (s)
    {
        static bool doOnce = true;
        if (doOnce)
        {
            doOnce = false;
            if (s->CurrentAnimation()) {
                s->PlayAnimation(s->CurrentAnimation());
            }
        }
        s->Update(deltaTime);
    }

    m_pImmediateContext->PSSetShaderResources(0, 1, &m_pTextureDiffuse);
    m_pImmediateContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

    ConstantBuffer cb1;

    cb1.mView = XMMatrixTranspose(getCamera()->getViewMatrix());
    cb1.mProjection = XMMatrixTranspose(getCamera()->getProjectionMatrix());

    DirectX::XMMATRIX worldMat = m_sceneobject.GetRootNode(0)->GetWorldMtrx();
    cb1.mWorld = XMMatrixTranspose(worldMat);

    cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);

    if (s)
    {
        s->GetSkinningMatrices(cb1.boneTransforms, 100);
    }

    m_pImmediateContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb1, 0, 0);

    m_lightProperties.EyePosition = XMFLOAT4(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);

    m_pImmediateContext->UpdateSubresource(m_pLightConstantBuffer.Get(), 0, nullptr, &m_lightProperties, 0, 0);
    ID3D11Buffer* buf = m_pLightConstantBuffer.Get();
    m_pImmediateContext->PSSetConstantBuffers(1, 1, &buf);

    m_sceneobject.AnimateFrame(m_ctx);

    m_sceneobject.RenderFrame(m_ctx, deltaTime);
}

Animation Scene::CreateWaveAnimation(Skeleton* s) {
    Animation anim;
    anim.m_name = "waveHand_Simple";

    CreateWaveAnimationSamplerForPreSkin(1, &anim, s);

    s->AddAnimation(&anim);

    s->PlayAnimation(s->GetAnimationCount() - 1);;

    return anim;
}

// Helper for Translation
DirectX::XMFLOAT3 BakeTranslationOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& animTranslation)
{
    // 1. Create the animation matrix from the vector.
    DirectX::XMMATRIX animMatrix = DirectX::XMMatrixTranslation(animTranslation.x, animTranslation.y, animTranslation.z);
    // 2. Bake the animation onto the bind pose.
    DirectX::XMMATRIX finalLocalMatrix = animMatrix * bindPose;
    // 3. Extract and return the final position.
    DirectX::XMFLOAT3 finalPosition;
    DirectX::XMStoreFloat3(&finalPosition, finalLocalMatrix.r[3]);
    return finalPosition;
}
// Helper for Rotation
DirectX::XMFLOAT4 BakeRotationOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& axis, float angleRadians)
{
    // 1. Create the animation matrix from the axis and angle.
    DirectX::XMMATRIX animMatrix = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&axis), angleRadians);
    // 2. Bake the animation onto the bind pose.
    DirectX::XMMATRIX finalLocalMatrix = animMatrix * bindPose;
    // 3. Decompose to safely extract and return the final rotationquaternion.
    DirectX::XMVECTOR scale, finalRotationQuat, translation;
    DirectX::XMMatrixDecompose(&scale, &finalRotationQuat, &translation,finalLocalMatrix);
    DirectX::XMFLOAT4 finalRotation;
    DirectX::XMStoreFloat4(&finalRotation, finalRotationQuat);
    return finalRotation;
}
// Helper for Scale
DirectX::XMFLOAT3 BakeScaleOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& animScale)
{
    // 1. Create the animation matrix from the vector.
    DirectX::XMMATRIX animMatrix = DirectX::XMMatrixScaling(animScale.x, animScale.y, animScale.z);
    // 2. Bake the animation onto the bind pose.
    DirectX::XMMATRIX finalLocalMatrix = animMatrix * bindPose;
    // 3. Decompose to safely extract and return the final scale vector.
    DirectX::XMVECTOR finalScale, rotation, translation;
    DirectX::XMMatrixDecompose(&finalScale, &rotation, &translation, finalLocalMatrix);
    DirectX::XMFLOAT3 finalScaleVec;
    DirectX::XMStoreFloat3(&finalScaleVec, finalScale);
    return finalScaleVec;
}

void Scene::CreateWaveAnimationSamplerForPreSkin(int nodeIndex, Animation* anim, Skeleton* skeleton) {
    AnimationSampler nodeTranslationSampler, nodeRotationSampler;

    DirectX::XMMATRIX nodeBindPose = DirectX::XMLoadFloat4x4(&skeleton->GetJoint(nodeIndex)->localBindTransform);

    XMFLOAT3 constantPos = BakeTranslationOntoBindPose(nodeBindPose, { 0.0f, 0.0f, 0.0f });

    nodeTranslationSampler.timestamps.push_back(0.0f);
    nodeTranslationSampler.vec3_values.push_back(constantPos);

    XMFLOAT4 startRot = BakeRotationOntoBindPose(nodeBindPose, { 1, 0, 0 }, 0.0f);
    nodeRotationSampler.timestamps.push_back(0.0f);
    nodeRotationSampler.vec4_values.push_back(startRot);

    nodeTranslationSampler.timestamps.push_back(2.0f);
    nodeTranslationSampler.vec3_values.push_back(constantPos);

    XMFLOAT4 endRot = BakeRotationOntoBindPose(nodeBindPose, { 1, 0, 0 }, DirectX::XM_PIDIV4);
    nodeRotationSampler.timestamps.push_back(2.0f);
    nodeRotationSampler.vec4_values.push_back(endRot);

    anim->m_samplers.push_back(nodeTranslationSampler);
    int nodeTranslationSamplerIndex = anim->m_samplers.size() - 1;

    anim->m_samplers.push_back(nodeRotationSampler);
    int nodeRotationSamplerIndex = anim->m_samplers.size() - 1;

    AnimationChannel transChannel;
    transChannel.path = AnimationChannel::TRANSLATION;
    transChannel.samplerIndex = nodeTranslationSamplerIndex;
    transChannel.jointIndex = nodeIndex;
    anim->m_channels.push_back(transChannel);

    AnimationChannel rotChannel;
    rotChannel.path = AnimationChannel::ROTATION;
    rotChannel.samplerIndex = nodeRotationSamplerIndex;
    rotChannel.jointIndex = nodeIndex;
    anim->m_channels.push_back(rotChannel);
}

void Scene::CreateWaveAnimationSampler(int nodeIndex, Animation* anim)
{
    // Samplers for the hand's translation and rotation.
    AnimationSampler nodeTranslationSampler, nodeRotationSampler;
    // Get the hand's structural bind pose.
    DirectX::XMMATRIX nodeBindPose = DirectX::XMLoadFloat4x4(&m_robotArmSkeleton.GetJoint(nodeIndex) -> localBindTransform);
    // --- Keyframe 1: The Start Pose (t = 0.0s) ---
    // The hand is in its default, non-animated state.
    XMFLOAT3 startPos = BakeTranslationOntoBindPose(nodeBindPose, { 0.0f, 0.0f, 0.0f });
    nodeTranslationSampler.timestamps.push_back(0.0f);
    nodeTranslationSampler.vec3_values.push_back(startPos);
    XMFLOAT4 startRot = BakeRotationOntoBindPose(nodeBindPose, { 0, 0, 1 }, 0.0f); // No rotation
    nodeRotationSampler.timestamps.push_back(0.0f);
    nodeRotationSampler.vec4_values.push_back(startRot);
    // --- Keyframe 2: The End Pose (t = 2.0s) ---
    // The hand is translated up and rotated 90 degrees to the side.
    XMFLOAT3 endPos = BakeTranslationOntoBindPose(nodeBindPose, { 0.0f, 2.0f, 0.0f }); // Move up slightly
    nodeTranslationSampler.timestamps.push_back(2.0f);
    nodeTranslationSampler.vec3_values.push_back(endPos);
    XMFLOAT4 endRot = BakeRotationOntoBindPose(nodeBindPose, { 0, 0, 1 }, DirectX::XM_PIDIV2); // Rotate 90 degrees
    nodeRotationSampler.timestamps.push_back(2.0f);
    nodeRotationSampler.vec4_values.push_back(endRot);
    // --- Add Samplers and Channels for the node ---
    anim->m_samplers.push_back(nodeTranslationSampler); // Sampler x
    int nodeTranslationSamplerIndex = anim->m_samplers.size() - 1;
    anim->m_samplers.push_back(nodeRotationSampler); // Sampler x+1
    int nodeRotationSamplerIndex = anim->m_samplers.size() - 1;
    AnimationChannel transChannel;
    transChannel.path = AnimationChannel::TRANSLATION;
    transChannel.samplerIndex = nodeTranslationSamplerIndex;
    transChannel.jointIndex = nodeIndex;
    anim->m_channels.push_back(transChannel);
    AnimationChannel rotChannel;
    rotChannel.path = AnimationChannel::ROTATION;
    rotChannel.samplerIndex = nodeRotationSamplerIndex;
    rotChannel.jointIndex = nodeIndex;
    anim->m_channels.push_back(rotChannel);
}