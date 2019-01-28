// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/15.

#include <iostream>
#include "LitConfig.h"
#include "TestingConfig.h"
#include "formats/GoogleTest.h"
#include "polarphp/basic/adt/StringRef.h"

using polar::lit::LitConfig;
using polar::lit::TestingConfig;
using polar::lit::GoogleTest;

extern "C" {
void root_cfgsetter(TestingConfig *config, LitConfig *litConfig)
{
   config->setName("googletest-timeout");
   std::list<std::string> tests{
      TIMEOUT_GTEST_BIN
   };
   config->setTestFormat(std::make_shared<GoogleTest>(tests));
   if (litConfig->hasParam("set_timeout")) {
      if (litConfig->getParam("set_timeout") == "1") {
         litConfig->setMaxIndividualTestTime(1);
      }
   }
}
}
