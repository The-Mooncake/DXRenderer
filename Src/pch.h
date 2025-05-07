#pragma once

#include "DirectXMath.h"

// Global ptr to the main window instance.
inline class MainWindow* G_MainWindow = nullptr;

// ConstBuffer
struct CB_WVP
{
    DirectX::XMMATRIX ModelMatrix = DirectX::XMMatrixIdentity(); // Model to World
    DirectX::XMMATRIX ViewMatrix = DirectX::XMMatrixIdentity(); // World to View / Camera 
    DirectX::XMMATRIX ProjectionMatrix = DirectX::XMMatrixIdentity(); // View to 2D Projection
};