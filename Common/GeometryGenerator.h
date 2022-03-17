//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
//***************************************************************************************

#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

class GeometryGenerator
{
public:

	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;

	struct Vertex
	{
		Vertex() {}
		Vertex(
			const DirectX::XMFLOAT3& p,
			const DirectX::XMFLOAT3& n,
			const DirectX::XMFLOAT3& t,
			const DirectX::XMFLOAT2& uv) :
			Position(p),
			Normal(n),
			TangentU(t),
			TexC(uv) {}
		Vertex(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v) :
			Position(px, py, pz),
			Normal(nx, ny, nz),
			TangentU(tx, ty, tz),
			TexC(u, v) {}

		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT3 TangentU;
		DirectX::XMFLOAT2 TexC;
	};

	struct MeshData
	{
		std::vector<Vertex> Vertices;
		std::vector<uint32> Indices32;

		std::vector<uint16>& GetIndices16()
		{
			if (mIndices16.empty())
			{
				mIndices16.resize(Indices32.size());
				for (size_t i = 0; i < Indices32.size(); ++i)
					mIndices16[i] = static_cast<uint16>(Indices32[i]);
			}

			return mIndices16;
		}

	private:
		std::vector<uint16> mIndices16;
	};

	///<summary>
	/// Creates a box centered at the origin with the given dimensions, where each
	/// face has m rows and n columns of vertices.
	///</summary>
	MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);

	///<summary>
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	///</summary>
	MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// Creates a geosphere centered at the origin with the given radius.  The
	/// depth controls the level of tessellation.
	///</summary>
	MeshData CreateGeosphere(float radius, uint32 numSubdivisions);

	///<summary>
	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	/// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	///</summary>
	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// By eliminating 2 corners of a cube and rerouting the indices we can create a wedge
	///</summary>
	MeshData CreateWedge(float width, float height, float depth, uint32 numSubdivisions);

	///<summary>
	/// Much like createCylinder except we create a second inverted cylinder of half the 
	/// radius in the first, and then using these two radii we can create a top and
	/// bottom cap
	///</summary>
	MeshData CreatePipe(float topRadius, float bottomRadius, float height, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// By Drawing two pyramids on top of each other we can create a Diamond
	///</summary>
	MeshData CreateDiamond(float radius, float height, float depth, uint32 numSubdivisions);

	///<summary>
	/// Using create cylinder we can create a cone by making one of the Radii equal to zero
	/// in this case we make the topRadius equal to 0 so we have an upwards pointing cone
	///</summary>
	MeshData CreateCone(float bottomRadius, float height, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// Much like CreateCone we can make a pyramid by making one of the Radii equal to zero
	/// and by changing the slice count to a fixed number (in this case 4) we can have a 
	/// square base pyramid with it's point facing upwards
	///</summary>
	MeshData CreatePyramid(float bottomRadius, float height, uint32 stackCount);

	///<summary>
	/// By modifying CreateCylinder's Slice count we can create a Triangular Prism
	///</summary>
	MeshData CreateTrianglePrism(float bottomRadius, float height, uint32 stackCount);

	///<summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
	MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

	///<summary>
	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
	///</summary>
	MeshData CreateQuad(float x, float y, float w, float h, float depth);
	void Subdivide(MeshData& meshData);
private:

	Vertex MidPoint(const Vertex& v0, const Vertex& v1);
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
	void BuildInnerPipe(float topRadius, float bottomRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
	void BuildPipeTopCap(float topRadius, float height, uint32 sliceCount, MeshData& meshData);
	void BuildPipeBottomCap(float bottomRadius, float height, uint32 sliceCount, MeshData& meshData);
};