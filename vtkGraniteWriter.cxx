/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: vtkGraniteWriter.cxx
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#include <string>
#include <stdio.h>
#include <memory>
#include <sys/stat.h>

#ifdef _WIN32
	#include <direct.h>
#endif

#include "qstring.h"
#include "qxml.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkImageResample.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkGraniteWriter.h"

// VTK Instantiation Macro (Provides NEW definition)
vtkStandardNewMacro(vtkGraniteWriter);

void vtkGraniteWriter::PrintSelf(ostream& retStream, vtkIndent passIndent) {
  //Superclass::PrintSelf(retStream, passIndent);

  retStream << passIndent << "File Base: " << (!_fileBase.empty() ? _fileBase : "(none)") << "\n";
}

void vtkGraniteWriter::setFileBase(const char * passName) {
	int pathPos;
	std::string fullPath;

	fullPath = passName;

	if (fullPath.length() < 5) return;

	// Remove extension from name
	pathPos = fullPath.find_last_of("/\\");
	if (pathPos == std::string::npos) {
		pathPos = 0;
	}
	else {
		pathPos++;
		_filePath = fullPath.substr(0, pathPos);
	}

	_fileBase = fullPath.substr(pathPos);
	_fileBase = _fileBase.substr(0, _fileBase.length() - 5);
}

const char * vtkGraniteWriter::getFileBase() {
	return (_filePath + _fileBase + ".xfdl").c_str();
}

void vtkGraniteWriter::setResample(bool passResample) {
	_resample = passResample;
}

bool vtkGraniteWriter::getResample() {
	return _resample;
}

void vtkGraniteWriter::setMultiresolution(int passCount, int passSteps) {
	_mrCount = passCount;
	_mrSteps = passSteps;
}

int vtkGraniteWriter::ProcessRequest(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector * retOutput) {
	// Information request
	if(passRequest->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION())) {
		return this->RequestInformation(passRequest, passInput, retOutput);
	}

	return this->Superclass::ProcessRequest(passRequest, passInput, retOutput);
}

int vtkGraniteWriter::RequestInformation(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector* retOutput) {
	// Check for supported input data type, and set output accordingly
	if (checkDataType(passInput[0]->GetInformationObject(0)) == false) return false;

	// Set to initialized
	_ready = true;

	return 1;
}

int vtkGraniteWriter::FillInputPortInformation(int passPort, vtkInformation * passInfo) {
	// Pipeline will accept any datatype, but Write() will reject unsupported types
	passInfo->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");

	return 1;
}

void vtkGraniteWriter::WriteData() {
	vtkDataSet * inputData;
	vtkImageResample * resampleData;

	// Stop if not intialized
	if (!_ready) return;

    inputData = vtkDataSet::SafeDownCast(this->GetInput());

	// Resample image if requested or writing multiresolution
	resampleData = NULL;
	if (inputData->IsA("vtkImageData") && (_resample || _mrCount > 1)) {
		resampleData = vtkImageResample::New();
		resampleData->AddInputData(inputData);

		resampleData->SetAxisMagnificationFactor(0, ((vtkImageData *) inputData)->GetSpacing()[0]);
		resampleData->SetAxisMagnificationFactor(1, ((vtkImageData *) inputData)->GetSpacing()[1]);
		resampleData->SetAxisMagnificationFactor(2, ((vtkImageData *) inputData)->GetSpacing()[2]);
		resampleData->Update();

		inputData = resampleData->GetOutput(0);
	}

	if (_mrCount == 1) {
		// Single resolution - simply write XFDL header and data
		writeXFDL(inputData, _filePath + _fileBase + ".xfdl", _fileBase + ".bin");
		writeBinary(inputData, _filePath + _fileBase + ".bin");
	}
	else {
		// Multiresolution
		writeMRData(inputData);
	}

	if (resampleData) resampleData->Delete();
}
vtkGraniteWriter::vtkGraniteWriter()
{
	_ready = false;
	_filePath = "";
	_fileBase = "";
	_resample = true;
}

vtkGraniteWriter::~vtkGraniteWriter() { }

bool vtkGraniteWriter::checkDataType(vtkInformation * passInput) {
	vtkDataSet * inputData;

	inputData = vtkDataSet::SafeDownCast(passInput->Get(vtkDataObject::DATA_OBJECT()));

	if (inputData != NULL) {
		// Uniform Rectilinear Data
		if (inputData->IsA("vtkImageData")) return true;

		// Non-Uniform Rectilinear Data
		if (inputData->IsA("vtkRectilinearGrid")) {
			if (_mrCount > 1) {
				vtkOutputWindowDisplayErrorText("ERROR: Can only write multiresolution from vtkImageData type.");
				return false;
			}

			return true;
		}
	}

	vtkOutputWindowDisplayErrorText("ERROR: Granite plugin only supports writing from vtkImageData and vtkRectilinearGrid types.");

	return false;
}

void vtkGraniteWriter::writeMRData(vtkDataSet * passData) {
	vtkImageResample * resampleData;
	std::string currentDirectory;
	std::string mrPostfix;

	// Write starting XFDL
	writeXFDL(passData, _filePath + _fileBase + ".xfdl", "@" + _fileBase + "/" + _fileBase + ".bin");
	currentDirectory = _fileBase + "/";

	// Iterate through each resolution level
	for (int levelIdx = 0 ; levelIdx < _mrCount ; levelIdx++) {
		// Create directory for current resolution
		#ifdef _WIN32
			mkdir((_filePath + currentDirectory).c_str());
		#else
			mkdir((_filePath + currentDirectory).c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
		#endif

		if (levelIdx == 0) {
			// For root level, create header and footer with name of datasource
			writeXFDL(passData, _filePath + currentDirectory + _fileBase + ".xfdl", _fileBase + ".bin");
			writeBinary(passData, _filePath + currentDirectory + _fileBase + ".bin");
		}
		else {
			// For child resolutions, resample
			resampleData = vtkImageResample::New();
			resampleData->AddInputData(passData);
			resampleData->SetAxisMagnificationFactor(0, 1.0 / (double) pow(_mrSteps, levelIdx));
			resampleData->SetAxisMagnificationFactor(1, 1.0 / (double) pow(_mrSteps, levelIdx));
			resampleData->SetAxisMagnificationFactor(2, 1.0 / (double) pow(_mrSteps, levelIdx));
			resampleData->Update();
			
			// Write two headers and binary
			mrPostfix = ".d" + std::to_string(levelIdx);
			writeXFDL(resampleData->GetOutput(0), _filePath + currentDirectory + _fileBase + ".bin" + mrPostfix + ".fdl", _fileBase + ".bin" + mrPostfix);
			writeXFDL(resampleData->GetOutput(0), _filePath + currentDirectory + "data.fdl", _fileBase + ".bin" + mrPostfix);
			writeBinary(resampleData->GetOutput(0), _filePath + currentDirectory + _fileBase + ".bin" + mrPostfix);

			resampleData->Delete();
		}

		// Add (potential) next iteration to path
		currentDirectory += "level" + std::to_string(levelIdx + 1) + "/";
	}
}

void vtkGraniteWriter::writeXFDL(vtkDataSet * passData, std::string passXFDLName, std::string passBinaryName) {
	vtkDataArray * currentArray;
	std::string arrayName, compName;
	QString xmlString;	
	QXmlStreamWriter xmlStream(&xmlString);
	std::auto_ptr<ofstream> fileStream;

	xmlStream.setAutoFormatting(true);
	
	// Header information
	xmlStream.writeStartDocument();
	xmlStream.writeDTD("<!DOCTYPE FileDescriptor PUBLIC \"-//SDB//DTD//EN\"  \"fdl.dtd\">");
	xmlStream.writeStartElement("FileDescriptor");
	xmlStream.writeAttribute("fileName", passBinaryName.c_str());
	xmlStream.writeAttribute("fileType", "binary");

	// Fields
	for (int arrayIdx = 0 ; arrayIdx < passData->GetPointData()->GetNumberOfArrays() ; arrayIdx++) {
		currentArray = passData->GetPointData()->GetArray(arrayIdx);

		for (int compIdx = 0 ; compIdx < currentArray->GetNumberOfComponents() ; compIdx++) {
			// Compose field name (array.componenet)
			arrayName = (currentArray->GetName() != NULL ? currentArray->GetName() : std::string("array") + std::to_string(arrayIdx));
			compName = (currentArray->GetComponentName(compIdx) != NULL ? currentArray->GetComponentName(compIdx) : std::string("comp") + std::to_string(compIdx));

			// Write field
			xmlStream.writeStartElement("Field");
			xmlStream.writeAttribute("fieldName", (arrayName + "." + compName).c_str());
			//xmlStream.writeAttribute("fieldType", currentArray->GetDataTypeAsString());
			xmlStream.writeAttribute("fieldType", "float");
			xmlStream.writeEndElement();
		}
	}
	
	// Custom - ParaView type
	xmlStream.writeTextElement("CustomParaViewType", passData->GetClassName());

	// Fields specific to Uniform Rectilinear Data
	if (passData->IsA("vtkImageData")) {
		writeXFDLTypeData((vtkImageData *) passData, &xmlStream);
	}

	// Fields specific to Non-Uniform Rectilinear Data
	if (passData->IsA("vtkRectilinearGrid")) {
		writeXFDLTypeData((vtkRectilinearGrid *) passData, &xmlStream);
	}
	
	xmlStream.writeEndElement();
	xmlStream.writeEndDocument();
	
	// Create XFDL file
	#ifdef _WIN32
		fileStream.reset(new ofstream(passXFDLName.c_str(), ios::out | ios::binary));
	#else
		fileStream.reset(new ofstream(passXFDLName.c_str(), ios::out));
	#endif

	// Write header info
	*fileStream << xmlString.toStdString();
	fileStream->close();
}

void vtkGraniteWriter::writeXFDLTypeData(vtkImageData * passData, QXmlStreamWriter * passStream) {
	// Bounds
	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		passStream->writeStartElement("Bounds");
		passStream->writeAttribute("lower", std::to_string((int) passData->GetExtent()[4 - 2 * dimIdx]).c_str());
		passStream->writeAttribute("upper", std::to_string((int) passData->GetExtent()[4 - 2 * dimIdx + 1]).c_str());
		passStream->writeEndElement();
	}

	// Custom - ParaView origin
	passStream->writeStartElement("CustomParaViewOrigin");
	passStream->writeAttribute("x", std::to_string((float) passData->GetOrigin()[0]).c_str());
	passStream->writeAttribute("y", std::to_string((float) passData->GetOrigin()[1]).c_str());
	passStream->writeAttribute("z", std::to_string((float) passData->GetOrigin()[2]).c_str());
	passStream->writeEndElement();

	// Custom - ParaView spacing
	passStream->writeStartElement("CustomParaViewSpacing");
	passStream->writeAttribute("x", std::to_string((float) passData->GetSpacing()[0]).c_str());
	passStream->writeAttribute("y", std::to_string((float) passData->GetSpacing()[1]).c_str());
	passStream->writeAttribute("z", std::to_string((float) passData->GetSpacing()[2]).c_str());
	passStream->writeEndElement();
}

void vtkGraniteWriter::writeXFDLTypeData(vtkRectilinearGrid * passData, QXmlStreamWriter * passStream) {
	vtkDataArray * gridArrays[3];
	std::string gridString[3];

	// Bounds
	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		passStream->writeStartElement("Bounds");
		passStream->writeAttribute("lower", std::to_string((int) passData->GetExtent()[4 - 2 * dimIdx]).c_str());
		passStream->writeAttribute("upper", std::to_string((int) passData->GetExtent()[4 - 2 * dimIdx + 1]).c_str());
		passStream->writeEndElement();
	}

	// Custom - ParaView grid spacing
	gridArrays[0] = passData->GetXCoordinates();
	gridArrays[1] = passData->GetYCoordinates();
	gridArrays[2] = passData->GetZCoordinates();

	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		gridString[dimIdx] = "";

		for (int coordIdx = 0 ; coordIdx < gridArrays[dimIdx]->GetSize() ; coordIdx++) {
			gridString[dimIdx] += std::to_string(gridArrays[dimIdx]->GetTuple1(coordIdx)) + " ";
		}
	}

	passStream->writeStartElement("CustomParaViewGrid");
	passStream->writeAttribute("x", gridString[0].substr(0, gridString[0].length() - 1).c_str());
	passStream->writeAttribute("y", gridString[1].substr(0, gridString[1].length() - 1).c_str());
	passStream->writeAttribute("z", gridString[2].substr(0, gridString[2].length() - 1).c_str());
	passStream->writeEndElement();
}


void vtkGraniteWriter::writeBinary(vtkDataSet * passData, std::string passBinaryName) {
	std::auto_ptr<ofstream> fileStream;
	char currentVal[sizeof(float)];

	// Create Binary file
	#ifdef _WIN32
		fileStream.reset(new ofstream(passBinaryName.c_str(), ios::out | ios::binary));
	#else
		fileStream.reset(new ofstream(passBinaryName.c_str(), ios::out));
	#endif
		
	for (int dataIdx = 0 ; dataIdx < passData->GetPointData()->GetArray(0)->GetSize() ; dataIdx++) {
		for (int arrayIdx = 0 ; arrayIdx < passData->GetPointData()->GetNumberOfArrays() ; arrayIdx++) {
			for (int compIdx = 0 ; compIdx < passData->GetPointData()->GetArray(arrayIdx)->GetNumberOfComponents() ; compIdx++) {
				*((float *) currentVal) = passData->GetPointData()->GetArray(arrayIdx)->GetComponent(dataIdx, compIdx);
				for (int valueIdx = sizeof(float) - 1 ; valueIdx >= 0 ; valueIdx--) {
					fileStream->put(currentVal[valueIdx]);
				}
			}
		}
	}

	fileStream->close();
}
