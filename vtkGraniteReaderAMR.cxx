/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: vtkGraniteReaderAMR.cxx
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#include <cmath>

#include "vtkGraniteReaderAMR.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkOverlappingAMR.h"
#include "vtkAMRBox.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"

// VTK Instantiation Macro (Provides NEW definition)
vtkStandardNewMacro(vtkGraniteReaderAMR);

void vtkGraniteReaderAMR::PrintSelf(ostream& retStream, vtkIndent passIndent) {
  retStream << passIndent << "File Name: " << (_graniteInfo._fileName != "" ? _graniteInfo._fileName : "(none)") << "\n";
}

int vtkGraniteReaderAMR::CanReadFile(const char * passName) {
	vtkDebugMacro("*** CanReadFile AMR ***");

	// Check validity of file
	if (_graniteInfo.initialize(passName) == false) return 0;

	// Only activate AMR reader for multi resolution data
	if (_graniteInfo._interop.isMultiresolution() == false) return 0;

	// AMR reader currently only supports a single attribute
	if (_graniteInfo._interop.getAttributeCount() > 1) {
		vtkOutputWindowDisplayErrorText("ERROR: Granite AMR reader only supports data sets with a single attribute"); 
		return 0;
	}

	return 1;
}

const char * vtkGraniteReaderAMR::getFileName() {
	return _graniteInfo._fileName.c_str();
}

void vtkGraniteReaderAMR::setFileName(const char * passName) {
	_graniteInfo._fileName = passName;

	this->Modified();
}

void vtkGraniteReaderAMR::SetFileName(const char * passName) {
	setFileName(passName);
}

vtkGraniteReaderAMR::vtkGraniteReaderAMR() {
  this->Initialize();
}

vtkGraniteReaderAMR::~vtkGraniteReaderAMR() { }

int vtkGraniteReaderAMR::FillMetaData() {
	int levelCount, blockLevelCount;
	std::vector< int > blocksLevel;
	int currentLevel, currentBlock, currentBounds[6];
	vtkAMRBox currentBox;

	vtkDebugMacro("*** FillMetaData ***");

	// Load in meta data from Granite
	if (_graniteInfo.initialize() == false) return 0;

	// Set level and block counts
	levelCount = _graniteInfo._interop.getLevelCount();
	blockLevelCount = pow(_graniteInfo.getAMRDivisions(), 3);
	blocksLevel.resize(levelCount, blockLevelCount);

	this->Metadata->Initialize(levelCount, &blocksLevel[0]);
	this->Metadata->SetGridDescription(VTK_XYZ_GRID);
	this->Metadata->SetOrigin(_graniteInfo._origin);

	// Set bounds and resolution per block
	for (int blockIdx = 0 ; blockIdx < levelCount * blockLevelCount  ; blockIdx++) {
		// Calculate level and bounds from block ID
		_graniteInfo.getAMRBlock(blockIdx, &currentLevel, currentBounds);
		
		// Select level
		_graniteInfo._interop.setLevel(currentLevel);

		// Convert bounds from point to cell bounds
		for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
			currentBounds[2 * dimIdx + 1]++;
		}

		// Set bounding box
		currentBox.SetDimensions(currentBounds);

		// Set meta info
		currentBlock = blockIdx - currentLevel * pow(_graniteInfo.getAMRDivisions(), 3);
		this->Metadata->SetSpacing(currentLevel, &_graniteInfo._spacing.at(currentLevel)[0]);
		this->Metadata->SetAMRBox(currentLevel, currentBlock, currentBox);
		this->Metadata->SetAMRBlockSourceIndex(currentLevel, currentBlock, blockIdx);
	}

	SetUpDataArraySelections();

	return 1;
}

vtkUniformGrid * vtkGraniteReaderAMR::GetAMRGrid(const int blockIdx) {
	int currentLevel, currentBounds[6];
	vtkUniformGrid * currentGrid;

	vtkDebugMacro("*** GetAMRGrid ***");

	// Calculate level and bounds from block ID
	_graniteInfo.getAMRBlock(blockIdx, &currentLevel, currentBounds);
		
	// Select level
	_graniteInfo._interop.setLevel(currentLevel);

	// Convert bounds from point to cell bounds
	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		currentBounds[2 * dimIdx + 1]++;
	}

	// Return empty dataset corresponding to block
	currentGrid = vtkUniformGrid::New();
	currentGrid->SetExtent(currentBounds);
	currentGrid->SetOrigin(_graniteInfo._origin);
	currentGrid->SetSpacing(&_graniteInfo._spacing.at(currentLevel)[0]);

	return(currentGrid);
}

int vtkGraniteReaderAMR::GetNumberOfBlocks() { 
	return _graniteInfo._interop.getLevelCount() * pow(_graniteInfo.getAMRDivisions(), 3);
}

int vtkGraniteReaderAMR::GetNumberOfLevels() { 
	return _graniteInfo._interop.getLevelCount(); 
}

void vtkGraniteReaderAMR::ReadMetaData() { }

int vtkGraniteReaderAMR::GetBlockLevel( const int blockIdx ) { 
	int currentLevel, currentBounds[6];

	// Calculate level and bounds from block ID
	_graniteInfo.getAMRBlock(blockIdx, &currentLevel, currentBounds);

	return currentLevel;
}

void vtkGraniteReaderAMR::GetAMRGridData(const int blockIdx, vtkUniformGrid *block, const char *field) {
	int currentLevel, currentBounds[6];
	vtkDoubleArray * dataArray;

	vtkDebugMacro("*** GetAMRGridData ***");

	// Calculate level and bounds from block ID
	_graniteInfo.getAMRBlock(blockIdx, &currentLevel, currentBounds);
	
	// Select level
	_graniteInfo._interop.setLevel(currentLevel);

	// Create data array of appropriate size for current level
	dataArray = vtkDoubleArray::New();
	dataArray->SetName(field);
	dataArray->SetNumberOfComponents(1);
	dataArray->SetNumberOfTuples(_graniteInfo.getVolumeSize(blockIdx));

	block->GetCellData()->AddArray(dataArray);

	// Copy float data
	_graniteInfo._interop.copyFloatData(currentBounds, block->GetCellData());
}

void vtkGraniteReaderAMR::SetUpDataArraySelections() {
	this->CellDataArraySelection->AddArray("Granite Values");
}
