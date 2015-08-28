/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: vtkGraniteReader.h
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#ifndef __vtkGraniteReader_h
#define __vtkGraniteReader_h

#include "GraniteShared.h"
#include "vtkAlgorithm.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

class vtkGraniteReader : public vtkAlgorithm {
	public:
		// VTK Setup
		static vtkGraniteReader * New(); // VTK NEW with reference count tracking
		vtkTypeMacro(vtkGraniteReader,vtkAlgorithm); // Type conversion to VTK datatype
		void PrintSelf(ostream& retStream, vtkIndent passIndent); // Debug print
		int CanReadFile(const char* filename); // Is file valid XFDL

		// GUI methods (See Granite.xml)
		const char * getFileName();
		void setFileName(const char * passName);
		void setVOIBounds(int passXLow, int passXHigh, int passYLow, int passYHigh, int passZLow, int passZHigh);

	protected:
		vtkGraniteReader();

		// VTK Pipeline methods
		int ProcessRequest(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector* retOutput);
		int RequestInformation(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector * retOutput);
		int RequestData(vtkInformation * passRequest, vtkInformationVector ** passInput, vtkInformationVector * retOutput);
		int FillOutputPortInformation(int passPort, vtkInformation * passInfo);

	private:
		vtkGraniteReader(const vtkGraniteReader&);  // Not implemented per VTK standard
		void operator=(const vtkGraniteReader&);  // Not implemented per VTK standard
		
		GraniteShared _graniteInfo;
};

#endif // __vtkGraniteReader_h