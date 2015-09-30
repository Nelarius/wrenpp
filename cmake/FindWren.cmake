# Try to find Wren
# Once done, this will define
# WREN_FOUND - cmake found Wren
# WREN_INCLUDE_DIRS - the folder where wren.h is located
# WREN_LIBRARY - the static library for wren

find_path(WREN_INCLUDE_DIR NAMES wren.h
    HINTS
)

find_library(WREN_LIBRARY NAMES libwren)

if (WREN_INCLUDE_DIR)
    set(WREN_FOUND TRUE)
endif()

if (WREN_FOUND)
    message("Found Wren ${WREN_INCLUDE_DIR}")
endif()
