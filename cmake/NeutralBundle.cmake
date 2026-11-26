# SDK 3.7.12 expands $(TargetFileName) after MSBuild command quoting.
# Stage the actual resources with a real, quoted path instead.
function(awe64_stage_bundle_resources target)
  get_target_property(package_path ${target} SMTG_PLUGIN_PACKAGE_PATH)
  if(NOT package_path)
    message(FATAL_ERROR "VST3 package path is missing")
  endif()
  set(resource_dir "${package_path}/Contents/Resources")
  add_custom_target(${target}_bundle_resources
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${resource_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${ARGN} "${resource_dir}"
    DEPENDS ${ARGN}
    VERBATIM)
  add_dependencies(${target} ${target}_bundle_resources)
endfunction()

function(awe64_write_module_info target)
  if(NOT TARGET moduleinfotool)
    message(FATAL_ERROR "Pinned SDK moduleinfotool is missing")
  endif()
  get_target_property(package_path ${target} SMTG_PLUGIN_PACKAGE_PATH)
  add_dependencies(${target} moduleinfotool)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "$<TARGET_FILE:moduleinfotool>" -create -version "${PROJECT_VERSION}"
      -path "${package_path}"
      -output "${package_path}/Contents/Resources/moduleinfo.json"
    VERBATIM)
endfunction()
