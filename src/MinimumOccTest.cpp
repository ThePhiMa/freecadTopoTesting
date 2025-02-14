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

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopOpeBRepBuild_HBuilder.hxx>
#include <TNaming.hxx>
#include "TopoNamingHelper.h"
#include <TNaming_NamedShape.hxx>
#include <TNaming_Tool.hxx>
#include "FakeTopoShape.h"
#include <STEPControl_Controller.hxx>
#include <STEPControl_Writer.hxx>
#include <TNaming_Selector.hxx>
#include <BRepAlgo_Cut.hxx>
#include <BRepAlgo.hxx>
#include <TDataStd_AsciiString.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_IDFilter.hxx>
#include <TNaming_UsedShapes.hxx>
#include <TDF_Tool.hxx>
#include <TNaming_Selector.hxx>

#define OCCT_DEBUG_NBS
#define OCCT_DEBUG_CC
#define OCCT_DEBUG_SEL


void ExportTopoShapeAsSTEP(TopoDS_Shape source, Standard_CString fileName)
{
	STEPControl_Controller::Init(); // Is this necessary?
	STEPControl_Writer writer;
	writer.Transfer(source, STEPControl_ManifoldSolidBrep);
	//FString projDir = FPaths::ProjectDir();
	//FString fileName = ((*shape)->GetShapeTypeString()).Append(".step");
	//projDir.Append(fileName);
	//Standard_CString name(TCHAR_TO_ANSI(*projDir));

	writer.Write(fileName);
}


void TestMkFillet()
{
	TopoDS_Shape Box = BRepPrimAPI_MakeBox(10., 10., 10.);
	BRepFilletAPI_MakeFillet mkFillet(Box);

	TopTools_IndexedMapOfShape edges;
	TopExp::MapShapes(Box, TopAbs_EDGE, edges);
	TopoDS_Edge edge = TopoDS::Edge(edges.FindKey(3));

	mkFillet.Add(1., 1., edge);
	mkFillet.Build();

	TopoDS_Shape result = mkFillet.Shape();


	TopTools_IndexedMapOfShape faces;
	TopExp::MapShapes(Box, TopAbs_FACE, faces);
	TopoDS_Face face = TopoDS::Face(faces.FindKey(1));

	for (int i = 1; i <= faces.Extent(); i++)
	{
		TopoDS_Face baseFace = TopoDS::Face(faces.FindKey(i));
		std::ostringstream name;
		name << "00_0" << i << "_BaseFace";
		TopoNamingHelper::WriteShape(baseFace, name.str());

		const TopTools_ListOfShape& modified = mkFillet.Modified(baseFace);
		TopTools_ListIteratorOfListOfShape modIt(modified);
		for (int j = 1; modIt.More(); modIt.Next(), j++)
		{
			TopoDS_Face curFace = TopoDS::Face(modIt.Value());
			std::ostringstream name2;
			name2 << "00_0" << i << "_ModifiedFace";
			TopoNamingHelper::WriteShape(curFace, name2.str(), j);
		}
	}

	TopoNamingHelper::WriteShape(Box, "00_00_Box");
	TopoNamingHelper::WriteShape(result, "00_00_FilletResult");
	std::clog << "Done." << std::endl;

	ExportTopoShapeAsSTEP(result, Standard_CString("0_ResulTMKFilletShape.step"));
}

class Part
{
public:
	Part();
	~Part();
	void setShape(const TopoShape& shape);
	const TopoShape& getShape() const;

private:
	TopoShape _Shape;
};

Part::Part()
{}

Part::~Part()
{}

void Part::setShape(const TopoShape& shape)
{
	_Shape = shape;
}

const TopoShape& Part::getShape() const
{
	return _Shape;
}

void TestResizeBox()
{
	Part BoxPart;

	TopoShape BoxShape = BoxPart.getShape();

	// initial box  
	std::clog << "------------------------------" << std::endl;
	std::clog << "Creating box" << std::endl;
	std::clog << "------------------------------" << std::endl;
	BoxData BData(10., 10., 10.);
	BoxShape.CreateBox(BData);
	//BoxShape._TopoNamer.DeepDump();
	BoxPart.setShape(BoxShape);

	TopoDS_Shape originalBox = BoxShape.GetShape();

	std::clog << "Dumping history after first box" << std::endl;
	std::clog << BoxShape.GetTopoHelper().DeepDump2();

	ExportTopoShapeAsSTEP(BoxShape.GetShape(), Standard_CString("0_InitialBox.step"));

	std::clog << std::endl;
	std::clog << "------------------------------" << std::endl;
	std::clog << "Selecting edge and saving selection" << std::endl;
	std::clog << "------------------------------" << std::endl;
	TDF_Label selectionNode = BoxShape.GetTopoHelper().GetSelectionNode();
	TDF_Label edgeSelection = TDF_TagSource::NewChild(selectionNode);
	TNaming_Selector selector(edgeSelection);

	int selectedEdgeID = 7;

	std::string selectedEdge = BoxShape.SelectEdge(selectedEdgeID, selector, edgeSelection);

	std::clog << "Dumping history after edge selection" << std::endl;
	std::clog << BoxShape.GetTopoHelper().DeepDump2();

	//ExportTopoShapeAsSTEP(selectedEdge, Standard_CString("1_SelectedEdge.step"));

	// initial fillet
	std::clog << std::endl;
	std::clog << "------------------------------" << std::endl;
	std::clog << "Creating fillet" << std::endl;
	std::clog << "------------------------------" << std::endl;

	// First set the BaseShape
	Part FilletPart;
	TopoShape FilletShape = FilletPart.getShape();      // Is this needed if I just equalize it on the line after?
	FilletShape = BoxShape;
	//std::clog << "dumping" << std::endl;
	//std::clog << FilletShape.getTopoHelper().DeepDump2() << std::endl;
	//FilletShape.createFilletBaseShape(BoxPart.getShape());    

	// Next, gather the fillet data
	std::vector<FilletElement> FDatas;
	FilletElement FData;
	// Don't forget to select the appropriate Edge
	FData.edgeTag = selectedEdge;
	FData.edgeid = selectedEdgeID;
	FData.radius1 = 1.;
	FData.radius2 = 1.;
	FDatas.push_back(FData);

	// finally, create the Fillet
	BRepFilletAPI_MakeFillet mkFillet = FilletShape.CreateFillet(BoxPart.getShape(), FDatas);
	//FilletPart.setShape(FilletShape);

	ExportTopoShapeAsSTEP(FilletShape.GetShape(), Standard_CString("1_FirstFillet.step"));

	std::clog << "Dumping history after fillet" << std::endl;
	std::clog << FilletShape.GetTopoHelper().DeepDump2();

	// taller box
	std::clog << std::endl;
	std::clog << "------------------------------" << std::endl;
	std::clog << "Updating box" << std::endl;
	std::clog << "------------------------------" << std::endl;
	BData.Height = 15.;
	BoxShape.UpdateBox(BData);

	std::clog << "Dumping history after updateBox" << std::endl;
	std::clog << BoxShape.GetTopoHelper().DeepDump2();

	ExportTopoShapeAsSTEP(BoxShape.GetShape(), Standard_CString("2_RebuildBox.step"));

	//// rebuild fillet
	std::clog << std::endl;
	std::clog << "------------------------------" << std::endl;
	std::clog << "rebuilding fillet" << std::endl;
	std::clog << "------------------------------" << std::endl;

	Part FilletPart2;
	TopoShape FilletShape2 = FilletPart2.getShape();        // Is this needed if I just equalize it on the line after?
	FilletShape2 = BoxShape;
	std::clog << "dumping" << std::endl;
	std::clog << FilletShape2.GetTopoHelper().DeepDump2() << std::endl;
	//FilletShape2.createFilletBaseShape2(BoxPart.getShape());

	FilletShape2.UpdateFillet(BoxShape, FDatas);
	std::clog << "Dumping history after updateFillet" << std::endl;
	std::clog << FilletShape2.GetTopoHelper().DeepDump2();

	ExportTopoShapeAsSTEP(FilletShape2.GetShape(), Standard_CString("3_ModifiedFillet.step"));
}

void runCase3()
{
	// This is for the Data Framework
	Handle(TDF_Data) DF = new TDF_Data();
	TDF_Label aLabel = DF->Root();

	// Create our base box
	BRepPrimAPI_MakeBox myBox(10., 20., 30.);

	// Load the faces of the box into the Data Framework. This adds nodes to the
	// DataFramework tree, I believe. Note that myBoxLabel is added to the Root, while all
	// the rest are added underneath myBoxLabel, so they're at level 3. NOTE: it is not
	// necesarry to pass the second argument to FindChild, but I like to do so in order to
	// make it more explicit that I am asking it to create this Label if it doesn't exist.
	const Standard_Boolean createIfNotExists = Standard_True;
	TDF_Label myBoxLabel = aLabel.FindChild(1, createIfNotExists); // First Label in Root
	TDF_Label myBoxTopLabel = myBoxLabel.FindChild(1, createIfNotExists); // All these are under the first Label
	TDF_Label myBoxBotLabel = myBoxLabel.FindChild(2, createIfNotExists);
	TDF_Label myBoxRightLabel = myBoxLabel.FindChild(3, createIfNotExists);
	TDF_Label myBoxLeftLabel = myBoxLabel.FindChild(4, createIfNotExists);
	TDF_Label myBoxFrontLabel = myBoxLabel.FindChild(5, createIfNotExists);
	TDF_Label myBoxBackLabel = myBoxLabel.FindChild(6, createIfNotExists);

	// TNaming_Builder takes a label as it's constructor. It allows us to add 'attributes'
	// to the Label (see the OCAF documentation about Data Framework for more info). These
	// attributes, I believe, help us track changes to a Topological structure. This is
	// how Generated, Modified, and Deleted attributes are kept track of. Oh, and
	// apparently "One evolution type per builder must be used"
	TNaming_Builder myBoxBuilder(myBoxLabel);
	// Add the 'Generated' attribute to the myBox TopoDS_Shape. The myBoxLabel Label is
	// where we are storing attributes for the whole myBox TopoDS_Shape. Underneath that
	// label, we created other labels for each of the Faces, where we can track changes to
	// those Topological structures.
	myBoxBuilder.Generated(myBox.Shape());

	// create a Builder for each Face and add the 'Generated' attribute to the Data
	// Framework
	TNaming_Builder topBuilder(myBoxTopLabel);
	// Note, this is sort of cheating, for other Topo structures we'll have to do this on our own
	TopoDS_Face myBoxTopFace = myBox.TopFace();
	topBuilder.Generated(myBoxTopFace);

	// And, do the rest of the faces...
	TNaming_Builder botBuilder(myBoxBotLabel);
	TopoDS_Face myBoxBotFace = myBox.BottomFace();
	botBuilder.Generated(myBoxBotFace);

	TNaming_Builder rightBuilder(myBoxRightLabel);
	TopoDS_Face myBoxRightFace = myBox.RightFace();
	rightBuilder.Generated(myBoxRightFace);

	TNaming_Builder leftBuilder(myBoxLeftLabel);
	TopoDS_Face myBoxLeftFace = myBox.LeftFace();
	leftBuilder.Generated(myBoxLeftFace);

	TNaming_Builder frontBuilder(myBoxFrontLabel);
	TopoDS_Face myBoxFrontFace = myBox.FrontFace();
	frontBuilder.Generated(myBoxFrontFace);

	TNaming_Builder backBuilder(myBoxBackLabel);
	TopoDS_Face myBoxBackFace = myBox.BackFace();
	backBuilder.Generated(myBoxBackFace);

	// Create second box, which will be Cut from the first
	gp_Pnt cut_loc = gp_Pnt(7, 5, 30); // location for axis of cutBox
	gp_Dir cut_dir = gp_Dir(0, 0, -1); // direction for axis of cutBox
	gp_Ax2 cut_ax2 = gp_Ax2(cut_loc, cut_dir); // axis at cut_loc pointing down, so into our myBox
	BRepPrimAPI_MakeBox cutBox(cut_ax2, 5., 5., 5.);

	// It seems that even though cutBox is not a Fused part of our TopoDS_Shape, since it
	// is still a 'part' of it by means of being Cut out of it, we must store information
	// about it in our Data Framework. This is the equivalent of what we did for myBox,
	// and I think finding a way to write a generic function that accomplishes this is in
	// order, as doing it manually like this is not sustainable for the greater FreeCAD
	// project

	// number here is arbitrary, though we do have to keep track of our numbers somehow.
	// i.e. we used 1 for myBoxLabel, so we can't reuse it, and I have to remember that 1
	// correspondes to myBoxLabel, not cutBox. Seems like a job for a custom class or
	// something. hrm...
	TDF_Label cutBoxLabel = aLabel.FindChild(2, createIfNotExists);
	TDF_Label cutBoxTopLabel = cutBoxLabel.FindChild(1, createIfNotExists); // All these are under the first Label
	TDF_Label cutBoxBotLabel = cutBoxLabel.FindChild(2, createIfNotExists);
	TDF_Label cutBoxRightLabel = cutBoxLabel.FindChild(3, createIfNotExists);
	TDF_Label cutBoxLeftLabel = cutBoxLabel.FindChild(4, createIfNotExists);
	TDF_Label cutBoxFrontLabel = cutBoxLabel.FindChild(5, createIfNotExists);
	TDF_Label cutBoxBackLabel = cutBoxLabel.FindChild(6, createIfNotExists);

	TNaming_Builder cutBoxBuilder(cutBoxLabel);
	cutBoxBuilder.Generated(cutBox.Shape());

	TNaming_Builder topCutBuilder(cutBoxTopLabel);
	TopoDS_Face cutBoxTopFace = cutBox.TopFace();
	topCutBuilder.Generated(cutBoxTopFace);

	TNaming_Builder botCutBuilder(cutBoxBotLabel);
	TopoDS_Face cutBoxBotFace = cutBox.BottomFace();
	botCutBuilder.Generated(cutBoxBotFace);

	TNaming_Builder rightCutBuilder(cutBoxRightLabel);
	TopoDS_Face cutBoxRightFace = cutBox.RightFace();
	rightCutBuilder.Generated(cutBoxRightFace);

	TNaming_Builder leftCutBuilder(cutBoxLeftLabel);
	TopoDS_Face cutBoxLeftFace = cutBox.LeftFace();
	leftCutBuilder.Generated(cutBoxLeftFace);

	TNaming_Builder frontCutBuilder(cutBoxFrontLabel);
	TopoDS_Face cutBoxFrontFace = cutBox.FrontFace();
	frontCutBuilder.Generated(cutBoxFrontFace);

	TNaming_Builder backCutBuilder(cutBoxBackLabel);
	TopoDS_Face cutBoxBackFace = cutBox.BackFace();
	backCutBuilder.Generated(cutBoxBackFace);

	// OK, here's the hard/fun part. Let's cut cutBox from myBox and do all the
	// appropriate things to the DataFramework

	// First, we create anoter label in our DataFramework under Root. In other words, our
	// first label was for myBox, our second was for cutBox (both TopoDS_Shapes) and our
	// third label is for the resultant box of the Cut operation. we'll call it myBoxCut
	TDF_Label myBoxCutLabel = aLabel.FindChild(3, createIfNotExists);

	// Note: this is an example of how to recover an object from a particular node in our
	// Data Framework. This is key, as this is where opencascade takes care of the Robust
	// Reference thing for us. You'll notice that in this particular instance we don't
	// actually need to use this mechanism, as I still have a reference to myBox handy and
	// it hasn't been modified, but I'm doing this here for educational purposes.

	Handle(TNaming_NamedShape) namedShapeObject; // store the retrieved object here
	// This part of the occ example confounds me. How in the world could this CLASS METHOD
	// GetID know anything about an instance of TNaming_NamedShape enough to return to me
	// the GUID that I'm looking for? The occ documentation under "OCAF Shape
	// Attributes-->Using Naming Resources" suggests that there is also a method that each
	// object "of an attribute class" has that will return the GUID. this makes more sense
	// to me..
	myBoxLabel.FindAttribute(TNaming_NamedShape::GetID(), namedShapeObject);
	TopoDS_Shape myRecoveredBox = namedShapeObject->Get();

	// let's check if it's the same box!
	Standard_Boolean checkBox = myBox.Shape().IsEqual(myRecoveredBox);
	std::string output;
	if (checkBox)
		output = "True";
	else
		output = "False";
	std::cout << "myBox == myRecovered Box == " << output << std::endl;
	// output "myBox == myRecoveredBox == True" woah! it works! So, this should also work
	// for faces and stuff, right....?

	// Ok, I think I got a bit ahead of myself. I think it's this TNaming_Selector thing
	// that is key - we create a new TNaming_Selector for each TNaming_Label that we need
	// a constant reference too, and then it gives us a consistent, er, Name for each
	// TopoDS_Shape?

	TDF_Label resultCutLabel = myBoxCutLabel.FindChild(1); // child 1 of myBoxCutLabel, which is the third node in Root right now
	TNaming_Selector myBoxCutSelector(resultCutLabel);
	Handle(TNaming_NamedShape) cutBoxNamedShape;
	cutBoxLabel.FindAttribute(TNaming_NamedShape::GetID(), cutBoxNamedShape);

	// We're going to add data to the Data Framework. This data will be added to
	// the third node under the Root: we called it myBoxCutLabel. The data we are
	// adding corresponds to the result box from the Cut operation, and will store
	// the Modified, Deleted, and other relevant data that the Data Framework
	// needs.

	TDF_Label modifiedFacesLabel = myBoxCutLabel.FindChild(2); // remember, 1 was for the result of the Cut operation
	TDF_Label deletedFacesLabel = myBoxCutLabel.FindChild(3);
	TDF_Label intersectingFacesLabel = myBoxCutLabel.FindChild(4);
	TDF_Label newFacesLabel = myBoxCutLabel.FindChild(5);


	// *** TDF TEST ***
	TDF_LabelMap map;
	bool solved = myBoxCutSelector.Solve(map);
	std::clog << "--> Is the my box cut selector solved? " << (solved ? "Yes" : "No") << std::endl;


	// like above, this is not necessary b/c I still have a reference to cutBox from
	// earlier, but for completeness and b/c IRL (in real life) this will be done in a
	// separate function, I'm putting it in.
	const TopoDS_Shape& cutBoxRecovered = cutBoxNamedShape->Get();
	// The two arguments here are 'selection' and 'context', respectively. This means
	// "store in NamedShape (which is an instance attribute of this TNaming_Selector
	// instance) the name for the TopoDS_Shape 'selection' which is found with the
	// TopoDS_Shape 'context'. The 'context' thing probably makes more sense when
	// searching for a Face within a Shape, or an Edge within a Face.
	myBoxCutSelector.Select(cutBoxRecovered, cutBoxRecovered);

	// Finally, grab the TopoDS_Shape that resulted from the 'Select' operation. I could
	// do this in two steps, i.e. `TNaming_NamedShape ns = myBoxCutSelector.NamedShape()`
	// then `TopoDS_Shape& myShape = ns->Get()` but I won't. Doing it in two steps,
	// though, does highlight how the 'Select' method works 'behind the scenes' and stores
	// the output TNaming_NamedShape inside of itself, and then you have to go back and
	// grab it.
	//
	// So, why is this step necessary, if we alread have cutBoxRecovered? I think it's
	// because of the following: cutBoxRecovered is very 'young' in our Data Framework
	// tree. It was the orignial Box that was cut from the original grand-daddy Box myBox.
	// Other stuff may have happened since then. In this simple example, nothing else has
	// happened, but realistically we could be WAY down the Data Framework tree by now,
	// after Fillet operations and Fuse operations oter Boolean operations. So, although
	// we have a reference to the very 'young' cutBox, we don't really have a reference to
	// that box within the current context of our TopoDS_Shape. So, this next operation
	// get's us the same cutBox, but relative to the current state of the TopoDS_Shape.
	//
	// I think. This is all pure speculation. But I think it's right!
	const TopoDS_Shape& cutBoxSelected = myBoxCutSelector.NamedShape()->Get();

	// finally, now that we've retrieved both the box to cut and the box that we're going
	// to cut from it, we can perform the Cut operation and then store the necessary stuff
	// in our Data Framework

	BRepAlgo_Cut mkCut(myRecoveredBox, cutBoxSelected);
	TopoDS_Shape resultBox = mkCut.Shape();
	if (!mkCut.IsDone())
	{
		std::cout << "CUT: Algorithm failed" << std::endl;
		return;
	}
	else
	{
		// Alright, take a deep breath, b/c this stuff goes deep

		// First, it seems we must use BRepAlgo::IsValid to check if the faces generated
		// by the mkCut operation are Valid. *shrug* the occ example does it, so I do it
		TopTools_ListOfShape listBoxes;
		listBoxes.Append(myRecoveredBox);
		listBoxes.Append(cutBoxSelected);

		if (!BRepAlgo::IsValid(listBoxes, resultBox, Standard_True, Standard_False))
		{
			std::cout << "CUT: Result is not valid" << std::endl;
			return;
		}
		else
		{
			// The result of the operation gets stored in the root of this branch of our tree. Underneath this node we
			// have Children for the Box used to cut from the original, as well as for deleted faces, modified  faces,
			// etc...
			TNaming_Builder resultBoxBuilder(myBoxCutLabel);
			const TopoDS_Shape& preCutBox = mkCut.Shape1(); // first shape involved in boolean operation, i.e. myBox
			// The two arguments here are OldShape and NewShape, respectively. This stores in myBoxCutLabel a Modified
			// attribute which describes NewShape as a Modification of OldShape
			resultBoxBuilder.Modify(preCutBox, resultBox);

			// Going one level deeper, we need to store in the Data Framework the modified faces.
			TopTools_MapOfShape uniqueShapes;
			TNaming_Builder modifiedFacesBuilder(modifiedFacesLabel);

			// Create a TopExp_Explorer to traverse the FACE's of the original Box
			TopExp_Explorer FaceExplorer(preCutBox, TopAbs_FACE);

			// Alright, let's work through the logic on this nested For-loop structure. First, it loops through each
			// TopoDS_FACE in the preCutBox, i.e. the original Box before it was cut. For each Face in the preCutBox,
			// use the BRepAlgo_Cut instance we have from earlier, mkCut, to check whether or not that Face was Modified
			// during the Cut operation. In fact, mkCut.Modified returns a _list_ of Modified Shapes. Why? Hm..I think
			// it's b/c the Face that we're checking _could_ have gotten split into multiple Faces during the Cut
			// operation, and therefore the Modified check needs to give allowance for that. Not really sure though...
			// But, you know, assuming that's true, the next step is to iterate over overy one of these Modified Face's.
			// At each step of this second iteration, we make sure this possiblyModifiedFace is not the same as
			// currentFace before adding it to the Data Framework. 
			//
			// Why do we have to preform this check? And why doesn't currentFace itself get added to the Data Framework?
			// I don't know! Lol, in fact, I'm a bit confused myself. It seems to me that even if the Face returned from
			// mkCut.Modified _is_ the same as currentFace, that we'd still want to add it to the Data Framework, since,
			// well, it was Modified. For now, I'll blindly follow the OCAF example, and hopefully I'll become
			// enlightened as to what is going on here...
			for (; FaceExplorer.More(); FaceExplorer.Next())
			{
				const TopoDS_Shape& currentFace = FaceExplorer.Current();
				Standard_Boolean res = uniqueShapes.Add(currentFace);
				if (!res)
					// Face wasn't added, I think b/c it was already in the Map. Therefore, skip to the next Face. I
					// think in FreeCAD we use TopExp_MapShapes instead of this, not sure why the OCAF sample does this.
					// For now we'll do it their way...
					continue;
				const TopTools_ListOfShape& modifiedFaces = mkCut.Modified(currentFace);
				TopTools_ListIteratorOfListOfShape modifiedFacesIterator(modifiedFaces);
				for (; modifiedFacesIterator.More(); modifiedFacesIterator.Next())
				{
					const TopoDS_Shape& possiblyModifiedFace = modifiedFacesIterator.Value();
					if (!currentFace.IsSame(possiblyModifiedFace))
						std::cout << "Added a face to Modified" << std::endl;
					modifiedFacesBuilder.Modify(currentFace, possiblyModifiedFace);
				}
			}

			// Do the same as above for Deleted faces
			uniqueShapes.Clear();
			TNaming_Builder deletedFacesBuilder(deletedFacesLabel);
			FaceExplorer.Init(preCutBox, TopAbs_FACE);
			// So, why is this loop simpler than the Modified loop? My best guess is that, unlike with Modified, a
			// Deleted Face does not ever result in multiple Faces being produces - I THINK! Instead, a Face is either
			// Deleted or it's not. *shrug*
			for (; FaceExplorer.More(); FaceExplorer.Next())
			{
				const TopoDS_Shape& currentFace = FaceExplorer.Current();
				Standard_Boolean res = uniqueShapes.Add(currentFace);
				if (!res)
					continue;
				if (mkCut.IsDeleted(currentFace))
					deletedFacesBuilder.Delete(currentFace);
			}

			// Add to the Data Framework 'section edges'. I'm not gonna lie, don't really understand this whole
			// 'Intersecting Faces' thing, but it has something to do with the Boolean operation
			TNaming_Builder intersectingFacesBuilder(intersectingFacesLabel);
			Handle(TopOpeBRepBuild_HBuilder) build = mkCut.Builder();
			TopTools_ListIteratorOfListOfShape intersectingSections = build->Section();
			for (; intersectingSections.More(); intersectingSections.Next())
			{
				// So, Select I guess adds a Shape to the label - a shape that wasn't modified. Pretty confused about
				// this, why does it need two arguements?
				intersectingFacesBuilder.Select(intersectingSections.Value(), intersectingSections.Value());
			}

			// Finally, add to the Data Framework faces that were added
			const TopoDS_Shape& boxThatWasCutOut = mkCut.Shape2();
			TNaming_Builder newFacesBuilder(newFacesLabel);
			FaceExplorer.Init(boxThatWasCutOut, TopAbs_FACE);
			for (; FaceExplorer.More(); FaceExplorer.Next())
			{
				const TopoDS_Shape& currentFace = FaceExplorer.Current();
				const TopTools_ListOfShape& modifiedFaces = mkCut.Modified(currentFace);
				if (!modifiedFaces.IsEmpty())
				{
					TopTools_ListIteratorOfListOfShape modifiedFacesIterator(modifiedFaces);
					for (; modifiedFacesIterator.More(); modifiedFacesIterator.Next())
					{
						const TopoDS_Shape& newShape = modifiedFacesIterator.Value();
						// Uhm, the documentation has a lot to say about this NamedShape method. :-/
						Handle(TNaming_NamedShape) aNamedShape = TNaming_Tool::NamedShape(newShape, newFacesLabel);
						if (aNamedShape.IsNull() || aNamedShape->Evolution() != TNaming_MODIFY)
						{
							std::cout << "Added New Face" << std::endl;
							newFacesBuilder.Generated(currentFace, newShape);
						}
					}
				}
			}
		}
	}

	// PHEW! Maybe one day I'll understand all of that... For now, let's just accept that we're done with the CUT
	// operation: we've cut stuff, and we've stored all the necessary data in the Data Framework. So, this means that,
	// although the Indexes between the Faces of myBox and resultBox may be different, I SHOULD be able to use the
	// Selector thing from earlier to get a constant reference to a given face...
	//printShapeInfos(myBox, cutBox, resultBox);

	// NOTE: if you pipe this output into a file, say "./occTest > tdfShapes.txt" you can then feed that txt file to the
	// parser2.py that I posted on the Forum "python parser2.py tdfShapes.txt" to get a side-by-side comparison

	// Success! Well, sort of. The Face indexes between myBox and resultBox are indeed different. So, now let's take
	// this Data Framework thing for a test drive...

	// scratch that, move on to selector
	//// First, let's ignore that Selector thing and see ho good this FindAttribute thing works
	//Handle(TNaming_NamedShape) namedShapeObject; // store the retrieved object here
	//myBoxLabel.FindAttribute(TNaming_NamedShape::GetID(), namedShapeObject);
	//TopoDS_Shape myRecoveredBox = namedShapeObject->Get();

	//TDF_Label myTestingNodeLabel = aLabel.FindChild(4, createIfNotExists);
	//TDF_Label myTopFaceLabel     = myTestingNodeLabel.FindChild(1, createIfNotExists);

	//TNaming_Selector myTopFaceSelector(resultCutLabel); // creates a selector at this Label

	//Handle(TNaming_NamedShape) topFaceNamedShape;
	//// myBoxTopLabel was defined waaaaay at the beginning of all this, when we first created myBox, before we Cut
	//// anything out of it. This FindAttribute stores the Shape associated with this label in topFaceNamedShape
	//myBoxTopLabel.FindAttribute(TNaming_NamedShape::GetID(), topFaceNamedShape);
	//const TopoDS_Shape& theTopFace = TNaming_Tool::CurrentShape(topFaceNamedShape);
	//std::cout << "The shape info for the recovered top face" << std::endl;
	//printShapeInfo(theTopFace);
	//TopTools_IndexedMapOfShape mapOfEdges;
	//TopExp::MapShapes(theTopFace, TopAbs_EDGE, mapOfEdges);
	//std::cout << "num Edges = " << mapOfEdges.Extent() << std::endl;

	//// ok, now time to check if theTopFace is the same as the original Top Face of myBox
	//TopTools_IndexedMapOfShape mapOfShapes;
	//TopExp::MapShapes(myBox, TopAbs_FACE, mapOfShapes);
	//int i=0;
	//for (int i = 1; i <= mapOfShapes.Extent(); i++){
		//TopoDS_Face curFace = TopoDS::Face(mapOfShapes.FindKey(i));
		//Standard_Boolean res1 = curFace.IsEqual(theTopFace);
		//Standard_Boolean res2 = curFace.IsSame(theTopFace);
		//TopTools_IndexedMapOfShape mapOfEdges;
		//TopExp::MapShapes(curFace, TopAbs_EDGE, mapOfEdges);
		//std::cout << "For i=" << i <<" IsEqual=" << res1 << " IsSame=" << res2 << " numEdges=" << mapOfEdges.Extent() << " ";
		//printShapeInfo(curFace);
	//}

	//std::cout << "--------Dump of NamedShape Attribute of newFacesLabel ------" << std::endl;
	//Handle(TNaming_NamedShape) newFacesNamedShape;
	//newFacesLabel.FindAttribute(TNaming_NamedShape::GetID(), newFacesNamedShape);
	//newFacesNamedShape->Dump(std::cout);
	//std::cout << "--------Dump of newFacesLabel-------" << std::endl;
	//newFacesLabel.Dump(std::cout);

}

void runCase4()
{
	// Created on: 1999-12-29
	// Created by: Sergey RUIN
	// Copyright (c) 1999-1999 Matra Datavision
	// Copyright (c) 1999-2014 OPEN CASCADE SAS
	//
	// This file is part of Open CASCADE Technology software library.
	//
	// This library is free software; you can redistribute it and / or modify it
	// under the terms of the GNU Lesser General Public version 2.1 as published
	// by the Free Software Foundation, with special exception defined in the file
	// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
	// distribution for complete text of the license and disclaimer of any warranty.
	//
	// Alternatively, this file may be used under the terms of Open CASCADE
	// commercial license or contractual agreement.


	// =======================================================================================
	// This sample contains template for typical actions with OCAF Topologigal Naming services
	// =======================================================================================

#define Box1POS          1
#define Box2POS          2
#define SelectedEdgesPOS 3
#define FilletPOS        4
#define CutPOS           5

	// Starting with data framework 
	Handle(TDF_Data) DF = new TDF_Data();
	TDF_Label aLabel = DF->Root();

	TopoDS_Shape Shape, Context;
	TCollection_AsciiString myName;
	Handle(TDataStd_AsciiString) nameAttribute;
	nameAttribute = new TDataStd_AsciiString();

	// ======================================================
	// Creating  NamedShapes with different type of Evolution
	// Scenario:
	// 1.Create Box1 and push it as PRIMITIVE in DF
	// 2.Create Box2 and push it as PRIMITIVE in DF
	// 3.Move Box2 (applying a transformation)
	// 4.Push a selected edges of top face of Box1 in DF,
	//   create Fillet (using selected edges) and push result as modification of Box1
	// 5.Create a Cut (Box1, Box2) as modification of Box1 and push it in DF
	// 6.Recover result from DF
	// ======================================================

	// =====================================
	// 1.Box1, TNaming_Evolution == PRIMITIVE
	// =====================================
	BRepPrimAPI_MakeBox MKBOX1(100, 100, 100); // creating Box1

	//Load the faces of the box in DF
	TDF_Label Box1Label = aLabel.FindChild(Box1POS);
	TDF_Label Top1 = Box1Label.FindChild(1);
	TDF_Label Bottom1 = Box1Label.FindChild(2);
	TDF_Label Right1 = Box1Label.FindChild(3);
	TDF_Label Left1 = Box1Label.FindChild(4);
	TDF_Label Front1 = Box1Label.FindChild(5);
	TDF_Label Back1 = Box1Label.FindChild(6);

	TNaming_Builder Box1Ins(Box1Label);
	Box1Ins.Generated(MKBOX1.Shape());
	myName = "Box1, TopoDS_Shape type, parent";
	nameAttribute = new TDataStd_AsciiString();
	nameAttribute->Set(myName);
	Box1Label.AddAttribute(nameAttribute);

	TNaming_Builder Top1FaceIns(Top1);
	TopoDS_Face Top1Face = MKBOX1.TopFace();
	Top1FaceIns.Generated(Top1Face);

	TopoDS_Face Bottom1Face = MKBOX1.BottomFace();
	TNaming_Builder Bottom1FaceIns(Bottom1);
	Bottom1FaceIns.Generated(Bottom1Face);

	TopoDS_Face Right1Face = MKBOX1.RightFace();
	TNaming_Builder Right1FaceIns(Right1);
	Right1FaceIns.Generated(Right1Face);

	TopoDS_Face Left1Face = MKBOX1.LeftFace();
	TNaming_Builder Left1FaceIns(Left1);
	Left1FaceIns.Generated(Left1Face);

	TopoDS_Face Front1Face = MKBOX1.FrontFace();
	TNaming_Builder Front1FaceIns(Front1);
	Front1FaceIns.Generated(Front1Face);

	TopoDS_Face Back1Face = MKBOX1.BackFace();
	TNaming_Builder Back1FaceIns(Back1);
	Back1FaceIns.Generated(Back1Face);

	// =====================================
	// 2.Box2, TNaming_Evolution == PRIMITIVE
	// =====================================
	BRepPrimAPI_MakeBox MKBOX2(150, 150, 150); // creating Box2

	//Load the faces of the box2 in DF
	TDF_Label Box2Label = aLabel.FindChild(Box2POS);
	TDF_Label Top2 = Box2Label.FindChild(1);
	TDF_Label Bottom2 = Box2Label.FindChild(2);
	TDF_Label Right2 = Box2Label.FindChild(3);
	TDF_Label Left2 = Box2Label.FindChild(4);
	TDF_Label Front2 = Box2Label.FindChild(5);
	TDF_Label Back2 = Box2Label.FindChild(6);

	TNaming_Builder Box2Ins(Box2Label);
	Box2Ins.Generated(MKBOX2.Shape());
	myName = "Box2, TopoDS_Shape type, parent";
	nameAttribute = new TDataStd_AsciiString();
	nameAttribute->Set(myName);
	Box2Label.AddAttribute(nameAttribute);

	TNaming_Builder Top2FaceIns(Top2);
	TopoDS_Face Top2Face = MKBOX2.TopFace();
	Top2FaceIns.Generated(Top2Face);

	TopoDS_Face Bottom2Face = MKBOX2.BottomFace();
	TNaming_Builder Bottom2FaceIns(Bottom2);
	Bottom2FaceIns.Generated(Bottom2Face);

	TopoDS_Face Right2Face = MKBOX2.RightFace();
	TNaming_Builder Right2FaceIns(Right2);
	Right2FaceIns.Generated(Right2Face);

	TopoDS_Face Left2Face = MKBOX2.LeftFace();
	TNaming_Builder Left2FaceIns(Left2);
	Left2FaceIns.Generated(Left2Face);

	TopoDS_Face Front2Face = MKBOX2.FrontFace();
	TNaming_Builder Front2FaceIns(Front2);
	Front2FaceIns.Generated(Front2Face);

	TopoDS_Face Back2Face = MKBOX2.BackFace();
	TNaming_Builder Back2FaceIns(Back2);
	Back2FaceIns.Generated(Back2Face);

	// ====================================
	// 3.Applying a transformation to Box2
	// ====================================
	gp_Vec vec1(gp_Pnt(0., 0., 0.), gp_Pnt(50., 50., 20.));
	gp_Trsf TRSF;
	TRSF.SetTranslation(vec1);
	TopLoc_Location loc(TRSF);
	TDF_LabelMap scope;
	TDF_ChildIterator itchild;
	for (itchild.Initialize(Box2Label, Standard_True); itchild.More(); itchild.Next())
	{
		if (itchild.Value().IsAttribute(TNaming_NamedShape::GetID()))
			scope.Add(itchild.Value());
	}
	if (Box2Label.IsAttribute(TNaming_NamedShape::GetID())) scope.Add(Box2Label);
	TDF_MapIteratorOfLabelMap it(scope);
	for (; it.More(); it.Next())
		TNaming::Displace(it.Key(), loc, Standard_True);//with oldshapes


	//============================================================================
	// 4.Push a selected edges of top face of Box1 in DF,
	// create Fillet (using selected edges) and push result as modification of Box1
	//=============================================================================
	Handle(TNaming_NamedShape) B1NS;
	Box1Label.FindAttribute(TNaming_NamedShape::GetID(), B1NS);
	const TopoDS_Shape& box1 = TNaming_Tool::GetShape(B1NS);
	Handle(TNaming_NamedShape) Top1NS;
	Top1.FindAttribute(TNaming_NamedShape::GetID(), Top1NS);
	const TopoDS_Shape& top1face = TNaming_Tool::GetShape(Top1NS);

	BRepFilletAPI_MakeFillet MKFILLET(box1);// fillet's algo
	TDF_Label SelectedEdgesLabel = aLabel.FindChild(SelectedEdgesPOS); //Label for selected edges

	myName = "SelectedEdgesLabel, this node contains no TopoDS_Shape, children contain TopoDS_Edges";
	nameAttribute = new TDataStd_AsciiString();
	nameAttribute->Set(myName);
	SelectedEdgesLabel.AddAttribute(nameAttribute);

	TopTools_IndexedMapOfShape mapOfEdges;
	TopExp::MapShapes(top1face, TopAbs_EDGE, mapOfEdges);
	Standard_Integer i = 1;
	std::cout << "numEdges = " << mapOfEdges.Extent() << std::endl;
	for (; i <= mapOfEdges.Extent(); i++)
	{
		const TopoDS_Edge& E = TopoDS::Edge(mapOfEdges(i));
		const TDF_Label& SelEdge = SelectedEdgesLabel.FindChild(i);
		// Creating TNaming_Selector on label
		TNaming_Selector Selector(SelEdge);

		// Inserting shape into data framework, we need the context to find neighbourhood of shape
		// For example the context for a lateral face of cone is cone itself
		// If a shape is standalone the context will be the shape itself

		// Selector.Select(Shape, Context);
		// TNaming_Evolution == SELECTED
		Selector.Select(E, box1);
		// Recover selected edge from DF, only for example
		const TopoDS_Edge& FE = TopoDS::Edge(Selector.NamedShape()->Get());
		//MKFILLET.Add(5., 5., FE);
		MKFILLET.Add(5., 5., E);
		std::cout << "i = " << i << " FE.IsNull = " << FE.IsNull() << " E.IsNull = " << E.IsNull() << std::endl;
	}

	MKFILLET.Build();
	if (!MKFILLET.IsDone())
	{
		std::cout << "fillet failed, bailing out" << std::endl;
		return; //Algorithm failed
	}

	// ...put fillet in the DataFramework as modification of Box1
	TDF_Label FilletLabel = aLabel.FindChild(FilletPOS);
	TDF_Label DeletedFaces = FilletLabel.FindChild(i++);
	TDF_Label ModifiedFaces = FilletLabel.FindChild(i++);
	TDF_Label FacesFromEdges = FilletLabel.FindChild(i++);
	TDF_Label FacesFromVertices = FilletLabel.FindChild(i);

	// TNaming_Evolution == MODIFY
	TNaming_Builder bFillet(FilletLabel);
	bFillet.Modify(box1, MKFILLET.Shape());

	myName = "FilletLabel, TopoDS_Shape type, parent -> filletedBox. Children are faces and such";
	nameAttribute = new TDataStd_AsciiString();
	nameAttribute->Set(myName);
	FilletLabel.AddAttribute(nameAttribute);

	//New faces generated from edges
	TopTools_MapOfShape View;
	TNaming_Builder FaceFromEdgeBuilder(FacesFromEdges);
	TopExp_Explorer ShapeExplorer(box1, TopAbs_EDGE);
	for (; ShapeExplorer.More(); ShapeExplorer.Next())
	{
		const TopoDS_Shape& Root = ShapeExplorer.Current();
		if (!View.Add(Root)) continue;
		const TopTools_ListOfShape& Shapes = MKFILLET.Generated(Root);
		TopTools_ListIteratorOfListOfShape ShapesIterator(Shapes);
		for (; ShapesIterator.More(); ShapesIterator.Next())
		{
			const TopoDS_Shape& newShape = ShapesIterator.Value();
			// TNaming_Evolution == GENERATED
			if (!Root.IsSame(newShape))
				FaceFromEdgeBuilder.Generated(Root, newShape);
		}
	}

	//Faces of the initial shape modified by MKFILLET
	View.Clear();
	TNaming_Builder ModFacesBuilder(ModifiedFaces);
	ShapeExplorer.Init(box1, TopAbs_FACE);
	for (; ShapeExplorer.More(); ShapeExplorer.Next())
	{
		const TopoDS_Shape& Root = ShapeExplorer.Current();
		if (!View.Add(Root)) continue;
		const TopTools_ListOfShape& Shapes = MKFILLET.Modified(Root);
		TopTools_ListIteratorOfListOfShape ShapesIterator(Shapes);
		for (; ShapesIterator.More(); ShapesIterator.Next())
		{
			const TopoDS_Shape& newShape = ShapesIterator.Value();
			// TNaming_Evolution == MODIFY
			if (!Root.IsSame(newShape))
				ModFacesBuilder.Modify(Root, newShape);
		}
	}

	//Deleted faces of the initial shape
	View.Clear();
	TNaming_Builder DelFacesBuilder(DeletedFaces);
	ShapeExplorer.Init(box1, TopAbs_FACE);
	for (; ShapeExplorer.More(); ShapeExplorer.Next())
	{
		const TopoDS_Shape& Root = ShapeExplorer.Current();
		if (!View.Add(Root)) continue;
		// TNaming_Evolution == DELETE
		if (MKFILLET.IsDeleted(Root)) DelFacesBuilder.Delete(Root);
	}

	//New faces generated from vertices
	View.Clear();
	TNaming_Builder FaceFromVertexBuilder(FacesFromVertices);
	ShapeExplorer.Init(box1, TopAbs_VERTEX);
	for (; ShapeExplorer.More(); ShapeExplorer.Next())
	{
		const TopoDS_Shape& Root = ShapeExplorer.Current();
		if (!View.Add(Root)) continue;
		const TopTools_ListOfShape& Shapes = MKFILLET.Generated(Root);
		TopTools_ListIteratorOfListOfShape ShapesIterator(Shapes);
		for (; ShapesIterator.More(); ShapesIterator.Next())
		{
			const TopoDS_Shape& newShape = ShapesIterator.Value();
			// TNaming_Evolution == GENERATED
			if (!Root.IsSame(newShape)) FaceFromVertexBuilder.Generated(Root, newShape);
		}
	}
	// =====================================================================
	// 5.Create a Cut (Box1, Box2) as modification of Box1 and push it in DF
	// Boolean operation - CUT Object=Box1, Tool=Box2
	// =====================================================================

	TDF_Label CutLabel = aLabel.FindChild(CutPOS);

	// recover Object
	Handle(TNaming_NamedShape) ObjectNS;
	FilletLabel.FindAttribute(TNaming_NamedShape::GetID(), ObjectNS);
	TopoDS_Shape OBJECT = ObjectNS->Get();

	// Select Tool
	TDF_Label ToolLabel = CutLabel.FindChild(1);
	TNaming_Selector ToolSelector(ToolLabel);
	Handle(TNaming_NamedShape) ToolNS;
	Box2Label.FindAttribute(TNaming_NamedShape::GetID(), ToolNS);
	const TopoDS_Shape& Tool = ToolNS->Get();
	//TNaming_Evolution == SELECTED

	std::cout << "CUT: Solving first" << std::endl;
	TDF_LabelMap map;
	bool solved = ToolSelector.Solve(map);
	std::clog << "----> Is the tool solve successful? " << (solved ? "yes" : "no") << std::endl;

	bool verify = ToolSelector.Select(Tool, Tool);
	std::clog << "----> Is the tool selection successful? " << (verify ? "yes" : "no") << std::endl;
	const TopoDS_Shape& TOOL = ToolSelector.NamedShape()->Get();

	BRepAlgo_Cut mkCUT(OBJECT, TOOL);

	if (!mkCUT.IsDone())
	{
		std::cout << "CUT: Algorithm failed" << std::endl;
		return;
	}
	else
	{
		TopTools_ListOfShape Larg;
		Larg.Append(OBJECT);
		Larg.Append(TOOL);

		if (!BRepAlgo::IsValid(Larg, mkCUT.Shape(), Standard_True, Standard_False))
		{
			std::cout << "CUT: Result is not valid" << std::endl;
			return;
		}
		else
		{
			// push CUT results in DF as modification of Box1
			TDF_Label Modified = CutLabel.FindChild(2);
			TDF_Label Deleted = CutLabel.FindChild(3);
			TDF_Label Intersections = CutLabel.FindChild(4);
			TDF_Label NewFaces = CutLabel.FindChild(5);

			TopoDS_Shape newS1 = mkCUT.Shape();
			const TopoDS_Shape& ObjSh = mkCUT.Shape1();

			//push in the DF result of CUT
			TNaming_Builder CutBuilder(CutLabel);
			// TNaming_Evolution == MODIFY
			CutBuilder.Modify(ObjSh, newS1);
			myName = "CutLabel, TopoDS_Shape type, parent -> cutBox. Children are faces and such";
			nameAttribute = new TDataStd_AsciiString();
			nameAttribute->Set(myName);
			CutLabel.AddAttribute(nameAttribute);

			//push in the DF modified faces
			View.Clear();
			TNaming_Builder ModBuilder(Modified);
			ShapeExplorer.Init(ObjSh, TopAbs_FACE);
			for (; ShapeExplorer.More(); ShapeExplorer.Next())
			{
				const TopoDS_Shape& Root = ShapeExplorer.Current();
				if (!View.Add(Root)) continue;
				const TopTools_ListOfShape& Shapes = mkCUT.Modified(Root);
				TopTools_ListIteratorOfListOfShape ShapesIterator(Shapes);
				for (; ShapesIterator.More(); ShapesIterator.Next())
				{
					const TopoDS_Shape& newShape = ShapesIterator.Value();
					// TNaming_Evolution == MODIFY
					if (!Root.IsSame(newShape))
						ModBuilder.Modify(Root, newShape);
				}
			}

			//push in the DF deleted faces
			View.Clear();
			TNaming_Builder DelBuilder(Deleted);
			ShapeExplorer.Init(ObjSh, TopAbs_FACE);
			for (; ShapeExplorer.More(); ShapeExplorer.Next())
			{
				const TopoDS_Shape& Root = ShapeExplorer.Current();
				if (!View.Add(Root)) continue;
				// TNaming_Evolution == DELETE
				if (mkCUT.IsDeleted(Root))
					DelBuilder.Delete(Root);
			}

			// push in the DF section edges
			TNaming_Builder IntersBuilder(Intersections);
			Handle(TopOpeBRepBuild_HBuilder) build = mkCUT.Builder();
			TopTools_ListIteratorOfListOfShape its = build->Section();
			for (; its.More(); its.Next())
			{
				// TNaming_Evolution == SELECTED
				IntersBuilder.Select(its.Value(), its.Value());
			}

			// push in the DF new faces added to the object:
			const TopoDS_Shape& ToolSh = mkCUT.Shape2();
			TNaming_Builder newBuilder(NewFaces);
			ShapeExplorer.Init(ToolSh, TopAbs_FACE);
			for (; ShapeExplorer.More(); ShapeExplorer.Next())
			{
				const TopoDS_Shape& F = ShapeExplorer.Current();
				const TopTools_ListOfShape& modified = mkCUT.Modified(F);
				if (!modified.IsEmpty())
				{
					TopTools_ListIteratorOfListOfShape itr(modified);
					for (; itr.More(); itr.Next())
					{
						const TopoDS_Shape& newShape = itr.Value();
						Handle(TNaming_NamedShape) NS = TNaming_Tool::NamedShape(newShape, NewFaces);
						if (NS.IsNull() || NS->Evolution() != TNaming_MODIFY)
						{
							// TNaming_Evolution == GENERATED
							newBuilder.Generated(F, newShape);
						} // if (NS.IsNul())...
					} // for (; itr.More()...
				} // if (!modified.IsEmpty()...
			} //for (; ShapeExplorer.More()...
		} // if (!BRepAlgo::IsValid...{..} else 
	} // if (!mkCUT.IsDone()){..} else
	// end of CUT

	// =================================================
	// 6.Recover result from DF
	// get final result - Box1 shape after CUT operation
	// =================================================
	Handle(TNaming_NamedShape) ResultNS;
	CutLabel.FindAttribute(TNaming_NamedShape::GetID(), ResultNS);
	const TopoDS_Shape& Result_1 = ResultNS->Get(); // here is result of cut operation
	std::cout << "Result_1, i.e. shape in CutLabel" << std::endl;
	ExportTopoShapeAsSTEP(Result_1, Standard_CString("Result_1.step"));
	//printShapeInfo(Result_1);
	ResultNS.Nullify();
	Box1Label.FindAttribute(TNaming_NamedShape::GetID(), ResultNS);
	const TopoDS_Shape& Result_2 = TNaming_Tool::CurrentShape(ResultNS);//here is also result of cut operation
	std::cout << "Result_2, i.e. shape in Box1Label" << std::endl;
	ExportTopoShapeAsSTEP(Result_2, Standard_CString("Result_2.step"));
	//printShapeInfo(Result_2);

	//
	//Result_1 and Result_2 are the same shapes
	//=========================================


	//-------------------------------------------------------------
	// ------- END OF ORIGINAL OPENCASCADE TNAMING EXAMPLE --------
	//-------------------------------------------------------------



	//-------------------------------------------------------------
	// 7. Break two edges such that instead of being one edge, they 
	//    are two. We'll do this on one of the Selected edges and on
	//    one that wasn't selected, and check the difference.
	//-------------------------------------------------------------

	// Move it a bit.
	//gp_Pnt cut_loc = gp_Pnt(25, -5, -5);
	//gp_Dir cut_dir = gp_Dir(0, 0, 1);
	//gp_Ax2 cut_ax2 = gp_Ax2(cut_loc, cut_dir);
	//BRepPrimAPI_MakeBox MKBOX3(cut_ax2, 20., 20., 150);

	//TopoDS_Shape myBox3 = MKBOX3.Shape();
	//std::cout << "Box3" << std::endl;

	//// New nodes on root of the Data Frame
	//TDF_Label Box3Label = aLabel.FindChild(6);
	//// The following is wrong. See comment below.
	//// TDF_Label NewCutBoxLabel = TDF_TagSource::NewChild(aLabel);
	//TDF_Label NewCutBoxLabel = aLabel.FindChild(7);

	// Children nodes, to hold faces. Note the use of TDF_TagSource. You must label ALL children labels using
	// TDF_TagSource, or NONE.
	//TDF_Label bx3TopLbl = TDF_TagSource::NewChild(Box3Label);
	//TDF_Label bx3BotLbl = TDF_TagSource::NewChild(Box3Label);
	//TDF_Label bx3FrtLbl = TDF_TagSource::NewChild(Box3Label);
	//TDF_Label bx3BckLbl = TDF_TagSource::NewChild(Box3Label);
	//TDF_Label bx3LftLbl = TDF_TagSource::NewChild(Box3Label);
	//TDF_Label bx3RgtLbl = TDF_TagSource::NewChild(Box3Label);
	//std::cout << "Tag for bx3TopLbl = " << bx3TopLbl.Tag() << std::endl;
	//std::cout << "Tag for bx3BckLbl = " << bx3BckLbl.Tag() << std::endl;

	//TDF_Label newCutTopLbl = TDF_TagSource::NewChild(NewCutBoxLabel);
	//TDF_Label newCutBotLbl = TDF_TagSource::NewChild(NewCutBoxLabel);
	//TDF_Label newCutFrtLbl = TDF_TagSource::NewChild(NewCutBoxLabel);
	//TDF_Label newCutBckLbl = TDF_TagSource::NewChild(NewCutBoxLabel);
	//TDF_Label newCutLftLbl = TDF_TagSource::NewChild(NewCutBoxLabel);
	//TDF_Label newCutRgtLbl = TDF_TagSource::NewChild(NewCutBoxLabel);

	//TNaming_Builder myBuilder(Box3Label);
	//myBuilder.Generated(MKBOX3.Shape());

	//TNaming_Builder myBuilder1(bx3TopLbl);
	//myBuilder1.Generated(MKBOX3.TopFace());

	//TNaming_Builder myBuilder2(bx3BotLbl);
	//myBuilder2.Generated(MKBOX3.BottomFace());

	//TNaming_Builder myBuilder3(bx3FrtLbl);
	//myBuilder3.Generated(MKBOX3.FrontFace());

	//TNaming_Builder myBuilder4(bx3BckLbl);
	//myBuilder4.Generated(MKBOX3.BackFace());

	//TNaming_Builder myBuilder5(bx3LftLbl);
	//myBuilder5.Generated(MKBOX3.LeftFace());

	//TNaming_Builder myBuilder6(bx3RgtLbl);
	//myBuilder6.Generated(MKBOX3.RightFace());

	//// recover one of the selected edges
	//Handle(TNaming_NamedShape) rcvdEdgeNS;
	//SelectedEdgesLabel.FindChild(4, Standard_False).FindAttribute(TNaming_NamedShape::GetID(), rcvdEdgeNS);
	//std::cout << "Recovered edge data" << std::endl;
	//printShapeInfo(rcvdEdgeNS->Get(), TopAbs_EDGE);
	//std::cout << "shape type = " << rcvdEdgeNS->Get().ShapeType() << std::endl;
	//TopTools_IndexedMapOfShape mapOfShapes;
	//TopExp::MapShapes(rcvdEdgeNS->Get(), TopAbs_EDGE, mapOfShapes);
	//i=1;
	//for (; i<=mapOfShapes.Extent(); i++){
		//TopoDS_Edge curEdge = TopoDS::Edge(mapOfShapes.FindKey(i));
		//std::cout << "     edge in wire ->";
		//printShapeInfo(curEdge, TopAbs_EDGE);
	//}

	////// Using one of the references from the occ demo, cut out box3
	////BRepAlgo_Cut bx3MkCut (Result_2, MKBOX3.Shape());
	//MakeTrackedCut(Result_2, MKBOX3.Shape(), NewCutBoxLabel);
	//// write it out so we can make sure it looks right. it does.
	//BRepTools::Write(bx3MkCut.Shape(), "myBox3.brep");

	//// Do we still get a good reference to the selected edge?
	//std::cout << "Recovered edge data, after cut" << std::endl;
	//printShapeInfo(rcvdEdgeNS->Get(), TopAbs_EDGE);
	//std::cout << "shape type = " << rcvdEdgeNS->Get().ShapeType() << std::endl;
	//mapOfShapes.Clear();
	//TopExp::MapShapes(rcvdEdgeNS->Get(), TopAbs_EDGE, mapOfShapes);
	//i=1;
	//for (; i<=mapOfShapes.Extent(); i++){
		//TopoDS_Edge curEdge = TopoDS::Edge(mapOfShapes.FindKey(i));
		//std::cout << "     edge in wire ->";
		//printShapeInfo(curEdge, TopAbs_EDGE);
	//}

	//Handle(TNaming_NamedShape) rcvdEdgeNS2;
	//SelectedEdgesLabel.FindChild(4, Standard_False).FindAttribute(TNaming_NamedShape::GetID(), rcvdEdgeNS2);
	//std::cout << "re-Recovered edge data, after cut" << std::endl;
	//printShapeInfo(rcvdEdgeNS2->Get(), TopAbs_EDGE);
	//std::cout << "shape type = " << rcvdEdgeNS2->Get().ShapeType() << std::endl;
	//mapOfShapes.Clear();
	//TopExp::MapShapes(rcvdEdgeNS2->Get(), TopAbs_EDGE, mapOfShapes);
	//i=1;
	//for (; i<=mapOfShapes.Extent(); i++){
		//TopoDS_Edge curEdge = TopoDS::Edge(mapOfShapes.FindKey(i));
		//std::cout << "     edge in wire ->";
		//printShapeInfo(curEdge, TopAbs_EDGE);
	//}

	// So, it's still a single edge. Should be two edges, since it got cut. Let's update the Data Framework and see what
	// happens

	//-----------------------------------------------------------
	// Bottom Matter
	//----------------------------------------------------------

	//Handle(TNaming_NamedShape) origBox;
	//Box1Label.FindAttribute(TNaming_NamedShape::GetID(), origBox);
	//std::cout << "About to print out recovered box, I think" << std::endl;
	//printShapeInfo(origBox->Get());
	//std::cout << "About to print out edges from recovered box, I think" << std::endl;
	//printShapeInfo(origBox->Get(), TopAbs_EDGE);

	//Handle(TNaming_NamedShape) filletedBox;
	//FilletLabel.FindAttribute(TNaming_NamedShape::GetID(), filletedBox);
	//std::cout << "About to print out faces from recovered filleted box" << std::endl;
	//printShapeInfo(filletedBox->Get());
	//std::cout << "About to print out edges from recovered filleted box, I think" << std::endl;
	//printShapeInfo(filletedBox->Get(), TopAbs_EDGE);

	//TDF_ChildIterator selectedEdges(SelectedEdgesLabel);
	//BRepFilletAPI_MakeFillet modFillet(origBox->Get());

	// Let's try to change the Fillet radius an a single one of the edges from 5 to 2
	//i=0;
	//for(; selectedEdges.More(); selectedEdges.Next(), i++){
		//Standard_Real radius = 5.;
		//if (i == 2){
			//std::cout << "Changing radius from 5 to 2 for following edge:" << std::endl;
			//radius = 2.;
		//}
		//TDF_Label anEdgeLabel = selectedEdges.Value();
		//Handle(TNaming_NamedShape) anEdgeNamedShape; 
		//anEdgeLabel.FindAttribute(TNaming_NamedShape::GetID(), anEdgeNamedShape);
		//std::cout << "Printing attributes for a 'selected' edge" << std::endl;
		//printShapeInfo(anEdgeNamedShape->Get(), TopAbs_EDGE);
		//modFillet.Add(radius, TopoDS::Edge(anEdgeNamedShape->Get()));
	//}

	// TODO: need to update the Data Framework since we've changed things. New node?
	//modFillet.Build();
	//TopoDS_Shape newFilletedBox = modFillet.Shape();
	//std::cout << "These are the Faces of the new Filleted box" << std::endl;
	//printShapeInfo(newFilletedBox);
	//std::cout << "These are the edges of the new Filleted box" << std::endl;
	//printShapeInfo(newFilletedBox, TopAbs_EDGE);

	// output: it appears to work. The box doesn't have the hole in it though. I guess I have to rebuild the whole
	// solid? That makes sense, actually. And I think FreeCAD already takes care of rebuilding stuff, right? We'll just
	// have to get FreeCAD to use these Labels instead of the Indexes that it's using now.
	//Handle(TNaming_NamedShape) recoveredCutBox;
	//Box2Label.FindAttribute(TNaming_NamedShape::GetID(), recoveredCutBox);
	//std::cout << "Here is the recovered Cut Box. Is it in the correct location?" << std::endl;
	//printShapeInfo(recoveredCutBox->Get());

	// Yup, it's in the correct location!
	//BRepAlgo_Cut mkCut(origBox->Get(), recoveredCutBox->Get());
	//TopoDS_Shape newFilletedBoxWithCut = mkCut.Shape();
	//std::cout << "Here is the final Box with the modified Fillet and the Cut rebuilt" << std::endl;
	//printShapeInfo(newFilletedBoxWithCut);
	//std::cout << "And the edges..." << std::endl;
	//printShapeInfo(newFilletedBoxWithCut, TopAbs_EDGE);

	//std::cout << "Here is a dump of the whole tree" << std::endl;
	TDF_IDFilter myFilter;
	TDF_AttributeIndexedMap myMap;
	myFilter.Keep(TNaming_NamedShape::GetID());
	myFilter.Keep(TDataStd_AsciiString::GetID());
	myFilter.Keep(TNaming_UsedShapes::GetID());
	//aLabel.ExtendedDump(std::cout, myFilter, myMap);
	//aLabel.Dump(std::cout);
	//TDF_Tool::ExtendedDeepDump(std::cout, DF, myFilter);
	TDF_Tool::DeepDump(std::cout, DF);
}

int main()
{
	//TestMkFillet();

	TestResizeBox();

	//runCase3();
	//runCase4();
	return 0;
}
