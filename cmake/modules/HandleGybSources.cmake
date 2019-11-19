# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/11/07.

include(AddCustomCommandTarget)

# Create a target to process single gyb source with the 'gyb' tool.
#
# handle_gyb_source_single(
#     dependency_out_var_name
#     SOURCE src_gyb
#     OUTPUT output
#     [FLAGS [flags ...]])
#     [DEPENDS [depends ...]]
#     [COMMENT comment])
#
# dependency_out_var_name
#   The name of a variable, to be set in the parent scope to be the target
#   target that invoke gyb.
#
# src_gyb
#   .gyb suffixed source file
#
# output
#   Output filename to be generated
#
# flags ...
#    gyb flags in addition to ${POLAR_GYB_FLAGS}.
#
# depends ...
#    gyb flags in addition to 'src_gyb' and sources of gyb itself.
#
# comment
#    Additional comment.

function(handle_gyb_source_single dependency_out_var_name)
   set(options)
   set(single_value_args SOURCE OUTPUT COMMENT)
   set(multi_value_args FLAGS DEPENDS)
   cmake_parse_arguments(
      GYB_SINGLE # prefix
      "${options}" "${single_value_args}" "${multi_value_args}" ${ARGN})

   set(gyb_tool "${POLAR_DEVTOOLS_DIR}/gyb/gyb")
   set(gyb_tool_source "${gyb_tool}")

   get_filename_component(dir "${GYB_SINGLE_OUTPUT}" DIRECTORY)
   get_filename_component(basename "${GYB_SINGLE_OUTPUT}" NAME)

   # Handle foo.gyb in pattern ``gyb.expand('foo.gyb'`` as a dependency
   set(gyb_expand_deps "")
   file(READ "${GYB_SINGLE_SOURCE}" gyb_file)
   string(REGEX MATCHALL "\\\$\{[\r\n\t ]*gyb.expand\\\([\r\n\t ]*[\'\"]([^\'\"]*)[\'\"]" gyb_expand_matches "${gyb_file}")
      foreach(match ${gyb_expand_matches})
         string(REGEX MATCH "[\'\"]\([^\'\"]*\)[\'\"]" gyb_dep "${match}")
         list(APPEND gyb_expand_deps "${CMAKE_MATCH_1}")
      endforeach()
   list(REMOVE_DUPLICATES gyb_expand_deps)

   add_custom_command_target(
      dependency_target
      COMMAND
      "${CMAKE_COMMAND}" -E make_directory "${dir}"
      COMMAND
      "${PHP_EXECUTABLE}" "${gyb_tool}" generate ${GYB_SINGLE_FLAGS} -o "${GYB_SINGLE_OUTPUT}.tmp" "${GYB_SINGLE_SOURCE}"
      COMMAND
      "${CMAKE_COMMAND}" -E copy_if_different "${GYB_SINGLE_OUTPUT}.tmp" "${GYB_SINGLE_OUTPUT}"
      COMMAND
      "${CMAKE_COMMAND}" -E remove "${GYB_SINGLE_OUTPUT}.tmp"
      OUTPUT "${GYB_SINGLE_OUTPUT}"
      DEPENDS "${gyb_tool_source}" "${GYB_SINGLE_DEPENDS}" "${GYB_SINGLE_SOURCE}" "${gyb_expand_deps}"
      COMMENT "Generating ${basename} from ${GYB_SINGLE_SOURCE} ${GYB_SINGLE_COMMENT}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      SOURCES "${GYB_SINGLE_SOURCE}"
      IDEMPOTENT
   )
   set("${dependency_out_var_name}" "${dependency_target}" PARENT_SCOPE)
endfunction()

# Create a target to process .gyb files with the 'gyb' tool.
#
# handle_gyb_sources(
#     dependency_out_var_name
#     sources_var_name
#     arch)
#
# Replace, in ${sources_var_name}, the given .gyb-suffixed sources with
# their un-suffixed intermediate files, which will be generated by processing
# the .gyb files with gyb.
#
# dependency_out_var_name
#   The name of a variable, to be set in the parent scope to the list of
#   targets that invoke gyb.  Every target that depends on the generated
#   sources should depend on ${dependency_out_var_name} targets.
#
# arch
#   The architecture that the files will be compiled for.  If this is
#   false, the files are architecture-independent and will be emitted
#   into ${CMAKE_CURRENT_BINARY_DIR} instead of an architecture-specific
#   destination; this is useful for generated include files.

function(handle_gyb_sources dependency_out_var_name sources_var_name)
   set(extra_gyb_flags "")
   set(dependency_targets)
   set(de_gybbed_sources)
   set(gyb_extra_sources)
   set(gyb_dir ${POLAR_DEVTOOLS_DIR}/gyb/src)
   file(GLOB_RECURSE gyb_extra_sources
      LIST_DIRECTORIES false
      ${gyb_dir}/*.php)
   foreach (src ${${sources_var_name}})
      string(REGEX REPLACE "[.]gyb$" "" src_sans_gyb "${src}")
      if(src STREQUAL src_sans_gyb)
         list(APPEND de_gybbed_sources "${src}")
      else()

         # On Windows (using Visual Studio), the generated project files assume that the
         # generated GYB files will be in the source, not binary directory.
         # We can work around this by modifying the root directory when generating VS projects.
#         if ("${CMAKE_GENERATOR_PLATFORM}" MATCHES "Visual Studio")
#            set(dir_root ${CMAKE_CURRENT_SOURCE_DIR})
#         else()
#            set(dir_root ${CMAKE_CURRENT_BINARY_DIR})
#         endif()
         set(dir_root ${CMAKE_CURRENT_SOURCE_DIR})

         set(output_file_name "${dir_root}/${src_sans_gyb}")
         list(APPEND de_gybbed_sources "${output_file_name}")
         handle_gyb_source_single(dependency_target
            SOURCE "${src}"
            OUTPUT "${output_file_name}"
            FLAGS ${extra_gyb_flags}
            DEPENDS "${gyb_extra_sources}")
         list(APPEND dependency_targets "${dependency_target}")
      endif()
   endforeach()
   set("${dependency_out_var_name}" "${dependency_targets}" PARENT_SCOPE)
   set("${sources_var_name}" "${de_gybbed_sources}" PARENT_SCOPE)
endfunction()

function(add_gyb_target target sources)
  set(options)
  set(single_value_args)
  set(multi_value_args)
  cmake_parse_arguments(GYB
    "${options}" "${single_value_args}" "${multi_value_args}" ${ARGN})

  handle_gyb_sources(gyb_sources_depends sources)

  add_custom_target(${target}
    DEPENDS "${gyb_sources_depends}")
endfunction()


