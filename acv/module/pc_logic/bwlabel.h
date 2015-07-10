#ifndef __BWLABEL_WANGFANGLIN__
#define __BWLABEL_WANGFANGLIN__

#include "string.h"		//for function "memset"
#include "opencv2/opencv.hpp"
#include <vector>
#include <iostream>

#define     NO_OBJECT       0
#ifndef		MIN(x,y)
#define     MIN(x,y)       (((x) < (y)) ? (x) : (y))
#endif
#define     ARRAY_ELEM(A, r, c, width) (A)[(r) * (width) + c]

static inline int find( const int set[], int x )
{
    while ( set[x] != x )
        x = set[x];
    return x;
}

template<class T>
static inline bool IsForeground(const T& value)
{
	if (value > 0) return true;
	return false;
}

/**
 * Label connected components in a 2D image and return the number of labels.
 * The label values are greater than or equal to 0. The pixels labeled 0 are the background.
 * The pixels labeled 1 make up one component; the pixels labeled 2 make up a second component; and so on.
 *
 * @param data The input data to be labeled which is an array but actually represents a 2-dimensional data
 * @param width The width of the 2D data
 * @param height The height of the 2D data
 * @param n The number of neighbors, should be only 4, 6 or 8 which are explained below
 * @param labels The labeling result with the same size as "data", containing labels for the connected object in data.
 *
 * Meaning of 4, 6 and 8 connected neighbors:
 * +-+-+-+
 * |D|C|E|
 * +-+-+-+
 * |B|A| |
 * +-+-+-+
 * | | | |
 * +-+-+-+
 * A is the center pixel of a neighborhood.  In the 3 versions of
 * connectedness:
 * 4:  A connects to B and C
 * 6:  A connects to B, C, and D
 * 8:  A connects to B, C, D, and E
 */
template< class T >
int bwlabel(T *data, int width, int height, int n, int *labels)
{
    if(n != 4 && n != 8 && n!= 6)
        n = 4;
    int nr = height;
    int nc = width;
    int total = height * width;
    // Labeling result
    memset(labels, 0, total * sizeof(int));
    int nobj = 0;                               // number of connected components found in image
    // Labels' correspondence table, which shows the mapping between two labels for the first pass
	// E.g., label 2, 5, 7, 11 are actually the same connected component, then lset[2] == lset[5] == lset[7] == lset[11] == 2;
    int* lset = new int[total];
    memset(lset, 0, total * sizeof(int));
    int ntable = 0;
    for( int r = 0; r < nr; r++ )
    {
        for( int c = 0; c < nc; c++ )
        {
            if ( IsForeground(ARRAY_ELEM(data, r, c, width)) )   // if a pixel belongs to foreground
            {
                // Get the neighboring labels of B, C, D, and E by "find" method
                int B, C, D, E;
                if ( c == 0 )  B = 0;
                else B = find( lset, ARRAY_ELEM(labels, r, c - 1, nc) );

                if ( r == 0 ) C = 0;
                else C = find( lset, ARRAY_ELEM(labels, r - 1, c, nc) );

                if ( r == 0 || c == 0 ) D = 0;
                else D = find( lset, ARRAY_ELEM(labels, r - 1, c - 1, nc) );

                if ( r == 0 || c == nc - 1 ) E = 0;
                else E = find( lset, ARRAY_ELEM(labels, r - 1, c + 1, nc) );

                if ( n == 4 )
                {
                    // apply 4 connectedness
                    if ( B && C )
                    {
						// B and C are labeled
                        if ( B == C )
                            ARRAY_ELEM(labels, r, c, nc) = B;
                        else {
                            lset[C] = B;
                            ARRAY_ELEM(labels, r, c, nc) = B;
                        }
                    }
                    else if ( B )             // B is object but C is not
                        ARRAY_ELEM(labels, r, c, nc) = B;
                    else if ( C )               // C is object but B is not
                        ARRAY_ELEM(labels, r, c, nc) = C;
                    else
                    {                      // B, C not object - new object
                        //   label and put into table
                        ntable++;
                        ARRAY_ELEM(labels, r, c, nc) = lset[ ntable ] = ntable;
                    }
                }
                else if ( n == 6 )
                {
                    // apply 6 connected ness
                    if ( D )                    // D object, copy label and move on
                        ARRAY_ELEM(labels, r, c, nc) = D;
                    else if ( B && C )
                    {        // B and C are labeled
                        if ( B == C )
                            ARRAY_ELEM(labels, r, c, nc) = B;
                        else
                        {
                            int tlabel = MIN(B,C);
                            lset[B] = tlabel;
                            lset[C] = tlabel;
                            ARRAY_ELEM(labels, r, c, nc) = tlabel;
                        }
                    }
                    else if ( B )             // B is object but C is not
                        ARRAY_ELEM(labels, r, c, nc) = B;
                    else if ( C )               // C is object but B is not
                        ARRAY_ELEM(labels, r, c, nc) = C;
                    else
                    {                      // B, C, D not object - new object
                        //   label and put into table
                        ntable++;
                        ARRAY_ELEM(labels, r, c, nc) = lset[ ntable ] = ntable;
                    }
                }
                else if ( n == 8 )
                {
                    // apply 8 connectedness
                    if ( B || C || D || E )
                    {
                        int tlabel = B;
                        if ( B ) tlabel = B;
                        else if ( C ) tlabel = C;
                        else if ( D ) tlabel = D;
                        else if ( E ) tlabel = E;

                        ARRAY_ELEM(labels, r, c, nc) = tlabel;

                        if ( B && B != tlabel ) lset[B] = tlabel;
                        if ( C && C != tlabel ) lset[C] = tlabel;
                        if ( D && D != tlabel ) lset[D] = tlabel;
                        if ( E && E != tlabel ) lset[E] = tlabel;
                    }
                    else
                    {
                        // label and put into table
                        ntable++;
                        ARRAY_ELEM(labels, r, c, nc) = lset[ ntable ] = ntable;
                    }
                }
            }
            else
            {
                ARRAY_ELEM(labels, r, c, nc) = NO_OBJECT;      // A is not an object so leave it
            }
        }
    }

    // consolidate component table
    for( int i = 0; i <= ntable; i++ )
        lset[i] = find( lset, i );

    // run image through the look-up table
    for( int r = 0; r < nr; r++ )
        for( int c = 0; c < nc; c++ )
            ARRAY_ELEM(labels, r, c, nc) = lset[ ARRAY_ELEM(labels, r, c, nc) ];

    // count up the objects in the image
    for( int i = 0; i <= ntable; i++ )
        lset[i] = 0;

    for( int r = 0; r < nr; r++ )
        for( int c = 0; c < nc; c++ )
            lset[ ARRAY_ELEM(labels, r, c, nc) ]++;

    // number the objects from 1 through n objects
    nobj = 0;
    lset[0] = 0;
    for( int i = 1; i <= ntable; i++ )
        if ( lset[i] > 0 )
            lset[i] = ++nobj;

    // run through the look-up table again
    for( int r = 0; r < nr; r++ )
        for( int c = 0; c < nc; c++ )
            ARRAY_ELEM(labels, r, c, nc) = lset[ ARRAY_ELEM(labels, r, c, nc) ];
    //
    delete[] lset;
    return nobj;
}

/**
 * Remove all the small connected components except the largest one
 *
 * @param data The input data which is an array but actually represents a 2-dimensional data
 * @param width The width of the 2D data
 * @param height The height of the 2D data
 *
 * NOTE: do not make a mistake when setting "width" and "height"
 */
template <class T>
void keepLargestRegion(T *data, int width, int height)
{
	int npoints = width*height;
	int *labels = new int[npoints];
	int nlabels = bwlabel(data, width, height, 8, labels);
	// The number of pixels in each connected component
	int* quantities = new int[nlabels];
	memset(quantities, 0, nlabels*sizeof(int));

	for (int i = 0; i < npoints; i++)
		if (labels[i] != 0) quantities[labels[i]-1]++;
	int maxLabel = 0, maxQuantity = 0;

	for (int i = 0; i < nlabels; i++) {
		if (maxQuantity < quantities[i]) {
			maxQuantity = quantities[i];
			maxLabel = i + 1;
		}
	}
	for (int i = 0; i < npoints; i++)
		if (labels[i] != maxLabel) data[i] = 0;
	delete []quantities;
	delete []labels;

}

/**
 * Remove all the small connected components whose number of pixels is below the specified value
 *
 * @param data The input data which is an array but actually represents a 2-dimensional data
 * @param width The width of the 2D data
 * @param height The height of the 2D data
 *
 * NOTE: do not make a mistake when setting "width" and "height"
 */
template <class T>
void removeSmallRegions(T *data, std::vector<cv::Rect>& rois, int width, int height, int threshold)
{
	int npoints = width*height;
	int *labels = new int[npoints];
	int nlabels = bwlabel(data, width, height, 4, labels);
	// The number of pixels in each connected component
	int* quantities = new int[nlabels];
	memset(quantities, 0, nlabels*sizeof(int));

	for (int i = 0; i < npoints; i++)
		if (labels[i] != 0) quantities[labels[i]-1]++;

	std::vector<bool> removedLabelTable(nlabels);
	std::fill(removedLabelTable.begin(), removedLabelTable.end(), true);
	int nrois = 0;
	for (int i = 0; i < nlabels; i++) {
		if (quantities[i] > threshold) {
			removedLabelTable[i] = false;
			++nrois;
		}
	}
	rois.clear();
	rois.resize(nlabels);
	for (int i = 0; i < rois.size(); i++) {
		rois[i].x = width;
		rois[i].y = height;
		rois[i].width = 0;
		rois[i].height = 0;
	}

	for (int i = 0; i < npoints; i++) {
		if (labels[i] == 0) continue;
		int label = labels[i] - 1;
		if (removedLabelTable[label])
			data[i] = 0;
		else {
			int x = i % width;
			int y = i / width;
			if (x < rois[label].x) rois[label].x = x;
			if (y < rois[label].y) rois[label].y = y;
			if (x - rois[label].x > rois[label].width) rois[label].width = x - rois[label].x;
			if (y - rois[label].y > rois[label].height) rois[label].height = y - rois[label].y;
		}

	}

	//Remove the "zero" rect
	for (int i = 0; i < rois.size(); i++) {
		if(rois[i].width == 0 || rois[i].height == 0) {
			rois.erase(rois.begin()+i);
			i--;
		}
	}

	delete []quantities;
	delete []labels;
	//cout << rois.size() << endl;
}

#endif
