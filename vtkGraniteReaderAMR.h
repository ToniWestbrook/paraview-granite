/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: vtkGraniteReaderAMR.h
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#ifndef __vtkGraniteReaderAMR_h
#define __vtkGraniteReaderAMR_h

#include "GraniteShared.h"
#include "vtkAMRBaseReader.h"

class vtkGraniteReaderAMR : public vtkAMRBaseReader {
	public:
		static vtkGraniteReaderAMR * New();
		vtkTypeMacro( vtkGraniteReaderAMR, vtkAMRBaseReader );
		void PrintSelf(ostream &os, vtkIndent indent );
		int CanReadFile(const char* filename); // Is file valid XFDL

		// GUI methods (See Granite.xml)
		const char * getFileName();
		void setFileName(const char * passName); 
		void SetFileName(const char * passName); // Irregular caps defined by parent class

	protected:
		vtkGraniteReaderAMR();
		~vtkGraniteReaderAMR();

		// VTK Pipeline methods
		int FillMetaData();
		int GetNumberOfBlocks();
		int GetNumberOfLevels();
		void ReadMetaData();
		int GetBlockLevel( const int blockIdx );
		vtkUniformGrid* GetAMRGrid( const int blockIdx );
		void GetAMRGridData(const int blockIdx, vtkUniformGrid *block, const char *field);
		void SetUpDataArraySelections();

	private:
		vtkGraniteReaderAMR(const vtkGraniteReaderAMR&);  // Not implemented per VTK standard
		void operator=(const vtkGraniteReaderAMR&);  // Not implemented per VTK standard
		
		GraniteShared _graniteInfo;
};

#endif // __vtkGraniteReaderAMR_h