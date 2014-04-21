cmake_minimum_required(VERSION 2.8)
project("wsgate")

# if MSVC, prevent it from capturing output
if(MSVC)
	set(ENV{VS_UNICODE_OUTPUT} "")
endif()
# define js files
set(JS_FILES modernizr-debug.js mootools-debug.js simpletabs-debug.js vkb-debug.js wsgate-debug.js)
set(JS_STRIPPED_FILES)

# create destionation dir for js files
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/js)

# define css files
set(CSS_FILES style-debug.css vkb-debug.css)
set(CSS_STRIPPED_FILES)

# create destionation dir for css files
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/css)

# search for perl and java
find_package(Perl)
find_package(Java)

if (PERL_FOUND)
	add_custom_command(
		OUTPUT index.html
		COMMAND ${PERL_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/strip-debug.pl ${CMAKE_CURRENT_SOURCE_DIR}/index-debug.html > ${CMAKE_CURRENT_BINARY_DIR}/index.html
		#COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/index.html.tmp ${CMAKE_CURRENT_BINARY_DIR}/index.html
		# clean-up tempfile
		#COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/index.html.tmp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/index-debug.html
	)

	# strip debug and generate js files
	foreach(js_file ${JS_FILES})
		string(REPLACE "-debug" "" stripped_js_file ${js_file})

		add_custom_command(
			OUTPUT js/${stripped_js_file}
			COMMAND ${PERL_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/strip-debug.pl ${CMAKE_CURRENT_SOURCE_DIR}/js/${js_file} > ${CMAKE_CURRENT_BINARY_DIR}/js/${stripped_js_file}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/js/${js_file}
		)

		set(JS_STRIPPED_FILES ${JS_STRIPPED_FILES} js/${stripped_js_file})
	endforeach()

	# strip debug and generate css files
	foreach(css_file ${CSS_FILES})
		string(REPLACE "-debug" "" stripped_css_file ${css_file})
		add_custom_command(
			OUTPUT css/${stripped_css_file}
			COMMAND ${PERL_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/strip-debug.pl ${CMAKE_CURRENT_SOURCE_DIR}/css/${css_file} > ${CMAKE_CURRENT_BINARY_DIR}/css/${stripped_css_file}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/css/${css_file}
		)

		set(CSS_STRIPPED_FILES ${CSS_STRIPPED_FILES} css/${stripped_css_file})
	endforeach()

else()
	message(WARNING "Perl not found. *-debug.{html|js|css} will only be renamed (not stripped).")

	# just copy index-debug.html and change its name to index.html
	add_custom_command(
		OUTPUT index.html
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/index-debug.html ${CMAKE_CURRENT_BINARY_DIR}/index.html
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/index-debug.html
	)

	# just copy *-debug.js and change its name to *.js
	foreach(js_file ${JS_FILES})
		string(REPLACE "-debug" "" stripped_js_file ${js_file})
		add_custom_command(
			OUTPUT js/${stripped_js_file}
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/js/${js_file} ${CMAKE_CURRENT_BINARY_DIR}/js/${stripped_js_file}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/js/${js_file}
		)
		set(JS_STRIPPED_FILES ${JS_STRIPPED_FILES} js/${stripped_js_file})
	endforeach()

	# just copy *-debug.css and change its name to *.css
	foreach(css_file ${CSS_FILES})
		string(REPLACE "-debug" "" stripped_css_file ${css_file})
		add_custom_command(
			OUTPUT css/${stripped_css_file}
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/css/${css_file} ${CMAKE_CURRENT_BINARY_DIR}/css/${stripped_css_file}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/css/${css_file}
		)
		set(CSS_STRIPPED_FILES ${CSS_STRIPPED_FILES} css/${stripped_css_file})
	endforeach()
endif()

# the following section has not been tested yet
# if java found, use yuicompressor.jar to compress the js and css files
if (JAVA_FOUND)
	# use the same work-around as above for stdout redirection.
	foreach(stripped_js_file ${JS_STRIPPED_FILES})
		add_custom_command(
			OUTPUT js/${stripped_js_file}
			COMMAND ${Java_JAVA_EXECUTABLE} -jar ${CMAKE_SOURCE_DIR}/tools/yuicompressor.jar --type js ${CMAKE_CURRENT_BINARY_DIR}/js/${stripped_js_file} > ${CMAKE_CURRENT_BINARY_DIR}/js/${stripped_js_file}
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/js/${stripped_js_file}
		)

	endforeach()

	foreach(stripped_css_file ${CSS_STRIPPED_FILES})
		add_custom_command(
			OUTPUT css/${stripped_css_file}
			COMMAND ${Java_JAVA_EXECUTABLE} -jar ${CMAKE_SOURCE_DIR}/tools/yuicompressor.jar --type css ${CMAKE_CURRENT_BINARY_DIR}/css/${stripped_css_file} > ${CMAKE_CURRENT_BINARY_DIR}/css/${stripped_css_file}
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/css/${stripped_css_file}
		)
	endforeach()
else()
	message(STATUS "Java executable not found. Files will not be compressed.")
endif()

SET_SOURCE_FILES_PROPERTIES(${JS_STRIPPED_FILES} PROPERTIES GENERATED TRUE)

SET_SOURCE_FILES_PROPERTIES(${CSS_STRIPPED_FILES} PROPERTIES GENERATED TRUE)

add_custom_target(strip_debug ALL DEPENDS index.html ${JS_STRIPPED_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${CSS_STRIPPED_FILES})