; -- setup.nsi --
; Created: JohnE, 2008-03-15


; DISCLAIMER:
; The author(s) of this file's contents release it into the public domain,
; without express or implied warranty of any kind. You may use, modify, and
; redistribute this file freely.


OutFile "output\tdm-gcc-webdl.exe"
!packhdr "exehead.tmp" 'upx --best exehead.tmp'

!include "main.nsh"
