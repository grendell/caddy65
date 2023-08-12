; repeated blank lines
  

; implied instructions
  clc    ; yes
   sec; abc - xyz !
pha   
  pla   ; 12 def
  SEC
  inX
  
; immediate instructions
  lda #0 ; foo
    ldx  #$42; blah blah
ldy #%10100011   
  eor    #$f0  
 
; address instructions
  lda $00 ; bar
 cmp $0200
   adc  $42
  sbc $6000  

; indexed instructions
  lda $00, x
   ldy $0300,X  
  adc  $42  ,  Y  ; what?

; indirect instructions
  jmp ( $42 )
   JMP ($00) ; jump jump  

; indirect x instructions
  and ($0042, x)
   lda  ( $00,X)
 adc ($07,  x); clc?  
  eor ($00, X)

; indirect y instructions
  and ($0042), y
   lda  ( $00),Y
 adc ($07  ),  y; clc?  
  eor ($00), Y

; control commands
  .proc foo ; I know proc fu
; .byte $00, $00, $00, $00
 .byte $00, $01, $02, $03
  rts
.endproc  

; macros
.macro swap addr1,  addr2
  ldx addr1
  ldy addr2
  stx addr2
  sty addr1
.endmacro

.macro xor  b
eor b
.endmacro

  swap $00, $01
  xor #$42

; named labels
bar :
  baz:
 star: ; twinkle twinkle
buttons:        .res  1       ; Pressed buttons (A|B|Sel|Start|Up|Dwn|Lft|Rgt)

; unnamed labels
:
:; again?
: lda $00
:  ldx $01
  :ldy $02

; relative jumps
  beq :+
  bne  :-
 bcc:--
   bcs :++
:jmp :-; silence warnings
: jmp :--
jmp :---
jmp :----
jmp :-----
jmp :------
jmp :-------

; address formatting
  lda $7
 ldx $300
  ldy $200,  X ; oam

; hex literals
  lda #$04
  cmp  #$42
 adc #$0

; binary literals
  lda #%00000100
  cmp  #%1000010
 adc #%0

; operators
  lda $00 + 4 ; +4
  sta $00+1
  lda #1 <<  2
  lda #$42  >> 3

.linecont +
.define jump_table\
  bar-1,\
  baz - 1, \
   star - 1
.linecont -

; byte operators
  lda #> foo
  ldx #<foo
.define foofoo #<foo , #>foo  

; comma separated lists
  .byte $00, $01, $02, $03
  .byte $04,$05,$06,$07

; parentheses
.repeat 4, K
  stx $0250 + (4 * K) + 1 ; ( I need space )
  stx $0260 + ( 4 * K ) + 1
  stx $0270 + (4 * K)+1
  stx $0280 +(4 * K) + 1
  sta $0290 + (4 * K)
  inx
.endrep

  adc #.HIBYTE( $00 )

; preformatting
  sta $00+1 ; #pre-formatted

; #pre-formatted-start
.byte $00, $01, $02, $03
banana = $742
; #pre-formatted-end

; tab expansion
	sei
	 tax	; scary stuff

var: .res 1
sixteenBitVar: .res 2
  adc sixteenBitVar + 1
  adc var .shr 3

; Movax12's test cases
  lda #$42         & 1    +   3
  test1 = *  +  1
  test2 =  -1
  test3 = ( * + 2 ) *  2
  test4 = 3 + ( -test1 )

.segment "CHARS"
.incbin   "CHR-ROM.bin"