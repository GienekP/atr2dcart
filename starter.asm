;-----------------------------------------------------------------------		
;
; ATR2DCART starter for D-Cart
; (c) 2024 GienekP
;
;-----------------------------------------------------------------------

TMP     = $2C
RAMPROC = $0100
D5PROC  = $D500

;-----------------------------------------------------------------------

CASINI  = $02
BOOTQ   = $09
DOSVEC  = $0A
DOSINI  = $0C
BUFADR  = $15
CRITIC  = $42
RAMTOP  = $6A
DMACTLS = $022F
COLDST  = $0244
DSKTIM  = $0246
DSCTLN  = $02D5
MEMTOP  = $02E5
DVSTAT  = $02EA
DDEVIC  = $0300
DUNIT   = $0301
DCMND   = $0302
DSTATS  = $0303
DBUFA   = $0304
DTIMLO  = $0306
DBYT    = $0308
DBYTLO  = $0308
DBYTHI  = $0309
DAUX1	= $030A
DAUX2	= $030B
BASICF  = $03F8
GINTLK  = $03FA
TRIG3   = $D013
CONSOL  = $D01F
IRQEN   = $D20E
IRQST   = $D20E
PORTB   = $D301
DMACTL  = $D400
VCOUNT  = $D40B
NMIEN   = $D40E
BOOT    = $C58B
JSIOINT = $E459
RESETWM = $E474
RESETCD = $E477
EDOPN   = $EF94

;-----------------------------------------------------------------------		
; D-CART CARTRIDGE

		OPT h-f+
		
;-----------------------------------------------------------------------		
; SectorMap generated by SectorMap.c

		ORG $A000

		INS "SectorMap.dta"

;-----------------------------------------------------------------------		
; $D5XX MiniBank

		ORG $B500

MAINSIO
.local D5WIN,D5PROC
.def :BACKADR = (D5PROC+BACK-START)
;--------------------------
START	lda DUNIT
		cmp #$01
		beq FSIO
;--------------------------
		lda PORTB
		pha
 		ora #$01
		sta PORTB
		jsr	JSIOINT
		pla
		sta PORTB
		ldy DSTATS
		rts
;--------------------------
		dta 'FD1I'
;--------------------------
FDSKINT	lda #$31
		sta DDEVIC
		lda DSKTIM
		ldx DCMND
		cpx #$21
		beq STM
		lda #$07
STM		sta DTIMLO
		ldx #$40
		lda DCMND
		cmp #$50
		beq WRT
		cmp #$57
		bne READ
WRT		ldx #$80
READ	cmp #$53
		bne DSL
		lda #>DVSTAT
		sta DBUFA
		lda #<DVSTAT
		sta DBUFA+1
		ldy #$04
		lda #$00
		beq SPM
DSL		ldy DSCTLN
		lda DSCTLN+1
SPM		stx DSTATS
		sty DBYT
		sta DBYT+1
;--------------------------
FSIO	ldy #$00
		lda DCMND
		cmp #$52
		beq SECREAD
		cmp #$57
		beq STATOK
		cmp #$50
		beq STATOK
		cmp #$53
		bne UNKWCMD		
		lda #<DVSTAT
		sta DBUFA
		lda #>DVSTAT
		sta DBUFA+1
		ldx #$03
@		lda D1STAT,x
		sta DVSTAT,x
		dex
		bpl @-			
STATOK	iny
UNKWCMD	sty DSTATS
		jmp BACKADR
;--------------------------
SECREAD	ldy #$01
		sty DSTATS
		
		lda TMP
		pha
		lda TMP+1
		pha
		lda TMP+2
		pha
		lda TMP+3
		pha	
		
		lda #$77
LICNT	cmp VCOUNT
		bne LICNT
		
		sty CRITIC
		sty $D500
		jsr EXTRA

CPYSEC	sta $D500,X
		lda (TMP),Y
		sta $D5FF
		sta (TMP+2),Y
		dey
		cpy #$FF
		bne CPYSEC
		
		pla
		sta TMP+3
		pla
		sta TMP+2
		pla
		sta TMP+1
		pla
		sta TMP

BACK	sta $D5FF
		lda TRIG3
		sta GINTLK
		lda #$00
		sta CRITIC
		ldy DSTATS
		rts
;--------------------------
D1STAT  dta $18,$ff,$01,$00		; 18 - 128 sector / 38 - 256 sector
;--------------------------
.end
ENDMAIN
;--------------------------
; DIRECT RESTART CARTRIDGE 'RUN $D5FA'

		:19 dta $EA
		sta $D500
		jmp RESETCD
		
		ORG $B600
		dta $FF
	
;-----------------------------------------------------------------------		
; SET SOURCE AND DESTINATION 

		ORG $BD00

EXTRA	ldy #$00
		lda DAUX1
		asl 
		sta TMP
		lda DAUX2
		rol
		and #$1F
		clc
		adc #$A0
		sta TMP+1		
		lda (TMP),Y
		sta TMP+2
		iny
		lda (TMP),Y
		sta TMP+3		
		dey
		sty TMP
		clc
		adc #$A0
		sta TMP+1
		
		lda TMP+2
		and #$7F
		tax			; Bank -> X		
		lda DBUFA
		sta TMP+2
		lda DBUFA+1
		sta TMP+3	
		ldy DBYTLO
		dey
		rts		
;-----------------------------------------------------------------------		
; CARTRUN ROUTINE

		ORG $BD80
	
BEGIN	jsr BASDIS	; Disable BASIC & reopen editor
		jsr IRQDIS	; Disable IRQ
		jsr ROM2RAM	; Copy ROM to RAM
		jsr SETNVCT	; Set new vectors
		jsr IRQENB	; Enable IRQ
		jmp BYEBYE	; Back to OS
;-----------------------------------------------------------------------		
; DISABLE BASIC AND REOPEN EDITOR

BASDIS	lda #01
		sta CRITIC
		lda PORTB
		ora #$02
		sta PORTB
		lda #$01
		sta BASICF
		lda #$1F
		sta MEMTOP
		lda #$BC
		sta MEMTOP+1
		lda #$C0
		sta RAMTOP
		ldx #(CLPRE-CLPRS-1)
@		lda CLPRS,X
		sta RAMPROC,X
		dex
		bpl @-
		jsr RAMPROC
		lda #$00
		sta DMACTLS
		sta DMACTL
		ldx #(CLPRE-CLPRS-1)
@		sta RAMPROC,X
		dex
		bpl @-		
		lda #<RESETCD
		sta DOSVEC
		sta DOSINI
		sta CASINI	
		lda #>RESETCD
		sta DOSVEC+1
		sta DOSINI+1
		sta CASINI+1
		lda #$03
		sta BOOTQ
		lda #$00
		sta COLDST
		rts
;-----------------------------------------------------------------------
; Clear RAM under CART

CLPRS	sta $D5FF
		lda #$A0
		sta TMP+1
		lda #$00
		sta TMP
		ldy #$00
NEWPAG	lda #$00
@		sta (TMP),Y
		iny
		bne @-
		inc TMP+1
		lda TMP+1
		cmp #$C0
		bne NEWPAG
		jsr EDOPN
		lda #$00
		sta TMP
		sta TMP+1
		sta DMACTL
		sta $D500
		rts
CLPRE
;-----------------------------------------------------------------------		
; IRQ ENABLE

IRQENB	lda #$40
		sta NMIEN
		lda #$F7
		sta IRQST
		lda #$22
		sta DMACTLS
		cli
		rts
;-----------------------------------------------------------------------		
; IRQ DISABLE

IRQDIS	sei	
		lda #$00
		sta DMACTL
		sta NMIEN
		sta IRQEN
		sta IRQST
		rts
;-----------------------------------------------------------------------		
; COPY ROM TO RAM
	
ROM2RAM	lda #$C0
		sta TMP+1
		ldy #$00
		sty TMP
		ldx #$FF	
CPOS	stx PORTB
		lda (TMP),Y
		dex
		stx PORTB
		sta (TMP),Y
		inx
		iny
		bne CPOS
		inc TMP+1
		lda TMP+1
		cmp #$D0
		bne OSOK
		lda #$D8
		sta TMP+1
OSOK	cmp #$00
		bne CPOS
		lda #$00
		sta TMP
		sta TMP+1
		lda #$FE
		sta PORTB
		rts
;-----------------------------------------------------------------------		
; SET NEW VETORS

SETNVCT	lda #<D5PROC
		sta JSIOINT+1
		lda #>D5PROC
		sta JSIOINT+2
		rts		
;-----------------------------------------------------------------------		
; LEAVE CART SPACE
		
BYEBYE	lda #>RESETCD
		pha
		lda #<RESETCD-1
		pha
		lda #>BOOT
		pha
		lda #<BOOT-1
		pha
		lda #$22
		sta DMACTLS
		jmp BACKADR
;-----------------------------------------------------------------------		
; 	CLONE FOR ALL BANKS

		ORG $BF00
		
		:$DA dta $FF
;-----------------------------------------------------------------------		
; INITCART ROUTINE

		ORG $BFDA
			
INIT	lda CONSOL
		and #$02
		bne CONTIN
		ldx #(CONTIN-STANDR-1)
@		lda STANDR,X
		sta RAMPROC,x
		dex
		bpl @-
		jmp RAMPROC
		
STANDR  sta $D5FF
		jmp RESETCD
		
CONTIN	sta $D500
		rts
;-----------------------------------------------------------------------		
; BANK NUMBER

		ORG $BFF9
		
BANKNUM	dta $00
;-----------------------------------------------------------------------		
; CARTRIDGE HEADER

		ORG $BFFA
		
		dta <BEGIN, >BEGIN, $00, $04, <INIT, >INIT
;-----------------------------------------------------------------------		
