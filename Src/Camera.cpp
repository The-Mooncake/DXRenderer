#include "Camera.h"
#include "DirectXMath.h"
#include "MainWindow.h"
#include "pch.h"
#include "Renderer.h"


void Camera::UpdateWVP(CB_WVP& WVP) const
{
    // Using Left handed coordinate systems, but matrices need to be transposed for hlsl.
    WVP.ViewMatrix = DirectX::XMMatrixLookAtRH(Position, FocusPosition, UpAxis); 
    WVP.ViewMatrix = DirectX::XMMatrixTranspose(WVP.ViewMatrix);

    float AR = G_MainWindow->RendererDX->AspectRatio;
    WVP.ProjectionMatrix = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(FieldOfView), AR, NearPlane, FarPlane);
    WVP.ProjectionMatrix = DirectX::XMMatrixTranspose(WVP.ProjectionMatrix);
}

void Camera::Rotate(float X, float Y)
{
    X *= RotationScale;
    Y *= RotationScale;
    
    DirectX::XMVECTOR RotationOffset{0.0f, X, Y};
    RotationOffset = DirectX::XMQuaternionRotationRollPitchYawFromVector(RotationOffset);
    Position = DirectX::XMVector3Rotate(Position, RotationOffset);
}

void Camera::Translate(float X, float Y, float Z)
{
    X *= TranslateScale;
    Y *= TranslateScale;
    Z *= TranslateScale;
    
    DirectX::XMVECTOR Offset = DirectX::XMVECTOR{X, Y, Z};
    Position = DirectX::XMVectorAdd(Position, Offset);
}

void Camera::Pan(float X, float Y)
{
    X *= TranslateScale;
    Y *= TranslateScale;
    
    DirectX::XMVECTOR ViewDir = GetViewDirection();
    DirectX::XMVECTOR RightVector = DirectX::XMVector3Cross(ViewDir, DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));
    DirectX::XMVECTOR UpVector = DirectX::XMVector3Cross(ViewDir, RightVector);

    DirectX::XMVECTOR ViewOffset = DirectX::XMVectorAdd(DirectX::XMVectorScale(RightVector, X), DirectX::XMVectorScale(UpVector, Y));
    Position = DirectX::XMVectorAdd(Position, ViewOffset);
    FocusPosition = DirectX::XMVectorAdd(FocusPosition, ViewOffset);
}

void Camera::Zoom(float Zoom)
{
    DirectX::XMVECTOR Offset = DirectX::XMVectorScale(GetViewDirection(), Zoom);
    Position = DirectX::XMVectorAdd(Position, Offset);
}

DirectX::XMVECTOR Camera::GetViewDirection() const
{
    return  DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(FocusPosition, Position));
}


