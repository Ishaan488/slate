cmake_minimum_required(VERSION 3.16)

project(Slate VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Find Qt 6 packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

qt_standard_project_setup()

# Collect sources
set(SOURCES
    src/main.cpp
    src/InfiniteCanvas.cpp
    src/DesktopEmbedder.cpp
    src/Toolbar.cpp
    src/CanvasStore.cpp
    src/items/FreehandLineItem.cpp
    src/items/TextNoteItem.cpp
    src/items/ImageDropItem.cpp
    src/items/ShapeItem.cpp
)

set(HEADERS
    src/InfiniteCanvas.h
    src/DesktopEmbedder.h
    src/Toolbar.h
    src/CanvasStore.h
    src/items/FreehandLineItem.h
    src/items/TextNoteItem.h
    src/items/ImageDropItem.h
    src/items/ShapeItem.h
)

# Build as a Windows GUI app (no console window)
qt_add_executable(Slate WIN32
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(Slate PRIVATE src)

# Link Qt and Windows system libraries
target_link_libraries(Slate PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

# Windows-specific system libraries (works with both MSVC and MinGW)
if(WIN32)
    target_link_libraries(Slate PRIVATE
        dwmapi
        user32
    )
endif()
