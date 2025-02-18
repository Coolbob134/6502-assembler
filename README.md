# 6502-assembler
A barebones 6502 assembler contained in a single C header file.
## Syntax

Syntax is similar to other 6502 assemblers, with a few exceptions:
- Lables and variable declarations have to start with `,`
  - Lables need to be in their own line and end with `:`
- All preprocessor directives are in caps.
```
;This is a comment
,label1:        ;Lable
,var1 = $400F   ;Variable assignment

JMP label1      ;Lable usage
LDA var1        ;Variable usage
```

## Preprocessor
The assembler features 3 preprocessor directives, `BYTE`, `ORG`, and `ASCII`

- `ORG` works the same as with other assemblers.
- `BYTE` works similar, except it can only write hex data.
- `ASCII` replaces the text functionality of `BYTE`.

```
.ORG   $0012
.BYTE  $03, $FF,$21
.ASCII "Hello world"
```
