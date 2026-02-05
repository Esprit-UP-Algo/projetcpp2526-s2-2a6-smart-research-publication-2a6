# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\ResearchPublication_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\ResearchPublication_autogen.dir\\ParseCache.txt"
  "ResearchPublication_autogen"
  )
endif()
