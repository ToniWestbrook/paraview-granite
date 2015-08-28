/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: GraniteShared.cxx
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#include <memory>

#include "qxml.h"
#include "qxmlstream.h"
#include "GraniteShared.h"
#include "vtkGraniteSettings.h"

GraniteShared::GraniteShared() {
	// Set defaults if not contained in XFDL
	_origin[0] = 0;
	_origin[1] = 0;
	_origin[2] = 0;

	_spacing.resize(1);
	_spacing.back().resize(3, 1);

	_dataType = "vtkImageData";
	_voiOverride = false;
	_grid[0] = vtkFloatArray::New();
	_grid[1] = vtkFloatArray::New();
	_grid[2] = vtkFloatArray::New();
}

GraniteShared::~GraniteShared() {
	_grid[0]->Delete();
	_grid[1]->Delete();
	_grid[2]->Delete();
}

bool GraniteShared::initialize(std::string passFileName) {
	std::string fileName;

	// Set XFDL filename to use
	if (passFileName.empty()) fileName = _fileName;
	else fileName = passFileName;

	// Attempt to open the data source
	if (_interop.openDataSource(fileName.c_str(), true) == false) return false;

	// Ready spacing for multiresolution data sets
	for (int levelIdx = 1 ; levelIdx < _interop.getLevelCount() ; levelIdx++) {
		_spacing.push_back(std::vector< double >());
		_spacing.back().resize(3, 1);
	}

	// Read Paraview specific metadata from XFDL extended by GraniteWriter
	readCustomData(fileName);

	// Calculate spacing relative to root level
	calculateSpacing();

	return true;
}

int GraniteShared::getVolumeSize() {
	int total;
	int * currentBounds;
	
	total = 1;
	currentBounds = (_voiOverride ? _voiBounds : _interop.getBounds());

	// Multiply each dimension
	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		total *= (currentBounds[2 * dimIdx + 1] - currentBounds[2 * dimIdx] + 1);
	}

	return total;
}

int GraniteShared::getVolumeSize(int passBlockID) {
	int total;
	int currentLevel, currentBounds[6];

	// Calculate level and bounds from block ID
	getAMRBlock(passBlockID, &currentLevel, currentBounds);

	// Multiply each dimension
	total = 1;
	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		total *= (currentBounds[2 * dimIdx + 1] - currentBounds[2 * dimIdx] + 1);
	}

	return total;
}

int GraniteShared::getAMRDivisions() {
	return vtkGraniteSettings::GetInstance()->getAMRDivisions();
}

void GraniteShared::getAMRBlock(int passBlockID, int * retLevel, int * retBounds) {
	int location[3]; 
	int length[3];
	int tempStart;

	// Calculate block location
	*retLevel = passBlockID / pow(getAMRDivisions(), 3);
	tempStart = passBlockID - *retLevel * pow(getAMRDivisions(), 3);

	for (int dimIdx = 2 ; dimIdx >= 0 ; dimIdx--) {
		location[dimIdx] = tempStart / pow(getAMRDivisions(), dimIdx);
		tempStart -= location[dimIdx] * pow(getAMRDivisions(), dimIdx);
	}

	// Calculate block length for each dimension
	_interop.setLevel(*retLevel);
	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		length[dimIdx] = (_interop.getBounds()[2 * dimIdx + 1] - _interop.getBounds()[2 * dimIdx] + 1) / getAMRDivisions();
	}

	// Set return bounds
	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		retBounds[2 * dimIdx] = _interop.getBounds()[2 * dimIdx] + location[dimIdx] * length[dimIdx];
		retBounds[2 * dimIdx + 1] = _interop.getBounds()[2 * dimIdx] + (location[dimIdx] + 1) * length[dimIdx];

		//Check for uneven divisions
		if (_interop.getBounds()[2 * dimIdx + 1] - retBounds[2 * dimIdx + 1] < length[dimIdx]) {
			retBounds[2 * dimIdx + 1] = _interop.getBounds()[2 * dimIdx + 1];
		}
	}
}

void GraniteShared::readCustomData(std::string passFileName) {
	QXmlStreamReader::TokenType xmlToken;
	std::auto_ptr<ifstream> fileStream;
	std::string xmlContents;

	// Read XFDL file
	#ifdef _WIN32
		fileStream.reset(new ifstream(passFileName, ios::out | ios::binary));
	#else
		fileStream.reset(new ifstream(passFileName, ios::out));
	#endif

	// Parse XML
	xmlContents.assign((std::istreambuf_iterator<char>(*fileStream)), std::istreambuf_iterator<char>());
	fileStream->close();

	QXmlStreamReader xmlStream(xmlContents.c_str());

	while(!xmlStream.atEnd()) {
		xmlToken = xmlStream.readNext();

		// Check for supported custom element
		if(xmlToken == QXmlStreamReader::StartElement) {
			// CustomParaViewType
			if(xmlStream.name() == "CustomParaViewType") {
				_dataType = xmlStream.readElementText().toStdString();
			}

			// CustomParaViewOrigin
			if(xmlStream.name() == "CustomParaViewOrigin") {
				_origin[0] = xmlStream.attributes().value("x").toString().toDouble();
				_origin[1] = xmlStream.attributes().value("y").toString().toDouble();
				_origin[2] = xmlStream.attributes().value("z").toString().toDouble();
			}

			// CustomParaViewSpacing
			if(xmlStream.name() == "CustomParaViewSpacing") {
				_spacing.back().at(0) = xmlStream.attributes().value("x").toString().toDouble();
				_spacing.back().at(1) = xmlStream.attributes().value("y").toString().toDouble();
				_spacing.back().at(2) = xmlStream.attributes().value("z").toString().toDouble();
			}

			// CustomParaViewGrid
			if(xmlStream.name() == "CustomParaViewGrid") {
				convertQList(xmlStream.attributes().value("x").toString().split(" "), _grid[0]);
				convertQList(xmlStream.attributes().value("y").toString().split(" "), _grid[1]);
				convertQList(xmlStream.attributes().value("z").toString().split(" "), _grid[2]);
			}
		}
	}
}

void GraniteShared::readFieldData(vtkPointData * passData) {
	std::string compositeName, arrayName, componentName;
	vtkDataArray * tempArray, * currentArray;

	// Iterate through all attributes
	for (int attrIdx = 0 ; attrIdx < _interop.getAttributeCount() ; attrIdx++) {
		// Parse array from components names
		compositeName = _interop.getAttributeName(attrIdx);
		if (compositeName.find(".") == std::string::npos) {
			arrayName = "Granite Values";
			componentName = compositeName;
		}
		else {
			arrayName = compositeName.substr(0, compositeName.find("."));
			componentName = compositeName.substr(compositeName.find(".") + 1);
		}

		// Add array if it doesn't exist
		if ((currentArray = passData->GetArray(arrayName.c_str())) == NULL) {
			tempArray = vtkDataArray::CreateDataArray(VTK_FLOAT);
			tempArray->SetName(arrayName.c_str());
			currentArray = passData->GetArray(passData->AddArray(tempArray));
			tempArray->Delete();

			// Make first array default scalars array (regardless of component count for now)
			if (attrIdx == 0) {
				passData->SetActiveScalars(arrayName.c_str());
			}
		}

		// Add component (Arrays start out with one default component - remove after)
		currentArray->SetNumberOfComponents(currentArray->GetNumberOfComponents() + 1);
		currentArray->SetComponentName(currentArray->GetNumberOfComponents() - 2, componentName.c_str());
	}

	// Allocate space for all arrays
	for (int arrayIdx = 0 ; arrayIdx < passData->GetNumberOfArrays() ; arrayIdx++) {
		passData->GetArray(arrayIdx)->SetNumberOfComponents(passData->GetArray(arrayIdx)->GetNumberOfComponents() - 1);
		passData->GetArray(arrayIdx)->SetNumberOfTuples(getVolumeSize());
	}
}

void GraniteShared::calculateSpacing() {
	int * rootBounds;
	double rootLength, childLength;

	// Record root bounds
	_interop.setLevel(_interop.getLevelCount() - 1);
	rootBounds = _interop.getBounds();

	// Calculate each child spacing relative to relation between its data bounds and root's data bounds
	for (int levelIdx = _interop.getLevelCount() - 2 ; levelIdx >= 0 ; levelIdx--) {
		_interop.setLevel(levelIdx);

		for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
			// Child.Space = Root.Space * Root.Length / Child.Length
			rootLength = rootBounds[2 * dimIdx + 1] + 1 - rootBounds[2 * dimIdx];
			childLength = _interop.getBounds()[2 * dimIdx + 1] + 1 - _interop.getBounds()[2 * dimIdx];
			_spacing.at(levelIdx).at(dimIdx) = _spacing.back().at(dimIdx) * rootLength / childLength;
		}
	}
}

void GraniteShared::convertQList(QStringList passList, vtkFloatArray * retArray) {
	// Parse float-formatted strings and add to floatArray
	for (int listIdx = 0 ; listIdx < passList.size() ; listIdx++) {
		retArray->InsertNextValue(passList[listIdx].toDouble());
	}
}