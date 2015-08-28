#Granite SDB Plugin for ParaView#

OVERVIEW
---------------------------------------------------------------------------

The Granite Plugin for ParaView is designed to provide interoperability
between the ParaView/VTK framework and the UNH Granite Scientific Database
System.  It extends ParaView by providing multi-featured Reader and Writer
modules to accomplish this functionality.  

Example images and videos can be seen at the [author's blog](http://www.toniwestbrook.com/archives/818).

Specific features are as follows:

  1. Reader
    1. Opens standard Granite XFDL files to visualize uniform rectilinear data
    2. Opens ParaView created Granite XFDL files to visualize non-uniform rectilinear data
    3. Opens standard multi-resolution Granite XFDL files to visualize multi-resolution uniform rectilinear data in a streaming overlapping AMR fashion
    4. For single resolution data, allows data extents to be user specified pre-read, to visualize a specific VOI (volume of interest) 
    
  2. Writer
    1. Writes uniform and non-uniform rectilinear data sets (VTK, DICOM, binary, etc) to Granite XFDL/BIN files
    2. Supports resampling output data so data extents match the spatial bounds
    3. Supports writing uniform rectilinear data sets to Granitemulti-resolution XFDL/BIN files and directory structures at a specified number of resolution levels and steps per level
    4. Supports the following custom meta-data tags for increased functionality with ParaView:
      1. CustomParaViewOrigin - Spatial origin on axes (3 doubles)
      2. CustomParaViewSpacing - Spacing/magnification of data against spatial coordinates per axis (3 doubles)
      3. CustomParaViewGrid - Spatial coordinate arrays per axis representing the spacing of each point lattice in a non-uniform rectilinear grid (3 double arrays)
      4. CustomParaViewType - VTK data type to be used for this dataset.  vtkImageData for uniform rectilinear (default if not specified), or vtkRectilinearGrid for non-uniform rectilinear data
      5. "Array.Component" formatting for attribute names.  Since VTK supports the concept of multiple arrays of data, each having its own components, the plugin will emulate importing this information from Granite by parsing dots found in component names into "Array.Component" - e.g. "VectorVel.x", "VectorVel.y", "VectorVel.z", "temperature.amount" would create two arrays, one named "VectorVel" with 3 components "x", "y", and "z", and one array "temperature" with a single component "amount"

  3. General
    1. Directly interfaces with Granite library via JNI, andallows standard command-line arguments to be specified within GUI for the Java VM (memory allocation, debugging, garbage collection, etc)
    2. Allows the number of AMR subdivisions to be specified
    3. Adheres to (mostly) all VTK standards and implementation requirements for maximum compatibility with all filters, mappers, and other ParaView functionality
    4. Supports data sets as large as ParaView and physical memory permits
    5. Successfully tested on all major platforms (Windows, Linux, OSX)

INSTALLATION
---------------------------------------------------------------------------

Software requirements:

   1. ParaView source (verified with 4.2.0)
   2. CMake (verified with 3.0.2)
   3. Qt libraries (verified with 4.8.6 - version 5 has issues with CMake)
   4. Java JDK of matching architecture as C++ build configuration (32/64)
      (verified with 1.7.0.710)
   5. Granite.jar
   6. CMake supported build environment (Make, Visual Studio, Xcode, etc)
   7. Compiler supporting C++11 standard (Preconfigured to set gcc and VC
      for correct standard.  Other compilers may need additional
      switches in Cmakelists.txt)
   8. POSIX compliant OS (Pre-compiler adjusts for Windows libraries)
   
After ensuring all requirements are installed, perform the following steps:

  1. Ensure a system restart has occurred since Qt installation
  2. Copy "Granite" directory from this package in its entirety to the "Plugins" directory in the ParaView source
  3. Execute CMake GUI.  Select ParaView source directory, and a choose a new target build directory (different than source)
  4. Run Configure in CMake. Will likely require location of QMake executable.  Choose from Qt installation, and configure any further desired options.  Run Configure again until no errors are returned. Then run Generate.
  5. Open generated project in build directory using specified IDE/compiler.  Build "ParaView" and "Granite" projects.
  6. Before executing ParaView, ensure the following have been added into your system path:
    1. Qt bin directory (contains Qt dynamic link libraries)
    2. jre/bin/client directory of the JDK installation (contains jvm.dll).  NOTE: This is the biggest area where the plugin can fail - you must be sure to choose the JDK installation that matches the architecture of the build, and you must include the directory containing jvm.dll in your path - due to Java restrictions, you cannot simply copy jvm.dll to the directory containing paraview.exe - the plugin will fail
  7. Run ParaView, then navigate to Tools->Manage Plugins.  Expand "Granite" plugin (if it is not listed, there was an error above, do not manually specify a DLL/XML file). Select "Auto Load" and "Load Selected".  Close Plugin Manager
  8. Navigate to Edit->Settings->Granite Settings, and specify the path and filename of granite.jar.  Quit ParaView.
  9. Installation complete. When running ParaView for normal use, specify the "--enable-streaming" switch for AMR support.
	  
Note: After activating the plugin, if ParaView will not load, displays a "T()" error, or crashes, double check step 6b above.


KNOWN ISSUES
---------------------------------------------------------------------------
  1. The core VTK AMR code currently only supports cell data.  The Granite plugin adjusts for this by loading point data into the cell arrays - however, this leads to blockier visualization.  Partial  compensation for this effect can be achieved by adding a cell-to-point data filter on the output - however, due to interpolation, the quality of multi-resolution rendering is always impacted
  2. Due to relative path limitations in Granite, Linux has issues opening MR datasets that are not in the current working directory
  3. Writing multiresolution datasets that generate very low resolution resolution levels (e.g. too many levels, too large of steps) will not re-open in ParaView
  4. Multiresolution datasets only support uniform rectilinear data, and  a single component
  5. Plugin will read and write all components as floats, regardless of what format they are written as in Granite binary file.  This can lead to larger file sizes if writing from char/short datasets
  6. Plugin will display a "Queue Empty" error message after showing all blocks of the highest resolution in a multiresolution dataset. This error does not impact functionality, and can be ignored
  7. VOI extents UI fields will not automatically update to the extents of the dataset upon opening a new XFDL file - they will read 0.   To overcome this, the Granite plugin will only use VOI extents if one of the fields is updated from 0 to another value.
   
