; repeated blank lines

; implied instructions
  clc    ; yes
  sec ; abc - xyz !
  pha
  pla   ; 12 def
  sec
  inx

; immediate instructions
  lda #0 ; foo
  ldx #$42 ; blah blah
  ldy #%10100011
  eor #$f0

; address instructions
  lda $00 ; bar
  cmp $0200
  adc $42
  sbc $6000

; indexed instructions
  lda $00, x
  ldy $0300, x
  adc $42, y  ; what?

; indirect instructions
  jmp ($42)
  jmp ($00) ; jump jump

; indirect x instructions
  and ($0042, x)
  lda ($00, x)
  adc ($07, x) ; clc?
  eor ($00, x)

; indirect y instructions
  and ($0042), y
  lda ($00), y
  adc ($07), y ; clc?
  eor ($00), y

; control commands
.proc foo ; I know proc fu
  .byte $00, $01, $02, $03
  rts
.endproc

; macros
.macro swap addr1, addr2
  ldx addr1
  ldy addr2
  stx addr2
  sty addr1
.endmacro

.macro xor b
  eor b
.endmacro

  swap $00, $01
  xor #$42

; named labels
bar:
baz:
star: ; twinkle twinkle
buttons: .res 1       ; Pressed buttons (A|B|Sel|Start|Up|Dwn|Lft|Rgt)

; unnamed labels
:
: ; again?
: lda $00
: ldx $01
: ldy $02

; relative jumps
  beq :+
  bne :-
  bcc :--
  bcs :++
: jmp :- ; silence warnings
: jmp :--
  jmp :---
  jmp :----
  jmp :-----
  jmp :------
  jmp :-------

; address formatting
  lda $07
  ldx $0300
  ldy $0200, x ; oam

; hex literals
  lda #$4
  cmp #$42
  adc #$0

; binary literals
  lda #%00000100
  cmp #%01000010
  adc #%00000000

; operators
  lda $00 + 4 ; +4
  sta $00 + 1

; comma separated lists
  .byte $00, $01, $02, $03
  .byte $04, $05, $06, $07

; parentheses
.repeat 4, K
  stx $0250 + (4 * K) + 1 ; ( I need space )
  stx $0260 + (4 * K) + 1
  stx $0270 + (4 * K) + 1
  stx $0280 + (4 * K) + 1
  sta $0290 + (4 * K)
  inx
.endrep

  adc #.hibyte($00)

; preformatting
  sta $00+1 ; #pre-formatted

; #pre-formatted-start
.byte $00, $01, $02, $03
banana = $742
; #pre-formatted-end

; tab expansion
  sei
  tax  ; scary stuff

var: .res 1
sixteenBitVar: .res 2
  adc sixteenBitVar + 1
  adc var .shr 3

.segment "CHARS"
.incbin "CHR-ROM.bin"