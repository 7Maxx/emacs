;;; css-mode-tests.el --- Test suite for CSS mode  -*- lexical-binding: t; -*-

;; Copyright (C) 2016 Free Software Foundation, Inc.

;; Author: Simen Heggestøyl <simenheg@gmail.com>
;; Keywords: internal

;; This file is part of GNU Emacs.

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;;; Code:

(require 'css-mode)
(require 'ert)
(require 'seq)

(ert-deftest css-test-property-values ()
  ;; The `float' property has a flat value list.
  (should
   (equal (sort (css--property-values "float") #'string-lessp)
          '("left" "none" "right")))

  ;; The `list-style' property refers to several other properties.
  (should
   (equal (sort (css--property-values "list-style") #'string-lessp)
          (sort (seq-uniq
                 (append (css--property-values "list-style-type")
                         (css--property-values "list-style-position")
                         (css--property-values "list-style-image")))
                #'string-lessp)))

  ;; The `position' property is tricky because it's also the name of a
  ;; value class.
  (should
   (equal (sort (css--property-values "position") #'string-lessp)
          '("absolute" "fixed" "relative" "static")))

  ;; The `background-position' property should refer to the `position'
  ;; value class, not the property of the same name.
  (should
   (equal (css--property-values "background-position")
          (css--value-class-lookup 'position)))

  ;; Check that the `color' property doesn't cause infinite recursion
  ;; because it refers to the value class of the same name.
  (should (= (length (css--property-values "color")) 18)))

(ert-deftest css-test-property-value-cache ()
  "Test that `css--property-value-cache' is in use."
  (should-not (gethash "word-wrap" css--property-value-cache))
  (let ((word-wrap-values (css--property-values "word-wrap")))
    (should (equal (gethash "word-wrap" css--property-value-cache)
                   word-wrap-values))))

(ert-deftest css-test-property-values-no-duplicates ()
  "Test that `css--property-values' returns no duplicates."
  ;; The `flex' property is prone to duplicate values; if they aren't
  ;; removed, it'll contain at least two instances of `auto'.
  (should
   (equal (sort (css--property-values "flex") #'string-lessp)
          '("auto" "calc()" "content" "none"))))

(ert-deftest css-test-value-class-lookup ()
  (should
   (equal (sort (css--value-class-lookup 'position) #'string-lessp)
          '("bottom" "calc()" "center" "left" "right" "top"))))

(provide 'css-mode-tests)
;;; css-mode-tests.el ends here
