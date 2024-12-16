//===--- AccessTypeIDZone.def -----------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This definition file describes the types in the access-control
//  TypeID zone, for use with the TypeID template.
//
//===----------------------------------------------------------------------===//

POLAR_REQUEST(AccessControl, AccessLevelRequest, AccessLevel(ValueDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(AccessControl, DefaultAndMaxAccessLevelRequest,
              DefaultAndMax(ExtensionDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(AccessControl, SetterAccessLevelRequest,
              AccessLevel(AbstractStorageDecl *), SeparatelyCached,
              NoLocationInfo)
