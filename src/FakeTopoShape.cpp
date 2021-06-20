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
#include <TNaming_Selector.hxx>

#include "FakeTopoShape.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>

TopoShape::TopoShape(){
}

TopoShape::TopoShape(const TopoDS_Shape& sh) : _Shape(sh){
}

TopoShape::TopoShape(const TopoShape& sh) : _Shape(sh.getShape()){
}


TopoShape::~TopoShape()
{ }

void TopoShape::operator = (const TopoShape& sh)
{
    //std::clog << "-----FakeTopoShape = operator" << std::endl;
    this->_Shape     = sh._Shape;
    this->_TopoNamer = sh._TopoNamer;    
}

TopoDS_Shape TopoShape::getShape() const
{
    return this->_Shape;
}

TopoDS_Shape TopoShape::getNonConstShape()
{
    return this->_Shape;
}

TopoNamingHelper TopoShape::getTopoHelper() const
{
    return this->_TopoNamer;
}

void TopoShape::setShape(const TopoDS_Shape& shape)
{
    this->_Shape = shape;
}

void TopoShape::setShape(const TopoShape& shape)
{
    std::clog << "setShape(TopoShape) called" << std::endl;
    this->_Shape     = shape._Shape;
    this->_TopoNamer = shape._TopoNamer;

    //std::clog << "---> other data framework dump: " << std::endl;
    //shape._TopoNamer.DeepDump();
    //std::clog << "---> dump end: " << std::endl;

    //Handle(TDF_Data) topoFramework = shape._TopoNamer.GetDataFramework();
    //this->_TopoNamer.SetDataFramework(topoFramework);
    //TDF_Label rootNode = shape._TopoNamer.GetRootNode();
    //this->_TopoNamer.SetRootNode(rootNode);    
}

void TopoShape::createBox(const BoxData& BData)
{
    // Node 2
    this->_TopoNamer.AddNode("Tracked Shape");
    TopoData TData;
    BRepPrimAPI_MakeBox mkBox(BData.Length, BData.Width, BData.Height);
    // NOTE: The order here matters, as updateBox depends on it. That's why we use
    // `getBoxFacesVector` so that we always get the Faces in the same order.
    TData.NewShape      = mkBox.Shape();
    TData.GeneratedFaces = this->getBoxFacesVector(mkBox);
    TData.text = "Create Box";
    //this->_TopoNamer.TrackGeneratedShape("0:2", TData, "Generated Box Node");
    this->_TopoNamer.TrackShape(TData, "0:2");
    this->setShape(mkBox.Shape());
    //std::clog << "-----Dumpnig history after createBox" << std::endl;
    //std::clog << this->_TopoNamer.DeepDump();
}

void TopoShape::updateBox(const BoxData& BData){
    // TODO Do I need to check to ensure the Topo History is for a Box?
    //std::clog << "----dumping tree in updateBox" << std::endl;
    //std::clog << this->_TopoNamer.DeepDump() <<  std::endl;
    std::clog << "update box start." << std::endl;
    TopoData TData;
    BRepPrimAPI_MakeBox mkBox(BData.Length, BData.Width, BData.Height);
    TData.OldShape = this->getShape();
    TData.NewShape = mkBox.Shape();
    TData.text = "Updated Box";
    std::clog << "made new box." << std::endl;
    TopoDS_Face origFace, newFace;
    std::vector<TopoData::TrackedData<TopoDS_Face>> newFaces = this->getBoxFacesVector(mkBox);

    //std::clog << "-----Dumping history in update box" << std::endl;
    //std::clog << this->_TopoNamer.DeepDump();
    for (int i=0; i<=5; i++){
        // Tag 0:2:1:1:i+1 should hold the original face. GetLatestShape returns the latest
        // modification of this face in the Topological History
        std::ostringstream tag;
        tag << "0:2:1:1:" << (i+1);
        TopoDS_Shape origShape = _TopoNamer.GetLatestShape(tag.str());
        origFace = TopoDS::Face(origShape);
        newFace  = newFaces[i].origShape;

        if (!_TopoNamer.CompareTwoFaceTopologies(origFace, newFace)){
            TData.ModifiedFaces.push_back({origFace, newFace});
        }
    }
    //std::clog << "-----Dumping TopoHistory after update" << std::endl;
    //std::clog << _TopoNamer.DeepDump() << std::endl;

    //this->_TopoNamer.TrackModifiedShape("0:2", TData, "Modified Box Node");
    this->_TopoNamer.TrackShape(TData, "0:2");
    this->setShape(mkBox.Shape());
}

void TopoShape::CreateShallowFilletBaseShape(const TopoShape& BaseShape)
{
    //this->setShape(BaseShape);

    // Node 2
    //this->_TopoNamer.AddNode("BaseShapes");
    //// Node 3
    this->_TopoNamer.AddNode("FilletShapes");    
}

void TopoShape::createFilletBaseShape(const TopoShape& BaseShape)
{
    // Node 2
    //this->_TopoNamer.AddNode("BaseShapes");
    // Node 3
    this->_TopoNamer.AddNode("FilletShapes");

    // Collect and track appropriate data
    TopoData TData;

    TopTools_IndexedMapOfShape faces;
    // TODO need to handle possible seam edges
    TopExp::MapShapes(BaseShape.getShape(), TopAbs_FACE, faces);
    for (int i=1; i <= faces.Extent(); i++){
        TopoDS_Face face = TopoDS::Face(faces.FindKey(i));
        TopoData::TrackedData<TopoDS_Face> data;
        data.origShape = face;
        TData.GeneratedFaces.push_back(data);
    }

    TData.NewShape = BaseShape.getShape();
    TData.text = "Fillet Base Shape";
    //this->_TopoNamer.TrackGeneratedShape(this->_TopoNamer.GetNode(2), TData, "Generated Base Shape");
    this->_TopoNamer.TrackShape(TData, this->_TopoNamer.GetNode(2));
    this->setShape(BaseShape.getShape());
}

BRepFilletAPI_MakeFillet TopoShape::createFillet(const TopoShape& BaseShape, const std::vector<FilletElement>& FDatas)
{
    TopoDS_Shape& topodds_shape = BaseShape.getShape();

    TopTools_IndexedMapOfShape listOfEdges;
    TopExp::MapShapes(topodds_shape, TopAbs_EDGE, listOfEdges);

    //TDF_Label SelectedLabel = TDF_TagSource::NewChild(this->_TopoNamer.GetSelectionLabel());
    //if (SelectedLabel.IsNull())
    //    std::clog << "\x1B[31m-->Selected label is null!\033[0m" << std::endl;
    //
    //TNaming_Selector SelectionBuilder(SelectedLabel);    

    // Make the fillets. NOTE: the edges should have already been 'selected' by
    // calling TopoShape::selectEdge(s) by the caller.
    BRepFilletAPI_MakeFillet mkFillet(topodds_shape);

    std::clog << "-----Dumping topohistory before filleting" << std::endl;
    std::clog << this->_TopoNamer.DeepDump() << std::endl;

    std::clog << "number of edges: " << listOfEdges.Size() << std::endl;

    for (auto&& FData: FDatas)
    {
        std::clog << "---------- Iterating FDatas for edge " << FData.edgeid << std::endl;
        
        const TopoDS_Shape& shape = *FData.selectedEdge; //listOfEdges.FindKey(FData.edgeid);

        //for (size_t i = 1; i <= listOfEdges.Extent(); i++)
        //{
        //    TopoDS_Shape edge = listOfEdges(i);
        //    if (!edge.IsNull())
        //    {
        //        std::clog << "Found edge with ID " << i << std::endl;
        //    }
        //}

        if (shape.IsNull())
            std::clog << "\x1B[31m-->Selected shape is null!\033[0m" << std::endl;

        // Get the specific edge, I hope
        const TopoDS_Edge& anEdge = TopoDS::Edge(shape);

        if (anEdge.IsNull())
            std::clog << "\x1B[31m-->Selected edge is null!\033[0m" << std::endl;

        std::clog << "-> Found edge!" << std::endl;

        //bool check = FData.selector->Select(anEdge, topodds_shape);
        //if (check) {
        //    std::clog << "---------- Selection \x1B[32mWAS\033[0m successful" << std::endl;
        //}
        //else {
        //    std::clog << "---------- Selection WAS \x1B[31mNOT\033[0m successful" << std::endl;
        //}
        //
        //std::clog << "Now solve the selection..." << std::endl;
        //
        //TDF_LabelMap MyMap;
        ////TopoDS_Edge edge = this->_TopoNamer.GetSelectedEdge(&MyMap, &SelectionBuilder, &SelectedLabel);
        //
        //if (FData.selector->NamedShape().IsNull())
        //    std::clog << "\x1B[31mPassed selector is null!\033[0m" << std::endl;

        //bool solved = FData.selector->Solve(MyMap);
        //
        //if (solved)
        //{            
        //    std::clog << "---------- Selection solve \x1B[32mWAS\033[0m successful......" << std::endl;
        //}
        //else
        //{
        //    std::clog << "---------- Selection solve was \x1B[31mNOT\033[0m successful......" << std::endl;
        //}
        //Handle(TNaming_NamedShape) EdgeNS = MySelector.NamedShape();
        //EdgeNode.FindAttribute(TNaming_NamedShape::GetID(), EdgeNS);
        //const TopoDS_Edge& SelectedEdge = TopoDS::Edge(MySelector.NamedShape()->Get());
        
        //TopoDS_Edge SelectedEdge = TopoDS::Edge(SelectionBuilder.NamedShape()->Get());

        mkFillet.Add(FData.radius1, FData.radius2, anEdge);
    }

    mkFillet.Build();

    std::clog << "-----Dumping topohistory before getfilletdata" << std::endl;
    std::clog << this->_TopoNamer.DeepDump() << std::endl;

    TopoData TFData = this->getFilletData(BaseShape, mkFillet);
    
    //this->_TopoNamer.TrackGeneratedShape(this->_TopoNamer.GetNode(3), TFData, "Filleted Shape");
    //std::clog << "---------- Track filleted shape" << std::endl;
    //std::clog << "-----Dumping topohistory before updateFillet" << std::endl;
    //std::clog << this->_TopoNamer.DeepDump() << std::endl;
    this->_TopoNamer.TrackShape(TFData, this->_TopoNamer.GetNode(3));
    this->setShape(mkFillet.Shape());
    std::clog << "-----Dumping topohistory after updateFillet" << std::endl;
    std::clog << this->_TopoNamer.DeepDump() << std::endl;

    std::clog << "---------- Fillet end." << std::endl;

    return mkFillet;
}

BRepFilletAPI_MakeFillet TopoShape::updateFillet2(const TopoShape& BaseShape, const std::vector<FilletElement>& FDatas)
{
    std::clog << "Update fillet 1" << std::endl;

    std::clog << "Update fillet 2" << std::endl;
    BRepFilletAPI_MakeFillet mkFillet(BaseShape.getShape());
    std::clog << "Update fillet 3" << std::endl;
    for (auto&& FData : FDatas) {
        TopoDS_Edge edge = this->_TopoNamer.GetSelectedEdge(nullptr, FData.selector, FData.selectionlabel);

        std::cout << "Is selected edge " << FData.edgeid << " existing? " << (!edge.IsNull()) << std::endl;

        // For debugging purposes
        this->_TopoNamer.WriteShape(edge, "SelectedEdge");

        mkFillet.Add(FData.radius1, FData.radius2, edge);
    }
    std::clog << "Update fillet 4" << std::endl;

    try
    {
        mkFillet.Build();

        if (!mkFillet.IsDone()) std::clog << "Update fillet ERROR" << std::endl;
    }
    catch (Standard_Failure sf)
    {
        std::clog << "\x1B[31mMKFillet error: " << sf << "\033[0m" << std::endl;
    }    

    std::clog << "Update fillet 5" << std::endl;
    TopoData TFData = this->getFilletData(BaseShape, mkFillet);
    TFData.text = "Updated Fillet";
    TFData.OldShape = this->getShape();
    TFData.NewShape = mkFillet.Shape();
    TFData.text = "Updated Fillet";
    //this->_TopoNamer.TrackModifiedShape(this->_TopoNamer.GetNode(3), TFData, "Filleted Shape");
    this->_TopoNamer.TrackShape(TFData, this->_TopoNamer.GetNode(3));
    this->setShape(mkFillet.Shape());
    //std::clog << "-----Dumping topohistory after updateFillet" << std::endl;
    //std::clog << this->_TopoNamer.DeepDump() << std::endl;
    std::clog << "Update fillet 6" << std::endl;
    return mkFillet;
}

BRepFilletAPI_MakeFillet TopoShape::updateFillet(const TopoShape& BaseShape, const std::vector<FilletElement>& FDatas)
{
    // Update the BaseShape topo history as appropriate
    std::clog << "Update fillet 1" << std::endl;
    this->_TopoNamer.AppendTopoHistory("0:2", BaseShape.getTopoHelper());
    //std::clog << "-----Dumping after Append" << std::endl;
    //std::clog << this->_TopoNamer.DeepDump();
    //std::clog << this->_TopoNamer.DFDump();
 
    // Make the fillets. NOTE: the edges should have already been 'selected' by
    // calling TopoShape::selectEdge(s) by the caller.
    std::clog << "Update fillet 2" << std::endl;
    TopoDS_Shape localBaseShape = this->_TopoNamer.GetTipShape("0:2");
    BRepFilletAPI_MakeFillet mkFillet(localBaseShape);
    std::clog << "Update fillet 3" << std::endl;
    for (auto&& FData: FDatas){
        TopoDS_Edge edge = this->_TopoNamer.GetSelectedEdge(nullptr, FData.selector, FData.selectionlabel);
    
        std::cout << "Is selected edge " << FData.edgeid << " existing? " << (!edge.IsNull()) << std::endl;

        // For debugging purposes
        this->_TopoNamer.WriteShape(edge, "SelectedEdge");
    
        mkFillet.Add(FData.radius1, FData.radius2, edge);
    }
    std::clog << "Update fillet 4" << std::endl;
    
    try
    {
        mkFillet.Build();
    }
    catch (Standard_Failure sf)
    {
        std::clog << "\x1B[31mMKFillet error: " << sf << "\033[0m" << std::endl;        
    }

    if (!mkFillet.IsDone()) std::clog << "Update fillet ERROR" << std::endl;

    std::clog << "Update fillet 5" << std::endl;
    TopoData TFData = this->getFilletData(BaseShape, mkFillet);
    TFData.text = "Updated Fillet";
    TFData.OldShape = this->getShape();
    TFData.NewShape = mkFillet.Shape();
    TFData.text = "Updated Fillet";
    //this->_TopoNamer.TrackModifiedShape(this->_TopoNamer.GetNode(3), TFData, "Filleted Shape");
    this->_TopoNamer.TrackShape(TFData, this->_TopoNamer.GetNode(3));
    this->setShape(mkFillet.Shape());
    //std::clog << "-----Dumping topohistory after updateFillet" << std::endl;
    //std::clog << this->_TopoNamer.DeepDump() << std::endl;
    std::clog << "Update fillet 6" << std::endl;
    return mkFillet;
}

std::string TopoShape::selectEdge(const int edgeID)
{
    TopTools_IndexedMapOfShape listOfEdges;
    TopExp::MapShapes(_Shape, TopAbs_EDGE, listOfEdges);

    // Get the specific edge, I hope
    const TopoDS_Edge& anEdge = TopoDS::Edge(listOfEdges.FindKey(edgeID));

    std::string edgeLabel = this->_TopoNamer.SelectEdge(anEdge, _Shape);
    return edgeLabel;
}

TopoDS_Edge TopoShape::selectEdge(const int edgeID, TNaming_Selector* selector, TDF_Label* selectionLabel)
{
    TopTools_IndexedMapOfShape listOfEdges;
    TopExp::MapShapes(_Shape, TopAbs_EDGE, listOfEdges);

    // Get the specific edge, I hope
    const TopoDS_Edge& anEdge = TopoDS::Edge(listOfEdges.FindKey(edgeID));

    std::string edgeLabel = this->_TopoNamer.SelectEdge(anEdge, _Shape, selector, selectionLabel);
    return anEdge;
}

//-------------------- Private Methods--------------------

std::vector<TopoData::TrackedData<TopoDS_Face>> TopoShape::getBoxFacesVector(BRepPrimAPI_MakeBox mkBox) const{
    //std::vector<TopoDS_Face> OutFaces = {mkBox.TopFace(), mkBox.BottomFace(), mkBox.LeftFace(), mkBox.RightFace(), mkBox.FrontFace(), mkBox.BackFace()};

    std::vector<TopoData::TrackedData<TopoDS_Face>> OutFaces;
    TopoData::TrackedData<TopoDS_Face> top;
    top.origShape = mkBox.TopFace();
    OutFaces.push_back(top);

    TopoData::TrackedData<TopoDS_Face> bottom;
    bottom.origShape = mkBox.BottomFace();
    OutFaces.push_back(bottom);

    TopoData::TrackedData<TopoDS_Face> left;
    left.origShape = mkBox.LeftFace();
    OutFaces.push_back(left);

    TopoData::TrackedData<TopoDS_Face> right;
    right.origShape = mkBox.RightFace();
    OutFaces.push_back(right);

    TopoData::TrackedData<TopoDS_Face> front;
    front.origShape = mkBox.FrontFace();
    OutFaces.push_back(front);

    TopoData::TrackedData<TopoDS_Face> back;
    back.origShape = mkBox.BackFace();
    OutFaces.push_back(back);

    return OutFaces;
}
TopTools_ListOfShape TopoShape::getBoxFaces(BRepPrimAPI_MakeBox mkBox) const{
    TopTools_ListOfShape OutFaces;
    OutFaces.Append(mkBox.TopFace());
    OutFaces.Append(mkBox.BottomFace());
    OutFaces.Append(mkBox.LeftFace());
    OutFaces.Append(mkBox.RightFace());
    OutFaces.Append(mkBox.FrontFace());
    OutFaces.Append(mkBox.BackFace());
    return OutFaces;
}

TopoData TopoShape::getFilletData(const TopoShape& BaseShape, BRepFilletAPI_MakeFillet& mkFillet) const{
    // Get the data we need for topo history
    TopoData TFData;

    TopTools_IndexedMapOfShape faces;
    // TODO need to handle possible seam edges
    // TODO need to pull BaseShape from topo tree
    TopExp::MapShapes(BaseShape.getShape(), TopAbs_FACE, faces);
    for (int i=1; i <= faces.Extent(); i++){
        TopoDS_Face face = TopoDS::Face(faces.FindKey(i));
        TopTools_ListOfShape modified = mkFillet.Modified(face);
        if (modified.Extent() == 1){
            TopoDS_Face newFace = TopoDS::Face(modified.First());
            TFData.ModifiedFaces.push_back({face, newFace});
        }
        else if (modified.Extent() != 0){
            throw std::runtime_error("Fillet should only produce a single modified face per face, or none");
        }
    }

    TopTools_IndexedMapOfShape edges;

    TopExp::MapShapes(BaseShape.getShape(), TopAbs_EDGE, edges);
    for (int i=1; i<=edges.Extent(); i++){
        TopoDS_Edge edge = TopoDS::Edge(edges.FindKey(i));
        TopTools_ListOfShape generated = mkFillet.Generated(edge);
        TopTools_ListIteratorOfListOfShape genIt(generated);
        for (; genIt.More(); genIt.Next()){
            TopoDS_Face genFace = TopoDS::Face(genIt.Value());
            TFData.GeneratedFacesFromEdge.push_back({edge, genFace});
        }
    }

    TopTools_IndexedMapOfShape vertexes;
    TopExp::MapShapes(BaseShape.getShape(), TopAbs_VERTEX, vertexes);
    for (int i=1; i<=vertexes.Extent(); i++){
        TopoDS_Vertex vertex = TopoDS::Vertex(vertexes.FindKey(i));
        TopTools_ListOfShape generated = mkFillet.Generated(vertex);
        TopTools_ListIteratorOfListOfShape genIt(generated);
        for (; genIt.More(); genIt.Next()){
            TopoDS_Face genFace = TopoDS::Face(genIt.Value());
            TFData.GeneratedFacesFromVertex.push_back({vertex, genFace});
        }
    }

    TFData.NewShape = mkFillet.Shape();
    return TFData;
}