if (BUILD_TESTING)
    if (NOT doctest)
        add_library(doctest INTERFACE)
        target_include_directories(doctest SYSTEM INTERFACE doctest-2.3.4/include)
    endif()
endif()
