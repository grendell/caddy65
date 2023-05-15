# Caddy65
* Source code formatter for [ca65](https://cc65.github.io/)-compatible 6502 assembly.
## Important Note!
* caddy65 is currently experimental. As such, please take care to back up your source code before use.
## Reasons to Use a Code Formatter
* Clean, consistant code makes a better first impression.
* Removes clutter from source control. All modifications are meaningful.
* Removes clutter from code review. Feedback should be focused on implementation.
* Removes the mental burden of formatting while programming. Focus on the task at hand.
* Removes some sources of interpersonal awkwardness. Facilitates collaboration.
* Enables global formatting changes. Simplifies adaptation.
## Requirements
* Building
    * POSIX Environment (Linux, MacOS, WSL)
        * Cygwin, MinGW, and GnuWin32 should also work, but I have been unable to verify compatibility.
    * make
    * clang
* Testing
    * ca65
    * diff
* General Use
    * Source code must not exceed 4096 characters per line.
## Building
* Simply run `make`
## Usage
* `./caddy65 [-c config.cfg] <source.s>`
## Unit Tests
* Simply run `make test`

# Features
## Preformatted Tags
* If caddy65 produces unwanted results, you can disable formatting for single lines or code blocks using the following tags in comments:
    * `#pre-formatted`
    * `#pre-formatted-start`
    * `#pre-formatted-end`
### Examples
```
adc sixteenBitVar+1 ; #pre-formatted
```
```
; #pre-formatted-start
.byte $0,$1,$2,$3
adc sixteenBitVar+1
; #pre-formatted-end
```
## Caddy65 Configuration File
* Allows for the disabling of specific rules.
* By default, all rules are enabled.
* If the `-c` flag is not specified, caddy65 will attempt to load `caddy65.cfg` in the current working directory.
* Disabling one or more rules might have unintended consequences, so use with caution!
    * For example, disabling the `indexedInstruction` rule to allow `adc $42,x` will still result in `adc $42, x` due to the `commaSpacing` rule enforcement.
    * Please see the debug flag description below should you encounter unexpected behavior.
### Examples
* `addressFormatting: enabled`
* `hexLiteralFormatting: disabled`

If neither of these features is used, caddy65 applies the following rules to each source line.

# Rules
## Only Comment
* If the source line is only a comment:
    * If the comment starts at column 0, it will not be indented.
    * Otherwise enforces indention.
### Examples
* `; This is a comment.`
* `  ; This is also a comment.`
* `;   This is also a comment.`
## Trim Leading Space
* Trims leading whitespace from the source line.
* Indention considerations occur later.
## Trim Trailing Space
* Trims trailing whitespace from the source line.
* Checks for blank lines.
* Enforces maximum of one blank line between source code.
## Tab Expansion
* Converts all tab characters to the `indention` string (defaults to two spaces).
## Address Formatting
* Enforces lowercase hexadecimal digits.
* Enforces zero-padding to two or four digits.
* Does not attempt to detect operations that could be converted to Zero Page.
### Examples
* `  adc $0f`
* `  adc $0742`
## Hexadecimal Literal Formatting
* Enforces lowercase hexadecimal digits.
* Trims zero-padding.
### Examples
* `  adc #$f`
## Binary Literal Formatting
* Enforces eight digits.
### Examples
* `  and #%00001100`
## Open Parenthesis Spacing
* Enforces no space before open parenthesis character.
* Enforces no space after open parenthesis character.
* Does not modify comments.
### Examples
* `  adc #.hibyte($42)`
## Close Parenthesis Spacing
* Enforces no space before close parenthesis character.
* Enforces no space after close parenthesis character.
* Does not modify comments.
### Examples
* `  adc #.hibyte($42)`
## Operator Formatting
* Enforces lowercase operator names.
* Enforces one and only one space before operator.
* Enforces one and only one space after operator.
* Does not modify comments.
### Examples
* `  adc sixteenBitVar + 1`
* `  adc var .shr 3`
## Comma Spacing
* Enforces no space before comma character.
* Enforces one and only one space after comma character.
* Does not modify comments.
### Examples
* `  .byte $00, $01, $02, $03`
## Control Command
* Enforces lowercase control command names.
* Enforces one and only one space between control commands and arguments.
* Enforces indention for the following commands:
    * `.asciiz`
    * `.addr`
    * `.byt`
    * `.byte`
    * `.dbyt`
    * `.dword`
    * `.lobytes`
    * `.hibytes`
    * `.word`
* Otherwise enforces no indention.
### Examples
* `.proc reset`
* `.incbin "CHR-ROM.bin"`
* `.repeat 4, K`
## Macro Definition
* Enforces one and only one space between `.macro` control command and macro name.
* Enforces one and only one space between macro name and optional parameters.
### Examples
* `.macro inc16 addr`
## Macro Instance
* Enforces one and only one space between macro name and optional parameters.
* Enforces indention.
### Examples
* `  inc16 $42`
## Named Label
* Enforces no space between label name and colon character.
* Enforces one and only one space between label and optional arguments.
* Enforces no indention.
### Examples
* `nmi:`
* `buttons: .res 1`
## Unnamed Label
* Enforces optional indention via the `labeledIndention` string.
### Examples
* `:`
* `: rts`
## Implied Instruction
* Enforces lowercase instruction names.
* Enforces indention.
### Examples
* `  clc`
## Immediate Instruction
* Enforces lowercase instruction names.
* Enforces one and only one space between instruction and literal argument.
* Enforces indention.
### Examples
* `  adc #$42`
## Address Instruction
* Enforces lowercase instruction names.
* Enforces one and only one space between instruction and address argument.
* Enforces indention.
### Examples
* `  adc $42`
## Indexed Instruction
* Enforces lowercase instruction names.
* Enforces one and only one space between instruction and address argument.
* Enforces no space between address argument and comma character.
* Enforces one and only one space between comma character and index register name.
* Enforces lowercase index register name.
* Enforces indention.
### Examples
* `  adc $42, x`
## Indirection Instruction
* Enforces lowercase instruction names.
* Enforces one and only one space between instruction and open parenthesis character.
* Enforces no space between open parenthesis character and address argument.
* Enforces no space between address argument and close parenthesis character.
* Enforces indention.
### Examples
* `  jmp ($42)`
## Indexed Indirect Instruction
* Enforces lowercase instruction names.
* Enforces one and only one space between instruction and open parenthesis character.
* Enforces no space between open parenthesis character and address argument.
* Enforces no space between address argument and comma character.
* Enforces one and only one space between comma character and `x` register name.
* Enforces no space between `x` register name and close parenthesis character.
* Enforces lowercase `x` register name.
* Enforces indention.
### Examples
* `  adc ($42, x)`
## Indirect Indexed Instruction
* Enforces lowercase instruction names.
* Enforces one and only one space between instruction and open parenthesis character.
* Enforces no space between open parenthesis character and address argument.
* Enforces no space between address argument and close parenthesis character.
* Enforces no space between close parenthesis character and comma character.
* Enforces one and only one space between comma character and `y` register name.
* Enforces lowercase `y` register name.
* Enforces indention.
### Examples
* `  adc ($42), y`
## Relative Instruction
* Enforces lowercase instruction names.
* Enforces one and only one space between instruction and colon character.
* Enforces indention.
### Examples
* `  bcc :-`
* `  bcs :++`
## Comment Spacing
* Enforces lowercase instruction names.
* Enforces at least one space between source code and semicolon character.
* Enforces at least one space between semicolon character and comment.
* Does not modify additional spacing before or after the semicolon character.
### Examples
* `  clc      ; Clear the carry bit,`
* `  adc #$42 ;  then add 0x42 to register a.`
# FAQs
### Why "caddy65?"
* This tool takes your jumble of golf clubs (source code) and produces an organized (standardized) golf bag, as any good caddy should do.
### What if I prefer [alternative indention]?
* Modifying the `indention` and `labeledIndention` strings in the source code should suit your needs.
### I want to add a rule. How do I get started?
* Be sure to add the rule to the `rule_t` enum, `ruleNames` array, and `patterns` array.
* The rule will be applied automatically in the order specified by the `rule_t` enum.
* Once you're ready, consider [contributing](https://github.com/grendell/caddy65/pulls) your rule to the project!
### I found a bug. How do I report it?
* Please, _please_ open an issue with as much info as possible [here](https://github.com/grendell/caddy65/issues).
* At a minimum, please include sample input and expected output.
### Why did you use POSIX Regex for this and not [smarter solution]?
* I wanted to keep this project in c99 for portability, familiarity, and an excuse to learn the POSIX Regex API.
* Beyond that, I probably didn't know about the suggested solution! Feel free to let me know by opening an [issue](https://github.com/grendell/caddy65/issues) and I'll look into it.
### How can I debug what rules caddy65 is applying?
* There are two debug flags, `verbose` and `pedantic`, in the source code which can be set to 1 to increase logging levels.