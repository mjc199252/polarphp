//===-------- IDERequestIDZone.def ------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This definition file describes the types in the IDE requests
//  TypeID zone, for use with the TypeID template.
//
//===----------------------------------------------------------------------===//

POLAR_REQUEST(IDE, CollectOverriddenDeclsRequest,
   ArrayRef<ValueDecl *>(OverridenDeclsOwner), Cached,
   NoLocationInfo)
POLAR_REQUEST(IDE, CursorInfoRequest, ide::ResolvedCursorInfo(CursorInfoOwner),
   Cached, NoLocationInfo)
POLAR_REQUEST(IDE, ProvideDefaultImplForRequest,
   ArrayRef<ValueDecl *>(ValueDecl *), Cached,
   NoLocationInfo)
POLAR_REQUEST(IDE, RangeInfoRequest, ide::ResolvedRangeInfo(RangeInfoOwner),
   Cached, NoLocationInfo)
POLAR_REQUEST(IDE, ResolveInterfaceNameRequest,
   InterfaceDecl *(InterfaceNameOwner), Cached,
NoLocationInfo)
