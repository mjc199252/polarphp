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
// Created by polarboy on 2019/10/28.

namespace Gyb\Syntax;
/**
 * A Syntax node, possibly with children.
 * If the kind is "SyntaxCollection", then this node is considered a Syntax
 * Collection that will expose itself as a typedef rather than a concrete
 * subclass.
 *
 * @package Gyb\Syntax
 */
class Node
{
   /**
    * @var int $syntaxKind
    */
   private $syntaxKind;
   /**
    * @var int $polarSyntaxKind
    */
   private $polarSyntaxKind;
   /**
    * @var string $name
    */
   private $name;
   /**
    * @var string $descriptions
    */
   private $description;
   /**
    * @var array $traits
    */
   private $traits;
   /**
    * @var array $children
    */
   private $children;
   /**
    * @var string $baseKind
    */
   private $baseKind;
   /**
    * @var string $baseType
    */
   private $baseType;
   /**
    * @var bool $omitWhenEmpty
    */
   private $omitWhenEmpty;
   /**
    * @var string $collectionElement
    */
   private $collectionElementKind;
   /**
    * @var string $collectionElementName
    */
   private $collectionElementName;
   /**
    * @var string $collectionElementType
    */
   private $collectionElementType;
   /**
    * @var array $collectionElementChoices
    */
   private $collectionElementChoices;

   public function __construct(string $kind, $baseKind = null, string $description = '',
                               array $traits = [], array $children = [], string $elementKind = '',
                               string $elementName = '', array $elementChoices = [],
                               bool $omitWhenEmpty = false)
   {
      $this->syntaxKind = $kind;
      $this->polarSyntaxKind = lcfirst($kind);
      $this->name = Kinds::convertKindToType($kind);
      $this->description = $description;
      $this->traits = $traits;
      $this->children = $children;
      $this->baseKind = $kind;
      if ($this->baseKind == Kinds::SYNTAX_COLLECTION) {
         $this->baseType = Kinds::SYNTAX;
      } else {
         $this->baseType = Kinds::convertKindToType($this->baseKind);
      }
      if (!Kinds::isValidBaseKind($this->baseKind)) {
         throw new \RuntimeException(sprintf("unknown base kind '%s' for node '%s'",
            $this->baseKind, $this->syntaxKind));
      }
      $this->omitWhenEmpty = $omitWhenEmpty;
      $this->collectionElementKind = trim($elementKind);
      $elementName = trim($elementName);
      $elementKind = trim($elementKind);
      // For SyntaxCollections make sure that the element_name is set.
      assert(!$this->isSyntaxCollection() || !empty($elementName) ||
         (!empty($elementKind) && $elementKind != Kinds::SYNTAX));
      // If there's a preferred name for the collection element that differs
      // from its supertype, use that.
      $this->collectionElementName = !empty($elementName) ? $elementName: $this->collectionElementKind;
      $this->collectionElementType = Kinds::convertKindToType($this->collectionElementKind);
      $this->collectionElementChoices = $elementChoices;
   }

   /**
    * Returns `True` if this node declares one of the base syntax kinds.
    *
    * @return bool
    */
   public function isBase(): bool
   {
      return Kinds::isValidBaseKind($this->syntaxKind);
   }

   /**
    * Returns `True` if this node is a subclass of SyntaxCollection.
    *
    * @return bool
    */
   public function isSyntaxCollection(): bool
   {
      return $this->baseKind == Kinds::SYNTAX_COLLECTION;
   }

   /**
    * @return int
    */
   public function getSyntaxKind(): int
   {
      return $this->syntaxKind;
   }

   /**
    * Returns `True` if this node should have a `validate` method associated.
    * @return bool
    */
   public function requiresValidation(): bool
   {
      return $this->isBuildable();
   }

   /**
    * Returns `True` if this node is an `Unknown` syntax subclass.
    * @return bool
    */
   public function isUnknown(): bool
   {
      return \Gyb\Utils\has_substr($this->syntaxKind, Kinds::UNKNOWN);
   }

   /**
    * Returns `True` if this node should have a builder associated.
    *
    * @return bool
    */
   public function isBuildable(): bool
   {
      return !$this->isBase() && !$this->isUnknown() && !$this->isSyntaxCollection();
   }

   /**
    * Returns 'True' if this node shall not be created while parsing if it
    * has no children.
    */
   public function shallBeOmittedWhenEmpty(): bool
   {
      return $this->omitWhenEmpty;
   }

   /**
    * @return string
    */
   public function getCollectionElementKind(): string
   {
      return $this->collectionElementKind;
   }

   /**
    * @param int $syntaxKind
    * @return Node
    */
   public function setSyntaxKind(int $syntaxKind): Node
   {
      $this->syntaxKind = $syntaxKind;
      return $this;
   }

   /**
    * @return int
    */
   public function getPolarSyntaxKind(): int
   {
      return $this->polarSyntaxKind;
   }

   /**
    * @param int $polarSyntaxKind
    * @return Node
    */
   public function setPolarSyntaxKind(int $polarSyntaxKind): Node
   {
      $this->polarSyntaxKind = $polarSyntaxKind;
      return $this;
   }

   /**
    * @return string
    */
   public function getName(): string
   {
      return $this->name;
   }

   /**
    * @param string $name
    * @return Node
    */
   public function setName(string $name): Node
   {
      $this->name = $name;
      return $this;
   }

   /**
    * @return string
    */
   public function getDescription(): string
   {
      return $this->description;
   }

   /**
    * @param string $description
    * @return Node
    */
   public function setDescription(string $description): Node
   {
      $this->description = $description;
      return $this;
   }

   /**
    * @return array
    */
   public function getTraits(): array
   {
      return $this->traits;
   }

   /**
    * @param array $traits
    * @return Node
    */
   public function setTraits(array $traits): Node
   {
      $this->traits = $traits;
      return $this;
   }

   /**
    * @return array
    */
   public function getChildren(): array
   {
      return $this->children;
   }

   /**
    * @param array $children
    * @return Node
    */
   public function setChildren(array $children): Node
   {
      $this->children = $children;
      return $this;
   }

   /**
    * @return string
    */
   public function getBaseKind(): string
   {
      return $this->baseKind;
   }

   /**
    * @param string $baseKind
    * @return Node
    */
   public function setBaseKind(string $baseKind): Node
   {
      $this->baseKind = $baseKind;
      return $this;
   }

   /**
    * @return string
    */
   public function getBaseType(): string
   {
      return $this->baseType;
   }

   /**
    * @param string $baseType
    * @return Node
    */
   public function setBaseType(string $baseType): Node
   {
      $this->baseType = $baseType;
      return $this;
   }

   /**
    * @return bool
    */
   public function isOmitWhenEmpty(): bool
   {
      return $this->omitWhenEmpty;
   }

   /**
    * @param bool $omitWhenEmpty
    * @return Node
    */
   public function setOmitWhenEmpty(bool $omitWhenEmpty): Node
   {
      $this->omitWhenEmpty = $omitWhenEmpty;
      return $this;
   }

   /**
    * @return string
    */
   public function getCollectionElement(): string
   {
      return $this->collectionElementKind;
   }

   /**
    * @param string $collectionElement
    * @return Node
    */
   public function setCollectionElement(string $collectionElement): Node
   {
      $this->collectionElementKind = $collectionElement;
      return $this;
   }

   /**
    * @return string
    */
   public function getCollectionElementName(): string
   {
      return $this->collectionElementName;
   }

   /**
    * @param string $collectionElementName
    * @return Node
    */
   public function setCollectionElementName(string $collectionElementName): Node
   {
      $this->collectionElementName = $collectionElementName;
      return $this;
   }

   /**
    * @return string
    */
   public function getCollectionElementType(): string
   {
      return $this->collectionElementType;
   }

   /**
    * @param string $collectionElementType
    * @return Node
    */
   public function setCollectionElementType(string $collectionElementType): Node
   {
      $this->collectionElementType = $collectionElementType;
      return $this;
   }

   /**
    * @return array
    */
   public function getCollectionElementChoices(): array
   {
      return $this->collectionElementChoices;
   }

   /**
    * @param array $collectionElementChoices
    * @return Node
    */
   public function setCollectionElementChoices(array $collectionElementChoices): Node
   {
      $this->collectionElementChoices = $collectionElementChoices;
      return $this;
   }
}