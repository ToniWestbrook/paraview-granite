# =========================================================================
#
# Program: Granite Plugin for Paraview
# Module: CMakeLists.txt
# Author: Toni Westbrook
#
# Please see the included README file for full description, 
# build/installation instructions, and known issues.  
# 
# =========================================================================

# --- Ensure C++11 standard is being used for gcc ---
SET(CMAKE_CXX_FLAGS "-std=c++11")

# --- Find and Use ParaView ---
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

# --- Add Plugin ---
ADD_PARAVIEW_PLUGIN(Granite "1.0"
   SERVER_MANAGER_XML Granite.xml
   SERVER_MANAGER_SOURCES vtkGraniteReader.cxx vtkGraniteReaderAMR.cxx vtkGraniteWriter.cxx vtkGraniteSettings.cxx
   SERVER_SOURCES GraniteShared.h GraniteShared.cxx GraniteInterop.h GraniteInterop.cxx GraniteWrapper.h GraniteWrapper.cxx
   REQUIRED_ON_SERVER)

# --- Link to VTK libraries
TARGET_LINK_LIBRARIES(Granite LINK_PUBLIC ${VTK_LIBRARIES} )
   
# --- Find and Use JNI (Java/C Interop) ---
FIND_PACKAGE(JNI)
INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(Granite LINK_PUBLIC ${JNI_LIBRARIES})