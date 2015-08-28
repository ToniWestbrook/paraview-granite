/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: GraniteWrapper.cxx
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#include <algorithm>
#include <string>
#include <vector>

// Windows makes use of io.h for POSIX API
#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

#include "GraniteWrapper.h"
#include "vtkGraniteSettings.h"
#include "vtkSetGet.h"

GraniteWrapper::GraniteWrapper() {
	javaEnv = NULL;

	// Create the JVM
	if (createJVM() == false) return;

	// Init global references to supported Granite classes and methods
	initClasses();
	initMethods();
}

GraniteWrapper::~GraniteWrapper() {
	// Free global references to supported Granite classes and methods
	freeClasses();
	freeMethods();
}

bool GraniteWrapper::createJVM() {
	std::string jvmArgString;
	std::vector< std::string > tempArgVector;
	JavaVMInitArgs jvmInitArgs;
	JavaVMOption * jvmOptions;
	int lastPos;

	// Ensure the Granite jar file exists
	if (access(vtkGraniteSettings::GetInstance()->getGraniteFileName(), 0) == -1) {
		displayError();
		return false;
	}

	// Parse Java arguments
	jvmArgString = "-Djava.class.path=";
	jvmArgString += vtkGraniteSettings::GetInstance()->getGraniteFileName();
	std::replace(jvmArgString.begin(), jvmArgString.end(), '\\', '/');

	lastPos = 0;
	jvmArgString += " " + std::string(vtkGraniteSettings::GetInstance()->getJavaArguments());
	if (std::string(vtkGraniteSettings::GetInstance()->getJavaArguments()).length() > 0) jvmArgString += " ";
	
	for (int stringIdx = 0 ; stringIdx < jvmArgString.length() ; stringIdx++) {
		if (jvmArgString[stringIdx] == ' ') {
			tempArgVector.push_back(jvmArgString.substr(lastPos, stringIdx - lastPos));
			lastPos = stringIdx + 1;
		}
	}

	// Transfer arguments to options structure
	jvmOptions = new JavaVMOption[tempArgVector.size()];
	for (int argIdx = 0 ; argIdx < tempArgVector.size() ; argIdx++) {
		jvmOptions[argIdx].optionString = (char *) tempArgVector.at(argIdx).c_str();
	}

	// Set JVM arguments and options	
	jvmInitArgs.version = JNI_VERSION_1_6;
	jvmInitArgs.nOptions = tempArgVector.size();
	jvmInitArgs.options = jvmOptions;
	jvmInitArgs.ignoreUnrecognized = JNI_FALSE;

	// Create JVM
	if (JNI_CreateJavaVM(&_javaVM, (void * *) &javaEnv, &jvmInitArgs) != JNI_OK) {
		delete(jvmOptions);
		displayError();
		return false;
	}

	delete(jvmOptions);

	return true;
}

void GraniteWrapper::initClasses() {
	graniteClasses[ClassDef::DataBlock] = (jclass) javaEnv->NewGlobalRef(javaEnv->FindClass("edu/unh/sdb/datasource/DataBlock")); 
	graniteClasses[ClassDef::DataCollection] = (jclass) javaEnv->NewGlobalRef(javaEnv->FindClass("edu/unh/sdb/datasource/DataCollection")); 
	graniteClasses[ClassDef::DataSource] = (jclass) javaEnv->NewGlobalRef(javaEnv->FindClass("edu/unh/sdb/datasource/DataSource")); 
	graniteClasses[ClassDef::MRDataSource] = (jclass) javaEnv->NewGlobalRef(javaEnv->FindClass("edu/unh/sdb/datasource/MRDataSource")); 
	graniteClasses[ClassDef::ISBounds] = (jclass) javaEnv->NewGlobalRef(javaEnv->FindClass("edu/unh/sdb/datasource/ISBounds")); 
	graniteClasses[ClassDef::RecordDescriptor] = (jclass) javaEnv->NewGlobalRef(javaEnv->FindClass("edu/unh/sdb/common/RecordDescriptor")); 
}

void GraniteWrapper::initMethods() {
	graniteMethods[MethodDef::StaticDataSourceCreate] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetStaticMethodID(graniteClasses[ClassDef::DataSource], "create", "(Ljava/lang/String;Ljava/lang/String;)Ledu/unh/sdb/datasource/DataSource;"));
	graniteMethods[MethodDef::DataSourceActivate] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::DataSource], "activate", "()V"));
	graniteMethods[MethodDef::DataSourceDim] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::DataSource], "dim", "()I"));
	graniteMethods[MethodDef::DataSourceSubblock] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::DataSource], "subblock", "(Ledu/unh/sdb/datasource/ISBounds;)Ledu/unh/sdb/datasource/DataBlock;"));
	graniteMethods[MethodDef::DataCollectionGetBounds] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::DataCollection], "getBounds", "()Ledu/unh/sdb/datasource/ISBounds;"));
	graniteMethods[MethodDef::DataCollectionGetFloats] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::DataCollection], "getFloats", "()[F"));
	graniteMethods[MethodDef::DataCollectionGetNumAttributes] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::DataCollection], "getNumAttributes", "()I"));
	graniteMethods[MethodDef::DataCollectionGetRecordDescriptor] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::DataCollection], "getRecordDescriptor", "()Ledu/unh/sdb/common/RecordDescriptor;"));
	graniteMethods[MethodDef::MRDataSourceChangeResolution] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::MRDataSource], "changeResolution", "(I)Z"));
	graniteMethods[MethodDef::MRDataSourceCoarser] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::MRDataSource], "coarser", "()Z"));
	graniteMethods[MethodDef::MRDataSourceGetNumResolutionLevels] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::MRDataSource], "getNumResolutionLevels", "()I"));
	graniteMethods[MethodDef::ISBoundsGetLower] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::ISBounds], "getLower", "(I)I"));
	graniteMethods[MethodDef::ISBoundsGetUpper] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::ISBounds], "getUpper", "(I)I"));
	graniteMethods[MethodDef::ISBoundsISBounds] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::ISBounds], "<init>", "([I[I)V"));
	graniteMethods[MethodDef::RecordDescriptorName] = (jmethodID) javaEnv->NewGlobalRef((jobject) javaEnv->GetMethodID(graniteClasses[ClassDef::RecordDescriptor], "name", "(I)Ljava/lang/String;"));
}

void GraniteWrapper::freeClasses() {
	for (int classIdx = 0 ; classIdx < ClassDef::ClassCount ; classIdx++) {
		javaEnv->DeleteGlobalRef((jobject) graniteClasses[classIdx]);
	}
}

void GraniteWrapper::freeMethods() {
	for (int methodIdx = 0 ; methodIdx < MethodDef::MethodCount ; methodIdx++) {
		javaEnv->DeleteGlobalRef((jobject) graniteMethods[methodIdx]);
	}
}

void GraniteWrapper::displayError() {
	javaEnv = NULL;
	vtkOutputWindowDisplayErrorText("ERROR: Unable to initialize Java Virtual Machine - please check Granite configuration and ensure jvm.dll in your PATH.\n");
}