/*********************************************************************************
*   Copyright (C) 2016 Wolfgang E. Sanyer (ezzieyguywuf@gmail.com)               *
*                                                                                *
*   This program is free software: you can redistribute it and/or modify         *
*   it under the terms of the GNU General Public License as published by         *
*   the Free Software Foundation, either version 3 of the License, or            *
*   (at your option) any later version.                                          *
*                                                                                *
*   This program is distributed in the hope that it will be useful,              *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of               *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
*   GNU General Public License for more details.                                 *
*                                                                                *
*   You should have received a copy of the GNU General Public License            *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.        *
******************************************************************************** */
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopAbs_ShapeEnum.hxx>

#include "FakeTopoShape.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>

TopoShape::TopoShape()
{}

TopoShape::TopoShape(const TopoDS_Shape& sh) : _Shape(sh)
{}

TopoShape::TopoShape(const TopoShape& sh) : _Shape(sh.GetShape())
{}


TopoShape::~TopoShape()
{}

void TopoShape::operator = (const TopoShape& sh)
{
	//std::clog << "-----FakeTopoShape = operator" << std::endl;
	this->_Shape = sh._Shape;
	this->_TopoNamer = sh._TopoNamer;
}

TopoDS_Shape TopoShape::GetShape() const
{
	return this->_Shape;
}

TopoNamingHelper TopoShape::GetTopoHelper() const
{
	return this->_TopoNamer;
}

void TopoShape::SetShape(const TopoDS_Shape& shape)
{
	this->_Shape = shape;
}

void TopoShape::SetShape(const TopoShape& shape)
{
	this->_Shape = shape._Shape;
	this->_TopoNamer = shape._TopoNamer;
}

void TopoShape::CreateBox(const BoxData& BData)
{
	this->_TopoNamer.AddNode("Tracked Shape");
	TopoData TData;
	BRepPrimAPI_MakeBox mkBox(BData.Length, BData.Width, BData.Height);

	// NOTE: The order here matters, as updateBox depends on it. That's why we use
	// `getBoxFacesVector` so that we always get the Faces in the same order.

	TData.NewShape = mkBox.Shape();
	TData.GeneratedFaces = this->GetBoxFacesVector(mkBox);

	this->_TopoNamer.TrackGeneratedShape("0:2", TData.NewShape, TData, "Generated Box Node");
	this->SetShape(TData.NewShape);

	//std::clog << "-----Dumpnig history after createBox" << std::endl;
	//std::clog << this->_TopoNamer.DeepDump();
}

void TopoShape::UpdateBox(const BoxData& BData)
{
	// TODO Do I need to check to ensure the Topo History is for a Box?

	TopoData TData;
	BRepPrimAPI_MakeBox mkBox(BData.Length, BData.Width, BData.Height);
	TData.OldShape = this->GetShape();
	TData.NewShape = mkBox.Shape();

	TopoDS_Face origFace, newFace;
	std::vector<TopoDS_Face> newFaces = this->GetBoxFacesVector(mkBox);

	for (int i = 0; i <= 5; i++)
	{
		// Tag 0:2:1:1:i+1 should hold the original face. GetLatestShape returns the latest
		// modification of this face in the Topological History
		std::ostringstream tag;
		tag << "0:2:1:1:" << (i + 1);
		TopoDS_Shape origShape = _TopoNamer.GetLatestShape(tag.str());
		origFace = TopoDS::Face(origShape);
		newFace = newFaces[i];

		if (!_TopoNamer.CompareTwoFaceTopologies(origFace, newFace))
		{
			TData.ModifiedFaces.push_back({ origFace, newFace });
		}
	}

	this->_TopoNamer.TrackModifiedShape("0:2", TData.NewShape, TData, "Modified Box Node");
	this->SetShape(mkBox.Shape());
}

void TopoShape::CreateFilletBaseShape(const TopoShape& BaseShape)
{
	// Node 2
	this->_TopoNamer.AddNode("BaseShapes");
	// Node 3
	this->_TopoNamer.AddNode("FilletShapes");

	// Collect and track appropriate data
	TopoData TData;

	TopTools_IndexedMapOfShape faces;
	// TODO need to handle possible seam edges
	TopExp::MapShapes(BaseShape.GetShape(), TopAbs_FACE, faces);
	for (int i = 1; i <= faces.Extent(); i++)
	{
		TopoDS_Face face = TopoDS::Face(faces.FindKey(i));
		TData.GeneratedFaces.push_back(face);
	}

	TData.NewShape = BaseShape.GetShape();
	this->_TopoNamer.TrackGeneratedShape(this->_TopoNamer.GetNode(2), TData.NewShape, TData, "Generated Base Shape");
	this->SetShape(BaseShape.GetShape());
}

BRepFilletAPI_MakeFillet TopoShape::CreateFillet(const TopoShape& BaseShape, const std::vector<FilletElement>& FDatas)
{
	// Make the fillets. NOTE: the edges should have already been 'selected' by
	// calling TopoShape::selectEdge(s) by the caller.
	BRepFilletAPI_MakeFillet mkFillet(BaseShape.GetShape());

	for (auto&& FData : FDatas)
	{
		TopoDS_Edge edge = this->_TopoNamer.GetSelectedEdge(FData.edgeTag);
		mkFillet.Add(FData.radius1, FData.radius2, edge);
	}

	mkFillet.Build();

	TopoData TFData = this->GetFilletData(BaseShape, mkFillet);
	TopoDS_Shape newShape = mkFillet.Shape();

	//this->_TopoNamer.TrackGeneratedShape(this->_TopoNamer.GetNode(3), newShape, TFData, "Filleted Shape");	
	this->_TopoNamer.TrackFilletOperation(BaseShape.GetShape(), newShape, mkFillet);
	this->SetShape(newShape);
	std::clog << "-----Dumping topohistory after tracking fillet op" << std::endl;
	std::clog << this->_TopoNamer.DeepDump2() << std::endl;
	return mkFillet;
}

void TopoShape::UpdateFillet(const TopoShape& BaseShape, const std::vector<FilletElement>& FDatas)
{
	// Make the fillets. NOTE: the edges should have already been 'selected' by
	// calling TopoShape::selectEdge(s) by the caller.

	FilletData TFData;

	try
	{
		BRepFilletAPI_MakeFillet mkFillet(BaseShape.GetShape());

		for (auto&& FData : FDatas)
		{
			TopoDS_Edge edge = this->_TopoNamer.GetSelectedEdge(FData.edgeTag);
			mkFillet.Add(FData.radius1, FData.radius2, edge);
		}

		mkFillet.Build();

		TFData = this->GetFilletData(BaseShape, mkFillet);

		TFData.OldShape = this->GetShape();
		TFData.NewShape = mkFillet.Shape();
	}
	catch (Standard_Failure sf)
	{
		std::cout << "\x1B[31mFilleting error: " << sf << "\033[0m" << std::endl;
		return;
	}

	//this->_TopoNamer.TrackModifiedShape(this->_TopoNamer.GetNode(3), TFData.NewShape, TFData, "Filleted Shape");
	this->_TopoNamer.TrackModifiedFilletBaseShape(TFData.NewShape);

	this->SetShape(TFData.NewShape);

	//std::clog << "-----Dumping topohistory after updateFillet" << std::endl;
	//std::clog << this->_TopoNamer.DeepDump() << std::endl;
}

bool TopoShape::SelectEdge(const int edgeID, SelectionElement& outSelection)
{
	TopTools_IndexedMapOfShape listOfEdges;
	TopExp::MapShapes(_Shape, TopAbs_EDGE, listOfEdges);

	try
	{
		// Get the specific edge, I hope
		const TopoDS_Edge& anEdge = TopoDS::Edge(listOfEdges.FindKey(edgeID));

		std::string edgeLabel = this->_TopoNamer.SelectEdge(anEdge, _Shape);
	}
	catch (Standard_Failure sf)
	{
		std::cout << "Error: " << sf << std::endl;
		return false;
	}

	return true;
}

std::string TopoShape::SelectEdge(const int edgeID, TNaming_Selector& selector, TDF_Label& selectionLabel)
{
	TopTools_IndexedMapOfShape listOfEdges;
	TopExp::MapShapes(_Shape, TopAbs_EDGE, listOfEdges);

	// Get the specific edge, I hope
	const TopoDS_Edge& anEdge = TopoDS::Edge(listOfEdges.FindKey(edgeID));

	std::string edgeLabel = this->_TopoNamer.SelectEdge(anEdge, _Shape, selector, selectionLabel);
	return edgeLabel;
}

//-------------------- Private Methods--------------------

std::vector<TopoDS_Face> TopoShape::GetBoxFacesVector(BRepPrimAPI_MakeBox mkBox) const
{
	std::vector<TopoDS_Face> OutFaces = { mkBox.TopFace(), mkBox.BottomFace(), mkBox.LeftFace(), mkBox.RightFace(), mkBox.FrontFace(), mkBox.BackFace() };
	return OutFaces;
}

TopTools_ListOfShape TopoShape::GetBoxFaces(BRepPrimAPI_MakeBox mkBox) const
{
	TopTools_ListOfShape OutFaces;
	OutFaces.Append(mkBox.TopFace());
	OutFaces.Append(mkBox.BottomFace());
	OutFaces.Append(mkBox.LeftFace());
	OutFaces.Append(mkBox.RightFace());
	OutFaces.Append(mkBox.FrontFace());
	OutFaces.Append(mkBox.BackFace());
	return OutFaces;
}

FilletData TopoShape::GetFilletData(const TopoShape& BaseShape, BRepFilletAPI_MakeFillet& mkFillet) const
{
	// Get the data we need for topo history
	FilletData TFData;

	TopTools_IndexedMapOfShape faces;
	// TODO need to handle possible seam edges
	// TODO need to pull BaseShape from topo tree
	TopExp::MapShapes(BaseShape.GetShape(), TopAbs_FACE, faces);
	for (int i = 1; i <= faces.Extent(); i++)
	{
		TopoDS_Face face = TopoDS::Face(faces.FindKey(i));
		TopTools_ListOfShape modified = mkFillet.Modified(face);
		if (modified.Extent() == 1)
		{
			TopoDS_Face newFace = TopoDS::Face(modified.First());
			TFData.ModifiedFaces.push_back({ face, newFace });
		}
		else if (modified.Extent() != 0)
		{
			throw std::runtime_error("Fillet should only produce a single modified face per face, or none");
		}
	}

	TopTools_IndexedMapOfShape edges;
	TopExp::MapShapes(BaseShape.GetShape(), TopAbs_EDGE, edges);
	for (int i = 1; i <= edges.Extent(); i++)
	{
		TopoDS_Edge edge = TopoDS::Edge(edges.FindKey(i));
		TopTools_ListOfShape generated = mkFillet.Generated(edge);
		TopTools_ListIteratorOfListOfShape genIt(generated);
		for (; genIt.More(); genIt.Next())
		{
			TopoDS_Face genFace = TopoDS::Face(genIt.Value());
			TFData.GeneratedFacesFromEdge.push_back({ edge, genFace });
		}
	}

	TopTools_IndexedMapOfShape vertexes;
	TopExp::MapShapes(BaseShape.GetShape(), TopAbs_VERTEX, vertexes);
	for (int i = 1; i <= vertexes.Extent(); i++)
	{
		TopoDS_Vertex vertex = TopoDS::Vertex(vertexes.FindKey(i));
		TopTools_ListOfShape generated = mkFillet.Generated(vertex);
		TopTools_ListIteratorOfListOfShape genIt(generated);
		for (; genIt.More(); genIt.Next())
		{
			TopoDS_Face genFace = TopoDS::Face(genIt.Value());
			TFData.GeneratedFacesFromVertex.push_back({ vertex, genFace });
		}
	}

	TFData.NewShape = mkFillet.Shape();
	return TFData;
}
