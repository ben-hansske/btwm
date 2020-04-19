
add_library("${PROJECT_NAME}_compiler_warnings" INTERFACE)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # enable Clang-warnings
	target_compile_options("${PROJECT_NAME}_compiler_warnings" INTERFACE
		-Weverything
		-Wno-c++98-compat
		-Wno-c++98-compat-pedantic
		-Wno-missing-prototypes
		-Wno-ctad-maybe-unsupported
		-Wno-padded
		-Wno-sign-conversion
		)
endif()

add_library("${PROJECT_NAME}::compiler_warnings" ALIAS "${PROJECT_NAME}_compiler_warnings")
