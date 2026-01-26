cmake_minimum_required(VERSION 3.16)
project(Nimo VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Sql)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appNimo
    main.cpp
)

qt_add_qml_module(appNimo
    URI Nimo
    QML_FILES
        Main.qml
        RESOURCES project_layout.md
        SOURCES logging/logger.h logging/logger.cpp
        SOURCES logging/requestcontext.h logging/requestcontext.cpp
        SOURCES database/databasemanager.h database/databasemanager.cpp
        SOURCES repositories/goalrepository.h repositories/goalrepository.cpp
        SOURCES services/goalservice.h services/goalservice.cpp
        QML_FILES config/logging.json
        SOURCES logging/requestscope.h
        SOURCES logging/loggermacros.h
        SOURCES repositories/occurrencerepository.h repositories/occurrencerepository.cpp
        SOURCES repositories/scorerepository.h repositories/scorerepository.cpp
        SOURCES repositories/streakrepository.h repositories/streakrepository.cpp
        SOURCES services/occurrenceservice.h services/occurrenceservice.cpp
        SOURCES services/scoreservice.h services/scoreservice.cpp
        SOURCES services/streakservice.h services/streakservice.cpp
        SOURCES services/calendarservice.h services/calendarservice.cpp
        SOURCES services/dashboardservice.h services/dashboardservice.cpp
)

set_target_properties(appNimo PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appNimo
    PRIVATE Qt6::Quick Qt6::Sql
)

include(GNUInstallDirs)
install(TARGETS appNimo
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
