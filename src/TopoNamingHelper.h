#ifndef TOPO_NAMINGHELPER_H
#define TOPO_NAMINGHELPER_H

#include <vector>
#include <string>

#include <TNaming.hxx>
#include <TNaming_Builder.hxx>

#include <TDF_Data.hxx>
#include <TDF_Label.hxx>
#include <TDF_TagSource.hxx>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopTools_Array1OfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

#include "TopoNamingData.h"

class TopoNamingHelper
{
public:
	TopoNamingHelper();
	TopoNamingHelper(const TopoNamingHelper& existing);
	~TopoNamingHelper();

	void operator = (const TopoNamingHelper&);

	// TODO Need to add methods for tracking a translation to a shape.
	// Make changes to the Data Framework to track Topological Changes
	// 
	void TrackGeneratedShape(const TopoDS_Shape& GeneratedShape, const std::string& name);
	void TrackGeneratedShape(const TopoDS_Shape& GeneratedShape, const TopoData& TData, const std::string& name);
	TDF_Label TrackGeneratedShape(const std::string& parent_tag, const TopoDS_Shape& GeneratedShape, const TopoData& TData, const std::string& name);
	TDF_Label TrackGeneratedShape(const std::string& parent_tag, const TopoDS_Shape& GeneratedShape, const FilletData& FData, const std::string& name);
	//void TrackFuseOperation(BRepAlgoAPI_Fuse& Fuser);
	void TrackFilletOperation(const TopoDS_Shape& BaseShape, TopoDS_Shape& ResultShape, BRepFilletAPI_MakeFillet& Filleter);
	void TrackModifiedShape(const TopoDS_Shape& NewShape, const TopoData& TData, const std::string& name);
	void TrackModifiedShape(const std::string& OrigShapeNodeTag, const TopoDS_Shape& NewShape, const TopoData& TData, const std::string& name);
	void TrackModifiedFilletBaseShape(const TopoDS_Shape& NewBaseShape);
	std::string SelectEdge(const TopoDS_Edge& anEdge, const TopoDS_Shape& aShape);
	std::string SelectEdge(const TopoDS_Edge& anEdge, const TopoDS_Shape& aShape, TNaming_Selector& selector, TDF_Label& selectionLabel);
	std::vector<std::string> SelectEdges(const std::vector<TopoDS_Edge> Edges, const TopoDS_Shape& aShape);
	// NOTE: This function is very fragile right now. It assumes that InputData is
	// the same as BaseRoot plus zero or more evolution nodes. It simply adds to
	// BaseRoot any nodes that it doesn't have. It doesn't check anything else!!!
	bool AppendTopoHistory(const std::string& BaseRoot, const TopoNamingHelper& InputData, const std::string& InputTargetNode);
	bool AppendTopoHistorySimple(const std::string& TargetRoot, const TopoNamingHelper& SourceData);

	// Various helper functions

	// Returns the edge at the NodeTag, i.e. "0:2"
	TopoDS_Edge GetSelectedEdge(const std::string NodeTag) const;
	// Returns the Context Shape for selected edge located at NodeTag: TODO does this
	// work 100% of the time? It seems sometimes the sub-node is NOT the context
	// shape...
	TopoDS_Shape GetSelectedBaseShape(const std::string NodeTag) const;
	// Return the TopoDS_Shape stored in the NodeTag, i.e. "0:4"
	TopoDS_Shape GetNodeShape(const std::string NodeTag) const;
	// Intended to be used for a Data Framework that is keeping track of the evolution
	// of a Filleted shape. If the Data Framework does not contain a Fillet node, this
	// method may not work as expected
	std::string GetLatestFilletBase() const;
	// Return the TopoDS_Shape at the very tip of the Data Framework. TODO do we need
	// to check to make sure the latest operation actually stored a TopoDS_Shape?
	TopoDS_Shape GetTipShape() const;
	// Return the tag to the TDF_Label at the very tip of the Data Framework
	std::string GetTipNode() const;
	// Get the tag to the Nth child node in the root tree
	std::string GetNode(const int& n) const;
	// get the tag to the Nth child node from the Parent label.
	std::string GetNode(const std::string& tag, const int& n) const;
	// Does the Topo tree have additional nodes aside from the Selection one created
	// at initialization?
	bool HasNodes() const;
	//// Are the two nodes equivalent?
	//bool TreesEquivalent(const TDF_Label& Node1, const TDF_Label& Node2) const;
	void AddNode(const std::string& Name = "");
	//TopoDS_Shape GetGeneratedShape(const TDF_Label& parent, const int& node);
	TopoDS_Shape GetLatestShape(const std::string& tag);
	//TopoDS_Shape GetModifiedNewShape(const TDF_Label& parent, const int& node);

	// Non-Member Class functions
	static bool CompareTwoFaceTopologies(const TopoDS_Shape& face1, const TopoDS_Shape& face2);
	static bool CompareTwoEdgeTopologies(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2, int numCheckPoints = 10);
	static void WriteShape(const TopoDS_Shape& aShape, const std::string& NameBase, const int& numb = -1);

	TDF_Label GetRootNode() { return myRootNode; }
	TDF_Label GetSelectionNode() { return mySelectionNode; }

	// debugging stuff

	// Dump actually uses OCC DeepDump. Sort of confusing, TODO should I rename these?
	void Dump() const;
	void Dump(std::ostream& stream) const;
	// A custom Dump function that is more concise and informative than the OCC one
	std::string DeepDump() const;
	std::string DeepDump2() const;
	void DeepDump(std::stringstream& stream) const;
	void DeepDump2(std::stringstream& stream) const;
	// Dump the whole Data Framework and attributes too
	std::string DFDump() const;

	std::string GetTextFromLabel(const TDF_Label& Label) const;

private:
	// This helps make the DeepDump output more legible
	void AddTextToLabel(const TDF_Label& Label, const std::string& name, const std::string& extra = "");
	bool CheckIfSelectionExists(const TDF_Label aNode, const TopoDS_Face aFace) const;
	// Get TopoDS_Shape stored in the nth node under the passed Label
	TopoDS_Shape GetChildShape(const TDF_Label& ParentLabel, const int& n) const;
	// Write out a BREP file of the TopoDS_Shape at aLabel. The file will be named
	// "<NameBase>_<numb>.brep"
	void WriteShape(const TDF_Label aLabel, const std::string NameBase, const int numb) const;
	// Write out a BREP file of the TopoDS_Shape at the label with the address
	// <NodeTag>. if <Deep> is true, also write out for all children, with
	// <NameBase>_1.brep, <NameBase>_2.brep etc... as the filename.
	void WriteNode(const std::string NodeTag, const std::string NameBase, const bool Deep) const;
	TDF_Label LabelFromTag(const std::string& tag) const;
	//void AppendNode(const TDF_Label& Parent, const TDF_Label& Target, const int& depth=0);
	void AppendNode(const TDF_Label& Parent, const TDF_Label& Target);

	// These are used for adding the respective types of Nodes to a parent Node
	void MakeGeneratedNode(const TDF_Label& Parent, const TopoDS_Face& aFace);
	void MakeGeneratedNodes(const TDF_Label& Parent, const std::vector<TopoDS_Face>& Faces);
	void MakeGeneratedFromEdgeNode(const TDF_Label& Parent, const std::pair<TopoDS_Edge, TopoDS_Face>& aPair);
	void MakeGeneratedFromEdgeNodes(const TDF_Label& Parent, const std::vector< std::pair<TopoDS_Edge, TopoDS_Face> >& Pairs);
	void MakeGeneratedFromVertexNode(const TDF_Label& Parent, const std::pair<TopoDS_Vertex, TopoDS_Face>& aPair);
	void MakeGeneratedFromVertexNodes(const TDF_Label& Parent, const std::vector< std::pair<TopoDS_Vertex, TopoDS_Face> >& Pairs);
	void MakeModifiedNode(const TDF_Label& Parent, const std::pair<TopoDS_Face, TopoDS_Face>& aPair);
	void MakeModifiedNodes(const TDF_Label& Parent, const std::vector< std::pair<TopoDS_Face, TopoDS_Face> >& aPairs);
	void MakeDeletedNode(const TDF_Label& Parent, const TopoDS_Face& aFace);
	void MakeDeletedNodes(const TDF_Label& Parent, const std::vector<TopoDS_Face>& Faces);
	//bool NodesAreEqual(const TDF_Label& Node1, const TDF_Label& Node2) const;

	// Finally, class member variables
	Handle(TDF_Data) myDataFramework = new TDF_Data();
	TDF_Label myRootNode = myDataFramework->Root();
	TDF_Label mySelectionNode;
};
#endif /* ifndef TOPONAMINGHELPER_H */
