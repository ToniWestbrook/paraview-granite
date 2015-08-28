/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: GraniteShared.h
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#ifndef __GraniteShared_h
#define __GraniteShared_h

#include <string>

#include "GraniteInterop.h"
#include "qstringlist.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"

class vtkGraniteReader;
class vtkGraniteReaderAMR;

class GraniteShared {
	// Allow trusted readers to access private common information without overhead
	friend class vtkGraniteReader;
	friend class vtkGraniteReaderAMR;

	public:
		GraniteShared();
		~GraniteShared();

		bool initialize(std::string passFileName = "");
		int getVolumeSize(); // Return number of tuples for active bounds (either total or enabled VOI)
		int getVolumeSize(int passBlockID); // Return number of tuples for the specified AMR block ID
		int getAMRDivisions(); // Return number of AMR divisions from settings menu
		void getAMRBlock(int passBlockID, int * retLevel, int * retBounds); // Calculate level and bounds for the specified block ID

	private:
		void readCustomData(std::string passFileName); // Read custom ParaView XML data
		void readFieldData(vtkPointData * passData); // Read field data from Granite	
		void calculateSpacing(); // Calculate multiresolution spacing
		void convertQList(QStringList passList, vtkFloatArray * retArray); // Convert QStringList of strings to double array

		// Data type specific
		std::vector< std::vector< double > > _spacing; // vtkImageData/vtkOverlappingAMR spacing per resolution
		vtkFloatArray * _grid[3]; // vtkRectilinearGrid spacing

		// Common
		std::string _fileName; // XFDL filename
		std::string _dataType; // Data set type
		double _origin[3]; // Axes origin
		bool _voiOverride; // Whether to use (or set) Volume of Interest
		int _voiBounds[6]; // Volume of Interest bounds
		GraniteInterop _interop; // Interoperability with Granite java lib
};

#endif // __GraniteShared_h