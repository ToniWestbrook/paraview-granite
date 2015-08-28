/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: GraniteInterop.h
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#ifndef __GraniteInterop_h
#define __GraniteInterop_h

#include <string>
#include <vector>
#include <jni.h>

#include "vtkDataSetAttributes.h"
#include "GraniteWrapper.h"

class GraniteInterop {
	public:
		GraniteInterop();
		~GraniteInterop();

		bool openDataSource(const char * passFileName, bool passActivate); // Open data source, return success

		// Methods acting on current data source
		void copyFloatData(int * passBounds, vtkDataSetAttributes * retData); // Copy float array from Granite to VTK for bounds specified
		int getAttributeCount(); // Number of attributes
		const char * getAttributeName(int passIdx); // Name of attribute
		int * getBounds(); // Bounding array across 3 dimensions (xLow, xHigh, yLow...)
		int getDimensions(); // Dimensionality of bounds
		
		bool isMultiresolution(); // Is data source multiresolution
		int getLevelCount(); // Number of multiresolution levels
		int getLevel(); // Get current level
		void setLevel(int passLevel); // Set current level

		// JVM related
		const char * getExceptionMessage();
		const char * getClassName(jobject passObject); // Get the class name of a Java object

	private:
		void clearValues(); // Clear all bounds and attribute data
		bool cacheValues(); // Cache Granite Java values into native objects
		bool calculateBounds(); // Calculate all resolution levels of bounds for cacheValues
		void convertBoundArrays(int * passBounds, jintArray * retLow, jintArray * retHigh); // Convert {xLow, xHigh, ...} to existing jintArrays
		

		// Granite data source Java handle
		jobject _jDataSource;

		// Native data
		static GraniteWrapper * _wrapper; // JNI doesn't allow JVM unloading, only initialize GraniteReaderWrapper once

		bool _multiresolution; // Is data multiresolution
		int _currentLevel; // Current number of resolution levels
		std::vector< std::vector< int > > _boundsCache; // Data bounds per level
		int _dimensionsCache; // Dimensionality of data
		std::vector< std::string > _attributeNames; // Component attribute names
};

#endif // __GraniteInterop_h