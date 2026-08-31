/* ------------------------------------------------------------------ */
/* Stroke font glyph data.                                          */
/*                                                                     */
/* This is a self-contained vector font, split out of glprintf.cpp so  */
/* that individual characters can be edited easily. Each glyph is a    */
/* flat list of polylines described below.                             */
/*                                                                     */
/* Coordinate system: baseline at y=0, cap height ~100, descenders     */
/* down to -33, fixed advance of 104 per character.                 */
/* ------------------------------------------------------------------ */

#ifndef STROKE_FONT_H
#define STROKE_FONT_H

/* ------------------------------------------------------------------ */
/* Stroke font used to draw text.                                      */
/*                                                                     */
/* This replaces the GLUT stroke font (glutStrokeCharacter /           */
/* glutStrokeWidth) with a self-contained vector font, so the game no  */
/* longer depends on GLUT (which is not available inside an APK).      */
/*                                                                     */
/* Coordinate system: baseline at y=0, cap height ~100, descenders     */
/* down to -33, fixed advance of 104 per character (matching the       */
/* proportions of GLUT's MONO_ROMAN stroke font that was used before). */
/* ------------------------------------------------------------------ */

/* Each glyph is a flat list of polylines. The pair (-1000,-1000) starts */
/* a new stroke (pen up), and the pair (-2000,-2000) terminates the      */
/* glyph. Regular pairs are consecutive point coordinates connected by   */
/* a straight line.                                                      */
#define PEN_UP   (-1000)
#define END_GLYPH (-2000)

#define NUL {PEN_UP,PEN_UP,END_GLYPH,END_GLYPH}

/* Space: */ 
static const short glyph_space[] = { END_GLYPH, END_GLYPH };

/* Exclamation mark ! */
static const short glyph_exclam[] = {
    40,100, 40,25,
    PEN_UP,PEN_UP,
    40,4, 40,4,
    END_GLYPH,END_GLYPH
};

/* Double quote " */
static const short glyph_quote[] = {
    24,100, 24,70,
    PEN_UP,PEN_UP,
    56,100, 56,70,
    END_GLYPH,END_GLYPH
};

/* Hash # */
static const short glyph_hash[] = {
    24,0, 40,100,
    PEN_UP,PEN_UP,
    56,0, 40,100,
    PEN_UP,PEN_UP,
    12,35, 68,35,
    PEN_UP,PEN_UP,
    8,65, 64,65,
    END_GLYPH,END_GLYPH
};

/* Dollar $ */
static const short glyph_dollar[] = {
    40,108, 40,-8,
    PEN_UP,PEN_UP,
    60,90, 24,90,
    24,90, 16,82,
    16,82, 16,62,
    16,62, 24,54,
    24,54, 56,46,
    56,46, 64,38,
    64,38, 64,16,
    64,16, 56,8,
    56,8, 20,8,
    PEN_UP,PEN_UP,
    16,62, 56,54,
    END_GLYPH,END_GLYPH
};

/* Percent % */
static const short glyph_percent[] = {
    12,0, 68,100,
    PEN_UP,PEN_UP,
    16,82, 16,100,
    PEN_UP,PEN_UP,
    16,100, 34,100,
    PEN_UP,PEN_UP,
    34,100, 34,82,
    PEN_UP,PEN_UP,
    16,82, 34,82,
    PEN_UP,PEN_UP,
    50,18, 50,0,
    PEN_UP,PEN_UP,
    50,0, 68,0,
    PEN_UP,PEN_UP,
    68,0, 68,18,
    PEN_UP,PEN_UP,
    50,18, 68,18,
    END_GLYPH,END_GLYPH
};

/* Ampersand & */
static const short glyph_ampersand[] = {
    60,80, 52,92,
    52,92, 32,100,
    32,100, 20,92,
    20,92, 20,78,
    20,78, 28,66,
    28,66, 56,38,
    56,38, 64,24,
    64,24, 64,12,
    64,12, 56,4,
    56,4, 32,4,
    32,4, 20,12,
    20,12, 20,28,
    20,28, 28,40,
    28,40, 60,72,
    PEN_UP,PEN_UP,
    44,52, 64,28,
    END_GLYPH,END_GLYPH
};

/* Single quote ' */
static const short glyph_apostrophe[] = {
    40,100, 40,70,
    END_GLYPH,END_GLYPH
};

/* Left parenthesis ( */
static const short glyph_lparen[] = {
    52,100, 40,88,
    40,88, 32,70,
    32,70, 28,50,
    28,50, 32,30,
    32,30, 40,12,
    40,12, 52,0,
    END_GLYPH,END_GLYPH
};

/* Right parenthesis ) */
static const short glyph_rparen[] = {
    28,100, 40,88,
    40,88, 48,70,
    48,70, 52,50,
    52,50, 48,30,
    48,30, 40,12,
    40,12, 28,0,
    END_GLYPH,END_GLYPH
};

/* Asterisk * */
static const short glyph_asterisk[] = {
    24,92, 56,60,
    PEN_UP,PEN_UP,
    56,92, 24,60,
    PEN_UP,PEN_UP,
    40,96, 40,56,
    END_GLYPH,END_GLYPH
};

/* Plus + */
static const short glyph_plus[] = {
    16,50, 64,50,
    PEN_UP,PEN_UP,
    40,26, 40,74,
    END_GLYPH,END_GLYPH
};

/* Comma , */
static const short glyph_comma[] = {
    40,18, 32,4,
    32,4, 24,0,
    END_GLYPH,END_GLYPH
};

/* Minus - */
static const short glyph_minus[] = {
    16,50, 64,50,
    END_GLYPH,END_GLYPH
};

/* Period . */
static const short glyph_period[] = {
    36,4, 44,4,
    END_GLYPH,END_GLYPH
};

/* Slash / */
static const short glyph_slash[] = {
    12,0, 68,100,
    END_GLYPH,END_GLYPH
};

/* Digit 0 */
static const short glyph_0[] = {
    20,100, 60,100,
    60,100, 60,0,
    60,0, 20,0,
    20,0, 20,100,
    PEN_UP,PEN_UP,
    20,0, 60,100,
    END_GLYPH,END_GLYPH
};

/* Digit 1 */
static const short glyph_1[] = {
    28,80, 40,100,
    40,100, 40,0,
    PEN_UP,PEN_UP,
    28,0, 52,0,
    END_GLYPH,END_GLYPH
};

/* Digit 2 */
static const short glyph_2[] = {
    20,100, 60,100,
    60,100, 60,65,
    60,65, 20,0,
    20,0, 60,0,
    END_GLYPH,END_GLYPH
};

/* Digit 3 */
static const short glyph_3[] = {
    20,100, 60,100,
    60,100, 60,55,
    60,55, 20,55,
    PEN_UP,PEN_UP,
    60,55, 60,0,
    60,0, 20,0,
    END_GLYPH,END_GLYPH
};

/* Digit 4 */
static const short glyph_4[] = {
    52,100, 20,35,
    PEN_UP,PEN_UP,
    52,100, 52,0,
    PEN_UP,PEN_UP,
    16,35, 60,35,
    END_GLYPH,END_GLYPH
};

/* Digit 5 */
static const short glyph_5[] = {
    60,100, 20,100,
    20,100, 20,55,
    20,55, 52,55,
    52,55, 60,47,
    60,47, 60,8,
    60,8, 52,0,
    52,0, 20,0,
    END_GLYPH,END_GLYPH
};

/* Digit 6 */
static const short glyph_6[] = {
    60,100, 20,100,
    20,100, 20,0,
    20,0, 60,0,
    60,0, 60,55,
    60,55, 20,55,
    END_GLYPH,END_GLYPH
};

/* Digit 7 */
static const short glyph_7[] = {
    20,100, 60,100,
    PEN_UP,PEN_UP,
    60,100, 20,0,
    END_GLYPH,END_GLYPH
};

/* Digit 8 */
static const short glyph_8[] = {
    20,100, 60,100,
    60,100, 60,0,
    60,0, 20,0,
    20,0, 20,100,
    PEN_UP,PEN_UP,
    20,50, 60,50,
    END_GLYPH,END_GLYPH
};

/* Digit 9 */
static const short glyph_9[] = {
    20,0, 60,0,
    60,0, 60,100,
    60,100, 20,100,
    20,100, 20,55,
    20,55, 60,55,
    END_GLYPH,END_GLYPH
};

/* Colon : */
static const short glyph_colon[] = {
    34,68, 46,68,
    PEN_UP,PEN_UP,
    34,68, 34,80,
    PEN_UP,PEN_UP,
    34,8, 46,8,
    PEN_UP,PEN_UP,
    34,8, 34,20,
    END_GLYPH,END_GLYPH
};

/* Semicolon ; */
static const short glyph_semicolon[] = {
    34,68, 46,68,
    PEN_UP,PEN_UP,
    34,68, 34,80,
    PEN_UP,PEN_UP,
    40,20, 32,0,
    END_GLYPH,END_GLYPH
};

/* Less than < */
static const short glyph_less[] = {
    60,80, 20,50,
    PEN_UP,PEN_UP,
    20,50, 60,20,
    END_GLYPH,END_GLYPH
};

/* Equal = */
static const short glyph_equal[] = {
    16,65, 64,65,
    PEN_UP,PEN_UP,
    16,35, 64,35,
    END_GLYPH,END_GLYPH
};

/* Greater than > */
static const short glyph_greater[] = {
    20,80, 60,50,
    PEN_UP,PEN_UP,
    60,50, 20,20,
    END_GLYPH,END_GLYPH
};

/* Question mark ? */
static const short glyph_question[] = {
    20,80, 52,80,
    52,80, 60,72,
    60,72, 60,60,
    60,60, 52,52,
    52,52, 40,44,
    40,44, 40,28,
    PEN_UP,PEN_UP,
    40,8, 40,8,
    END_GLYPH,END_GLYPH
};

/* At symbol @ */
static const short glyph_at[] = {
    28,90, 52,90,
    52,90, 64,78,
    64,78, 64,22,
    64,22, 52,10,
    52,10, 24,10,
    24,10, 12,22,
    12,22, 12,78,
    12,78, 24,90,
    PEN_UP,PEN_UP,

    28,28, 48,28,
    48,28, 52,32,
    52,32, 52,68,
    52,68, 48,72,
    48,72, 28,72,
    28,72, 24,68,
    24,68, 24,32,
    24,32, 28,28,
    PEN_UP,PEN_UP,

    52,72, 52,40,
    52,40, 60,32,

    END_GLYPH,END_GLYPH
};


/* Uppercase A */
static const short glyph_A[] = {
    12,0, 40,100,
    PEN_UP,PEN_UP,
    40,100, 68,0,
    PEN_UP,PEN_UP,
    24,45, 56,45,
    END_GLYPH,END_GLYPH
};

/* Uppercase B */
static const short glyph_B[] = {
    16,100, 16,0,
    PEN_UP,PEN_UP,

    16,100, 50,100,
    50,100, 60,90,
    60,90, 60,65,
    60,65, 50,55,
    50,55, 16,55,
    PEN_UP,PEN_UP,

    50,55, 60,45,
    60,45, 60,10,
    60,10, 50,0,
    50,0, 16,0,

    END_GLYPH,END_GLYPH
};

/* Uppercase C */
static const short glyph_C[] = {
    60,100, 24,100,
    PEN_UP,PEN_UP,
    24,100, 24,0,
    PEN_UP,PEN_UP,
    24,0, 60,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase D */
static const short glyph_D[] = {
    16,100, 16,0,
    PEN_UP,PEN_UP,
    16,100, 50,100,
    50,100, 60,90,
    60,90, 60,10,
    60,10, 50,0,
    50,0, 16,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase E */
static const short glyph_E[] = {
    60,100, 20,100,
    PEN_UP,PEN_UP,
    20,100, 20,0,
    PEN_UP,PEN_UP,
    20,50, 50,50,
    PEN_UP,PEN_UP,
    20,0, 60,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase F */
static const short glyph_F[] = {
    60,100, 20,100,
    PEN_UP,PEN_UP,
    20,100, 20,0,
    PEN_UP,PEN_UP,
    20,50, 50,50,
    END_GLYPH,END_GLYPH
};

/* Uppercase G */
static const short glyph_G[] = {
    60,100, 24,100,
    PEN_UP,PEN_UP,
    24,100, 24,0,
    PEN_UP,PEN_UP,
    24,0, 60,0,
    PEN_UP,PEN_UP,
    60,0, 60,45,
    PEN_UP,PEN_UP,
    60,45, 42,45,
    END_GLYPH,END_GLYPH
};

/* Uppercase H */
static const short glyph_H[] = {
    16,100, 16,0,
    PEN_UP,PEN_UP,
    64,100, 64,0,
    PEN_UP,PEN_UP,
    16,50, 64,50,
    END_GLYPH,END_GLYPH
};

/* Uppercase I */
static const short glyph_I[] = {
    24,100, 56,100,
    PEN_UP,PEN_UP,
    40,100, 40,0,
    PEN_UP,PEN_UP,
    24,0, 56,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase J */
static const short glyph_J[] = {
    20,100, 60,100,
    PEN_UP,PEN_UP,
    52,100, 52,20,
    PEN_UP,PEN_UP,
    52,20, 44,0,
    44,0, 28,0,
    28,0, 20,20,
    PEN_UP,PEN_UP,
    20,20, 20,40,
    END_GLYPH,END_GLYPH
};

/* Uppercase K */
static const short glyph_K[] = {
    16,100, 16,0,
    PEN_UP,PEN_UP,
    60,100, 16,50,
    PEN_UP,PEN_UP,
    16,50, 60,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase L */
static const short glyph_L[] = {
    20,100, 20,0,
    PEN_UP,PEN_UP,
    20,0, 60,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase M */
static const short glyph_M[] = {
    16,0, 16,100,
    PEN_UP,PEN_UP,
    16,100, 40,20,
    PEN_UP,PEN_UP,
    40,20, 64,100,
    PEN_UP,PEN_UP,
    64,100, 64,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase N */
static const short glyph_N[] = {
    16,0, 16,100,
    PEN_UP,PEN_UP,
    16,100, 64,0,
    PEN_UP,PEN_UP,
    64,0, 64,100,
    END_GLYPH,END_GLYPH
};

/* Uppercase O */
static const short glyph_O[] = {
    20,100, 60,100,
    PEN_UP,PEN_UP,
    60,100, 60,0,
    PEN_UP,PEN_UP,
    60,0, 20,0,
    PEN_UP,PEN_UP,
    20,0, 20,100,
    END_GLYPH,END_GLYPH
};

/* Uppercase P */
static const short glyph_P[] = {
    16,0, 16,100,
    PEN_UP,PEN_UP,
    16,100, 50,100,
    50,100, 60,90,
    60,90, 60,65,
    60,65, 50,55,
    50,55, 16,55,
    END_GLYPH,END_GLYPH
};

/* Uppercase Q */
static const short glyph_Q[] = {
    20,100, 60,100,
    PEN_UP,PEN_UP,
    60,100, 60,0,
    PEN_UP,PEN_UP,
    60,0, 20,0,
    PEN_UP,PEN_UP,
    20,0, 20,100,
    PEN_UP,PEN_UP,
    42,25, 64,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase R */
static const short glyph_R[] = {
    16,0, 16,100,
    PEN_UP,PEN_UP,
    16,100, 50,100,
    50,100, 60,90,
    60,90, 60,65,
    60,65, 50,55,
    50,55, 16,55,
    PEN_UP,PEN_UP,
    38,55, 64,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase S */
static const short glyph_S[] = {
    60,100, 20,100,
    PEN_UP,PEN_UP,
    20,100, 20,55,
    PEN_UP,PEN_UP,
    20,55, 60,55,
    PEN_UP,PEN_UP,
    60,55, 60,0,
    PEN_UP,PEN_UP,
    60,0, 20,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase T */
static const short glyph_T[] = {
    12,100, 68,100,
    PEN_UP,PEN_UP,
    40,100, 40,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase U */
static const short glyph_U[] = {
    16,100, 16,20,
    PEN_UP,PEN_UP,
    16,20, 24,0,
    24,0, 56,0,
    56,0, 64,20,
    PEN_UP,PEN_UP,
    64,20, 64,100,
    END_GLYPH,END_GLYPH
};

/* Uppercase V */
static const short glyph_V[] = {
    12,100, 40,0,
    PEN_UP,PEN_UP,
    40,0, 68,100,
    END_GLYPH,END_GLYPH
};

/* Uppercase W */
static const short glyph_W[] = {
    12,100, 28,0,
    PEN_UP,PEN_UP,
    28,0, 40,55,
    PEN_UP,PEN_UP,
    40,55, 52,0,
    PEN_UP,PEN_UP,
    52,0, 68,100,
    END_GLYPH,END_GLYPH
};

/* Uppercase X */
static const short glyph_X[] = {
    16,100, 64,0,
    PEN_UP,PEN_UP,
    64,100, 16,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase Y */
static const short glyph_Y[] = {
    16,100, 40,50,
    PEN_UP,PEN_UP,
    64,100, 40,50,
    PEN_UP,PEN_UP,
    40,50, 40,0,
    END_GLYPH,END_GLYPH
};

/* Uppercase Z */
static const short glyph_Z[] = {
    12,100, 68,100,
    PEN_UP,PEN_UP,
    68,100, 12,0,
    PEN_UP,PEN_UP,
    12,0, 68,0,
    END_GLYPH,END_GLYPH
};

/* Left bracket [: */ 
static const short glyph_lbracket[] = {
	48,100, 24,100,
	PEN_UP,PEN_UP,
	24,100, 24,0,
	PEN_UP,PEN_UP,
	24,0, 48,0,
	END_GLYPH,END_GLYPH
};

/* Backslash \: */ 
static const short glyph_backslash[] = {
	12,100, 68,0,
	END_GLYPH,END_GLYPH
};

/* Right bracket ]: */ 
static const short glyph_rbracket[] = {
	24,100, 48,100,
	PEN_UP,PEN_UP,
	48,100, 48,0,
	PEN_UP,PEN_UP,
	24,0, 48,0,
	END_GLYPH,END_GLYPH
};

/* Caret ^: */ 
static const short glyph_caret[] = {
	24,55, 40,80,
	PEN_UP,PEN_UP,
	40,80, 56,55,
	END_GLYPH,END_GLYPH
};

/* Underscore _: */ 
static const short glyph_underscore[] = {
	20,-25, 60,-25,
	END_GLYPH,END_GLYPH
};

/* Grave accent `: */ 
static const short glyph_grave[] = {
	30,100, 22,80,
	END_GLYPH,END_GLYPH
};


/* Lowercase a */
static const short glyph_a[] = {
    16,45, 16,12,
    PEN_UP,PEN_UP,
    16,12, 24,4,
    24,4, 52,4,
    52,4, 60,12,
    60,12, 60,65,
    PEN_UP,PEN_UP,
    16,45, 60,45,
    END_GLYPH,END_GLYPH
};

/* Lowercase b */
static const short glyph_b[] = {
    16,100, 16,4,
    PEN_UP,PEN_UP,
    16,45, 24,53,
    24,53, 52,53,
    52,53, 60,45,
    60,45, 60,12,
    60,12, 52,4,
    52,4, 24,4,
    24,4, 16,12,
    END_GLYPH,END_GLYPH
};

/* Lowercase c */
static const short glyph_c[] = {
    60,65, 24,65,
    PEN_UP,PEN_UP,
    24,65, 16,57,
    16,57, 16,12,
    16,12, 24,4,
    24,4, 60,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase d */
static const short glyph_d[] = {
    60,100, 60,4,
    PEN_UP,PEN_UP,
    60,45, 52,53,
    52,53, 24,53,
    24,53, 16,45,
    16,45, 16,12,
    16,12, 24,4,
    24,4, 52,4,
    52,4, 60,12,
    END_GLYPH,END_GLYPH
};

/* Lowercase e */
static const short glyph_e[] = {
    16,32, 60,32,
    PEN_UP,PEN_UP,
    16,32, 16,12,
    16,12, 24,4,
    24,4, 52,4,
    52,4, 60,12,
    PEN_UP,PEN_UP,
    16,32, 24,65,
    24,65, 52,65,
    52,65, 60,57,
    60,57, 60,32,
    END_GLYPH,END_GLYPH
};

/* Lowercase f */
static const short glyph_f[] = {
    56,100, 36,100,
    36,100, 28,92,
    28,92, 28,12,
    28,12, 36,4,
    36,4, 44,4,
    PEN_UP,PEN_UP,
    16,65, 48,65,
    END_GLYPH,END_GLYPH
};

/* Lowercase g */
static const short glyph_g[] = {
    16,12, 24,4,
    24,4, 52,4,
    52,4, 60,12,
    60,12, 60,65,
    PEN_UP,PEN_UP,
    16,45, 24,53,
    24,53, 52,53,
    52,53, 60,45,
    PEN_UP,PEN_UP,
    16,12, 16,-20,
    16,-20, 24,-28,
    24,-28, 52,-28,
    52,-28, 60,-20,
    60,-20, 60,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase h */
static const short glyph_h[] = {
    16,100, 16,4,
    PEN_UP,PEN_UP,
    16,45, 24,53,
    24,53, 52,53,
    52,53, 60,45,
    60,45, 60,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase i */
static const short glyph_i[] = {
    32,65, 32,4,
    PEN_UP,PEN_UP,
    24,4, 40,4,
    PEN_UP,PEN_UP,
    32,85, 32,100,
    END_GLYPH,END_GLYPH
};

/* Lowercase j */
static const short glyph_j[] = {
    40,65, 40,-20,
    PEN_UP,PEN_UP,
    40,-20, 32,-28,
    32,-28, 24,-28,
    PEN_UP,PEN_UP,
    32,85, 32,100,
    END_GLYPH,END_GLYPH
};

/* Lowercase k */
static const short glyph_k[] = {
    16,100, 16,4,
    PEN_UP,PEN_UP,
    16,38, 56,65,
    PEN_UP,PEN_UP,
    16,38, 56,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase l */
static const short glyph_l[] = {
    32,100, 32,12,
    32,12, 40,4,
    40,4, 48,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase m */
static const short glyph_m[] = {
    12,4, 12,65,
    PEN_UP,PEN_UP,
    12,45, 20,53,
    20,53, 32,53,
    32,53, 40,45,
    40,45, 40,4,
    PEN_UP,PEN_UP,
    40,45, 48,53,
    48,53, 60,53,
    60,53, 68,45,
    68,45, 68,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase n */
static const short glyph_n[] = {
    16,4, 16,65,
    PEN_UP,PEN_UP,
    16,45, 24,53,
    24,53, 48,53,
    48,53, 60,45,
    60,45, 60,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase o */
static const short glyph_o[] = {
    24,65, 52,65,
    52,65, 60,57,
    60,57, 60,12,
    60,12, 52,4,
    52,4, 24,4,
    24,4, 16,12,
    16,12, 16,57,
    16,57, 24,65,
    END_GLYPH,END_GLYPH
};

/* Lowercase p */
static const short glyph_p[] = {
    16,65, 16,-28,
    PEN_UP,PEN_UP,
    16,45, 24,53,
    24,53, 52,53,
    52,53, 60,45,
    60,45, 60,12,
    60,12, 52,4,
    52,4, 24,4,
    24,4, 16,12,
    END_GLYPH,END_GLYPH
};

/* Lowercase q */
static const short glyph_q[] = {
    60,65, 60,-28,
    PEN_UP,PEN_UP,
    60,45, 52,53,
    52,53, 24,53,
    24,53, 16,45,
    16,45, 16,12,
    16,12, 24,4,
    24,4, 52,4,
    52,4, 60,12,
    END_GLYPH,END_GLYPH
};

/* Lowercase r */
static const short glyph_r[] = {
    16,4, 16,65,
    PEN_UP,PEN_UP,
    16,45, 24,53,
    24,53, 40,53,
    40,53, 48,45,
    END_GLYPH,END_GLYPH
};

/* Lowercase s */
static const short glyph_s[] = {
    60,65, 24,65,
    24,65, 16,57,
    16,57, 16,40,
    16,40, 24,32,
    24,32, 52,32,
    52,32, 60,24,
    60,24, 60,12,
    60,12, 52,4,
    52,4, 16,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase t */
static const short glyph_t[] = {
    32,100, 32,12,
    32,12, 40,4,
    40,4, 52,4,
    PEN_UP,PEN_UP,
    16,65, 48,65,
    END_GLYPH,END_GLYPH
};

/* Lowercase u */
static const short glyph_u[] = {
    16,65, 16,12,
    16,12, 24,4,
    24,4, 52,4,
    52,4, 60,12,
    60,12, 60,65,
    END_GLYPH,END_GLYPH
};

/* Lowercase v */
static const short glyph_v[] = {
    16,65, 40,4,
    PEN_UP,PEN_UP,
    40,4, 64,65,
    END_GLYPH,END_GLYPH
};

/* Lowercase w */
static const short glyph_w[] = {
    12,65, 24,4,
    PEN_UP,PEN_UP,
    24,4, 40,45,
    PEN_UP,PEN_UP,
    40,45, 56,4,
    PEN_UP,PEN_UP,
    56,4, 68,65,
    END_GLYPH,END_GLYPH
};

/* Lowercase x */
static const short glyph_x[] = {
    16,65, 60,4,
    PEN_UP,PEN_UP,
    60,65, 16,4,
    END_GLYPH,END_GLYPH
};

/* Lowercase y */
static const short glyph_y[] = {
    16,65, 40,12,
    PEN_UP,PEN_UP,
    64,65, 40,12,
    PEN_UP,PEN_UP,
    40,12, 32,-28,
    32,-28, 24,-36,
    END_GLYPH,END_GLYPH
};

/* Lowercase z */
static const short glyph_z[] = {
    16,65, 60,65,
    PEN_UP,PEN_UP,
    60,65, 16,4,
    PEN_UP,PEN_UP,
    16,4, 60,4,
    END_GLYPH,END_GLYPH
};

/* Left brace { */
static const short glyph_lbrace[] = {
    52,100, 40,100,
    40,100, 32,92,
    32,92, 32,58,
    32,58, 24,50,
    24,50, 32,42,
    32,42, 32,8,
    32,8, 40,0,
    40,0, 52,0,
    END_GLYPH,END_GLYPH
};

/* Vertical bar | */
static const short glyph_bar[] = {
    40,100, 40,0,
    END_GLYPH,END_GLYPH
};

/* Right brace } */
static const short glyph_rbrace[] = {
    28,100, 40,100,
    40,100, 48,92,
    48,92, 48,58,
    48,58, 56,50,
    56,50, 48,42,
    48,42, 48,8,
    48,8, 40,0,
    40,0, 28,0,
    END_GLYPH,END_GLYPH
};

/* Tilde ~ */
static const short glyph_tilde[] = {
    16,45, 24,55,
    24,55, 36,55,
    36,55, 48,35,
    48,35, 56,25,
    56,25, 64,25,
    END_GLYPH,END_GLYPH
};


typedef const short *glyphptr;

static glyphptr glyph_table[128]={
	/* 0  */ glyph_space,
	/* 1  */ glyph_space,
	/* 2  */ glyph_space,
	/* 3  */ glyph_space,
	/* 4  */ glyph_space,
	/* 5  */ glyph_space,
	/* 6  */ glyph_space,
	/* 7  */ glyph_space,
	/* 8  */ glyph_space,
	/* 9  */ glyph_space,
	/* 10 */ glyph_space,
	/* 11 */ glyph_space,
	/* 12 */ glyph_space,
	/* 13 */ glyph_space,
	/* 14 */ glyph_space,
	/* 15 */ glyph_space,
	/* 16 */ glyph_space,
	/* 17 */ glyph_space,
	/* 18 */ glyph_space,
	/* 19 */ glyph_space,
	/* 20 */ glyph_space,
	/* 21 */ glyph_space,
	/* 22 */ glyph_space,
	/* 23 */ glyph_space,
	/* 24 */ glyph_space,
	/* 25 */ glyph_space,
	/* 26 */ glyph_space,
	/* 27 */ glyph_space,
	/* 28 */ glyph_space,
	/* 29 */ glyph_space,
	/* 30 */ glyph_space,
	/* 31 */ glyph_space,
	/* 32  space  */ glyph_space,
	/* 33  !      */ glyph_exclam,
	/* 34  "      */ glyph_quote,
	/* 35  #      */ glyph_hash,
	/* 36  $      */ glyph_dollar,
	/* 37  %      */ glyph_percent,
	/* 38  &      */ glyph_ampersand,
	/* 39  '      */ glyph_apostrophe,
	/* 40  (      */ glyph_lparen,
	/* 41  )      */ glyph_rparen,
	/* 42  *      */ glyph_asterisk,
	/* 43  +      */ glyph_plus,
	/* 44  ,      */ glyph_comma,
	/* 45  -      */ glyph_minus,
	/* 46  .      */ glyph_period,
	/* 47  /      */ glyph_slash,
	/* 48  0      */ glyph_0,
	/* 49  1      */ glyph_1,
	/* 50  2      */ glyph_2,
	/* 51  3      */ glyph_3,
	/* 52  4      */ glyph_4,
	/* 53  5      */ glyph_5,
	/* 54  6      */ glyph_6,
	/* 55  7      */ glyph_7,
	/* 56  8      */ glyph_8,
	/* 57  9      */ glyph_9,
	/* 58  :      */ glyph_colon,
	/* 59  ;      */ glyph_semicolon,
	/* 60  <      */ glyph_less,
	/* 61  =      */ glyph_equal,
	/* 62  >      */ glyph_greater,
	/* 63  ?      */ glyph_question,
	/* 64  @      */ glyph_at,
	/* 65  A      */ glyph_A,
	/* 66  B      */ glyph_B,
	/* 67  C      */ glyph_C,
	/* 68  D      */ glyph_D,
	/* 69  E      */ glyph_E,
	/* 70  F      */ glyph_F,
	/* 71  G      */ glyph_G,
	/* 72  H      */ glyph_H,
	/* 73  I      */ glyph_I,
	/* 74  J      */ glyph_J,
	/* 75  K      */ glyph_K,
	/* 76  L      */ glyph_L,
	/* 77  M      */ glyph_M,
	/* 78  N      */ glyph_N,
	/* 79  O      */ glyph_O,
	/* 80  P      */ glyph_P,
	/* 81  Q      */ glyph_Q,
	/* 82  R      */ glyph_R,
	/* 83  S      */ glyph_S,
	/* 84  T      */ glyph_T,
	/* 85  U      */ glyph_U,
	/* 86  V      */ glyph_V,
	/* 87  W      */ glyph_W,
	/* 88  X      */ glyph_X,
	/* 89  Y      */ glyph_Y,
	/* 90  Z      */ glyph_Z,
	/* 91  [      */ glyph_lbracket,
	/* 92  \      */ glyph_backslash,
	/* 93  ]      */ glyph_rbracket,
	/* 94  ^      */ glyph_caret,
	/* 95  _      */ glyph_underscore,
	/* 96  `      */ glyph_grave,
	/* 97  a      */ glyph_a,
	/* 98  b      */ glyph_b,
	/* 99  c      */ glyph_c,
	/* 100 d      */ glyph_d,
	/* 101 e      */ glyph_e,
	/* 102 f      */ glyph_f,
	/* 103 g      */ glyph_g,
	/* 104 h      */ glyph_h,
	/* 105 i      */ glyph_i,
	/* 106 j      */ glyph_j,
	/* 107 k      */ glyph_k,
	/* 108 l      */ glyph_l,
	/* 109 m      */ glyph_m,
	/* 110 n      */ glyph_n,
	/* 111 o      */ glyph_o,
	/* 112 p      */ glyph_p,
	/* 113 q      */ glyph_q,
	/* 114 r      */ glyph_r,
	/* 115 s      */ glyph_s,
	/* 116 t      */ glyph_t,
	/* 117 u      */ glyph_u,
	/* 118 v      */ glyph_v,
	/* 119 w      */ glyph_w,
	/* 120 x      */ glyph_x,
	/* 121 y      */ glyph_y,
	/* 122 z      */ glyph_z,
	/* 123 {      */ glyph_lbrace,
	/* 124 |      */ glyph_bar,
	/* 125 }      */ glyph_rbrace,
	/* 126 ~      */ glyph_tilde,
	/* 127        */ glyph_space
};


/* Advance width (matches GLUT MONO_ROMAN proportions). */ 
#define STROKE_ADVANCE  104

#endif /* STROKE_FONT_H */
