/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: GraniteWrapper.h
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#ifndef __GraniteWrapper_h
#define __GraniteWrapper_h

#include <jni.h>

class GraniteWrapper {
	public:
		// Supported Granite Classes
		enum ClassDef { DataBlock,
						DataCollection, 
						DataSource,
						MRDataSource,
						ISBounds, 
						RecordDescriptor, 
						ClassCount };

		// Supported Granite Methods
		enum MethodDef { StaticDataSourceCreate,
						 DataSourceActivate,
						 DataSourceDim, 
						 DataSourceSubblock,
						 DataCollectionGetBounds, 
						 DataCollectionGetFloats, 
						 DataCollectionGetNumAttributes, 
						 DataCollectionGetRecordDescriptor,
						 MRDataSourceChangeResolution,
						 MRDataSourceCoarser,
						 MRDataSourceGetNumResolutionLevels,
						 ISBoundsGetLower, 
						 ISBoundsGetUpper, 
						 ISBoundsISBounds,
						 RecordDescriptorName,
						 MethodCount };

		GraniteWrapper();
		~GraniteWrapper();

		// JVM Environment
		JNIEnv * javaEnv;

		// Granite global references
		jclass graniteClasses[ClassDef::ClassCount];
		jmethodID graniteMethods[MethodDef::MethodCount];

	private:
		bool createJVM(); // Create and initialize JVM
		void initClasses(); // Initialize and register all classes
		void initMethods(); // Initialize and register all methods
		void freeClasses(); // Free all class global references
		void freeMethods(); // Free all method global references

		void displayError();

		JavaVM * _javaVM;		
};
#endif // __GraniteWrapper_h
