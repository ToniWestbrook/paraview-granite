/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: GraniteInterop.cxx
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#include "vtkDataArray.h"
#include "GraniteInterop.h"

// Windows makes use of io.h for POSIX API
#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

GraniteInterop::GraniteInterop() {
	// Per JNI restrictions, the JVM must only be initialized once
	if (_wrapper == NULL) {
		_wrapper = new GraniteWrapper;
	}

	_jDataSource = NULL;

	clearValues();
}

GraniteInterop::~GraniteInterop() {
	// Free data source from JNI
	if (_jDataSource) {
		_wrapper->javaEnv->DeleteGlobalRef(_jDataSource);
	}
}

bool GraniteInterop::openDataSource(const char * passFileName, bool passActivate) {
	jstring jDSName, jFileName;
	jclass currentClass;
	jmethodID currentMethod;

	// Ensure JVM is initialized
	if (_wrapper->javaEnv == NULL) return false;

	// Clear existing exceptions
	_wrapper->javaEnv->ExceptionClear();
	
	// Convert filename
	jDSName = _wrapper->javaEnv->NewStringUTF("ParaViewSource");
	jFileName = _wrapper->javaEnv->NewStringUTF(passFileName);

	// Connect to datasource	
	currentClass = _wrapper->graniteClasses[GraniteWrapper::ClassDef::DataSource];
	currentMethod = _wrapper->graniteMethods[GraniteWrapper::MethodDef::StaticDataSourceCreate];
	_jDataSource = _wrapper->javaEnv->NewGlobalRef(_wrapper->javaEnv->CallStaticObjectMethod(currentClass, currentMethod, jDSName, jFileName));
	if (_wrapper->javaEnv->ExceptionCheck()) return false;

	// Activate
	if (passActivate) {
		currentMethod = _wrapper->graniteMethods[GraniteWrapper::MethodDef::DataSourceActivate];
		_wrapper->javaEnv->CallVoidMethod(_jDataSource, currentMethod);
		if (_wrapper->javaEnv->ExceptionCheck()) return false;

		// Cache commonly used data source values
		if (cacheValues() == false) return false;	
	}

	return true;
}

void GraniteInterop::copyFloatData(int * passBounds, vtkDataSetAttributes * retData) {
	jobject jDataBounds, jBlock;
	jintArray jBoundsLow, jBoundsHigh;
	jfloatArray jGraniteData;
	jfloat * jGraniteDataPtr;
	int sliceStart, sliceEnd, dataSize;
	int currentArray, currentComponent, currentData;

	// Initialize values
	jBoundsLow = _wrapper->javaEnv->NewIntArray(3);
	jBoundsHigh = _wrapper->javaEnv->NewIntArray(3);
	sliceStart = passBounds[4];
	sliceEnd = passBounds[5];
	currentData = 0;

	// Iterate through each slice (allows for reading of large data set)
	for (int sliceIdx = sliceStart ; sliceIdx <= sliceEnd ; sliceIdx++) {
		// Create ISBounds for current slice of requested bounds
		passBounds[4] = sliceIdx;
		passBounds[5] = sliceIdx;
		convertBoundArrays(passBounds, &jBoundsLow, &jBoundsHigh);
		jDataBounds = _wrapper->javaEnv->NewObject(_wrapper->graniteClasses[GraniteWrapper::ClassDef::ISBounds], _wrapper->graniteMethods[GraniteWrapper::MethodDef::ISBoundsISBounds], jBoundsLow, jBoundsHigh);

		// Obtain block for target ISBounds
		jBlock = _wrapper->javaEnv->CallObjectMethod(_jDataSource, _wrapper->graniteMethods[GraniteWrapper::MethodDef::DataSourceSubblock], jDataBounds);

		// Obtain float array from block
		jGraniteData = (jfloatArray) _wrapper->javaEnv->CallObjectMethod(jBlock, _wrapper->graniteMethods[GraniteWrapper::MethodDef::DataCollectionGetFloats]);
		jGraniteDataPtr = _wrapper->javaEnv->GetFloatArrayElements(jGraniteData, NULL);
		if (_wrapper->javaEnv->ExceptionCheck()) return;
		dataSize = _wrapper->javaEnv->GetArrayLength(jGraniteData);
		currentArray = 0;
		currentComponent = 0;

		// Copy floats to native array with array/component accounting
		for (int dataIdx = 0 ; dataIdx < dataSize ; dataIdx++) {
			retData->GetArray(currentArray)->SetComponent(currentData, currentComponent++, jGraniteDataPtr[dataIdx]);
		
			// Reset component/array counting
			if (currentComponent >= retData->GetArray(currentArray)->GetNumberOfComponents()) {
				currentComponent = 0;
				currentArray++;
			}
			if (currentArray >= retData->GetNumberOfArrays()) {
				currentArray = 0;
				currentData++;
			}
		}

		// Release memory from current iteration
		_wrapper->javaEnv->ReleaseFloatArrayElements(jGraniteData, jGraniteDataPtr, 0);
		_wrapper->javaEnv->DeleteLocalRef(jGraniteData);
		_wrapper->javaEnv->DeleteLocalRef(jBlock);
		_wrapper->javaEnv->DeleteLocalRef(jDataBounds);
	}
}

int GraniteInterop::getAttributeCount() {
	return _attributeNames.size();
}

const char * GraniteInterop::getAttributeName(int passIdx) {
	if (passIdx >= _attributeNames.size()) return "";
	return _attributeNames[passIdx].c_str();
}

int * GraniteInterop::getBounds() {
	return &_boundsCache[_currentLevel][0];
}

int GraniteInterop::getDimensions() {
	return _dimensionsCache;
}

bool GraniteInterop::isMultiresolution() {
	return _multiresolution;
}

int GraniteInterop::getLevelCount() {
	return _boundsCache.size();
}

int GraniteInterop::getLevel() {
	// Granite ordering is inverse to VTKs
	return _boundsCache.size() - 1 - _currentLevel;
}

void GraniteInterop::setLevel(int passLevel) {
	jmethodID jMethodResolution;

	jMethodResolution = _wrapper->graniteMethods[GraniteWrapper::MethodDef::MRDataSourceChangeResolution];

	// Ensure valid level
	if (passLevel >= _boundsCache.size()) return;

	// Granite ordering is inverse to VTKs
	_currentLevel = _boundsCache.size() -  1 - passLevel;

	// Set level in Granite
	_wrapper->javaEnv->CallBooleanMethod(_jDataSource, jMethodResolution, 0);
	_wrapper->javaEnv->CallBooleanMethod(_jDataSource, jMethodResolution, _currentLevel);
}

const char * GraniteInterop::getExceptionMessage() {
	jmethodID methodToString;
	jstring jExceptionString;

	// If Java exception exists, get associated message
	if (_wrapper->javaEnv->ExceptionCheck()) {
		methodToString = _wrapper->javaEnv->GetMethodID(_wrapper->javaEnv->FindClass("java/lang/Object"), "toString", "()Ljava/lang/String;");
		jExceptionString = (jstring) _wrapper->javaEnv->CallObjectMethod(_wrapper->javaEnv->ExceptionOccurred(), methodToString);

		return _wrapper->javaEnv->GetStringUTFChars(jExceptionString, NULL);
	}

	return "";
}

const char * GraniteInterop::getClassName(jobject passObject) {
	jobject jClassObject;
	jclass jObjectClass, jClassClass;
	jmethodID jClassMethod, jNameMethod;
	jstring jNameString;

	// Obtain java class object for target object
	jObjectClass = _wrapper->javaEnv->GetObjectClass(passObject);
	jClassMethod = _wrapper->javaEnv->GetMethodID(jObjectClass, "getClass", "()Ljava/lang/Class;");
	jClassObject = _wrapper->javaEnv->CallObjectMethod(passObject, jClassMethod);

	// Call getName on class
	jClassClass = _wrapper->javaEnv->GetObjectClass(jClassObject);
	jNameMethod = _wrapper->javaEnv->GetMethodID(jClassClass, "getName", "()Ljava/lang/String;");
	jNameString = (jstring) _wrapper->javaEnv->CallObjectMethod(jClassObject, jNameMethod);

	// Return name
	return _wrapper->javaEnv->GetStringUTFChars(jNameString, NULL);
}


void GraniteInterop::clearValues() {
	// Initial level info
	_multiresolution = false;
	_currentLevel = 0;

	// Bounds info
	_dimensionsCache = 0;

	_boundsCache.clear();
	_boundsCache.push_back(std::vector< int >());
	_boundsCache.back().resize(6, 0);

	// Attribute info
	_attributeNames.clear();
}

bool GraniteInterop::cacheValues() {
	jobject jRecordDescriptor;
	jmethodID jMethodDim, jMethodAttributes, jMethodDescriptor, jMethodName;
	jstring jTempString;
	int attributeCount;

	// Get method references
	jMethodDim = _wrapper->graniteMethods[GraniteWrapper::MethodDef::DataSourceDim];
	jMethodAttributes = _wrapper->graniteMethods[GraniteWrapper::MethodDef::DataCollectionGetNumAttributes];
	jMethodDescriptor = _wrapper->graniteMethods[GraniteWrapper::MethodDef::DataCollectionGetRecordDescriptor];
	jMethodName = _wrapper->graniteMethods[GraniteWrapper::MethodDef::RecordDescriptorName];

	// Initialize values
	clearValues();
	if (_wrapper->javaEnv->ExceptionCheck()) return false;
	
	// Cache if multiresolution
	_multiresolution = (std::string(getClassName(_jDataSource)).compare("edu.unh.sdb.datasource.MRDataSource") == 0);

	// Cache dimensionality and bounds
	_dimensionsCache = _wrapper->javaEnv->CallIntMethod(_jDataSource, jMethodDim);
	if (calculateBounds() == false) return false;

	// Cache attribute names
	attributeCount = _wrapper->javaEnv->CallIntMethod(_jDataSource, jMethodAttributes);
	jRecordDescriptor = _wrapper->javaEnv->CallObjectMethod(_jDataSource, jMethodDescriptor);
	if (_wrapper->javaEnv->ExceptionCheck()) return false;
	
	for (int attrIdx = 0 ; attrIdx < attributeCount ; attrIdx++) {
		jTempString = (jstring) _wrapper->javaEnv->CallObjectMethod(jRecordDescriptor, jMethodName, attrIdx);
		_attributeNames.push_back(_wrapper->javaEnv->GetStringUTFChars(jTempString, NULL));
	}

	if (_wrapper->javaEnv->ExceptionCheck()) return false;

	return true;
}

bool GraniteInterop::calculateBounds() {
	jobject jDataBounds;
	jmethodID jMethodGetBounds, jMethodLower, jMethodUpper, jMethodCoarser;

	jMethodLower = _wrapper->graniteMethods[GraniteWrapper::MethodDef::ISBoundsGetLower];
	jMethodUpper = _wrapper->graniteMethods[GraniteWrapper::MethodDef::ISBoundsGetUpper];
	jMethodGetBounds = _wrapper->graniteMethods[GraniteWrapper::MethodDef::DataCollectionGetBounds];
	jMethodCoarser = _wrapper->graniteMethods[GraniteWrapper::MethodDef::MRDataSourceCoarser];

	// Iterate through all resolution levels
	do {
		// Get bounds for current level
		jDataBounds = _wrapper->javaEnv->CallObjectMethod(_jDataSource, jMethodGetBounds);

		// Cache bounds for this level
		for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
			_boundsCache.back().at(2 * dimIdx) = _wrapper->javaEnv->CallIntMethod(jDataBounds, jMethodLower, 2 - dimIdx);
			_boundsCache.back().at(2 * dimIdx + 1) = _wrapper->javaEnv->CallIntMethod(jDataBounds, jMethodUpper, 2 - dimIdx);
		}

		if (_wrapper->javaEnv->ExceptionCheck()) return false;

		// Prepare next level
		_boundsCache.push_back(std::vector< int >());
		_boundsCache.back().resize(6, 0);

		// Exit if single resolution set
		if (_multiresolution == false) break;

	// Move to next level
	} while (_wrapper->javaEnv->CallBooleanMethod(_jDataSource, jMethodCoarser));

	_boundsCache.pop_back();

	return true;
}

void GraniteInterop::convertBoundArrays(int * passBounds, jintArray * retLow, jintArray * retHigh) {
	jint jLow[3], jHigh[3];

	// Load values into jint native arrays
	for (int dimIdx = 0 ; dimIdx < 3 ; dimIdx++) {
		jLow[2 - dimIdx] = passBounds[2 * dimIdx];
		jHigh[2 - dimIdx] = passBounds[2 * dimIdx + 1];
	}

	// Convert jint native arrays to jintArrays
	_wrapper->javaEnv->SetIntArrayRegion(*retLow, 0, 3, jLow);
	_wrapper->javaEnv->SetIntArrayRegion(*retHigh, 0, 3, jHigh);
}

GraniteWrapper * GraniteInterop::_wrapper; 