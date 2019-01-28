// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/26.

#ifndef POLARPHP_STDLIB_VMBINDER_NAMESPACE_DEFS_H
#define POLARPHP_STDLIB_VMBINDER_NAMESPACE_DEFS_H

namespace polar {
namespace vmapi {
class Module;
} // vmapi
} // polar

namespace php {
namespace vmbinder {

using polar::vmapi::Module;

void register_stdlib_namespaces(Module &module);

} // vmbinder
} // php

#endif // POLARPHP_STDLIB_VMBINDER_NAMESPACE_DEFS_H
