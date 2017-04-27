#pragma once
#include "groundwork.h"
//**********************************************************************************************************************************************
//
//			USAGE:
//
//			STEP 1: integrate the explosion.h file in your project, don't include "groundwork.h" twice!
//				good structure: homework4.cpp -> explosion.h -> groundwork.h
//				
//			STEP 2: make a global variable in the homework4 (?) .cpp :
//				explosion_handler  explosionhandler;
//
//			STEP 3: in the initdevice() function, initialize the explosion handler:
//				explosionhandler.init(g_pd3dDevice, g_pImmediateContext);
//
//			STEP 4: also in the init device, you can initialize the different explosions
//				explosionhandler.init_types(L"exp1.dds", 8, 8,1000000);		<- 1. argument: filename of the animated image
//																			   2. argument: count of subparts of the image in x
//																			   3. argument: count of subparts of the image in y
//																			   4. argument: lifespan in microsecond
//
//			STEP 5: in the render function, write this at the end BEFORE the swapchain->present() method:
//				g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
//				explosionhandler.render(&view, &g_Projection, elapsed);
//				g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
//				g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
//
//			STEP 6: make new explosions with
//				explosionhandler.new_explosion(XMFLOAT3(...), XMFLOAT3(...), 1, 4.0);	<-	1. argument: position
//																							2. argument: impulse in unit per second
//																							3. argument: type of explosions (how many have you initialized?) starting with 0
//																							4. argument: scaling of the explosion
//			HINT: do step 6 i.e. when hitting someting
//				
//**********************************************************************************************************************************************
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
struct vertexstruct
	{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	};
class explosion_spots
	{
	public:
		XMFLOAT3 pos;
		XMFLOAT3 imp;
		float scale;
		
		long lifespan;
		explosion_spots()
			{			
			lifespan = 0;
			pos = XMFLOAT3(0, 0, 0);
			imp = XMFLOAT3(0, 0, 0);
			scale = 1;
			}
		XMMATRIX animate(long elapsed)
			{
			lifespan += elapsed;
			pos.x = pos.x + imp.x*elapsed*0.000001;
			pos.y = pos.y + imp.y*elapsed*0.000001;
			pos.z = pos.z + imp.z*elapsed*0.000001;
			XMMATRIX S=XMMatrixScaling(scale, scale, scale);
			return S*XMMatrixTranslation(pos.x, pos.y, pos.z);
			}
	};
class explosions_types
	{
	public:
		vector<explosion_spots> spots;
		ID3D11ShaderResourceView*           texture = NULL;
		explosions_types()
			{
			xparts = 0;
			yparts = 0;
			lifespan = 3000000;//3 seconds
			}
		bool get_spot(int i,long elapsed,XMMATRIX *world, int *tx, int *ty)
			{
			if (i >= spots.size())return FALSE;
			double maxframes = (double) xparts*yparts + 0.00000001;
			explosion_spots *ex = &spots[i];
			*world = ex->animate(elapsed);
			long time_passed = ex->lifespan;
			double frame = time_passed / (double)lifespan;
			if (frame >= 1)
				{
				spots.erase(spots.begin() + i);
				return TRUE;
				}
			double actualframe = maxframes * frame;
			int aframe = (int)actualframe;
			*tx = aframe % xparts;
			*ty = aframe / yparts;			
			return TRUE;
			}
		long lifespan;
		int xparts;
		int yparts;
		
	};
class explosions_constantbuffer
	{
	public:
		XMMATRIX world, view, projection;
		XMFLOAT4 animation_offset;
		explosions_constantbuffer()
			{
			world = view = projection = XMMatrixIdentity();
			animation_offset = XMFLOAT4(0, 0, 0, 0);
			}
	};
class explosion_handler
	{ 
	private:
		vector<explosions_types> exp;
		ID3D11Device*                       Device;
		ID3D11DeviceContext*                DeviceContext;
		ID3D11PixelShader*                  PS;
		ID3D11VertexShader*                 VS;
		ID3D11Buffer*                       vertexbuffer;
		ID3D11Buffer*                       constantbuffer;
		ID3D11InputLayout*                  VertexLayout;
		explosions_constantbuffer			s_constantbuffer;
	public:
		explosion_handler()
			{
			VertexLayout = NULL;
			vertexbuffer=NULL;
			PS = NULL;
			VS = NULL;
			Device = NULL;
			DeviceContext = NULL;
			}
		HRESULT init(ID3D11Device* device, ID3D11DeviceContext* immediatecontext)
			{
			Device = device;
			DeviceContext = immediatecontext;
			// Compile the vertex shader
			ID3DBlob* pVSBlob = NULL;
			HRESULT hr = CompileShaderFromFile(L"explosion_shader.fx", "VS", "vs_4_0", &pVSBlob);
			if (FAILED(hr))
				{
				MessageBox(NULL,
						   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
				return hr;
				}

			// Create the vertex shader
			hr = Device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &VS);
			if (FAILED(hr))
				{
				pVSBlob->Release();
				return hr;
				}


			// Define the input layout
			D3D11_INPUT_ELEMENT_DESC layout[] =
				{
						{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				};
			UINT numElements = ARRAYSIZE(layout);

			// Create the input layout
			hr = Device->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
												 pVSBlob->GetBufferSize(), &VertexLayout);
			pVSBlob->Release();
			if (FAILED(hr))
				return hr;

			ID3DBlob* pPSBlob = NULL;
			hr = CompileShaderFromFile(L"explosion_shader.fx", "PS", "ps_5_0", &pPSBlob);
			if (FAILED(hr))
				{
				MessageBox(NULL,
						   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
				return hr;
				}

			// Create the pixel shader
			hr = Device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &PS);
			pPSBlob->Release();
			if (FAILED(hr))
				return hr;

			// Create vertex buffer
			vertexstruct vertices[] =
				{
						{ XMFLOAT3(-1,1,0),XMFLOAT2(0,0) },
						{ XMFLOAT3(1,1,0),XMFLOAT2(1,0) },
						{ XMFLOAT3(-1,-1,0),XMFLOAT2(0,1) },
						{ XMFLOAT3(1,1,0),XMFLOAT2(1,0) },
						{ XMFLOAT3(1,-1,0),XMFLOAT2(1,1) },
						{ XMFLOAT3(-1,-1,0),XMFLOAT2(0,1)}
				};

			//initialize d3dx verexbuff:
			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(vertexstruct) * 6;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			D3D11_SUBRESOURCE_DATA InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = vertices;
			hr = Device->CreateBuffer(&bd, &InitData, &vertexbuffer);
			if (FAILED(hr))
				return FALSE;

			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(explosions_constantbuffer);
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = 0;
			hr = Device->CreateBuffer(&bd, NULL, &constantbuffer);
			if (FAILED(hr))
				return hr;
			return S_OK;
			}
		HRESULT init_types(LPCWSTR file,int xparts,int yparts,long lifespan)
			{
			explosions_types et;
			HRESULT hr = D3DX11CreateShaderResourceViewFromFile(Device, file, NULL, NULL, &et.texture, NULL);
			if (FAILED(hr))
				return hr;
			et.lifespan = lifespan;
			et.xparts = xparts;
			et.yparts = yparts;
			exp.push_back(et);
			return S_OK;
			}
		void new_explosion(XMFLOAT3 position,XMFLOAT3 impulse, int type,float scale)
			{
			if (exp.size() <= 0) return;
			if (type >= exp.size())type = 0;
			explosion_spots ep;
			ep.imp = impulse;
			ep.pos = position;
			ep.scale = scale;
			exp[type].spots.push_back(ep);
			}
		void render(XMMATRIX *view, XMMATRIX *projection,long elapsed)
			{
			DeviceContext->IASetInputLayout(VertexLayout);
			UINT stride = sizeof(vertexstruct);
			UINT offset = 0;
			DeviceContext->IASetVertexBuffers(0, 1, &vertexbuffer, &stride, &offset);
			DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			XMVECTOR det;
			XMMATRIX V= XMMatrixInverse(&det,*view);
			V._41 = 0;
			V._42 = 0;
			V._43 = 0;
			V._44 = 1;

			s_constantbuffer.view = XMMatrixTranspose(*view);
			s_constantbuffer.projection = XMMatrixTranspose(*projection);
			DeviceContext->VSSetShader(VS, NULL, 0);
			DeviceContext->PSSetShader(PS, NULL, 0);
			DeviceContext->VSSetConstantBuffers(0, 1, &constantbuffer);
			DeviceContext->PSSetConstantBuffers(0, 1, &constantbuffer);
			for (int ii = 0; ii < exp.size(); ii++)
				{
				DeviceContext->PSSetShaderResources(0, 1, &exp[ii].texture);
				for (int uu = 0; uu < exp[ii].spots.size(); uu++)
					{					
					XMMATRIX world;
					int tx, ty;
					if (!exp[ii].get_spot(uu, elapsed, &world, &tx, &ty)) { uu--; continue; }
					s_constantbuffer.world = XMMatrixTranspose(V*world);
					s_constantbuffer.animation_offset.x = exp[ii].xparts;
					s_constantbuffer.animation_offset.y = exp[ii].yparts;
					s_constantbuffer.animation_offset.z = tx;
					s_constantbuffer.animation_offset.w = ty;
					DeviceContext->UpdateSubresource(constantbuffer, 0, NULL, &s_constantbuffer, 0, 0);
					DeviceContext->Draw(6, 0);
					}
				}
			}
	};

