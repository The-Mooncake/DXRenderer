#pragma once
#include <DirectXMath.h>


class Camera
{
public:

    void UpdateWVP(struct CB_WVP& WVP) const;
    void Rotate(float X, float Y);
    void Translate(float X, float Y, float Z);
    void Pan(float X, float Y);
    void Zoom(float Zoom);

    DirectX::XMVECTOR GetViewDirection() const;
    
private:
    
    float NearPlane{0.01f}; // Can't be zero due to depth buffer
    float FarPlane{100.0f}; // Max distance we can see
    float FieldOfView{45.0f};

    float TranslateScale{0.01f};
    float RotationScale{0.01f};
    
    DirectX::XMVECTOR Position{0.0f, 0.0f , -3.0f, 0.0f};
    DirectX::XMVECTOR FocusPosition{0.0f, 0.0f, 0.0f, 0.0f};
    DirectX::XMVECTOR UpAxis{0.0f, 1.0f, 0.0f, 0.0f};
};
