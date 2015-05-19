#pragma once

#include "GPGPUFractalGeneratorPCH.h"

#include "Globals.h"

using namespace DirectX;

//
//The DirectX class is resposnsible for initialising DiretcX/Direct3D etc.
//http://www.3dgep.com/introduction-to-directx-11/ was used as a reference for seting up DirectX 11 correctly.
//
class DirectXClass
{
private:
	struct TerrainVertex
	{
		XMVECTOR Position;
		XMVECTOR Normal;
	};

	ID3D11Device* m_3dDevice;
	ID3D11DeviceContext* m_3dDeviceContext;
	IDXGISwapChain* m_SwapChain;

	ID3D11RenderTargetView* m_RenderTarget;
	ID3D11DepthStencilView* m_DepthStencil;
	ID3D11Texture2D* m_DepthStencilBuffer;
	ID3D11DepthStencilState* m_DepthStencilState;

	ID3D11RasterizerState* m_RasterizerState;

	D3D11_VIEWPORT m_Viewport;

	ID3D11InputLayout* m_InputLayout;
	
	ID3D11Buffer* m_IndexBuffer;
	ID3D11Buffer* m_VertexBuffer;
	
	ID3D11VertexShader* m_VertexShader;
	ID3D11PixelShader* m_PixelShader;

	HWND m_windowHandel;

	ID3D11Buffer* g_d3dConstantBuffers[NumConstantBuffers];

	XMMATRIX m_WorldMatrix;
	XMMATRIX m_ViewMatrix;
	XMMATRIX m_ProjectionMatrix;

public:
	DirectXClass();
	bool Initialise(HWND hWnd, HINSTANCE hInstance);
	bool LoadContent();
	void Update(float deltaTime);
	void Render();
	void CleanUp();

	ID3D11Device* Device() const { return m_3dDevice; };
	ID3D11Buffer* VertexBuffer() const { return m_VertexBuffer; };
	ID3D11Buffer* IndexBuffer() const { return m_IndexBuffer; };
};