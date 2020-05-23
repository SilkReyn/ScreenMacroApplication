# Kind of generated project
TEMPLATE = vcapp

# Sources
HEADERS = Source/CScreenMacroMainWindow.h
SOURCES = Source/CScreenMacroMainWindow.cpp \
  Source/main.cpp
FORMS = Source/ScreenMacroForm.ui
INCLUDEPATH += $$(OCV_DIR)/include \
  ./Source/GeneratedFiles

# qmake configuration
CONFIG *= windows debug_and_release c++11
QT += widgets winextras
MOC_DIR += ./Source/GeneratedFiles/Debug
UI_DIR += ./Source/GeneratedFiles
RCC_DIR += ./Source/GeneratedFiles

# Dependencies
message(Platform specs: $$QMAKESPEC)
win32:contains(QMAKE_TARGET.arch, x86_64) {
  DEPENDPATH += $$(OCV_DIR)/x64/vc15/lib
  DSTDIR = ./Build/vc15_x64
  OBJECTS_DIR += ./Assembly/Debug_x64
} else {
  message(This project compiles only on Windows x64)
}

# Build configuration
CONFIG(debug, debug|release) {
  INCLUDEPATH += ./Source/GeneratedFiles/Debug
  LIBS += -lopencv_world342d
  TARGET = ScreenMacroApplicationDbg
} else {
  INCLUDEPATH += ./Source/GeneratedFiles/Release
  LIBS += -lopencv_world342
  TARGET = ScreenMacroApplication
}