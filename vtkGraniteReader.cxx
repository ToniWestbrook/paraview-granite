/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: vtkGraniteReader.cxx
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#include <stdio.h>
#include <string>
#include <memory>

#include "vtkGraniteReader.h"
#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

// VTK Instantiation Macro (Provides NEW definition)
vtkStandardNewMacro(vtkGraniteReader);

void vtkGraniteReader::PrintSelf(ostream& retStream, vtkIndent passIndent) {
  Superclass::PrintSelf(retStream, passIndent);

  retStream << passIndent << "File Name: " << (_graniteInfo._fileName != "" ? _graniteInfo._fileName : "(none)") << "\n";
}

int vtkGraniteReader::CanReadFile(const char * passName) {
	vtkDebugMacro("*** CanReadFile Standard ***");
	
	// Check validity of file
	if (_graniteInfo.initialize(passName) == false) return 0;

	// Only activate standard reader for single resolution data
	if (_graniteInfo._interop.isMultiresolution()) return 0;

	return 1;
}

const char * vtkGraniteReader::getFileName() {
	return _graniteInfo._fileName.c_str();
}

void vtkGraniteReader::setFileName(const char * passName) {
	// When opening new file, disable VOI override (for now)
	_graniteInfo._fileName = passName;
	_graniteInfo._voiOverride = false;

	this->Modified();
}

void vtkGraniteReader::setVOIBounds(int passXLow, int passXHigh, int passYLow, int passYHigh, int passZLow, int passZHigh) {
	// Set volume of interest and enable
	_graniteInfo._voiBounds[0] = passXLow;
	_graniteInfo._voiBounds[1] = passXHigh;
	_graniteInfo._voiBounds[2] = passYLow;
	_graniteInfo._voiBounds[3] = passYHigh;
	_graniteInfo._voiBounds[4] = passZLow;
	_graniteInfo._voiBounds[5] = passZHigh;
	
	_graniteInfo._voiOverride = true;
	
	this->Modified();
}

vtkGraniteReader::vtkGraniteReader() {
	// Reader requires no input, provides 1 output
	this->SetNumberOfInputPorts(0);
	this->SetNumberOfOutputPorts(1);
}

int vtkGraniteReader::ProcessRequest(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector * retOutput) {
	vtkDebugMacro("*** ProcessRequest ***");

	// Information request
	if(passRequest->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION())) {
		return this->RequestInformation(passRequest, passInput, retOutput);
	}

	// Data request
	if(passRequest->Has(vtkDemandDrivenPipeline::REQUEST_DATA())) {
		return this->RequestData(passRequest, passInput, retOutput);
	}

	return this->Superclass::ProcessRequest(passRequest, passInput, retOutput);
}


int vtkGraniteReader::RequestInformation(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector * retOutput) {
	vtkInformation * outputInfo;
	vtkDataSet * outputData;
	int * dataExtent;

	vtkDebugMacro("*** RequestInformation ***");

	// Obtain output information and data
	outputInfo = retOutput->GetInformationObject(0);
	outputData = vtkDataSet::SafeDownCast(outputInfo->Get(vtkDataObject::DATA_OBJECT()));
	
	// Set to VOI if specified, else full data extents
	if (_graniteInfo._voiOverride) {
		outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), _graniteInfo._voiBounds, 6);
	}
	else {
		dataExtent = _graniteInfo._interop.getBounds();
		outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), dataExtent, 6);
		setVOIBounds(dataExtent[0], dataExtent[1], dataExtent[2], dataExtent[3], dataExtent[4], dataExtent[5]);
	}
	
	// Set common attributes
	outputInfo->Set(vtkDataObject::ORIGIN(), _graniteInfo._origin, 3);
	outputInfo->Set(vtkDataObject::SPACING(), &_graniteInfo._spacing.back()[0], 3);

	return 1;	
}

int vtkGraniteReader::RequestData(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector * retOutput) {
	vtkInformation * outputInfo;
	vtkDataSet * outputData;
	vtkPointData * pointData;
	vtkDataArray * dataArray;
	int dataExtent[6];
	
	vtkDebugMacro("*** RequestData ***");

	// Obtain output information and data
	outputInfo = retOutput->GetInformationObject(0);
	outputData = vtkDataSet::SafeDownCast(outputInfo->Get(vtkDataObject::DATA_OBJECT()));
	pointData = outputData->GetPointData();


	// Set extents and spacing
	outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), dataExtent);	
	if (outputData->IsA("vtkImageData")) ((vtkImageData *) outputData)->SetExtent(dataExtent);
	if (outputData->IsA("vtkRectilinearGrid")) ((vtkRectilinearGrid *) outputData)->SetExtent(dataExtent);

	// Allocate arrays and components
	_graniteInfo.readFieldData(pointData);

	// Set grid spacing for vtkRectilinearGrid
	if (outputData->IsA("vtkRectilinearGrid")) {
		((vtkRectilinearGrid *) outputData)->SetXCoordinates(_graniteInfo._grid[0]);
		((vtkRectilinearGrid *) outputData)->SetYCoordinates(_graniteInfo._grid[1]);
		((vtkRectilinearGrid *) outputData)->SetZCoordinates(_graniteInfo._grid[2]);
	}

	// Copy data from Granite for specified extents
	_graniteInfo._interop.copyFloatData(dataExtent, pointData);

	return 1;
}

int vtkGraniteReader::FillOutputPortInformation(int passPort, vtkInformation * passInfo) {
	// Open data source and read ParaView specific metadata
	if (_graniteInfo.initialize() == false) return 0;

	// Set specified data type
	passInfo->Set(vtkDataObject::DATA_TYPE_NAME(), _graniteInfo._dataType.c_str());

	return 1;
}