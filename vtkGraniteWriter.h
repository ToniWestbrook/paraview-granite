/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: vtkGraniteWriter.h
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#ifndef __vtkGraniteWriter_h
#define __vtkGraniteWriter_h

#include "qxmlstream.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

class vtkGraniteWriter : public vtkWriter {
	public:
		// VTK Setup
		static vtkGraniteWriter * New(); // VTK new with reference count tracking
		vtkTypeMacro(vtkGraniteWriter, vtkWriter); // Type conversion to VTK datatype
		void PrintSelf(ostream& retStream, vtkIndent passIndent); // Debug print
		
		// GUI methods (See Granite.xml)
		void setFileBase(const char * passName);
		const char * getFileBase();
		void setResample(bool passResample);
		bool getResample();
		void setMultiresolution(int passCount, int passSteps);

	protected:
		vtkGraniteWriter();
		~vtkGraniteWriter();

		// VTK Pipeline Methods
		int ProcessRequest(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector* retOutput);
		int RequestInformation(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector* retOutput);
		int FillInputPortInformation(int passPort, vtkInformation * passInfo);
		void WriteData();

	private:
		bool checkDataType(vtkInformation * passInput); // Verify if writer supports input data type
		void writeMRData(vtkDataSet * passData); // Write data for a multiresolution source
		void writeXFDL(vtkDataSet * passData, std::string passXFDLName, std::string passBinaryName); // Write XFDL file
		void writeXFDLTypeData(vtkImageData * passData, QXmlStreamWriter * passStream); // Write vtkImageData specific data into XFDL file
		void writeXFDLTypeData(vtkRectilinearGrid * passData, QXmlStreamWriter * passStream); // Write vtkRectilinearGrid specific data into XFDL file
		void writeBinary(vtkDataSet * passData, std::string passBinaryName); // Write binary file

		vtkGraniteWriter(const vtkGraniteWriter&);  // Not implemented per VTK standard
		void operator=(const vtkGraniteWriter&);  // Not implemented per VTK standard

		bool _ready; // Writer properly intialized
		bool _resample; // Resample image data
		int _mrCount, _mrSteps; // Number of multiresolution levels and steps between level
		std::string _filePath; // File path
		std::string _fileBase; // File base
};

#endif // __vtkGraniteWriter_h

