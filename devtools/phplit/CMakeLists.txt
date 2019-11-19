# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/27.

configure_file(lit.php.in ${CMAKE_CURRENT_SOURCE_DIR}/lit @ONLY)

# just for expendiently edit

set(FILE_TYPES "*[.php|.yaml|.json]")

set(POLAR_PHPLIT_SOURCES "")

file(GLOB_RECURSE tempFiles
   LIST_DIRECTORIES false
   src/${FILE_TYPES})

list(APPEND POLAR_PHPLIT_SOURCES ${tempFiles})

file(GLOB_RECURSE tempFiles
   LIST_DIRECTORIES false
   config/${FILE_TYPES})

list(APPEND POLAR_PHPLIT_SOURCES ${tempFiles})

add_custom_target(
   phplit
   SOURCES ${POLAR_PHPLIT_SOURCES})
