#include <ctype.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char * const temp = ".temp.s";
const char * const indention = "  ";
const char * const labeledIndention = ": ";
const char * const preformatted = "#pre-formatted";
const char * const preformattedStart = "#pre-formatted-start";
const char * const preformattedEnd = "#pre-formatted-end";
const char * const defaultConfig = "caddy65.cfg";

const int verbose = 0;
const int pedantic = 0;

char scratch[4096];
int indentionSize;

#define spacing "([[:space:]]*)"
#define comment spacing ";" spacing "(.?)"

#define leadingSpace "^" spacing
#define trailingSpace spacing "$"
#define tab "(\t)"

#define bitwise "^(and|eor|ora)"

#define address "[^#][$]([[:xdigit:]]+)"
#define hexLiteral "[#][$]([[:xdigit:]]+)"
#define binaryLiteral "[#][%]([01]{1,8})"

#define openParen spacing "[(]" spacing
#define closeParen spacing "[)]" spacing
#define comma spacing "," spacing

#define operator "([-+*/&|^=<>\\]|"\
                 "<<|>>|<>|<=|>=|&&|[|][|]|"\
                 "[.](mod|bitand|bitor|bitxor|shl|shr|and|or|xor))"
#define byteOperator spacing "([#][<>])" spacing

#define argStart "([$%\"_[:alnum:]])"
#define control "[.]([[:alpha:]][[:alnum:]]*)" spacing argStart "?"
#define symbol "([_[:alpha:]]([_[:alnum:]]|(::))*)"
#define macroDef "^[.]macro" spacing "([^[:space:]]+)(" spacing "([^,[:space:]]*))"
#define macroUse "^" symbol "((([[:space:]]+)" argStart ")|$)"
#define label "^([@_[:alpha:]][_[:alnum:]]*)" spacing ":" spacing "([^+-]|$)"
#define unnamed "^:" spacing
#define relative ":([+]+|[-]+)"
#define index spacing "," spacing "([XYxy])"
#define indirection "[(]" spacing "([^,)[:space:]]+)" spacing "[)]"
#define indirectionX "[(]" spacing "([^,)[:space:]]+)" spacing "," spacing "([Xx])" spacing "[)]"
#define indirectionY "[(]" spacing "([^,)[:space:]]+)" spacing "[)]" spacing "," spacing "([Yy])"
#define instruction "^(adc|and|asl|bcc|bcs|beq|bit|bmi|bne|bpl|brk|bvc|bvs|clc|"\
                      "cld|cli|clv|cmp|cpx|cpy|dec|dex|dey|eor|inc|inx|iny|jmp|"\
                      "jsr|lda|ldx|ldy|lsr|nop|ora|pha|php|pla|plp|rol|ror|rti|"\
                      "rts|sbc|sec|sed|sei|sta|stx|sty|tax|tay|tsx|txa|txs|tya)"

#define matchLength(n) (match[n].rm_eo - match[n].rm_so)

typedef enum {
    onlyComment,
    trimLeading,
    trimTrailing,
    tabExpansion,
    bitwiseInstruction,
    addressFormatting,
    hexLiteralFormatting,
    binaryLiteralFormatting,
    openParenSpacing,
    closeParenSpacing,
    operatorFormatting,
    byteOperatorFormatting,
    commaSpacing,
    controlCommand,
    macroDefinition,
    macroInstance,
    namedLabel,
    unnamedLabel,
    impliedInstruction,
    immediateInstruction,
    addressInstruction,
    indexedInstruction,
    indirectInstruction,
    indirectXInstruction,
    indirectYInstruction,
    relativeInstruction,
    commentSpacing,
    numRules,
} rule_t;

const char * ruleNames[numRules] = {
    "onlyComment",
    "trimLeading",
    "trimTrailing",
    "tabExpansion",
    "bitwiseInstruction",
    "addressFormatting",
    "hexLiteralFormatting",
    "binaryLiteralFormatting",
    "openParenSpacing",
    "closeParenSpacing",
    "operatorFormatting",
    "byteOperatorFormatting",
    "commaSpacing",
    "controlCommand",
    "macroDefinition",
    "macroInstance",
    "namedLabel",
    "unnamedLabel",
    "impliedInstruction",
    "immediateInstruction",
    "addressInstruction",
    "indexedInstruction",
    "indirectInstruction",
    "indirectXInstruction",
    "indirectYInstruction",
    "relativeInstruction",
    "commentSpacing",
};

const char * patterns[numRules] = {
    "^" comment,
    leadingSpace,
    trailingSpace,
    tab,
    bitwise,
    address,
    hexLiteral,
    binaryLiteral,
    openParen,
    closeParen,
    "[^(#:+-]" spacing operator spacing "([^[:space:]]?)",
    byteOperator,
    comma,
    control,
    macroDef,
    macroUse,
    label,
    unnamed,
    instruction spacing "(;|$)",
    instruction spacing "(#)",
    instruction spacing "([$])",
    instruction spacing "([^,[:space:]]+)" index,
    instruction spacing indirection,
    instruction spacing indirectionX,
    instruction spacing indirectionY,
    instruction spacing relative,
    comment,
};

typedef enum {
    appendNewline = 1 << 0,
    prependIndention = 1 << 1,
    prependLabel = 1 << 2,
    bitwiseOperation = 1 << 3,
    done = 1 << 4,
    omit = 1 << 5,
} flags_t;

typedef enum {
    notApplied,
    compliant,
    applied,
    error,
} result_t;

const char * resultNames[] = {
    "notApplied",
    "compliant",
    "applied",
    "error",
};

void printError(int errorCode, regex_t * regex) {
    regerror(errorCode, regex, scratch, 4096);
    fprintf(stderr, "%s\n", scratch);
}

void printLineResult(FILE * output, const char * source, flags_t flags, int skipped) {
    int len = strlen(source);

    if (skipped) {
        if (verbose) {
            printf("preformatted:\n%s", source);
        }
        fprintf(output, "%s", source);
    } else if (flags & omit) {
        if (verbose) {
            printf("<omitted blank line>\n");
        }
    } else {
        sprintf(scratch, "%s%.*s%s",
               flags & prependLabel ? ":" : "",
               flags & prependLabel ? indentionSize - 1 : indentionSize,
               len && flags & (prependLabel | prependIndention) ? indention : "",
               source);

        if (verbose) {
            printf("\"%s\"\n", scratch);
        }

        fprintf(output, "%s%s",
            scratch,
            flags & appendNewline ? "\n" : "");
    }
}

void printRuleResult(rule_t rule, result_t result, flags_t flags) {
    if (pedantic) {
        printf("%24.24s: %10.10s, flags: %d\n", ruleNames[rule], resultNames[result], flags);
    }
}

void cleanup(FILE * input, FILE * output, regex_t * regex) {
    fclose(input);
    fclose(output);

    for (int i = 0; i < numRules; ++i) {
        regfree(regex + i);
    }
}

result_t applyRule(rule_t rule, char * const source, const flags_t flags, regex_t * regex) {
    regmatch_t match[16];
    int status = regexec(regex, source, 16, match, 0);

    if (!status) {
        switch (rule) {
            case onlyComment: {
                return compliant;
            }
            case trimLeading: {
                int s1 = matchLength(1);

                if (s1) {
                    sprintf(scratch, "%s", source + match[1].rm_eo);
                    strcpy(source, scratch);
                    return applied;
                }

                return compliant;
            }
            case trimTrailing: {
                int s1 = matchLength(1);

                if (s1) {
                    source[match[1].rm_so] = '\0';
                    return applied;
                }

                return compliant;
            }
            case tabExpansion: {
                sprintf(scratch, "%.*s%s%s",
                    (int) match[1].rm_so, source,
                    indention,
                    source + match[1].rm_eo);
                strcpy(source, scratch);

                result_t result = applied;
                result_t next = applyRule(rule, source + match[1].rm_so + indentionSize, flags, regex);
                return next > result ? next : result;
            }
            case bitwiseInstruction: {
                return compliant;
            }
            case addressFormatting: {
                result_t result = compliant;

                for (int i = match[1].rm_so; i < match[1].rm_eo; ++i) {
                    if (isupper(source[i])) {
                        source[i] = tolower(source[i]);
                        result = applied;
                    }
                }

                int s1 = matchLength(1);
                if (s1 & 1) {
                    sprintf(scratch, "%.*s0%s",
                        (int) match[1].rm_so, source,
                        source + match[1].rm_so);
                    strcpy(source, scratch);
                    result = applied;
                }

                result_t next = applyRule(rule, source + match[1].rm_so, flags, regex);
                return next > result ? next : result;
            }
            case hexLiteralFormatting: {
                result_t result = compliant;

                for (int i = match[1].rm_so; i < match[1].rm_eo; ++i) {
                    if (isupper(source[i])) {
                        source[i] = tolower(source[i]);
                        result = applied;
                    }
                }

                if (flags & bitwiseOperation) {
                    int s1 = matchLength(1);

                    if (s1 & 1) {
                        sprintf(scratch, "%.*s0%s",
                            (int) match[1].rm_so, source,
                            source + match[1].rm_so);
                        strcpy(source, scratch);
                        result = applied;
                    }
                } else {
                    int offset = match[1].rm_so;
                    while (source[offset] == '0' && isxdigit(source[offset + 1])) {
                        ++offset;
                    }

                    if (offset != match[1].rm_so) {
                        sprintf(scratch, "%.*s%s",
                            (int) match[1].rm_so, source,
                            source + offset);
                        strcpy(source, scratch);
                        result = applied;
                    }
                }

                result_t next = applyRule(rule, source + match[1].rm_so, flags, regex);
                return next > result ? next : result;
            }
            case binaryLiteralFormatting: {
                result_t result = compliant;
                int s1 = matchLength(1);

                if (s1 != 8) {
                    sprintf(scratch, "%.*s%.*s%s",
                        (int) match[1].rm_so, source,
                        8 - s1, "00000000",
                        source + match[1].rm_so);
                    strcpy(source, scratch);
                    result = applied;
                }

                result_t next = applyRule(rule, source + match[1].rm_so, flags, regex);
                return next > result ? next : result;
            }
            case openParenSpacing: {
                char * commentStart = strchr(source, ';');
                if (commentStart && source + match[0].rm_so >= commentStart) {
                    return notApplied;
                }

                result_t result = compliant;
                int s1 = matchLength(1);
                int s2 = matchLength(2);

                if (s1 || s2) {
                    sprintf(scratch, "%.*s(%s",
                        (int) match[1].rm_so, source,
                        source + match[2].rm_eo);
                    strcpy(source, scratch);
                    result = applied;
                }

                result_t next = applyRule(rule, source + match[1].rm_eo + 1, flags, regex);
                return next > result ? next : result;
            }
            case closeParenSpacing: {
                char * commentStart = strchr(source, ';');
                if (commentStart && source + match[0].rm_so >= commentStart) {
                    return notApplied;
                }

                result_t result = compliant;
                int s1 = matchLength(1);
                int s2 = matchLength(2);

                if (s1 || s2) {
                    sprintf(scratch, "%.*s)%s",
                        (int) match[1].rm_so, source,
                        source + match[2].rm_eo);
                    strcpy(source, scratch);
                    result = applied;
                }

                result_t next = applyRule(rule, source + match[1].rm_eo + 1, flags, regex);
                return next > result ? next : result;
            }
            case operatorFormatting: {
                char * commentStart = strchr(source, ';');
                if (commentStart && source + match[0].rm_so >= commentStart) {
                    return notApplied;
                }

                int inQuote = 0;
                for (int i = 0; i < match[0].rm_so; ++i) {
                    if (source[i] == '"') {
                        inQuote ^= 1;
                    }
                }

                if (inQuote) {
                    char * next = source + match[2].rm_eo;
                    while (*next && *next != '"') {
                        ++next;
                    }

                    if (*next) {
                        return applyRule(rule, next, flags, regex);
                    }

                    fprintf(stderr, "rule %d failed to parse quoted section\n", rule);
                    return error;
                }

                result_t result = compliant;
                int s2 = matchLength(2);

                if (s2 > 2) {
                    for (int i = match[2].rm_so; i < match[2].rm_eo; ++i) {
                        if (isupper(source[i])) {
                            source[i] = tolower(source[i]);
                            result = applied;
                        }
                    }
                }

                int s1 = matchLength(1);
                int s4 = matchLength(4);
                int s5 = matchLength(5);

                if (s1 != 1 || (s4 != 1 && s5) || (s4 && !s5)) {
                    sprintf(scratch, "%.*s %.*s%.*s%s",
                        (int) match[1].rm_so, source,
                        s2, source + match[2].rm_so,
                        s5, " ",
                        source + match[4].rm_eo);
                    strcpy(source, scratch);
                    result = applied;
                }

                result_t next = applyRule(rule, source + match[1].rm_so + s2 + 2, flags, regex);
                return next > result ? next : result;
            }
            case byteOperatorFormatting: {
                result_t result = compliant;
                int s1 = matchLength(1);
                int s3 = matchLength(3);

                if (s1 != 1 || s3) {
                    sprintf(scratch, "%.*s %.*s%s",
                        (int) match[1].rm_so, source,
                        2, source + match[2].rm_so,
                        source + match[2].rm_eo + 1);
                    strcpy(source, scratch);
                    result = applied;
                }

                result_t next = applyRule(rule, source + match[1].rm_so + 3, flags, regex);
                return next > result ? next : result;
            }
            case commaSpacing: {
                char * commentStart = strchr(source, ';');
                if (commentStart && source + match[0].rm_so >= commentStart) {
                    return notApplied;
                }

                result_t result = compliant;
                int s1 = matchLength(1);
                int s2 = matchLength(2);

                if (s1 || s2 != 1) {
                    sprintf(scratch, "%.*s, %s",
                        (int) match[1].rm_so, source,
                        source + match[2].rm_eo);
                    strcpy(source, scratch);
                    result = applied;
                }

                result_t next = applyRule(rule, source + match[1].rm_eo + 1, flags, regex);
                return next > result ? next : result;
            }
            case controlCommand: {
                result_t result = compliant;

                for (int i = match[1].rm_so; i < match[1].rm_eo; ++i) {
                    if (isupper(source[i])) {
                        source[i] = tolower(source[i]);
                        result = applied;
                    }
                }

                int s2 = matchLength(2);
                int s3 = matchLength(3);

                if (s2 > 1 && s3) {
                    sprintf(scratch, "%.*s %s",
                        (int) match[2].rm_so, source,
                        source + match[3].rm_so);
                    strcpy(source, scratch);
                    result = applied;
                }

                return result;
            }
            case macroDefinition: {
                int s1 = matchLength(1);
                int s3 = matchLength(3);
                int s4 = matchLength(4);

                if (s1 != 1 || (s3 && s4 != 1)) {
                    int s2 = matchLength(2);

                    sprintf(scratch, "%.*s %.*s%s%s",
                        (int) match[1].rm_so, source,
                        s2, source + match[2].rm_so,
                        s3 ? " " : "",
                        source + match[5].rm_so);
                    strcpy(source, scratch);
                    return applied;
                }

                return compliant;
            }
            case macroInstance: {
                int s4 = matchLength(4);
                int s6 = matchLength(6);

                if (s4 && s6 != 1) {
                    int s1 = matchLength(1);

                    sprintf(scratch, "%.*s %s",
                        s1, source,
                        source + match[7].rm_so);
                    strcpy(source, scratch);
                    return applied;
                }

                return compliant;
            }
            case namedLabel: {
                int s2 = matchLength(2);
                int s3 = matchLength(3);
                int s4 = matchLength(4);

                if (s2 || (s3 != 1 && s4)) {
                    int s1 = matchLength(1);

                    sprintf(scratch, "%.*s:%s%s",
                        s1, source,
                        s4 ? " " : "",
                        source + match[4].rm_so);
                    strcpy(source, scratch);
                    return applied;
                }

                return compliant;
            }
            case unnamedLabel: {
                sprintf(scratch, "%s", source + match[1].rm_eo);
                strcpy(source, scratch);

                return applied;
            }
            case impliedInstruction: {
                result_t result = compliant;

                for (int i = 0; i < 3; ++i) {
                    if (isupper(source[i])) {
                        source[i] = tolower(source[i]);
                        result = applied;
                    }
                }

                return result;
            }
            case immediateInstruction:
            case addressInstruction: {
                result_t result = compliant;
                int s2 = matchLength(2);

                if (s2 != 1) {
                    sprintf(scratch, "%c%c%c %s",
                        tolower(source[0]), tolower(source[1]), tolower(source[2]),
                        source + match[3].rm_so);
                    strcpy(source, scratch);
                    result = applied;
                } else {
                    for (int i = 0; i < 3; ++i) {
                        if (isupper(source[i])) {
                            source[i] = tolower(source[i]);
                            result = applied;
                        }
                    }
                }

                return result;
            }
            case indexedInstruction: {
                result_t result = compliant;
                int s2 = matchLength(2);
                int s4 = matchLength(4);
                int s5 = matchLength(5);
                int idx = match[6].rm_so;

                if (s2 != 1 || s4 || s5 != 1) {
                    int s3 = matchLength(3);

                    sprintf(scratch, "%c%c%c %.*s, %c%s",
                        tolower(source[0]), tolower(source[1]), tolower(source[2]),
                        s3, source + match[3].rm_so,
                        tolower(source[idx]),
                        source + match[6].rm_eo);
                    strcpy(source, scratch);
                    result = applied;
                } else {
                    for (int i = 0; i < 3; ++i) {
                        if (isupper(source[i])) {
                            source[i] = tolower(source[i]);
                            result = applied;
                        }
                    }

                    if (isupper(source[idx])) {
                        source[idx] = tolower(source[idx]);
                        result = applied;
                    }
                }

                return result;
            }
            case indirectInstruction: {
                result_t result = compliant;
                int s2 = matchLength(2);
                int s3 = matchLength(3);
                int s5 = matchLength(5);

                if (s2 != 1 || s3 || s5) {
                    int s4 = matchLength(4);

                    sprintf(scratch, "%c%c%c (%.*s)%s",
                        tolower(source[0]), tolower(source[1]), tolower(source[2]),
                        s4, source + match[4].rm_so,
                        source + match[5].rm_eo + 1);
                    strcpy(source, scratch);
                    result = applied;
                } else {
                    for (int i = 0; i < 3; ++i) {
                        if (isupper(source[i])) {
                            source[i] = tolower(source[i]);
                            result = applied;
                        }
                    }
                }

                return result;
            }
            case indirectXInstruction: {
                result_t result = compliant;
                int s2 = matchLength(2);
                int s3 = matchLength(3);
                int s5 = matchLength(5);
                int s6 = matchLength(6);
                int s8 = matchLength(8);

                if (s2 != 1 || s3 || s5 || s6 != 1 || s8) {
                    int s4 = matchLength(4);

                    sprintf(scratch, "%c%c%c (%.*s, x)%s",
                        tolower(source[0]), tolower(source[1]), tolower(source[2]),
                        s4, source + match[4].rm_so,
                        source + match[8].rm_eo + 1);
                    strcpy(source, scratch);
                    result = applied;
                } else {
                    for (int i = 0; i < 3; ++i) {
                        if (isupper(source[i])) {
                            source[i] = tolower(source[i]);
                            result = applied;
                        }
                    }

                    int idx = match[7].rm_so;
                    if (source[idx] == 'X') {
                        source[idx] = 'x';
                        result = applied;
                    }
                }

                return result;
            }
            case indirectYInstruction: {
                result_t result = compliant;
                int s2 = matchLength(2);
                int s3 = matchLength(3);
                int s5 = matchLength(5);
                int s6 = matchLength(6);
                int s7 = matchLength(7);

                if (s2 != 1 || s3 || s5 || s6 || s7 != 1) {
                    int s4 = matchLength(4);

                    sprintf(scratch, "%c%c%c (%.*s), y%s",
                        tolower(source[0]), tolower(source[1]), tolower(source[2]),
                        s4, source + match[4].rm_so,
                        source + match[8].rm_eo);
                    strcpy(source, scratch);
                    result = applied;
                } else {
                    for (int i = 0; i < 3; ++i) {
                        if (isupper(source[i])) {
                            source[i] = tolower(source[i]);
                            result = applied;
                        }
                    }

                    int idx = match[8].rm_so;
                    if (source[idx] == 'Y') {
                        source[idx] = 'y';
                        result = applied;
                    }
                }

                return result;
            }
            case relativeInstruction: {
                result_t result = compliant;
                int s2 = matchLength(2);

                if (s2 != 1) {
                    sprintf(scratch, "%c%c%c :%s",
                        tolower(source[0]), tolower(source[1]), tolower(source[2]),
                        source + match[3].rm_so);
                    strcpy(source, scratch);
                    result = applied;
                } else {
                    for (int i = 0; i < 3; ++i) {
                        if (isupper(source[i])) {
                            source[i] = tolower(source[i]);
                            result = applied;
                        }
                    }
                }

                return result;
            }
            case commentSpacing: {
                int s1 = matchLength(1);
                int s2 = matchLength(2);
                int s3 = matchLength(3);

                if (s1 != 1 || (s2 != 1 && s3)) {
                    int mlp = match[1].rm_eo == 0 ? 0 : 1;
                    int mtp = !s3 ? 0 : 1;

                    sprintf(scratch, "%.*s%*.*s;%*.*s%s",
                        (int) match[0].rm_so, source,
                        mlp, s1 > mlp ? s1 : mlp, s1 ? source + match[1].rm_so : " ",
                        mtp, s2 > mtp ? s2 : mtp, s2 ? source + match[2].rm_so : " ",
                        source + match[3].rm_so);
                    strcpy(source, scratch);

                    return applied;
                }

                return compliant;
            }
            default: {
                return error;
            }
        }
    } else if (status == REG_NOMATCH) {
        return notApplied;
    } else {
        fprintf(stderr, "\"%s\" failed to match (%d):\n", ruleNames[rule], status);
        fprintf(stderr, "string: %s\n", source);
        fprintf(stderr, "pattern: %s\n", patterns[rule]);
        printError(status, regex);
        return error;
    }

    return error;
}

int main(int argc, char ** argv) {
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "usage: %s [-c config.cfg] <source.s>\n", argv[0]);
        return 1;
    }

    const char * sourceCode = argv[1];
    const char * config = defaultConfig;

    if (argc == 4) {
        if (strcmp(argv[1], "-c") == 0) {
            config = argv[2];
            sourceCode = argv[3];
        } else if (strcmp(argv[2], "-c") == 0) {
            config = argv[3];
            sourceCode = argv[1];
        } else {
            fprintf(stderr, "usage: %s [-c config.cfg] <source.s>\n", argv[0]);
            return 1;
        }
    }

    char source[4096];
    uint32_t enabled = ~0u;

    FILE * cfg = fopen(config, "r");
    if (cfg) {
        while (fgets(source, 4096, cfg)) {
            char * c = strchr(source, ':');

            if (!c) {
                fprintf(stderr, "failed to read config file: %s\n", config);
                return 1;
            }

            int enable = strstr(c, "enabled") ? 1 : 0;

            *c = '\0';
            int found = 0;

            for (int i = 0; i < numRules; ++i) {
                if (strcmp(ruleNames[i], source) == 0) {
                    if (enable) {
                        enabled |= (1u << i);
                    } else {
                        enabled &= ~(1u << i);
                    }

                    found = 1;
                    break;
                }
            }

            if (!found) {
                printf("unknown rule read from config file: \"%s\"\n", source);
            }
        }

        fclose(cfg);
    } else if (argc == 4) {
        fprintf(stderr, "failed to open config file: %s\n", config);
        return 1;
    }

    FILE * input = fopen(sourceCode, "r");
    if (!input) {
        fprintf(stderr, "failed to open source file: %s\n", sourceCode);
        return 1;
    }

    FILE * output = fopen(temp, "w");
    if (!output) {
        fprintf(stderr, "failed to create temporary file\n");
        return 1;
    }

    regex_t regex[numRules] = { 0 };

    for (int i = 0; i < numRules; ++i) {
        int status = regcomp(regex + i, patterns[i], REG_EXTENDED | REG_ICASE);
        if (status) {
            fprintf(stderr, "regex %d failed to compile:\n", i);
            printError(status, regex + i);
            cleanup(input, output, regex);
            return 1;
        }
    }

    indentionSize = strlen(indention);
    int insidePreformattedBlock = 0;
    int lineNum = 0;
    int prevLineBlank = 0;

    while (fgets(source, 4096, input)) {
        ++lineNum;
        int skip = insidePreformattedBlock;

        if (!skip && strstr(source, preformatted)) {
            skip = 1;
        }

        if (strstr(source, preformattedStart)) {
            ++insidePreformattedBlock;
            skip = 1;
        }

        if (strstr(source, preformattedEnd)) {
            if (!insidePreformattedBlock) {
                printf("%s command on line %d is not inside a preformatted block: ignoring\n",
                    preformattedEnd, lineNum);
            } else {
                --insidePreformattedBlock;
            }

            skip = 1;
        }

        if (skip) {
            printLineResult(output, source, 0, 1);
            prevLineBlank = 0;
            continue;
        }

        flags_t flags = prependIndention;

        char * c = strchr(source, '\n');
        if (c) {
            flags |= appendNewline;
            *c = '\0';
        }

        for (int i = 0; i < numRules; ++i) {
            if (!(enabled & (1u << i))) {
                continue;
            }

            result_t result = applyRule(i, source, flags, regex + i);
            if (result == error) {
                cleanup(input, output, regex);
                return 1;
            }

            switch (i) {
                case onlyComment: {
                    if (result == compliant || result == applied) {
                        if (*source == ';') {
                            flags &= ~prependIndention;
                        } else {
                            flags |= prependIndention;
                        }
                    }
                    break;
                }
                case trimTrailing: {
                    if (!*source) {
                        if (prevLineBlank) {
                            flags |= omit;
                        } else {
                            prevLineBlank = 1;
                            flags &= ~prependIndention;
                            flags |= done;
                        }
                    } else {
                        prevLineBlank = 0;
                    }
                    break;
                }
                case bitwiseInstruction: {
                    if (result == compliant || result == applied) {
                        flags |= bitwiseOperation;
                    }
                    break;
                }
                case controlCommand: {
                    if ((result == compliant || result == applied) && *source != ';') {
                        if (strncmp(source + 1, "asciiz",  6) == 0 ||
                            strncmp(source + 1, "addr",    4) == 0 ||
                            strncmp(source + 1, "byt",     3) == 0 ||
                            strncmp(source + 1, "byte",    4) == 0 ||
                            strncmp(source + 1, "dbyt",    4) == 0 ||
                            strncmp(source + 1, "dword",   5) == 0 ||
                            strncmp(source + 1, "lobytes", 7) == 0 ||
                            strncmp(source + 1, "hibytes", 7) == 0 ||
                            strncmp(source + 1, "word",    4) == 0) {

                            flags |= prependIndention;
                        } else {
                            flags &= ~prependIndention;
                        }
                    }
                    break;
                }
                case namedLabel: {
                    if (result == compliant || result == applied) {
                        flags &= ~prependIndention;
                    }
                    break;
                }
                case unnamedLabel: {
                    if (result == compliant || result == applied) {
                        flags |= prependLabel;
                    }
                    break;
                }
                case macroInstance:
                case impliedInstruction:
                case immediateInstruction:
                case addressInstruction:
                case indexedInstruction:
                case indirectInstruction:
                case indirectXInstruction:
                case indirectYInstruction:
                case relativeInstruction: {
                    if (result == compliant || result == applied) {
                        flags |= prependIndention;
                    }
                    break;
                }
                default: {
                    break;
                }
            }

            printRuleResult(i, result, flags);

            if (flags & (done | omit)) {
                break;
            }
        }

        printLineResult(output, source, flags, 0);
    }

    cleanup(input, output, regex);

    if (rename(temp, sourceCode)) {
        fprintf(stderr, "failed to rename temporary file\n");
        return 1;
    }

    return 0;
}