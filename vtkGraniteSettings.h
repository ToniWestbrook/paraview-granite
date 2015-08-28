/*=========================================================================

 Program: Granite Plugin for Paraview
 Module: vtkGraniteSettings.h
 Author: Toni Westbrook

 Please see the included README file for full description, 
 build/installation instructions, and known issues.  
 
 =========================================================================*/

#ifndef __vtkGraniteSettings_h
#define __vtkGraniteSettings_h

#include <string>

#include "vtkObject.h"
#include "vtkSmartPointer.h"

class vtkGraniteSettings : public vtkObject {
	public:
		// VTK Setup
		static vtkGraniteSettings* New(); // VTK new with reference count tracking
		static vtkGraniteSettings * GetInstance(); // Obtain singleton instance
		vtkTypeMacro(vtkGraniteSettings, vtkObject); // Type conversion to VTK datatype
		void PrintSelf(ostream& os, vtkIndent indent); // Debug print
		
		// GUI methods (See Granite.xml)
		const char * getGraniteFileName();
		void setGraniteFileName(const char * passName);
		const char * getJavaArguments();
		void setJavaArguments(const char * passArguments);
		int getAMRDivisions();
		void setAMRDivisions(const int passDivisions);

	protected:
		vtkGraniteSettings();
		~vtkGraniteSettings();

	private:
		vtkGraniteSettings(const vtkGraniteSettings&);  // Not implemented per VTK standard
		void operator=(const vtkGraniteSettings&);  // Not implemented per VTK standard

		static vtkSmartPointer< vtkGraniteSettings > _instance; // Singleton instance
		std::string _graniteFileName; // Granite library pathname
		std::string _javaArguments; // Additional arguments for Java VM
		int _amrDivisions; // How many times to divide AMR data into subblocks
};

#endif //__vtkGraniteSettings_h