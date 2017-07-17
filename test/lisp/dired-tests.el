;;; dired-tests.el --- Test suite. -*- lexical-binding: t -*-

;; Copyright (C) 2015-2017 Free Software Foundation, Inc.

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

;;; Code:
(require 'ert)
(require 'dired)
(require 'nadvice)


(ert-deftest dired-autoload ()
  "Tests to see whether dired-x has been autoloaded"
  (should
   (fboundp 'dired-jump))
  (should
   (autoloadp
    (symbol-function
     'dired-jump))))

(ert-deftest dired-test-bug22694 ()
  "Test for http://debbugs.gnu.org/22694 ."
  (let* ((dir       (expand-file-name "bug22694" default-directory))
         (file      "test")
         (full-name (expand-file-name file dir))
         (regexp    "bar")
         (dired-always-read-filesystem t))
    (if (file-exists-p dir)
        (delete-directory dir 'recursive))
    (make-directory dir)
    (with-temp-file full-name (insert "foo"))
    (find-file-noselect full-name)
    (dired dir)
    (with-temp-file full-name (insert "bar"))
    (dired-mark-files-containing-regexp regexp)
    (unwind-protect
        (should (equal (dired-get-marked-files nil nil nil 'distinguish-1-mark)
                       `(t ,full-name)))
      ;; Clean up
      (delete-directory dir 'recursive))))

(ert-deftest dired-test-bug25609 ()
  "Test for http://debbugs.gnu.org/25609 ."
  (let* ((from (make-temp-file "foo" 'dir))
         (to (make-temp-file "bar" 'dir))
         (target (expand-file-name (file-name-nondirectory from) to))
         (nested (expand-file-name (file-name-nondirectory from) target))
         (dired-dwim-target t)
         (dired-recursive-copies 'always)) ; Don't prompt me.
    (advice-add 'dired-query ; Don't ask confirmation to overwrite a file.
                :override
                (lambda (_sym _prompt &rest _args) (setq dired-query t))
                '((name . "advice-dired-query")))
    (advice-add 'completing-read ; Just return init.
                :override
                (lambda (_prompt _coll &optional _pred _match init _hist _def _inherit _keymap)
                  init)
                '((name . "advice-completing-read")))
    (dired to)
    (dired-other-window temporary-file-directory)
    (dired-goto-file from)
    (dired-do-copy)
    (dired-do-copy); Again.
    (unwind-protect
        (progn
          (should (file-exists-p target))
          (should-not (file-exists-p nested)))
      (delete-directory from 'recursive)
      (delete-directory to 'recursive)
      (advice-remove 'dired-query "advice-dired-query")
      (advice-remove 'completing-read "advice-completing-read"))))

(ert-deftest dired-test-bug27243 ()
  "Test for http://debbugs.gnu.org/27243 ."
  (let ((test-dir (make-temp-file "test-dir-" t))
        (dired-auto-revert-buffer t))
    (with-current-buffer (find-file-noselect test-dir)
      (make-directory "test-subdir"))
    (dired test-dir)
    (unwind-protect
        (let ((buf (current-buffer))
              (pt1 (point))
              (test-file (concat (file-name-as-directory "test-subdir")
                                 "test-file")))
          (write-region "Test" nil test-file nil 'silent nil 'excl)
          ;; Sanity check: point should now be on the subdirectory.
          (should (equal (dired-file-name-at-point)
                         (concat (file-name-as-directory test-dir)
                                 (file-name-as-directory "test-subdir"))))
          (dired-find-file)
          (let ((pt2 (point)))          ; Point is on test-file.
            (switch-to-buffer buf)
            ;; Sanity check: point should now be back on the subdirectory.
            (should (eq (point) pt1))
            ;; Case 1: https://debbugs.gnu.org/cgi/bugreport.cgi?bug=27243#5
            (dired-find-file)
            (should (eq (point) pt2))
            ;; Case 2: https://debbugs.gnu.org/cgi/bugreport.cgi?bug=27243#28
            (dired test-dir)
            (should (eq (point) pt1))))
      (delete-directory test-dir t))))

(ert-deftest dired-test-bug27693 ()
  "Test for http://debbugs.gnu.org/27693 ."
  (require 'ls-lisp)
  (let ((size "")
	ls-lisp-use-insert-directory-program)
    (dired (list (expand-file-name "lisp" source-directory) "simple.el" "subr.el"))
    (setq size (number-to-string
                (file-attribute-size
                 (file-attributes (dired-get-filename)))))
    (search-backward-regexp size nil t)
    (should (looking-back "[[:space:]]" (1- (point))))))

(provide 'dired-tests)
;; dired-tests.el ends here
