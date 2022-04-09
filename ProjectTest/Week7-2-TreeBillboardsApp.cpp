/** @file Week7-2-TreeBillboardsApp.cpp
 *  @brief Tree Billboarding Demo
 *   Adding Billboarding to our previous Hills, Mountain, Crate, and Wave Demo
 * 
 *   Controls:
 *   Hold the left mouse button down and move the mouse to rotate.
 *   Hold the right mouse button down and move the mouse to zoom in and out.
 *
 *  @author Hooman Salamat
 */

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"

#include <iostream>
#include <string>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace std;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class TreeBillboardsApp : public D3DApp
{
public:
    TreeBillboardsApp(HINSTANCE hInstance);
    TreeBillboardsApp(const TreeBillboardsApp& rhs) = delete;
    TreeBillboardsApp& operator=(const TreeBillboardsApp& rhs) = delete;
    ~TreeBillboardsApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt); 

	void LoadTextures();
    void BuildRootSignature();
	void BuildDescriptorHeaps();
    void BuildShadersAndInputLayouts();

	void BuildShapeGeometry();

    void BuildLandGeometry();
    void BuildWavesGeometry();
	void BuildBoxGeometry();
	void BuildTreeSpritesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);


	void BuildShape(string shapeName, string textureName,	float ScaleX, float ScaleY, float ScaleZ, float OffsetX, float OffsetY, float OffsetZ,
															float xRotaion = 0.0f, float yRotation = 0.0f, float ZRotation = 0.0f,
															float xTexScale = 1.0f, float yTexScale = 1.0f, float zTexScale = 1.0f);


	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    float GetHillsHeight(float x, float z)const;
	float GetHillsHeight(float x, float z, float i)const;
    XMFLOAT3 GetHillsNormal(float x, float z)const;

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mStdInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

    RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

    PassConstants mMainPassCB;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = XM_PIDIV2 - 0.1f;
    float mRadius = 50.0f;

    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        TreeBillboardsApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

TreeBillboardsApp::~TreeBillboardsApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool TreeBillboardsApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
	
	LoadTextures();
    BuildRootSignature();
	BuildDescriptorHeaps();
    BuildShadersAndInputLayouts();
	BuildShapeGeometry();
    BuildLandGeometry();
    BuildWavesGeometry();
	BuildBoxGeometry();
	BuildTreeSpritesGeometry();
	BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void TreeBillboardsApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void TreeBillboardsApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
	UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
    UpdateWaves(gt);
}

void TreeBillboardsApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void TreeBillboardsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void TreeBillboardsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TreeBillboardsApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void TreeBillboardsApp::OnKeyboardInput(const GameTimer& gt)
{
}
 
void TreeBillboardsApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void TreeBillboardsApp::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if(tu >= 1.0f)
		tu -= 1.0f;

	if(tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;
}

void TreeBillboardsApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for(auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for(auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.375f, 0.375f, 0.4f, 1.0f };
	/*mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };*/

	mMainPassCB.Lights[0].Position = { 12.0f, 17.0f, -54.0f };
	mMainPassCB.Lights[0].Strength = { 50.0f, 25.0f, 10.0f };
	mMainPassCB.Lights[1].Position = { -12.0f, 17.0f, -54.0f };
	mMainPassCB.Lights[1].Strength = { 50.0f, 25.0f, 10.0f };
	mMainPassCB.Lights[2].Position = { 5.5f, 10.0f, -6.0f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -1.0f, 0.0f };
	mMainPassCB.Lights[2].Strength = { 2.0f, 2.0f, 2.0f };
	mMainPassCB.Lights[2].SpotPower = 1.0;

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void TreeBillboardsApp::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for(int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);
		
		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void TreeBillboardsApp::LoadTextures()
{
	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../../Textures/grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));

	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"../../Textures/water1.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap));

	auto drawBrigeTex = std::make_unique<Texture>();
	drawBrigeTex->Name = "drawBrigeTex";
	drawBrigeTex->Filename = L"../../Textures/DrawBridge.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), drawBrigeTex->Filename.c_str(),
		drawBrigeTex->Resource, drawBrigeTex->UploadHeap));

	auto blackStoneTex = std::make_unique<Texture>();
	blackStoneTex->Name = "blackStoneTex";
	blackStoneTex->Filename = L"../../Textures/BlackStone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), blackStoneTex->Filename.c_str(),
		blackStoneTex->Resource, blackStoneTex->UploadHeap));

	auto bloodStoneTex = std::make_unique<Texture>();
	bloodStoneTex->Name = "bloodStoneTex";
	bloodStoneTex->Filename = L"../../Textures/BloodStone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), bloodStoneTex->Filename.c_str(),
		bloodStoneTex->Resource, bloodStoneTex->UploadHeap));

	auto jadeWoodTex = std::make_unique<Texture>();
	jadeWoodTex->Name = "jadeWoodTex";
	jadeWoodTex->Filename = L"../../Textures/JadeWood.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), jadeWoodTex->Filename.c_str(),
		jadeWoodTex->Resource, jadeWoodTex->UploadHeap));

	auto poleTex = std::make_unique<Texture>();
	poleTex->Name = "poleTex";
	poleTex->Filename = L"../../Textures/Pole.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), poleTex->Filename.c_str(),
		poleTex->Resource, poleTex->UploadHeap));

	auto wellTex = std::make_unique<Texture>();
	wellTex->Name = "wellTex";
	wellTex->Filename = L"../../Textures/Well.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), wellTex->Filename.c_str(),
		wellTex->Resource, wellTex->UploadHeap));

	auto headgeTex = std::make_unique<Texture>();
	headgeTex->Name = "headgeTex";
	headgeTex->Filename = L"../../Textures/Headge.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), headgeTex->Filename.c_str(),
		headgeTex->Resource, headgeTex->UploadHeap));

	auto treeArrayTex = std::make_unique<Texture>();
	treeArrayTex->Name = "treeArrayTex";
	treeArrayTex->Filename = L"../../Textures/treeArr.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), treeArrayTex->Filename.c_str(),
		treeArrayTex->Resource, treeArrayTex->UploadHeap));

	auto qubeTex = std::make_unique<Texture>();
	qubeTex->Name = "quebertTex";
	qubeTex->Filename = L"../../Textures/QBert_Icon.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), qubeTex->Filename.c_str(),
		qubeTex->Resource, qubeTex->UploadHeap));

	mTextures[grassTex->Name] = std::move(grassTex);
	mTextures[waterTex->Name] = std::move(waterTex);
	mTextures[drawBrigeTex->Name] = std::move(drawBrigeTex);
	mTextures[blackStoneTex->Name] = std::move(blackStoneTex);
	mTextures[bloodStoneTex->Name] = std::move(bloodStoneTex);
	mTextures[jadeWoodTex->Name] = std::move(jadeWoodTex);
	mTextures[poleTex->Name] = std::move(poleTex);
	mTextures[wellTex->Name] = std::move(wellTex);
	mTextures[headgeTex->Name] = std::move(headgeTex);
	mTextures[treeArrayTex->Name] = std::move(treeArrayTex);
	mTextures[qubeTex->Name] = std::move(qubeTex);
}

void TreeBillboardsApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0);
    slotRootParameter[2].InitAsConstantBufferView(1);
    slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

    // A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void TreeBillboardsApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 11;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto grassTex = mTextures["grassTex"]->Resource;
	auto waterTex = mTextures["waterTex"]->Resource;
	auto drawBrigeTex = mTextures["drawBrigeTex"]->Resource;
	auto blackStoneTex = mTextures["blackStoneTex"]->Resource;
	auto bloodStoneTex = mTextures["bloodStoneTex"]->Resource;
	auto jadeWoodTex = mTextures["jadeWoodTex"]->Resource;
	auto poleTex = mTextures["poleTex"]->Resource;
	auto wellTex = mTextures["wellTex"]->Resource;
	auto headgeTex = mTextures["headgeTex"]->Resource;
	auto quebertTex = mTextures["quebertTex"]->Resource;
	auto treeArrayTex = mTextures["treeArrayTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = waterTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = drawBrigeTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(drawBrigeTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = blackStoneTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(blackStoneTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = bloodStoneTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(bloodStoneTex.Get(), &srvDesc, hDescriptor);
	
	//next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = jadeWoodTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(jadeWoodTex.Get(), &srvDesc, hDescriptor);
	
	//next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = poleTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(poleTex.Get(), &srvDesc, hDescriptor);
	
	//next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = wellTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(wellTex.Get(), &srvDesc, hDescriptor);

	//next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = headgeTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(headgeTex.Get(), &srvDesc, hDescriptor);

	//next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = quebertTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(quebertTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	auto desc = treeArrayTex->GetDesc();
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = treeArrayTex->GetDesc().Format;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);

	
}

void TreeBillboardsApp::BuildShadersAndInputLayouts()
{
	const D3D_SHADER_MACRO defines[] =
	{
		//"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		//"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	
	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");

    mStdInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void TreeBillboardsApp::BuildShapeGeometry()
{
	//GeometryGenerator is a utility class for generating simple geometric shapes like grids, sphere, cylinders, and boxes
	GeometryGenerator geoGen;
	//The MeshData structure is a simple structure nested inside GeometryGenerator that stores a vertex and index list

	// Step 1
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);

	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(1.0f, 1.0f, 1.0f, 15, 5);

	GeometryGenerator::MeshData cone = geoGen.CreateCone(1.0f, 1.0f, 15, 5);

	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1.0f, 1.0f, 5);

	GeometryGenerator::MeshData box2 = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);

	GeometryGenerator::MeshData wedge = geoGen.CreateWedge(1.0f, 1.0f, 1.0f, 3);

	GeometryGenerator::MeshData diamond = geoGen.CreateDiamond(1.0f, 1.0f, 1.0f, 3);

	GeometryGenerator::MeshData flag = geoGen.CreateTrianglePrism(1.0f, 1.0f, 3);

	GeometryGenerator::MeshData pipe = geoGen.CreatePipe(1.0f, 1.0f, 1.0f, 15, 5);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Step 2
	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT cylinderVertexOffset = (UINT)box.Vertices.size();
	UINT coneVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
	UINT pyramidVertexOffset = coneVertexOffset + (UINT)cone.Vertices.size();
	UINT box2VertexOffset = pyramidVertexOffset + (UINT)pyramid.Vertices.size();
	UINT wedgeVertexOffset = box2VertexOffset + (UINT)box2.Vertices.size();
	UINT diamondVertexOffset = wedgeVertexOffset + (UINT)wedge.Vertices.size();
	UINT flagVertexOffset = diamondVertexOffset + (UINT)diamond.Vertices.size();
	UINT pipeVertexOffset = flagVertexOffset + (UINT)flag.Vertices.size();


	// Step 3
	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT cylinderIndexOffset = (UINT)box.Indices32.size();
	UINT coneIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
	UINT pyramidIndexOffset = coneIndexOffset + (UINT)cone.Indices32.size();
	UINT box2IndexOffset = pyramidIndexOffset + (UINT)pyramid.Indices32.size();
	UINT wedgeIndexOffset = box2IndexOffset + (UINT)box2.Indices32.size();
	UINT diamondIndexOffset = wedgeIndexOffset + (UINT)wedge.Indices32.size();
	UINT flagIndexOffset = diamondIndexOffset + (UINT)diamond.Indices32.size();
	UINT pipeIndexOffset = flagIndexOffset + (UINT)flag.Indices32.size();


	// Step 4
	// Define the SubmeshGeometry that cover different 
	// regions of the vertex/index buffers.

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	SubmeshGeometry coneSubmesh;
	coneSubmesh.IndexCount = (UINT)cone.Indices32.size();
	coneSubmesh.StartIndexLocation = coneIndexOffset;
	coneSubmesh.BaseVertexLocation = coneVertexOffset;

	SubmeshGeometry pyramidSubmesh;
	pyramidSubmesh.IndexCount = (UINT)pyramid.Indices32.size();
	pyramidSubmesh.StartIndexLocation = pyramidIndexOffset;
	pyramidSubmesh.BaseVertexLocation = pyramidVertexOffset;

	SubmeshGeometry box2Submesh;
	box2Submesh.IndexCount = (UINT)box2.Indices32.size();
	box2Submesh.StartIndexLocation = box2IndexOffset;
	box2Submesh.BaseVertexLocation = box2VertexOffset;

	SubmeshGeometry wedgeSubmesh;
	wedgeSubmesh.IndexCount = (UINT)wedge.Indices32.size();
	wedgeSubmesh.StartIndexLocation = wedgeIndexOffset;
	wedgeSubmesh.BaseVertexLocation = wedgeVertexOffset;

	SubmeshGeometry diamondSubmesh;
	diamondSubmesh.IndexCount = (UINT)diamond.Indices32.size();
	diamondSubmesh.StartIndexLocation = diamondIndexOffset;
	diamondSubmesh.BaseVertexLocation = diamondVertexOffset;

	SubmeshGeometry flagSubmesh;
	flagSubmesh.IndexCount = (UINT)flag.Indices32.size();
	flagSubmesh.StartIndexLocation = flagIndexOffset;
	flagSubmesh.BaseVertexLocation = flagVertexOffset;

	SubmeshGeometry pipeSubmesh;
	pipeSubmesh.IndexCount = (UINT)pipe.Indices32.size();
	pipeSubmesh.StartIndexLocation = pipeIndexOffset;
	pipeSubmesh.BaseVertexLocation = pipeVertexOffset;


	// Step 5
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		cylinder.Vertices.size() +
		cone.Vertices.size() +
		pyramid.Vertices.size() +
		box2.Vertices.size() +
		wedge.Vertices.size() +
		diamond.Vertices.size() +
		flag.Vertices.size() +
		pipe.Vertices.size();


	// Step 6
	// Add all Vertex and color data into one vector -> vertices
	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cone.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cone.Vertices[i].Position;
		vertices[k].Normal = cone.Vertices[i].Normal;
		vertices[k].TexC = cone.Vertices[i].TexC;
	}

	for (size_t i = 0; i < pyramid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = pyramid.Vertices[i].Position;
		vertices[k].Normal = pyramid.Vertices[i].Normal;
		vertices[k].TexC = pyramid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < box2.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box2.Vertices[i].Position;
		vertices[k].Normal = box2.Vertices[i].Normal;
		vertices[k].TexC = box2.Vertices[i].TexC;
	}

	for (size_t i = 0; i < wedge.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = wedge.Vertices[i].Position;
		vertices[k].Normal = wedge.Vertices[i].Normal;
		vertices[k].TexC = wedge.Vertices[i].TexC;
	}

	for (size_t i = 0; i < diamond.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = diamond.Vertices[i].Position;
		vertices[k].Normal = diamond.Vertices[i].Normal;
		vertices[k].TexC = diamond.Vertices[i].TexC;
	}

	for (size_t i = 0; i < flag.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = flag.Vertices[i].Position;
		vertices[k].Normal = flag.Vertices[i].Normal;
		vertices[k].TexC = flag.Vertices[i].TexC;
	}

	for (size_t i = 0; i < pipe.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = pipe.Vertices[i].Position;
		vertices[k].Normal = pipe.Vertices[i].Normal;
		vertices[k].TexC = pipe.Vertices[i].TexC;
	}



	// step 7
	// Add all Index data into one vector -> indices
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(cone.GetIndices16()), std::end(cone.GetIndices16()));
	indices.insert(indices.end(), std::begin(pyramid.GetIndices16()), std::end(pyramid.GetIndices16()));
	indices.insert(indices.end(), std::begin(box2.GetIndices16()), std::end(box2.GetIndices16()));
	indices.insert(indices.end(), std::begin(wedge.GetIndices16()), std::end(wedge.GetIndices16()));
	indices.insert(indices.end(), std::begin(diamond.GetIndices16()), std::end(diamond.GetIndices16()));
	indices.insert(indices.end(), std::begin(flag.GetIndices16()), std::end(flag.GetIndices16()));
	indices.insert(indices.end(), std::begin(pipe.GetIndices16()), std::end(pipe.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	// step 8
	// Map saved subMesh data into mGeometries for later drawing use
	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["cone"] = coneSubmesh;
	geo->DrawArgs["pyramid"] = pyramidSubmesh;
	geo->DrawArgs["box2"] = box2Submesh;
	geo->DrawArgs["wedge"] = wedgeSubmesh;
	geo->DrawArgs["diamond"] = diamondSubmesh;
	geo->DrawArgs["flag"] = flagSubmesh;
	geo->DrawArgs["pipe"] = pipeSubmesh;



	mGeometries[geo->Name] = std::move(geo);
}

float TreeBillboardsApp::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

float TreeBillboardsApp::GetHillsHeight(float x, float z, float i)const
{
	if (fabsf(x) > 85 || fabsf(z) > 85)
		return -35.0f;
	if (fabsf(x) > 83 || fabsf(z) > 83)
		return -8.0f;
	else
		return (sin(0.01f * x) + cos(0.01f * z) + 1);
}


XMFLOAT3 TreeBillboardsApp::GetHillsNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void TreeBillboardsApp::BuildLandGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(200.0f, 200.0f, 50, 50);

    //
    // Extract the vertex elements we are interested and apply the height function to
    // each vertex.  In addition, color the vertices based on their height so we have
    // sandy looking beaches, grassy low hills, and snow mountain peaks.
    //

    std::vector<Vertex> vertices(grid.Vertices.size());
    for(size_t i = 0; i < grid.Vertices.size(); ++i)
    {
        auto& p = grid.Vertices[i].Position;
        vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z, p.x);
		vertices[i].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);//GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
    }

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    std::vector<std::uint16_t> indices = grid.GetIndices16();
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["landGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildWavesGeometry()
{
    std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for(int i = 0; i < m - 1; ++i)
    {
        for(int j = 0; j < n - 1; ++j)
        {
            indices[k] = i*n + j;
            indices[k + 1] = i*n + j + 1;
            indices[k + 2] = (i + 1)*n + j;

            indices[k + 3] = (i + 1)*n + j;
            indices[k + 4] = i*n + j + 1;
            indices[k + 5] = (i + 1)*n + j + 1;

            k += 6; // next quad
        }
    }

	UINT vbByteSize = mWaves->VertexCount()*sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size()*sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);

	std::vector<Vertex> vertices(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		auto& p = box.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = box.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["box"] = submesh;

	mGeometries["boxGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildTreeSpritesGeometry()
{
	//step5
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 24;
	std::array<TreeSpriteVertex, 24> vertices;

	// For Ring of trees
	float dTheta = 2.0f * XM_PI / treeCount;
	float TreeRadius = 75.0f;
	for(UINT i = 0; i < treeCount; ++i)
	{
		if (i == 18)
			continue;
		float x = TreeRadius * cosf(i * dTheta);
		float z = TreeRadius * sinf(i * dTheta);

		/*float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);*/
		float y = 1.0f;//GetHillsHeight(x, z);

		// Move tree slightly above land height.
		y += 25.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 60.0f);
	}

	std::array<std::uint16_t, 24> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21,
		22, 23
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpritesGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), 
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;

	//there is abug with F2 key that is supposed to turn on the multisampling!
//Set4xMsaaState(true);
	//m4xMsaaState = true;

	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//transparentPsoDesc.BlendState.AlphaToCoverageEnable = true;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// PSO for alpha tested objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	//
	// PSO for tree sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};
	//step1
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));
}

void TreeBillboardsApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
    }
}

void TreeBillboardsApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseSrvHeapIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	// This is not a good water material definition, but we do not have all the rendering
	// tools we need (transparency, environment reflection), so we fake it for now.
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseSrvHeapIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;

	auto DrawBridge = std::make_unique<Material>();
	DrawBridge->Name = "drawbridge";
	DrawBridge->MatCBIndex = 2;
	DrawBridge->DiffuseSrvHeapIndex = 2;
	DrawBridge->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	DrawBridge->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	DrawBridge->Roughness = 0.25f;

	auto BlackStone = std::make_unique<Material>();
	BlackStone->Name = "blackstone";
	BlackStone->MatCBIndex = 3;
	BlackStone->DiffuseSrvHeapIndex = 3;
	BlackStone->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	BlackStone->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	BlackStone->Roughness = 0.25f;

	auto BloodStone = std::make_unique<Material>();
	BloodStone->Name = "bloodstone";
	BloodStone->MatCBIndex = 4;
	BloodStone->DiffuseSrvHeapIndex = 4;
	BloodStone->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	BloodStone->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	BloodStone->Roughness = 0.25f;

	auto JadeWood = std::make_unique<Material>();
	JadeWood->Name = "jadewood";
	JadeWood->MatCBIndex = 5;
	JadeWood->DiffuseSrvHeapIndex = 5;
	JadeWood->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	JadeWood->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	JadeWood->Roughness = 0.25f;

	auto Pole = std::make_unique<Material>();
	Pole->Name = "pole";
	Pole->MatCBIndex = 6;
	Pole->DiffuseSrvHeapIndex = 6;
	Pole->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Pole->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	Pole->Roughness = 0.25f;

	auto Well = std::make_unique<Material>();
	Well->Name = "well";
	Well->MatCBIndex = 7;
	Well->DiffuseSrvHeapIndex = 7;
	Well->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Well->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	Well->Roughness = 0.25f;

	auto Headge = std::make_unique<Material>();
	Headge->Name = "headge";
	Headge->MatCBIndex = 8;
	Headge->DiffuseSrvHeapIndex = 8;
	Headge->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Headge->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	Headge->Roughness = 0.25f;

	auto Quebert = std::make_unique<Material>();
	Quebert->Name = "quebert";
	Quebert->MatCBIndex = 9;
	Quebert->DiffuseSrvHeapIndex = 9;
	Quebert->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Quebert->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	Quebert->Roughness = 0.25f;

	auto treeSprites = std::make_unique<Material>();
	treeSprites->Name = "treeSprites";
	treeSprites->MatCBIndex = 10;
	treeSprites->DiffuseSrvHeapIndex = 10;
	treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->Roughness = 0.125f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
	mMaterials["drawbridge"] = std::move(DrawBridge);
	mMaterials["blackstone"] = std::move(BlackStone);
	mMaterials["bloodstone"] = std::move(BloodStone);
	mMaterials["jadewood"] = std::move(JadeWood);
	mMaterials["pole"] = std::move(Pole);
	mMaterials["well"] = std::move(Well);
	mMaterials["headge"] = std::move(Headge);
	mMaterials["quebert"] = std::move(Quebert);
	mMaterials["treeSprites"] = std::move(treeSprites);
}

// Helper function to build any shape objects (Rotation is optional)
void TreeBillboardsApp::BuildShape(string shapeName, string textureName,	float ScaleX, float ScaleY, float ScaleZ, 
																			float OffsetX, float OffsetY, float OffsetZ, 
																			float xRotation, float yRotation, float ZRotation,
																			float xTexScale, float yTexScale, float zTexScale)
{
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(ScaleX, ScaleY, ScaleZ) *
		XMMatrixRotationRollPitchYaw(xRotation, yRotation, ZRotation) *
		XMMatrixTranslation(OffsetX, OffsetY, OffsetZ));

	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(xTexScale, yTexScale, zTexScale));

	boxRitem->ObjCBIndex = mAllRitems.size();

	boxRitem->Mat = mMaterials[textureName].get();	// For later changing we will add a material string

	boxRitem->Geo = mGeometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs[shapeName].IndexCount;  //36
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[shapeName].StartIndexLocation; //0
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[shapeName].BaseVertexLocation; //0

	mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());

	mAllRitems.push_back(std::move(boxRitem));
}

void TreeBillboardsApp::BuildRenderItems()
{
    auto wavesRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&wavesRitem->World, XMMatrixScaling(5.0f, 1.0f, 5.0f) *
		XMMatrixTranslation(0.0f, -5.0f, 0.0f));
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(20.0f, 20.0f, 20.0f));
	wavesRitem->ObjCBIndex = 0;
	wavesRitem->Mat = mMaterials["water"].get();
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

    mWavesRitem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(15.0f, 15.0f, 15.0f));
	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	auto wavesRitem2 = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&wavesRitem2->World, XMMatrixScaling(0.015f, 1.0f, 0.015f) *
		XMMatrixTranslation(5.5f, 3.5f, -6.0f));
	XMStoreFloat4x4(&wavesRitem2->TexTransform, XMMatrixScaling(0.5f, 0.5f, 0.5f));
	wavesRitem2->ObjCBIndex = 2;
	wavesRitem2->Mat = mMaterials["water"].get();
	wavesRitem2->Geo = mGeometries["waterGeo"].get();
	wavesRitem2->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem2->IndexCount = wavesRitem2->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem2->StartIndexLocation = wavesRitem2->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem2->BaseVertexLocation = wavesRitem2->Geo->DrawArgs["grid"].BaseVertexLocation;

	mWavesRitem = wavesRitem2.get();

	mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem2.get());

	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	treeSpritesRitem->ObjCBIndex = 3;
	treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
	//step2
	treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());

    mAllRitems.push_back(std::move(wavesRitem));
    mAllRitems.push_back(std::move(gridRitem));
	mAllRitems.push_back(std::move(wavesRitem2));
	mAllRitems.push_back(std::move(treeSpritesRitem));

	//Base
	BuildShape("box", "jadewood", 20.0f, 1.0f, 20.0f, 0.0f, 2.0f, 0.0f);

	// Front wall 1
	BuildShape("box", "blackstone", 8.0f, 5.0f, 1.0f, -6.0f, 5.0f, -9.5f);
	// Front wall 2
	BuildShape("box", "blackstone", 8.0f, 5.0f, 1.0f, 6.0f, 5.0f, -9.5f);
	// Left wall
	BuildShape("box", "blackstone", 1.0f, 5.0f, 20.0f, -9.5f, 5.0f, 0.0f);
	// Back wall
	BuildShape("box", "blackstone", 20.0f, 5.0f, 1.0f, 0.0f, 5.0f, 9.5f);
	// Right wall
	BuildShape("box", "blackstone", 1.0f, 5.0f, 20.0f, 9.5f, 5.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 20.0f);

	// Inner Building
	BuildShape("box2", "bloodstone", 6.0f, 7.0f, 6.0f, -5.0f, 6.0f, 0.0f);
	// Inner Building Roof
	BuildShape("pyramid", "jadewood", 6.0f, 3.5f, 6.0f, -5.0f, 11.25f, 0.0f, 0.0f, 3.95f, 0.0f);

	// Towers
	BuildShape("cylinder", "bloodstone", 2.0f, 10.0f, 2.0f, -9.5f, 6.5f, -9.5f);
	BuildShape("cylinder", "bloodstone", 2.0f, 10.0f, 2.0f, 9.5f, 6.5f, -9.5f);
	BuildShape("cylinder", "bloodstone", 2.0f, 10.0f, 2.0f, -9.5f, 6.5f, 9.5f);
	BuildShape("cylinder", "bloodstone", 2.0f, 10.0f, 2.0f, 9.5f, 6.5f, 9.5f);

	// Tower Toppers
	BuildShape("cone", "jadewood", 3.0f, 3.0f, 3.0f, 9.5f, 13.0f, 9.5f);
	BuildShape("cone", "jadewood", 3.0f, 3.0f, 3.0f, -9.5f, 13.0f, 9.5f);
	BuildShape("cone", "jadewood", 3.0f, 3.0f, 3.0f, 9.5f, 13.0f, -9.5f);
	BuildShape("cone", "jadewood", 3.0f, 3.0f, 3.0f, -9.5f, 13.0f, -9.5f);

	// Gate Decal
	BuildShape("box2", "drawbridge", 4.0f, 1.0f, 6.0f, 0.0f, 2.0f, -13.0f);

	// Stairs
	BuildShape("wedge", "pole", 8.0f, 5.25f, 1.0f, 0.0f, 5.0f, 8.5f, 22.0);

	// Fence Vertical
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 2.0f, 3.0f, -9.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 2.0f, 3.0f, -8.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 2.0f, 3.0f, -7.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 2.0f, 3.0f, -6.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 2.0f, 3.0f, -5.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 2.0f, 3.0f, -4.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 2.0f, 3.0f, -3.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 3.0f, 3.0f, -3.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 4.0f, 3.0f, -3.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 5.0f, 3.0f, -3.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 6.0f, 3.0f, -3.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 7.0f, 3.0f, -3.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 8.0f, 3.0f, -3.0f);
	BuildShape("box2", "drawbridge", 0.2f, 1.0f, 0.2f, 9.0f, 3.0f, -3.0f);

	// Fence Horizontal
	BuildShape("box2", "drawbridge", 0.2f, 0.2f, 6.0f, 2.0f, 3.0f, -6.0f);
	BuildShape("box2", "drawbridge", 7.0f, 0.2f, 0.2f, 5.5f, 3.0f, -3.0f);

	// Fence Decals
	BuildShape("diamond", "bloodstone", 0.2f, 0.2f, 0.2f, 2.0f, 4.0f, -8.0f);
	BuildShape("diamond", "bloodstone", 0.2f, 0.2f, 0.2f, 2.0f, 4.0f, -6.0f);
	BuildShape("diamond", "bloodstone", 0.2f, 0.2f, 0.2f, 2.0f, 4.0f, -4.0f);
	BuildShape("diamond", "bloodstone", 0.2f, 0.2f, 0.2f, 3.0f, 4.0f, -3.0f);
	BuildShape("diamond", "bloodstone", 0.2f, 0.2f, 0.2f, 5.0f, 4.0f, -3.0f);
	BuildShape("diamond", "bloodstone", 0.2f, 0.2f, 0.2f, 7.0f, 4.0f, -3.0f);
	BuildShape("diamond", "bloodstone", 0.2f, 0.2f, 0.2f, 7.0f, 4.0f, -3.0f);

	// Well
	BuildShape("box2", "blackstone", 4.0f, 0.5f, 4.0f, 5.5f, 3.0f, -6.0f);
	BuildShape("pipe", "well", 1.4f, 0.5f, 1.4f, 5.5f, 3.5f, -6.0f);

	//Flagpole
	BuildShape("cylinder", "blackstone", 1.0f, 1.0f, 1.0f, 5.0f, 3.0f, 0.0f);
	BuildShape("cylinder", "pole", 0.5f, 12.0f, 0.5f, 5.0f, 9.5f, 0.0f);
	BuildShape("flag", "jadewood", 3.0f, 1.0f, 2.0f, 7.0f, 13.5f, 0.0f, 4.7f, 0.0f, 0.0f);
	//Flag Decal
	BuildShape("box", "quebert", 1.70f, 1.70f, 1.05f, 6.9f, 13.5f, 0.0f);

	//Torchs
	BuildShape("cylinder", "blackstone", 0.25f, 3.0f, 0.25f, -3.0f, 3.7f, -13.0f);
	BuildShape("cylinder", "blackstone", 0.25f, 3.0f, 0.25f, 3.0f, 3.7f, -13.0f);

	//Maze
	/*Maze exit*/
	BuildShape("box", "headge", 11.0f, 3.0f, 1.0f, -7.0f, 3.0f, -20.5f	,0.0f,0.0f,0.0f, 11.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 11.0f, 3.0f, 1.0f, 7.0f, 3.0f, -20.5f	,0.0f,0.0f,0.0f, 11.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 42.0f, -13.0f, 3.0f, 0.0f	,0.0f,0.0f,0.0f, 42.0f, 3.0f, 42.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 42.0f, 13.0f, 3.0f, 0.0f	,0.0f,0.0f,0.0f, 42.0f, 3.0f, 42.0f);
	BuildShape("box", "headge", 25.0f, 3.0f, 1.0f, 0.0f, 3.0f, 20.5f	,0.0f,0.0f,0.0f, 25.0f, 3.0f, 1.0f);

	/*Maze Enterence*/
	BuildShape("box", "headge", 59.0f, 3.0f, 1.0f, 0.0f, 3.0f, -40.5f	,0.0f,0.0f,0.0f, 59.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 82.0f, -30.0f, 3.0f, 0.0f	,0.0f,0.0f,0.0f, 82.0f, 3.0f, 82.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 82.0f, 30.0f, 3.0f, 0.0f	,0.0f,0.0f,0.0f, 82.0f, 3.0f, 82.0f);
	BuildShape("box", "headge", 59.0f, 3.0f, 1.0f, 0.0f, 3.0f, 40.5f	,0.0f,0.0f,0.0f, 59.0f, 3.0f, 1.0f);
																					   
	/*The Maze*/																	   
	BuildShape("box", "headge", 25.0f, 3.0f, 1.0f, 0.0f, 3.0f, -27.5f	,0.0f,0.0f,0.0f, 25.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 8.0f, 3.0f, 1.0f, -23.0f, 3.0f, -20.5f	,0.0f,0.0f,0.0f, 8.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 13.0f, -23.0f, 3.0f, -20.5f	,0.0f,0.0f,0.0f, 1.0f, 3.0f, 13.0f);
	BuildShape("box", "headge", 11.0f, 3.0f, 1.0f, -18.0f, 3.0f, -27.5f	,0.0f,0.0f,0.0f, 11.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 7.0f, 3.0f, 1.0f, 16.0f, 3.0f, -27.5f	,0.0f,0.0f,0.0f, 7.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 7.0f, 3.0f, 1.0f, 17.0f, 3.0f, -20.5f	,0.0f,0.0f,0.0f, 7.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 4.0f, 3.0f, 1.0f, 25.0f, 3.0f, -20.5f	,0.0f,0.0f,0.0f, 4.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 7.0f, 3.0f, 1.0f, 23.0f, 3.0f, -27.5f	,0.0f,0.0f,0.0f, 7.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 6.0f, 23.5f, 3.0f, -24.0f	,0.0f,0.0f,0.0f, 6.0f, 3.0f, 6.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 6.0f, 23.5f, 3.0f, -17.0f	, 0.0f, 0.0f, 0.0f, 6.0f, 3.0f, 6.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 6.0f, 19.5f, 3.0f, -11.0f	, 0.0f, 0.0f, 0.0f, 6.0f, 3.0f, 6.0f);
	BuildShape("box", "headge", 4.0f, 3.0f, 1.0f, 21.0f, 3.0f, -14.5f	,0.0f,0.0f,0.0f, 4.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 40.0f, 19.5f, 3.0f, 12.0f	,0.0f,0.0f,0.0f, 40.0f, 3.0f, 40.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 6.0f, -13.0f, 3.0f, -24.0f	,0.0f,0.0f,0.0f, 6.0f, 3.0f, 6.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 15.0f, -23.0f, 3.0f, 0.0f	,0.0f,0.0f,0.0f, 15.0f, 3.0f, 15.0f);
	BuildShape("box", "headge", 6.0f, 3.0f, 1.0f, 23.0f, 3.0f, 35.0f	,0.0f,0.0f,0.0f, 6.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 10.0f, 3.0f, 1.0f, 14.0f, 3.0f, 31.5f	,0.0f,0.0f,0.0f, 10.0f, 3.0f, 1.0f);
	//BuildShape("box", "headge", 1.0f, 3.0f, 5.0f, 8.5f, 6.0f, 33.5f);				   
	BuildShape("box", "headge", 10.0f, 3.0f, 1.0f, 3.0f, 3.0f, 35.5f	,0.0f,0.0f,0.0f, 10.0f, 3.0f, 1.0f);//
	BuildShape("box", "headge", 10.0f, 3.0f, 1.0f, -10.0f, 3.0f, 35.5f	,0.0f,0.0f,0.0f, 10.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 5.0f, -1.5f, 3.0f, 32.5f	,0.0f,0.0f,0.0f, 1.0f, 3.0f, 5.0f);
	BuildShape("box", "headge", 10.0f, 3.0f, 1.0f, -7.0f, 3.0f, 30.5f	,0.0f,0.0f,0.0f, 10.0f, 3.0f, 1.0f);
	//BuildShape("box", "headge", 1.0f, 3.0f, 9.0f, -6.5f, 3.0f, 25.5f);			   
	BuildShape("box", "headge", 6.0f, 3.0f, 1.0f, -26.5f, 3.0f, 0.0f	,0.0f,0.0f,0.0f, 6.0f, 3.0f, 1.0f);
	//BuildShape("box", "headge", 3.0f, 3.0f, 1.0f, -18.0f, 3.0f, -8.0f);			   
	BuildShape("box", "headge", 1.0f, 3.0f, 40.0f, -18.5f, 3.0f, -1.0f	,0.0f,0.0f,0.0f, 40.0f, 3.0f, 40.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 6.0f, -15.5f, 3.0f, 33.0f	,0.0f,0.0f,0.0f, 1.0f, 3.0f, 6.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 6.0f, -18.5f, 3.0f, 22.0f	,0.0f,0.0f,0.0f, 1.0f, 3.0f, 6.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 4.0f, -18.5f, 3.0f, 27.0f	,0.0f,0.0f,0.0f, 1.0f, 3.0f, 4.0f);
	BuildShape("box", "headge", 4.0f, 3.0f, 1.0f, -17.0f, 3.0f, 29.5f	,0.0f,0.0f,0.0f, 4.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 9.0f, -11.5f, 3.0f, 25.5f	,0.0f,0.0f,0.0f, 1.0f, 3.0f, 9.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 8.0f, 3.0f, 3.0f, 25.0f		,0.0f,0.0f,0.0f, 1.0f, 3.0f, 8.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 34.0f, 25.0f, 3.0f, 17.5f	,0.0f,0.0f,0.0f, 34.0f, 3.0f, 34.0f);
	BuildShape("box", "headge", 1.0f, 3.0f, 4.0f, 19.5f, 3.0f, 34.0f	,0.0f,0.0f,0.0f, 1.0f, 3.0f, 4.0f);
	BuildShape("box", "headge", 8.0f, 3.0f, 1.0f, -23.0f, 3.0f, 20.5f	,0.0f,0.0f,0.0f, 8.0f, 3.0f, 1.0f);
	BuildShape("box", "headge", 8.0f, 3.0f, 1.0f, -20.0f, 3.0f, 35.5f	,0.0f,0.0f,0.0f, 8.0f, 3.0f, 1.0f);

}

void TreeBillboardsApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		//step3
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TreeBillboardsApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return { 
		pointWrap, pointClamp,
		linearWrap, linearClamp, 
		anisotropicWrap, anisotropicClamp };
}


