#include "Skeleton.h"
#include "Animation.h"

#include <map>

using namespace DirectX;


Skeleton::Skeleton() : m_currentAnimationTime(0), m_pCurrentAnimation(nullptr), m_animationCount(0)
{
    XMStoreFloat4x4(&m_rootTransform, XMMatrixIdentity());
}

void Skeleton::PlayAnimation(const unsigned int animation)
{

}

// Helper function to get a node's local transform.
// This avoids code duplication.
DirectX::XMFLOAT4X4 GetNodeLocalTransform(const tinygltf::Node& node)
{
    DirectX::XMFLOAT4X4 ret;

    return ret;
}

const void Skeleton::GetSkinningMatrices(DirectX::XMMATRIX* matrixlist, unsigned int arraylength) const
{

}

bool Skeleton::LoadFromGltf(const tinygltf::Model& model)
{
    return true;
}

void Skeleton::Update(float deltaTime)
{

}

DirectX::XMMATRIX GetLocalAnimatedMatrixForJoint(
    const Joint& joint,
    int jointIndex,
    const Animation* animation, // Use a pointer to allow for nullptr
    float timeInSeconds)
{
    
    return DirectX::XMMatrixIdentity();
}

void Skeleton::UpdateJointTransform(int jointIndex, const Animation* anim, float time, const DirectX::XMMATRIX& parentTransform)
{
    
}