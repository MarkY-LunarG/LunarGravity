
######################################################################################
# Resourcess

file(GLOB RESOURCES
  "${PROJECT_SOURCE_DIR}/resources/*"
)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources)
file(COPY ${RESOURCES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources)

######################################################################################
# Apps

SINGLE_SOURCE_EXECUTABLE(globe_cube)
