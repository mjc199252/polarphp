//===--- SourceInfoFormat.h - Format swiftsourceinfo files ---*- c++ -*---===//
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
///
/// \file Contains various constants and helper types to deal with serialized
/// source information (.swiftsourceinfo files).
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_SERIALIZATION_SOURCEINFOFORMAT_H
#define POLARPHP_SERIALIZATION_SOURCEINFOFORMAT_H

namespace polar {
namespace serialization {

/// Magic number for serialized source info files.
const unsigned char PHPSOURCEINFO_SIGNATURE[] = { 0xF0, 0x9F, 0x8F, 0x8E };

/// Serialized sourceinfo format major version number.
///
/// Increment this value when making a backwards-incompatible change, i.e. where
/// an \e old compiler will \e not be able to read the new format. This should
/// be rare. When incrementing this value, reset PHPSOURCEINFO_VERSION_MINOR to 0.
///
/// See docs/StableBitcode.md for information on how to make
/// backwards-compatible changes using the LLVM bitcode format.
const uint16_t PHPSOURCEINFO_VERSION_MAJOR = 1;

/// Serialized swiftsourceinfo format minor version number.
///
/// Increment this value when making a backwards-compatible change that might be
/// interesting to test for. A backwards-compatible change is one where an \e
/// old compiler can read the new format without any problems (usually by
/// ignoring new information).
const uint16_t PHPSOURCEINFO_VERSION_MINOR = 1; // Last change: skipping 0 for testing purposes

/// The hash seed used for the string hashes(llvm::djbHash) in a .swiftsourceinfo file.
const uint32_t PHPSOURCEINFO_HASH_SEED = 5387;

/// The record types within the DECL_LOCS block.
///
/// Though we strive to keep the format stable, breaking the format of
/// .swiftsourceinfo doesn't have consequences as serious as breaking the format
/// of .swiftdoc, because .swiftsourceinfo file is for local development use only.
///
/// When changing this block, backwards-compatible changes are prefered.
/// You may need to update the version when you do so. See docs/StableBitcode.md
/// for information on how to make backwards-compatible changes using the LLVM
/// bitcode format.
///
/// \sa DECL_LOCS_BLOCK_ID
namespace decl_locs_block {
enum RecordKind {
   BASIC_DECL_LOCS = 1,
   DECL_USRS,
   TEXT_DATA,
};

using BasicDeclLocsLayout = BCRecordLayout<
   BASIC_DECL_LOCS, // record ID
   BCBlob           // an array of fixed size location data
>;

using DeclUSRSLayout = BCRecordLayout<
   DECL_USRS,         // record ID
   BCVBR<16>,         // table offset within the blob (an llvm::OnDiskHashTable)
   BCBlob             // map from actual USR to USR Id
>;

using TextDataLayout = BCRecordLayout<
   TEXT_DATA,        // record ID
   BCBlob            // a list of 0-terminated string segments
>;

} // namespace sourceinfo_block

} // end namespace serialization
} // end namespace polar

#endif