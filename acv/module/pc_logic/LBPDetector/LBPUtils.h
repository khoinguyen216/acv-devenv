/*
 * 2013-10-20, WANG Fanglin
 * These code lines are from LBP.h and LBP.c located below
 * http://www.ee.oulu.fi/~topiolli/cpplibs/files/
 */

#ifndef _LBPUTILS_WFL_
#define _LBPUTILS_WFL_
/*
 * Compare a value pointed to by 'ptr' to the 'center' value and
 * increment pointer. Comparison is made by masking the most
 * significant bit of an integer (the sign) and shifting it to an
 * appropriate position.
 */
#define COMPAB_MASK_INC(value, ptr,shift) { value |= ((unsigned int)(*center - *ptr - 1) & 0x80000000) >> (31-shift); ptr++; }
/* Compare a value 'val' to the 'center' value. */
#define COMPAB_MASK(value, val,shift) { value |= ((unsigned int)(*center - (val) - 1) & 0x80000000) >> (31-shift); }
/* Predicate 1 for the 3x3 neighborhood */
//#define predicate 1
#define PREDICATE 1

/* The number of bits */
#define BITS 8

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

typedef struct
{
	int x,y;
} integerpoint;

typedef struct
{
	double x,y;
}doublepoint;

extern integerpoint points[BITS];
extern doublepoint offsets[BITS];

/*
 * Get a bilinearly interpolated value for a pixel.
 */
inline double interpolate_at_ptr(unsigned char* upperLeft, int i, int columns)
{
	double dx = 1-offsets[i].x;
	double dy = 1-offsets[i].y;
	return
		*upperLeft*dx*dy +
		*(upperLeft+1)*offsets[i].x*dy +
		*(upperLeft+columns)*dx*offsets[i].y +
		*(upperLeft+columns+1)*offsets[i].x*offsets[i].y;
}
/**
 * All the uniform LBP patterns.
 * We use 8-neighbor LBP patterns which include 256 patterns, i.e., 0-255
 * 58 of them belong to uniform pattern
 */
extern int uniformIndex[58];

#endif
