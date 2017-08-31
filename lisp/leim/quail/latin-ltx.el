;;; latin-ltx.el --- Quail package for TeX-style input -*-coding: utf-8;-*-

;; Copyright (C) 2001-2017 Free Software Foundation, Inc.
;; Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
;;   2010, 2011
;;   National Institute of Advanced Industrial Science and Technology (AIST)
;;   Registration Number H14PRO021

;; Author: TAKAHASHI Naoto <ntakahas@m17n.org>
;;         Dave Love <fx@gnu.org>
;; Keywords: multilingual, input, Greek, i18n

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

;;; Code:

(require 'quail)

(quail-define-package
 "TeX" "UTF-8" "\\" t
 "LaTeX-like input method for many characters.
These characters are from the charsets used by the `utf-8' coding
system, including many technical ones.  Examples:
 \\\\='a -> á  \\\\=`{a} -> à
 \\pi -> π  \\int -> ∫  ^1 -> ¹"

 '(("\t" . quail-completion))
 t t nil nil nil nil nil nil nil t)

(eval-when-compile
  (require 'cl-lib)

  (defconst latin-ltx--mark-map
    '(("DOT BELOW" . "d")
      ("DOT ABOVE" . ".")
      ("OGONEK" . "k")
      ("CEDILLA" . "c")
      ("CARON" . "v")
      ;; ("HOOK ABOVE" . ??)
      ("MACRON" . "=")
      ("BREVE" . "u")
      ("TILDE" . "~")
      ("GRAVE" . "`")
      ("CIRCUMFLEX" . "^")
      ("DIAERESIS" . "\"")
      ("DOUBLE ACUTE" . "H")
      ("ACUTE" . "'")))

  (defconst latin-ltx--mark-re (regexp-opt (mapcar #'car latin-ltx--mark-map)))

  (defun latin-ltx--ascii-p (char)
    (and (characterp char) (< char 128)))

  (defmacro latin-ltx--define-rules (&rest rules)
    (load "uni-name")
    (let ((newrules ()))
      (dolist (rule rules)
        (pcase rule
          (`(,_ ,(pred characterp)) (push rule newrules)) ;; Normal quail rule.
          (`(,seq ,re)
           (let ((count 0)
                 (re (eval re t)))
             (maphash
              (lambda (name char)
                (when (and (characterp char) ;; Ignore char-ranges.
                           (string-match re name))
                  (let ((keys (if (stringp seq)
                                  (replace-match seq nil nil name)
                                (funcall seq name char))))
                    (if (listp keys)
                        (dolist (x keys)
                          (setq count (1+ count))
                          (push (list x char) newrules))
                      (setq count (1+ count))
                      (push (list keys char) newrules)))))
               (ucs-names))
             ;; (message "latin-ltx: %d mappings for %S" count re)
	     ))))
      (setq newrules (delete-dups newrules))
      (let ((rules (copy-sequence newrules)))
        (while rules
          (let ((rule (pop rules)))
            (when (assoc (car rule) rules)
              (let ((conflicts (list (cadr rule)))
                    (tail rules)
                    c)
                (while (setq c (assoc (car rule) tail))
                  (push (cadr c) conflicts)
                  (setq tail (cdr (memq c tail)))
                  (setq rules (delq c rules)))
                (message "Conflict for %S: %S"
                         (car rule) (apply #'string conflicts)))))))
      (let ((inputs (mapcar #'car newrules)))
        (setq inputs (delete-dups inputs))
        (message "latin-ltx: %d rules (+ %d conflicts)!"
                 (length inputs) (- (length newrules) (length inputs))))
      `(quail-define-rules ,@(nreverse newrules)))))

(latin-ltx--define-rules
 ("!`" ?¡)
 ("\\pounds" ?£) ;; ("{\\pounds}" ?£)
 ("\\S" ?§) ;; ("{\\S}" ?§)
 ("$^a$" ?ª)
 ("$\\pm$" ?±) ("\\pm" ?±)
 ("$^2$" ?²)
 ("$^3$" ?³)
 ("\\P" ?¶) ;; ("{\\P}" ?¶)
 ;; Fixme: Yudit has the equivalent of ("\\cdot" ?⋅), for U+22C5, DOT
 ;; OPERATOR, whereas · is MIDDLE DOT.  JadeTeX translates both to
 ;; \cdot.
 ("$\\cdot$" ?·) ("\\cdot" ?·)
 ("$^1$" ?¹)
 ("$^o$" ?º)
 ("?`" ?¿)

 ((lambda (name char)
    (let* ((c (if (match-end 1)
                  (downcase (match-string 2 name))
                (match-string 2 name)))
           (mark1 (cdr (assoc (match-string 3 name) latin-ltx--mark-map)))
           (mark2 (if (match-end 4)
                      (cdr (assoc (match-string 4 name) latin-ltx--mark-map))))
           (marks (if mark2 (concat mark1 "\\" mark2) mark1)))
      (cl-assert mark1)
      (cons (format "\\%s{%s}" marks c)
            ;; Exclude "d" because we use "\\dh" for something else.
            (unless (member (or mark2 mark1) '("d"));; "k"
              (list (format "\\%s%s" marks c))))))
  (concat "\\`LATIN \\(?:CAPITAL\\|SMAL\\(L\\)\\) LETTER \\(.\\) WITH \\("
          latin-ltx--mark-re "\\)\\(?: AND \\("
          latin-ltx--mark-re "\\)\\)?\\'"))

 ((lambda (name char)
    (let* ((mark (cdr (assoc (match-string 1 name) latin-ltx--mark-map))))
      (cl-assert mark)
      (list (format "\\%s" mark))))
  (concat "\\`COMBINING \\(" latin-ltx--mark-re "\\)\\(?: ACCENT\\)?\\'"))

 ((lambda (name char)
    (unless (latin-ltx--ascii-p char)
      (let* ((mark (cdr (assoc (match-string 1 name) latin-ltx--mark-map))))
        (cl-assert mark)
        (list (format "\\%s{}" mark)))))
  (concat "\\`\\(?:SPACING \\)?\\(" latin-ltx--mark-re "\\)\\(?: ACCENT\\)?\\'"))

 ("\\AA" ?Å) ;; ("{\\AA}" ?Å)
 ("\\AE" ?Æ) ;; ("{\\AE}" ?Æ)

 ("$\\times$" ?×) ("\\times" ?×)
 ("\\O" ?Ø) ;; ("{\\O}" ?Ø)
 ("\\ss" ?ß) ;; ("{\\ss}" ?ß)

 ("\\aa" ?å) ;; ("{\\aa}" ?å)
 ("\\ae" ?æ) ;; ("{\\ae}" ?æ)

 ("$\\div$" ?÷) ("\\div" ?÷)
 ("\\o" ?ø) ;; ("{\\o}" ?ø)

 ("\\~{\\i}" ?ĩ)
 ("\\={\\i}" ?ī)
 ("\\u{\\i}" ?ĭ)

 ("\\i" ?ı) ;; ("{\\i}" ?ı)
 ("\\^{\\j}" ?ĵ)

 ("\\L" ?Ł) ;; ("{\\L}" ?Ł)
 ("\\l" ?ł) ;; ("{\\l}" ?ł)

 ("\\H" ?̋)
 ("\\H{}" ?˝)
 ("\\U{o}" ?ő) ("\\Uo" ?ő) ;; FIXME: Was it just a typo?

 ("\\OE" ?Œ) ;; ("{\\OE}" ?Œ)
 ("\\oe" ?œ) ;; ("{\\oe}" ?œ)

 ("\\v{\\i}" ?ǐ)

 ("\\={\\AE}" ?Ǣ) ("\\=\\AE" ?Ǣ)
 ("\\={\\ae}" ?ǣ) ("\\=\\ae" ?ǣ)

 ("\\v{\\j}" ?ǰ)
 ("\\'{\\AE}" ?Ǽ) ("\\'\\AE" ?Ǽ)
 ("\\'{\\ae}" ?ǽ) ("\\'\\ae" ?ǽ)
 ("\\'{\\O}" ?Ǿ) ("\\'\\O" ?Ǿ)
 ("\\'{\\o}" ?ǿ) ("\\'\\o" ?ǿ)

 ("\\," ? )
 ("\\/" ?‌)
 ("\\:" ? )
 ("\\;" ? )

 ((lambda (name char)
    (let* ((base (concat (match-string 1 name) (match-string 3 name)))
           (basechar (gethash base (ucs-names))))
      (when (latin-ltx--ascii-p basechar)
        (string (if (match-end 2) ?^ ?_) basechar))))
  "\\(.*\\)SU\\(?:B\\|\\(PER\\)\\)SCRIPT \\(.*\\)")

 ((lambda (name _char)
    (let* ((basename (match-string 2 name))
           (name (if (match-end 1) (capitalize basename) (downcase basename))))
      (concat "^" (if (> (length name) 1) "\\") name)))
  "\\`MODIFIER LETTER \\(?:SMALL\\|CAPITA\\(L\\)\\) \\([[:ascii:]]+\\)\\'")

 ;; ((lambda (name char) (format "^%s" (downcase (match-string 1 name))))
 ;;  "\\`MODIFIER LETTER SMALL \\(.\\)\\'")
 ;; ("^\\1" "\\`MODIFIER LETTER CAPITAL \\(.\\)\\'")
 ("^o_" ?º)
 ("^{SM}" ?℠)
 ("^{TEL}" ?℡)
 ("^{TM}" ?™)

 ("\\b" ?̱)

 ("\\rq" ?’)

 ;; FIXME: Provides some useful entries (yen, euro, copyright, registered,
 ;; currency, minus, micro), but also a lot of dubious ones.
 ((lambda (name char)
    (unless (or (latin-ltx--ascii-p char)
                ;; We prefer COMBINING LONG SOLIDUS OVERLAY for \not.
                (member name '("NOT SIGN")))
      (concat "\\" (downcase (match-string 1 name)))))
  "\\`\\([^- ]+\\) SIGN\\'")

 ((lambda (name char)
    ;; "GREEK SMALL LETTER PHI" (which is \phi) and "GREEK PHI SYMBOL"
    ;; (which is \varphi) are reversed in `ucs-names', so we define
    ;; them manually.
    (unless (string-match-p "\\<PHI\\>" name)
      (concat "\\" (funcall (if (match-end 1) #' capitalize #'downcase)
                            (match-string 2 name)))))
  "\\`GREEK \\(?:SMALL\\|CAPITA\\(L\\)\\) LETTER \\([^- ]+\\)\\'")

 ("\\phi" ?ϕ)
 ("\\Box" ?□)
 ("\\Bumpeq" ?≎)
 ("\\Cap" ?⋒)
 ("\\Cup" ?⋓)
 ("\\Diamond" ?◇)
 ("\\Downarrow" ?⇓)
 ("\\H{o}" ?ő)
 ("\\Im" ?ℑ)
 ("\\Join" ?⋈)
 ("\\Leftarrow" ?⇐)
 ("\\Leftrightarrow" ?⇔)
 ("\\Ll" ?⋘)
 ("\\Lleftarrow" ?⇚)
 ("\\Longleftarrow" ?⇐)
 ("\\Longleftrightarrow" ?⇔)
 ("\\Longrightarrow" ?⇒)
 ("\\Lsh" ?↰)
 ("\\Re" ?ℜ)
 ("\\Rightarrow" ?⇒)
 ("\\Rrightarrow" ?⇛)
 ("\\Rsh" ?↱)
 ("\\Subset" ?⋐)
 ("\\Supset" ?⋑)
 ("\\Uparrow" ?⇑)
 ("\\Updownarrow" ?⇕)
 ("\\Vdash" ?⊩)
 ("\\Vert" ?‖)
 ("\\Vvdash" ?⊪)
 ("\\aleph" ?ℵ)
 ("\\amalg" ?∐)
 ("\\angle" ?∠)
 ("\\approx" ?≈)
 ("\\approxeq" ?≊)
 ("\\ast" ?∗)
 ("\\asymp" ?≍)
 ("\\backcong" ?≌)
 ("\\backepsilon" ?∍)
 ("\\backprime" ?‵)
 ("\\backsim" ?∽)
 ("\\backsimeq" ?⋍)
 ("\\backslash" ?\\)
 ("\\barwedge" ?⊼)
 ("\\because" ?∵)
 ("\\beth" ?ℶ)
 ("\\between" ?≬)
 ("\\bigcap" ?⋂)
 ("\\bigcirc" ?◯)
 ("\\bigcup" ?⋃)
 ("\\bigstar" ?★)
 ("\\bigtriangledown" ?▽)
 ("\\bigtriangleup" ?△)
 ("\\bigvee" ?⋁)
 ("\\bigwedge" ?⋀)
 ("\\blacklozenge" ?✦)
 ("\\blacksquare" ?▪)
 ("\\blacktriangle" ?▴)
 ("\\blacktriangledown" ?▾)
 ("\\blacktriangleleft" ?◂)
 ("\\blacktriangleright" ?▸)
 ("\\bot" ?⊥)
 ("\\bowtie" ?⋈)
 ("\\boxminus" ?⊟)
 ("\\boxplus" ?⊞)
 ("\\boxtimes" ?⊠)
 ("\\bullet" ?•)
 ("\\bumpeq" ?≏)
 ("\\cap" ?∩)
 ("\\cdots" ?⋯)
 ("\\centerdot" ?·)
 ("\\checkmark" ?✓)
 ("\\chi" ?χ)
 ("\\circ" ?∘)
 ("\\circeq" ?≗)
 ("\\circlearrowleft" ?↺)
 ("\\circlearrowright" ?↻)
 ("\\circledR" ?®)
 ("\\circledS" ?Ⓢ)
 ("\\circledast" ?⊛)
 ("\\circledcirc" ?⊚)
 ("\\circleddash" ?⊝)
 ("\\clubsuit" ?♣)
 ("\\coloneq" ?≔)
 ("\\complement" ?∁)
 ("\\cong" ?≅)
 ("\\coprod" ?∐)
 ("\\cup" ?∪)
 ("\\curlyeqprec" ?⋞)
 ("\\curlyeqsucc" ?⋟)
 ("\\curlypreceq" ?≼)
 ("\\curlyvee" ?⋎)
 ("\\curlywedge" ?⋏)
 ("\\curvearrowleft" ?↶)
 ("\\curvearrowright" ?↷)

 ("\\dag" ?†)
 ("\\dagger" ?†)
 ("\\daleth" ?ℸ)
 ("\\dashv" ?⊣)
 ("\\ddag" ?‡)
 ("\\ddagger" ?‡)
 ("\\ddots" ?⋱)
 ("\\diamond" ?⋄)
 ("\\diamondsuit" ?♢)
 ("\\divideontimes" ?⋇)
 ("\\doteq" ?≐)
 ("\\doteqdot" ?≑)
 ("\\dotplus" ?∔)
 ("\\dotsquare" ?⊡)
 ("\\downarrow" ?↓)
 ("\\downdownarrows" ?⇊)
 ("\\downleftharpoon" ?⇃)
 ("\\downrightharpoon" ?⇂)
 ("\\ell" ?ℓ)
 ("\\emptyset" ?∅)
 ("\\eqcirc" ?≖)
 ("\\eqcolon" ?≕)
 ("\\eqslantgtr" ?⋝)
 ("\\eqslantless" ?⋜)
 ("\\equiv" ?≡)
 ("\\exists" ?∃)
 ("\\fallingdotseq" ?≒)
 ("\\flat" ?♭)
 ("\\forall" ?∀)
 ("\\frac1" ?⅟)
 ("\\frac12" ?½)
 ("\\frac13" ?⅓)
 ("\\frac14" ?¼)
 ("\\frac15" ?⅕)
 ("\\frac16" ?⅙)
 ("\\frac18" ?⅛)
 ("\\frac23" ?⅔)
 ("\\frac25" ?⅖)
 ("\\frac34" ?¾)
 ("\\frac35" ?⅗)
 ("\\frac38" ?⅜)
 ("\\frac45" ?⅘)
 ("\\frac56" ?⅚)
 ("\\frac58" ?⅝)
 ("\\frac78" ?⅞)
 ("\\frown" ?⌢)
 ("\\ge" ?≥)
 ("\\geq" ?≥)
 ("\\geqq" ?≧)
 ("\\geqslant" ?≥)
 ("\\gets" ?←)
 ("\\gg" ?≫)
 ("\\ggg" ?⋙)
 ("\\gimel" ?ℷ)
 ("\\gnapprox" ?⋧)
 ("\\gneq" ?≩)
 ("\\gneqq" ?≩)
 ("\\gnsim" ?⋧)
 ("\\gtrapprox" ?≳)
 ("\\gtrdot" ?⋗)
 ("\\gtreqless" ?⋛)
 ("\\gtreqqless" ?⋛)
 ("\\gtrless" ?≷)
 ("\\gtrsim" ?≳)
 ("\\gvertneqq" ?≩)
 ("\\hbar" ?ℏ)
 ("\\heartsuit" ?♥)
 ("\\hookleftarrow" ?↩)
 ("\\hookrightarrow" ?↪)
 ("\\iff" ?⇔)
 ("\\imath" ?ı)
 ("\\in" ?∈)
 ("\\infty" ?∞)
 ("\\int" ?∫)
 ("\\intercal" ?⊺)
 ("\\langle" ?⟨) ;; Was ?〈, see bug#12948.
 ("\\lbrace" ?{)
 ("\\lbrack" ?\[)
 ("\\lceil" ?⌈)
 ("\\ldots" ?…)
 ("\\le" ?≤)
 ("\\leadsto" ?↝)
 ("\\leftarrow" ?←)
 ("\\leftarrowtail" ?↢)
 ("\\leftharpoondown" ?↽)
 ("\\leftharpoonup" ?↼)
 ("\\leftleftarrows" ?⇇)
 ;; ("\\leftparengtr" ?〈), see bug#12948.
 ("\\leftrightarrow" ?↔)
 ("\\leftrightarrows" ?⇆)
 ("\\leftrightharpoons" ?⇋)
 ("\\leftrightsquigarrow" ?↭)
 ("\\leftthreetimes" ?⋋)
 ("\\leq" ?≤)
 ("\\leqq" ?≦)
 ("\\leqslant" ?≤)
 ("\\lessapprox" ?≲)
 ("\\lessdot" ?⋖)
 ("\\lesseqgtr" ?⋚)
 ("\\lesseqqgtr" ?⋚)
 ("\\lessgtr" ?≶)
 ("\\lesssim" ?≲)
 ("\\lfloor" ?⌊)
 ("\\lhd" ?◁)
 ("\\rhd" ?▷)
 ("\\ll" ?≪)
 ("\\llcorner" ?⌞)
 ("\\lnapprox" ?⋦)
 ("\\lneq" ?≨)
 ("\\lneqq" ?≨)
 ("\\lnsim" ?⋦)
 ("\\longleftarrow" ?←)
 ("\\longleftrightarrow" ?↔)
 ("\\longmapsto" ?↦)
 ("\\longrightarrow" ?→)
 ("\\looparrowleft" ?↫)
 ("\\looparrowright" ?↬)
 ("\\lozenge" ?✧)
 ("\\lq" ?‘)
 ("\\lrcorner" ?⌟)
 ("\\ltimes" ?⋉)
 ("\\lvertneqq" ?≨)
 ("\\maltese" ?✠)
 ("\\mapsto" ?↦)
 ("\\measuredangle" ?∡)
 ("\\mho" ?℧)
 ("\\mid" ?∣)
 ("\\models" ?⊧)
 ("\\mp" ?∓)
 ("\\multimap" ?⊸)
 ("\\nLeftarrow" ?⇍)
 ("\\nLeftrightarrow" ?⇎)
 ("\\nRightarrow" ?⇏)
 ("\\nVDash" ?⊯)
 ("\\nVdash" ?⊮)
 ("\\nabla" ?∇)
 ("\\napprox" ?≉)
 ("\\natural" ?♮)
 ("\\ncong" ?≇)
 ("\\ne" ?≠)
 ("\\nearrow" ?↗)
 ("\\neg" ?¬)
 ("\\neq" ?≠)
 ("\\nequiv" ?≢)
 ("\\newline" ? )
 ("\\nexists" ?∄)
 ("\\ngeq" ?≱)
 ("\\ngeqq" ?≱)
 ("\\ngeqslant" ?≱)
 ("\\ngtr" ?≯)
 ("\\ni" ?∋)
 ("\\nleftarrow" ?↚)
 ("\\nleftrightarrow" ?↮)
 ("\\nleq" ?≰)
 ("\\nleqq" ?≰)
 ("\\nleqslant" ?≰)
 ("\\nless" ?≮)
 ("\\nmid" ?∤)
 ("\\not" ?̸)                            ;FIXME: conflict with "NOT SIGN" ¬.
 ("\\notin" ?∉)
 ("\\nparallel" ?∦)
 ("\\nprec" ?⊀)
 ("\\npreceq" ?⋠)
 ("\\nrightarrow" ?↛)
 ("\\nshortmid" ?∤)
 ("\\nshortparallel" ?∦)
 ("\\nsim" ?≁)
 ("\\nsimeq" ?≄)
 ("\\nsubset" ?⊄)
 ("\\nsubseteq" ?⊈)
 ("\\nsubseteqq" ?⊈)
 ("\\nsucc" ?⊁)
 ("\\nsucceq" ?⋡)
 ("\\nsupset" ?⊅)
 ("\\nsupseteq" ?⊉)
 ("\\nsupseteqq" ?⊉)
 ("\\ntriangleleft" ?⋪)
 ("\\ntrianglelefteq" ?⋬)
 ("\\ntriangleright" ?⋫)
 ("\\ntrianglerighteq" ?⋭)
 ("\\nvDash" ?⊭)
 ("\\nvdash" ?⊬)
 ("\\nwarrow" ?↖)
 ("\\odot" ?⊙)
 ("\\oint" ?∮)
 ("\\ominus" ?⊖)
 ("\\oplus" ?⊕)
 ("\\oslash" ?⊘)
 ("\\otimes" ?⊗)
 ("\\par" ? )
 ("\\parallel" ?∥)
 ("\\partial" ?∂)
 ("\\perp" ?⊥)
 ("\\pitchfork" ?⋔)
 ("\\prec" ?≺)
 ("\\precapprox" ?≾)
 ("\\preceq" ?≼)
 ("\\precnapprox" ?⋨)
 ("\\precnsim" ?⋨)
 ("\\precsim" ?≾)
 ("\\prime" ?′)
 ("\\prod" ?∏)
 ("\\propto" ?∝)
 ("\\qed" ?∎)
 ("\\quad" ? )
 ("\\rangle" ?\⟩) ;; Was ?〉, see bug#12948.
 ("\\rbrace" ?})
 ("\\rbrack" ?\])
 ("\\rceil" ?⌉)
 ("\\rfloor" ?⌋)
 ("\\rightarrow" ?→)
 ("\\rightarrowtail" ?↣)
 ("\\rightharpoondown" ?⇁)
 ("\\rightharpoonup" ?⇀)
 ("\\rightleftarrows" ?⇄)
 ("\\rightleftharpoons" ?⇌)
 ;; ("\\rightparengtr" ?⦔) ;; Was ?〉, see bug#12948.
 ("\\rightrightarrows" ?⇉)
 ("\\rightthreetimes" ?⋌)
 ("\\risingdotseq" ?≓)
 ("\\rtimes" ?⋊)
 ("\\sbs" ?﹨)
 ("\\searrow" ?↘)
 ("\\setminus" ?∖)
 ("\\sharp" ?♯)
 ("\\shortmid" ?∣)
 ("\\shortparallel" ?∥)
 ("\\sim" ?∼)
 ("\\simeq" ?≃)
 ("\\smallamalg" ?∐)
 ("\\smallsetminus" ?∖)
 ("\\smallsmile" ?⌣)
 ("\\smile" ?⌣)
 ("\\spadesuit" ?♠)
 ("\\sphericalangle" ?∢)
 ("\\sqcap" ?⊓)
 ("\\sqcup" ?⊔)
 ("\\sqsubset" ?⊏)
 ("\\sqsubseteq" ?⊑)
 ("\\sqsupset" ?⊐)
 ("\\sqsupseteq" ?⊒)
 ("\\square" ?□)
 ("\\squigarrowright" ?⇝)
 ("\\star" ?⋆)
 ("\\straightphi" ?φ)
 ("\\subset" ?⊂)
 ("\\subseteq" ?⊆)
 ("\\subseteqq" ?⊆)
 ("\\subsetneq" ?⊊)
 ("\\subsetneqq" ?⊊)
 ("\\succ" ?≻)
 ("\\succapprox" ?≿)
 ("\\succcurlyeq" ?≽)
 ("\\succeq" ?≽)
 ("\\succnapprox" ?⋩)
 ("\\succnsim" ?⋩)
 ("\\succsim" ?≿)
 ("\\sum" ?∑)
 ("\\supset" ?⊃)
 ("\\supseteq" ?⊇)
 ("\\supseteqq" ?⊇)
 ("\\supsetneq" ?⊋)
 ("\\supsetneqq" ?⊋)
 ("\\surd" ?√)
 ("\\swarrow" ?↙)
 ("\\therefore" ?∴)
 ("\\thickapprox" ?≈)
 ("\\thicksim" ?∼)
 ("\\to" ?→)
 ("\\top" ?⊤)
 ("\\triangle" ?▵)
 ("\\triangledown" ?▿)
 ("\\triangleleft" ?◃)
 ("\\trianglelefteq" ?⊴)
 ("\\triangleq" ?≜)
 ("\\triangleright" ?▹)
 ("\\trianglerighteq" ?⊵)
 ("\\twoheadleftarrow" ?↞)
 ("\\twoheadrightarrow" ?↠)
 ("\\ulcorner" ?⌜)
 ("\\uparrow" ?↑)
 ("\\updownarrow" ?↕)
 ("\\upleftharpoon" ?↿)
 ("\\uplus" ?⊎)
 ("\\uprightharpoon" ?↾)
 ("\\upuparrows" ?⇈)
 ("\\urcorner" ?⌝)
 ("\\u{i}" ?ĭ)
 ("\\vDash" ?⊨)

 ((lambda (name char)
    ;; "GREEK SMALL LETTER PHI" (which is \phi) and "GREEK PHI SYMBOL"
    ;; (which is \varphi) are reversed in `ucs-names', so we define
    ;; them manually.
    (unless (string-match-p "\\<PHI\\>" name)
      (concat "\\var" (downcase (match-string 1 name)))))
  "\\`GREEK \\([^- ]+\\) SYMBOL\\'")

 ("\\varphi" ?φ)
 ("\\varprime" ?′)
 ("\\varpropto" ?∝)
 ("\\varsigma" ?ς)
 ("\\vartriangleleft" ?⊲)
 ("\\vartriangleright" ?⊳)
 ("\\vdash" ?⊢)
 ("\\vdots" ?⋮)
 ("\\vee" ?∨)
 ("\\veebar" ?⊻)
 ("\\vert" ?|)
 ("\\wedge" ?∧)
 ("\\wp" ?℘)
 ("\\wr" ?≀)

 ("\\Bbb{N}" ?ℕ)			; AMS commands for blackboard bold
 ("\\Bbb{P}" ?ℙ)			; Also sometimes \mathbb.
 ("\\Bbb{R}" ?ℝ)
 ("\\Bbb{Z}" ?ℤ)
 ("--" ?–)
 ("---" ?—)
 ;; We used to use ~ for NBSP but that's inconvenient and may even look like
 ;; a bug where the user finds his ~ key doesn't insert a ~ any more.
 ("\\ " ? )
 ("\\\\" ?\\)
 ("\\mathscr{I}" ?ℐ)			; moment of inertia
 ("\\Smiley" ?☺)
 ("\\blacksmiley" ?☻)
 ("\\Frowny" ?☹)
 ("\\Letter" ?✉)
 ("\\permil" ?‰)
 ;; Probably not useful enough:
 ;; ("\\Telefon" ?☎)			; there are other possibilities
 ;; ("\\Radioactivity" ?☢)
 ;; ("\\Biohazard" ?☣)
 ;; ("\\Male" ?♂)
 ;; ("\\Female" ?♀)
 ;; ("\\Lightning" ?☇)
 ;; ("\\Mercury" ?☿)
 ;; ("\\Earth" ?♁)
 ;; ("\\Jupiter" ?♃)
 ;; ("\\Saturn" ?♄)
 ;; ("\\Uranus" ?♅)
 ;; ("\\Neptune" ?♆)
 ;; ("\\Pluto" ?♇)
 ;; ("\\Sun" ?☉)
 ;; ("\\Writinghand" ?✍)
 ;; ("\\Yinyang" ?☯)
 ;; ("\\Heart" ?♡)
 ("\\dh" ?ð)
 ("\\DH" ?Ð)
 ("\\th" ?þ)
 ("\\TH" ?Þ)
 ("\\lnot" ?¬)
 ("\\ordfeminine" ?ª)
 ("\\ordmasculine" ?º)
 ("\\lambdabar" ?ƛ)
 ("\\celsius" ?℃)
 ;; by analogy with lq, rq:
 ("\\ldq" ?\“)
 ("\\rdq" ?\”)
 ("\\defs" ?≙)				; per fuzz/zed
 ;; ("\\sqrt[3]" ?∛)
 ("\\llbracket" ?\〚) 			; stmaryrd
 ("\\rrbracket" ?\〛)
 ;; ("\\lbag" ?\〚) 			; fuzz
 ;; ("\\rbag" ?\〛)
 ("\\ldata" ?\《) 			; fuzz/zed
 ("\\rdata" ?\》)
 ;; From Karl Eichwalder.
 ("\\glq"  ?‚)
 ("\\grq"  ?‘)
 ("\\glqq"  ?„) ("\\\"`"  ?„)
 ("\\grqq"  ?“) ("\\\"'"  ?“)
 ("\\flq" ?‹)
 ("\\frq" ?›)
 ("\\flqq" ?\«) ("\\\"<" ?\«)
 ("\\frqq" ?\») ("\\\">" ?\»)

 ("\\-" ?­)   ;; soft hyphen

 ("\\textmu" ?µ)
 ("\\textfractionsolidus" ?⁄)
 ("\\textbigcircle" ?⃝)
 ("\\textmusicalnote" ?♪)
 ("\\textdied" ?✝)
 ("\\textcolonmonetary" ?₡)
 ("\\textwon" ?₩)
 ("\\textnaira" ?₦)
 ("\\textpeso" ?₱)
 ("\\textlira" ?₤)
 ("\\textrecipe" ?℞)
 ("\\textinterrobang" ?‽)
 ("\\textpertenthousand" ?‱)
 ("\\textbaht" ?฿)
 ("\\textnumero" ?№)
 ("\\textdiscount" ?⁒)
 ("\\textestimated" ?℮)
 ("\\textopenbullet" ?◦)
 ("\\textlquill" ?\⁅)
 ("\\textrquill" ?\⁆)
 ("\\textcircledP" ?℗)
 ("\\textreferencemark" ?※)
 )

;;; latin-ltx.el ends here
