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
#ifndef TopoNamingData_H
#define TopoNamingData_H

#include <vector>
#include <array>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

//enum class BoxState{
    //length     = 1<<0,
    //height     = 1<<1,
    //width      = 1<<2,
    //translated = 1<<3,
//};

struct TopoData{
    template <typename T>
    struct TrackedData{
        // Either all three of these are null, or ONLY ONE is non-null
        T origShape;
        // If ane of the 'orig' are non-null, this can be null or not. If they are all
        // null, this MUST NOT be null.
        TopoDS_Face newFace;
        std::string text;
    };

    // OldShape for Modified or Generated or Deleted TNaming_Evolution
    TopoDS_Shape OldShape;
    // NewShape for any evolution except Generated
    TopoDS_Shape NewShape;
    std::string text;

    std::vector<TopoDS_Face> PrimitiveFaces;
    std::vector<TopoDS_Face> GeneratedFaces;
    std::vector<std::pair<TopoDS_Face, TopoDS_Face>> ModifiedFaces;
    std::vector<TopoDS_Face> DeletedFaces;
    std::vector< std::pair<std::string, gp_Trsf> > TranslatedFaces;
    std::vector<std::pair<TopoDS_Vertex, TopoDS_Face>> GeneratedFacesFromEdge;
    std::vector<std::pair<TopoDS_Vertex, TopoDS_Face>> GeneratedFacesFromVertex;

    //std::vector<TrackedData<TopoDS_Face>> PrimitiveFaces;
    //std::vector<TrackedData<TopoDS_Face>> GeneratedFaces;
    //std::vector<TrackedData<TopoDS_Face>> ModifiedFaces;
    //std::vector<TrackedData<TopoDS_Face>> DeletedFaces;
    //std::vector< std::pair<std::string, gp_Trsf> > TranslatedFaces;
    //std::vector<TrackedData<TopoDS_Edge>> GeneratedFacesFromEdge;
    //std::vector<TrackedData<TopoDS_Vertex>> GeneratedFacesFromVertex;

};

struct FilletData : TopoData
{
    //std::string edgeTag;
    std::vector< std::pair<TopoDS_Edge, TopoDS_Face>> GeneratedFacesFromEdge;
    std::vector< std::pair<TopoDS_Vertex, TopoDS_Face>> GeneratedFacesFromVertex;
};

struct BoxData{
    BoxData(double height, double length, double width){
        Height = height;
        Length = length;
        Width = width;
    }
    double Height=0;
    double Length=0;
    double Width=0;
};
#endif /* ifndef TopoNamingData_H */
