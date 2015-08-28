/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: vtkGraniteSettings.cxx
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#include <cassert>

#include "vtkObjectFactory.h"
#include "vtkGraniteSettings.h"

// VTK Instantiation Macro (Provides NEW definition)
vtkInstantiatorNewMacro(vtkGraniteSettings);

vtkGraniteSettings * vtkGraniteSettings::New() {
  vtkGraniteSettings * instance;
  
  instance = vtkGraniteSettings::GetInstance();
  assert(instance);
  instance->Register(NULL);
  
  return instance;
}

vtkGraniteSettings * vtkGraniteSettings::GetInstance() {
	vtkGraniteSettings * instance;

	if (!_instance) {
		instance = new vtkGraniteSettings();
		vtkObjectFactory::ConstructInstance(instance->GetClassName());
		_instance.TakeReference(instance);
	}
  
	return _instance;
}

void vtkGraniteSettings::PrintSelf(ostream& os, vtkIndent indent) {
  this->Superclass::PrintSelf(os, indent);
}


const char * vtkGraniteSettings::getGraniteFileName() {
	return _graniteFileName.c_str();
}

void vtkGraniteSettings::setGraniteFileName(const char * passName) {
	_graniteFileName = passName;
}

const char * vtkGraniteSettings::getJavaArguments() {
	return _javaArguments.c_str();
}

void vtkGraniteSettings::setJavaArguments(const char * passArguments) {
	_javaArguments = passArguments;
}

int vtkGraniteSettings::getAMRDivisions() {
	return _amrDivisions;
}

void vtkGraniteSettings::setAMRDivisions(const int passDivisions) {
	_amrDivisions = passDivisions;
}

vtkGraniteSettings::vtkGraniteSettings() { 
	_graniteFileName = "";
	_javaArguments = "";
	_amrDivisions = 3;
}

vtkGraniteSettings::~vtkGraniteSettings() { }

vtkSmartPointer< vtkGraniteSettings > vtkGraniteSettings::_instance;
