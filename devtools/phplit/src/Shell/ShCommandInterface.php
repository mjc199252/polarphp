<?php
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/10/10.

namespace Lit\Shell;

interface ShCommandInterface
{
   public function getArgsCount(): int;
   public function getArgs(): array;
   public function setArgs(array $args = array());
   public function __toString(): string;
   public function equalWith($other): bool;
   public function toShell($file, $pipeFail = false): void;
}