# Get all directories
file(GLOB entries RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/* )
set(dirs "")
foreach (entry IN LISTS entries)
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${entry} )
        list(APPEND dirs ${CMAKE_CURRENT_SOURCE_DIR}/${entry})
    endif ()
endforeach ()
message(STATUS "[Submission Nodes] " ${dirs})
# Get all targets
set(inner_targets "")
foreach (dir IN LISTS dirs)
    # 跳过无效目录（如 build 目录）
    if (dir MATCHES "build$")
        message(WARNING "Skipping build directory: ${dir}")
        continue()
    endif ()
    message(STATUS "[Add directory] " ${dir})
    add_subdirectory(${dir})
    get_property(current_targets DIRECTORY ${dir}/nodes PROPERTY BUILDSYSTEM_TARGETS)
    message(STATUS "[Sub targets] " ${current_targets})
    foreach (tgt IN LISTS current_targets)
        message(STATUS "[Pushback target]  " ${tgt})
        list(APPEND inner_targets ${tgt})
    endforeach ()
endforeach ()


foreach (dir IN LISTS dirs)
    # 跳过无效目录（如 build 目录）
    if (dir MATCHES "build$")
        message(WARNING "Skipping build directory: ${dir}")
        continue()
    endif ()
    message(STATUS "[Add directory RENDER] " ${dir})
    if (EXISTS ${dir}/nodes_render)
        message(INFO "${dir}/nodes_render exist")
        get_property(current_targets DIRECTORY ${dir}/nodes_render PROPERTY BUILDSYSTEM_TARGETS)
        message(STATUS "[Sub targets RENDER] " ${current_targets})
        foreach (tgt IN LISTS current_targets)
            message(STATUS "[Pushback target RENDER]  " ${tgt})
            list(APPEND inner_targets ${tgt})
        endforeach ()
    else()
        message(WARNING "Directory ${dir}/nodes_render dose not exist or is valid.")
    endif()
endforeach ()


message(STATUS "[All sub targets] " ${inner_targets})

get_filename_component(submission_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_custom_target(${submission_name}_nodes
    ALL
        COMMENT "Building submissions..."
)
add_dependencies(${submission_name}_nodes ${inner_targets})


