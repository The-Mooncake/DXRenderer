#include "Camera.h"
#include "DirectXMath.h"
#include "MainWindow.h"
#include "pch.h"
#include "Renderer.h"

using namespace DirectX;

void Camera::UpdateWVP(CB_WVP& WVP) const
{
    // Using Left handed coordinate systems, but matrices need to be transposed for hlsl.
    WVP.ViewMatrix = XMMatrixLookAtRH(Position, FocusPosition, UpAxis); 
    WVP.ViewMatrix = XMMatrixTranspose(WVP.ViewMatrix);

    float AR = G_MainWindow->RendererDX->AspectRatio;
    WVP.ProjectionMatrix = XMMatrixPerspectiveFovRH(XMConvertToRadians(FieldOfView), AR, NearPlane, FarPlane);
    WVP.ProjectionMatrix = XMMatrixTranspose(WVP.ProjectionMatrix);
}

void Camera::Rotate(float X, float Y)
{
    X *= RotationScale;
    Y *= RotationScale;
    
    const XMVECTOR ViewDir = GetViewDirection();

    // Limit to stop flipping/flickering when at axis
    const float ViewDirY = XMVectorGetY(ViewDir);
    if (ViewDirY <  -0.95f && Y >= 0.0f || ViewDirY > 0.95f && Y <= 0.0f)
    {
        return;
    }

    constexpr XMVECTOR UpAxis{0.0f, 1.0f, 0.0f};
    XMVECTOR RotationAxis = XMVector3Cross(UpAxis, ViewDir);
    XMVECTOR PitchQuat = XMQuaternionRotationAxis(RotationAxis, Y);    
    XMVECTOR YawQuat = XMQuaternionRotationAxis(UpAxis, X);
    XMVECTOR RotationOffset = XMQuaternionMultiply(PitchQuat, YawQuat);
    RotationOffset = XMQuaternionNormalize(RotationOffset);
    Position = XMVector3Rotate(Position, RotationOffset);
}

void Camera::Translate(float X, float Y, float Z)
{
    X *= TranslateScale;
    Y *= TranslateScale;
    Z *= TranslateScale;
    
    XMVECTOR Offset = XMVECTOR{X, Y, Z};
    Position = XMVectorAdd(Position, Offset);
}

void Camera::Pan(float X, float Y)
{
    X *= TranslateScale;
    Y *= TranslateScale;
    
    XMVECTOR ViewDir = GetViewDirection();
    XMVECTOR RightVector = XMVector3Cross(ViewDir, XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));
    XMVECTOR UpVector = XMVector3Cross(ViewDir, RightVector);

    XMVECTOR ViewOffset = XMVectorAdd(XMVectorScale(RightVector, X), XMVectorScale(UpVector, Y));
    Position = XMVectorAdd(Position, ViewOffset);
    FocusPosition = XMVectorAdd(FocusPosition, ViewOffset);
}

void Camera::Zoom(float Zoom)
{
    Zoom *= TranslateScale;
    XMVECTOR Offset = XMVectorScale(GetViewDirection(), Zoom);
    Position = XMVectorAdd(Position, Offset);
}

XMVECTOR Camera::GetViewDirection() const
{
    return  XMVector3Normalize(XMVectorSubtract(FocusPosition, Position));
}


