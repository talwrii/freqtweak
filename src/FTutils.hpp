/*
** Copyright (C) 2002 Jesse Chappell <jesse@essej.net>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**  
*/

#ifndef __FTUTILS_HPP__
#define __FTUTILS_HPP__



class FTutils
{
public:

/* Rough but quick versions of some basic math functions */
/* There is no input error checking.                     */
	
	
/* Logarithm base-10.      */
/* absolute error < 6.4e-4 */
/* input must be > 0       */
	static float fast_log10 (float x);

/* Logarithm base-2.      */
/* absolute error < 6.4e-4 */
/* input must be > 0       */
	static float fast_log2 (float x);
	

/* Square root.           */
/* relative error < 0.08% */
/* input must be >= 0     */
	static float fast_square_root (float x);


/* Fourth root.           */
/* relative error < 0.06% */
/* input must be >= 0     */
	static float fast_fourth_root (float x);


/* vector versions of the above.    */
/* "in" and "out" pointer can refer */
/* to the same array for in-place   */
/* computation                      */

	static void vector_fast_log2 (const float* x_input, float* y_output, int N);
	static void vector_fast_log10 (const float* x_input, float* y_output, int N);
	static void vector_fast_square_root (const float* x_input, float* y_output, int N);
	static void vector_fast_fourth_root (const float* x_input, float* y_output, int N);
	


	static inline float powerLogScale(float yval, float min);
	
	
};



inline float FTutils::powerLogScale(float yval, float min)
{
			
	if (yval <= min) {
		return -200.0;
	}
	
//   	if (yval > _maxval) {
//   		_maxval = yval;
//   	}
	
	//float nval = (10.0 * FTutils::fast_log10(yval / max));
	float nval = (10.0 * FTutils::fast_log10 (yval));
	// printf ("scaled value is %g   mincut=%g\n", nval, _minCutoff);
	return nval;
}


#endif
