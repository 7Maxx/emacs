;;; shell-tests.el  -*- lexical-binding:t -*-

;; Copyright (C) 2010-2016 Free Software Foundation, Inc.

;; This file is part of GNU Emacs.

;; GNU Emacs is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; GNU Emacs is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; Tests for comint and related modes.

;;; Code:

(require 'shell)
(require 'ert)

(ert-deftest shell-tests-unquote-1 ()
  "Test problem found by Filipp Gunbin in emacs-devel."
  (should (equal (car (shell--unquote&requote-argument "te'st" 2)) "test")))

;;; shell-tests.el ends here
