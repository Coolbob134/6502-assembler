; Hello world program
; Uses address $FF as output
.BYTE $01
,Loop:
LDA $00A0
INC $03
STA $FF
JMP Loop


.ORG $00A0
.ASCII "Hello world!"
