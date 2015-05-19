#include "GPGPUFractalGeneratorPCH.h"

#include "DirectXClass.h"
#include "Globals.h"

#include "VertexShader.h"
#include "PixelShader.h"


DirectXClass::DirectXClass()
{
	m_3dDevice = nullptr;
	m_3dDeviceContext = nullptr;
	m_SwapChain = nullptr;

	m_RenderTarget = nullptr;
	m_DepthStencil = nullptr;
	m_DepthStencilBuffer = nullptr;
	m_DepthStencilState = nullptr;

	m_RasterizerState = nullptr;

	m_Viewport = { 0 };

	m_InputLayout = nullptr;
	m_IndexBuffer = nullptr;
	m_VertexBuffer = nullptr;

	m_VertexShader = nullptr;
	m_PixelShader = nullptr;

	m_windowHandel = NULL;
}

//
// Initialise base DirectX
//
bool DirectXClass::Initialise(HWND hwnd, HINSTANCE hInstance)
{
	HRESULT hr = 0;
	m_windowHandel = hwnd;

	RECT rc;
	GetClientRect(m_windowHandel, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	D3D_FEATURE_LEVEL featureLevel;

	//
	// Describe and create swapchain
	//
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, createDeviceFlags, featureLevels, _countof(featureLevels),
		D3D11_SDK_VERSION, &swapChainDesc, &m_SwapChain, &m_3dDevice, &featureLevel, &m_3dDeviceContext);
	
	if (hr == E_FAIL)
	{
		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
			nullptr, createDeviceFlags, &featureLevels[1], _countof(featureLevels) - 1,
			D3D11_SDK_VERSION, &swapChainDesc, &m_SwapChain, &m_3dDevice, &featureLevel, &m_3dDeviceContext);
	}

	if (FAILED(hr))
		return false;

	//
	// Create back buffer
	//
	ID3D11Texture2D* backBuffer;
	hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(hr))
		return false;

	hr = m_3dDevice->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTarget);
	if (FAILED(hr))
		return false;

	backBuffer->Release();

	//
	// Create the depth buffer
	//
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0; // No CPU access required.
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = m_3dDevice->CreateTexture2D(&depthBufferDesc, nullptr, &m_DepthStencilBuffer);
	if (FAILED(hr))
		return false;

	hr = m_3dDevice->CreateDepthStencilView(m_DepthStencilBuffer, nullptr, &m_DepthStencil);
	if (FAILED(hr))
		return false;

	D3D11_DEPTH_STENCIL_DESC depthStencilState;
	ZeroMemory(&depthStencilState, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthStencilState.DepthEnable = TRUE;
	depthStencilState.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilState.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilState.StencilEnable = FALSE;

	hr = m_3dDevice->CreateDepthStencilState(&depthStencilState, &m_DepthStencilState);

	//
	// Setup rasterizer state.
	//
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	hr = m_3dDevice->CreateRasterizerState(&rasterizerDesc, &m_RasterizerState);
	if (FAILED(hr))
		return false;

	//
	// Initialize the viewport to occupy the entire client area.
	//
	m_Viewport.Width = static_cast<float>(width);
	m_Viewport.Height = static_cast<float>(height);
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	return LoadContent();
}

//
// Load unique Content.
//
bool DirectXClass::LoadContent()
{
	HRESULT hr = S_OK;
	
	//
	// Create the shared vertex buffer
	//
	D3D11_BUFFER_DESC sbd;
	ZeroMemory(&sbd, sizeof(D3D11_BUFFER_DESC));

	sbd.Usage = D3D11_USAGE_DEFAULT;
	sbd.ByteWidth = sizeof(TerrainVertex) * TOTAL_GRID_POINTS;
	sbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	sbd.CPUAccessFlags =  0;
	sbd.MiscFlags = 0;

	hr = m_3dDevice->CreateBuffer(&sbd, NULL, &m_VertexBuffer); //Empty buffer, no data
	if (FAILED(hr))
		return false;
	
	//
	// Create the shared index buffer.
	//
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.ByteWidth = sizeof(int) * NUM_INDICES;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	
	hr = m_3dDevice->CreateBuffer(&indexBufferDesc, NULL, &m_IndexBuffer); //Empty buffer, no data
	if (FAILED(hr))
		return false;

	//
	// Create Constant buffers
	//
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.ByteWidth = sizeof(XMMATRIX);
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = m_3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Application]);
	if (FAILED(hr))
		return false;

	hr = m_3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Frame]);
	if (FAILED(hr))
		return false;

	hr = m_3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Object]);
	if (FAILED(hr))
		return false;

	//
	// Load the compiled vertex shader.
	//
	ID3DBlob* vertexShaderBlob;
#if _DEBUG
	LPCWSTR compiledVertexShaderObject = L"VertexShader_d.cso";
#else
	LPCWSTR compiledVertexShaderObject = L"VertexShader.cso";
#endif

	hr = D3DReadFileToBlob(compiledVertexShaderObject, &vertexShaderBlob);
	if (FAILED(hr))
		return false;

	hr = m_3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &m_VertexShader);
	if (FAILED(hr))
		return false;

	//
	// Create the input layout for the vertex shader.
	//
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(TerrainVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(TerrainVertex, Normal),D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = m_3dDevice->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &m_InputLayout);
	if (FAILED(hr))
		return false;

	vertexShaderBlob->Release();

	//
	// Load the compiled pixel shader.
	//
	ID3DBlob* pixelShaderBlob;
#if _DEBUG
	LPCWSTR compiledPixelShaderObject = L"PixelShader_d.cso";
#else
	LPCWSTR compiledPixelShaderObject = L"PixelShader.cso";
#endif

	hr = D3DReadFileToBlob(compiledPixelShaderObject, &pixelShaderBlob);
	if (FAILED(hr))
		return false;

	hr = m_3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &m_PixelShader);
	if (FAILED(hr))
		return false;

	pixelShaderBlob->Release();

	//
	// Setup the projection matrix.
	//
	RECT clientRect;
	GetClientRect(m_windowHandel, &clientRect);

	// Compute the exact client dimensions.
	// This is required for a correct projection matrix.
	float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
	float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), clientWidth / clientHeight, 0.1f, 150.0f);
	
	m_3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_Application], 0, nullptr, &m_ProjectionMatrix, 0, 0);

	return true;
}

//
// Update the program's state
//
void DirectXClass::Update(float deltaTime)
{
	XMVECTOR eyePosition = XMVectorSet(0, 30, -50, 1);
	XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
	m_3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_Frame], 0, nullptr, &m_ViewMatrix, 0, 0);

	static float angle = 0.0f;
	angle += 90.0f * deltaTime/5;
	XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);

	m_WorldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
	m_3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_Object], 0, nullptr, &m_WorldMatrix, 0, 0);
}

//
// Render to the screen
//
void DirectXClass::Render()
{	
	m_3dDeviceContext->ClearRenderTargetView(m_RenderTarget, Colors::CornflowerBlue);
	m_3dDeviceContext->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	const UINT vertexStride = sizeof(TerrainVertex);
	const UINT offset = 0;
	
	m_3dDeviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &vertexStride, &offset);
	m_3dDeviceContext->IASetInputLayout(m_InputLayout);
	m_3dDeviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R16_SINT, 0);
	m_3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	m_3dDeviceContext->VSSetShader(m_VertexShader, nullptr, 0);
	m_3dDeviceContext->VSSetConstantBuffers(0, 3, g_d3dConstantBuffers);
	
	m_3dDeviceContext->RSSetState(m_RasterizerState);
	m_3dDeviceContext->RSSetViewports(1, &m_Viewport);
	
	m_3dDeviceContext->PSSetShader(m_PixelShader, nullptr, 0);
	
	m_3dDeviceContext->OMSetRenderTargets(1, &m_RenderTarget, m_DepthStencil);
	m_3dDeviceContext->OMSetDepthStencilState(m_DepthStencilState, 1);
	
	m_3dDeviceContext->DrawIndexed(NUM_INDICES, 0, 0);
	
	m_SwapChain->Present(1, 0);
}

//
// Release all the resources used by DirectX
//
void DirectXClass::CleanUp()
{
	if (m_3dDevice) m_3dDevice->Release();
	if (m_3dDeviceContext) m_3dDeviceContext->Release();
	if (m_SwapChain) m_SwapChain->Release();
	
	if (m_RenderTarget) m_RenderTarget->Release();
	if (m_DepthStencil) m_DepthStencil->Release();
	if (m_DepthStencilBuffer) m_DepthStencilBuffer->Release();
	if (m_DepthStencilState) m_DepthStencilState->Release();
	
	if (m_RasterizerState) m_RasterizerState->Release();
	
	if (m_InputLayout) m_InputLayout->Release();
	if (m_VertexBuffer) m_VertexBuffer->Release();
	if (m_IndexBuffer) m_IndexBuffer->Release();
	
	if (m_VertexShader) m_VertexShader->Release();
	if (m_PixelShader) m_PixelShader->Release();

	for (int i = 0; i < NumConstantBuffers; i++)
	{
		g_d3dConstantBuffers[i]->Release();
	}
}