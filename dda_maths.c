
/** \file
  \brief Mathematic algorithms for the digital differential analyser (DDA).
*/

#include "dda_maths.h"

#include <stdlib.h>
#include <stdint.h>

#ifdef SCARA_PRINTER
/*
	floating pointer is only needed for Scara-type printers at the moment.
*/
#include <math.h>
#endif

/*!
  Integer multiply-divide algorithm. Returns the same as muldiv(multiplicand, multiplier, divisor), but also allowing to use precalculated quotients and remainders.

  \param multiplicand
  \param qn ( = multiplier / divisor )
  \param rn ( = multiplier % divisor )
  \param divisor
  \return rounded result of multiplicand * multiplier / divisor

  Calculate a * b / c, without overflowing and without using 64-bit integers.
  Doing this the standard way, a * b could easily overflow, even if the correct
  overall result fits into 32 bits. This algorithm avoids this intermediate
  overflow and delivers valid results for all cases where each of the three
  operators as well as the result fits into 32 bits.

  Found on  http://stackoverflow.com/questions/4144232/
  how-to-calculate-a-times-b-divided-by-c-only-using-32-bit-integer-types-even-i
*/
const int32_t muldivQR(int32_t multiplicand, uint32_t qn, uint32_t rn,
                       uint32_t divisor) {
  uint32_t quotient = 0;
  uint32_t remainder = 0;
  uint8_t negative_flag = 0;

  if (multiplicand < 0) {
    negative_flag = 1;
    multiplicand = -multiplicand;
  }

  while(multiplicand) {
    if (multiplicand & 1) {
      quotient += qn;
      remainder += rn;
      if (remainder >= divisor) {
        quotient++;
        remainder -= divisor;
      }
    }
    multiplicand  >>= 1;
    qn <<= 1;
    rn <<= 1;
    if (rn >= divisor) {
      qn++; 
      rn -= divisor;
    }
  }

  // rounding
  if (remainder > divisor / 2)
    quotient++;

  // remainder is valid here, but not returned
  return negative_flag ? -((int32_t)quotient) : (int32_t)quotient;
}

// courtesy of http://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
/*! linear approximation 2d distance formula
  \param dx distance in X plane
  \param dy distance in Y plane
  \return 3-part linear approximation of \f$\sqrt{\Delta x^2 + \Delta y^2}\f$

  see http://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
*/
uint32_t approx_distance(uint32_t dx, uint32_t dy) {
  uint32_t min, max, approx;

  // If either axis is zero, return the other one.
  if (dx == 0 || dy == 0) return dx + dy;

  if ( dx < dy ) {
    min = dx;
    max = dy;
  } else {
    min = dy;
    max = dx;
  }

  approx = ( max * 1007 ) + ( min * 441 );
  if ( max < ( min << 4 ))
    approx -= ( max * 40 );

  // add 512 for proper rounding
  return (( approx + 512 ) >> 10 );
}

// courtesy of http://www.oroboro.com/rafael/docserv.php/index/programming/article/distance
/*! linear approximation 3d distance formula
  \param dx distance in X plane
  \param dy distance in Y plane
  \param dz distance in Z plane
  \return 3-part linear approximation of \f$\sqrt{\Delta x^2 + \Delta y^2 + \Delta z^2}\f$

  see http://www.oroboro.com/rafael/docserv.php/index/programming/article/distance
*/
uint32_t approx_distance_3(uint32_t dx, uint32_t dy, uint32_t dz) {
  uint32_t min, med, max, approx;

  if ( dx < dy ) {
    min = dy;
    med = dx;
  } else {
    min = dx;
    med = dy;
  }

  if ( dz < min ) {
    max = med;
    med = min;
    min = dz;
  } else if ( dz < med ) {
    max = med;
    med = dz;
  } else {
    max = dz;
  }

  approx = ( max * 860 ) + ( med * 851 ) + ( min * 520 );
  if ( max < ( med << 1 )) approx -= ( max * 294 );
  if ( max < ( min << 2 )) approx -= ( max * 113 );
  if ( med < ( min << 2 )) approx -= ( med *  40 );

  // add 512 for proper rounding
  return (( approx + 512 ) >> 10 );
}

/*!
  integer square root algorithm
  \param a find square root of this number
  \return sqrt(a - 1) < returnvalue <= sqrt(a)

  This is a binary search but it uses only the minimum required bits for
  each step.
*/
uint16_t int_sqrt(uint32_t a) {
  uint16_t b = a >> 16;
  uint8_t c = b >> 8;
  uint16_t x = 0;
  uint8_t z = 0;
  uint16_t i;
  uint8_t j;

  for (j = 0x8; j; j >>= 1) {
    uint8_t y2;

    z |= j;
    y2 = z * z;
    if (y2 > c)
      z ^= j;
  }
  
  x = z << 4;
  for(i = 0x8; i; i >>= 1) {
    uint16_t y2;

    x |= i;
    y2 = x * x;
    if (y2 > b)
      x ^= i;
  }
  
  x <<= 8;
  for(i = 0x80; i; i >>= 1) {
    uint32_t y2;

    x |= i;
    y2 = (uint32_t)x * x;
    if (y2 > a)
      x ^= i;
  }

  return x;
}

/*!
  integer inverse square root algorithm
  \param a find the inverse of the square root of this number
  \return 0x1000 / sqrt(a) - 1 < returnvalue <= 0x1000 / sqrt(a)

  This is a binary search but it uses only the minimum required bits for each step.
*/
uint16_t int_inv_sqrt(uint16_t a) {
  /// 16bits inverse (much faster than doing a full 32bits inverse)
  /// the 0xFFFFU instead of 0x10000UL hack allows using 16bits and 8bits
  /// variable for the first 8 steps without overflowing and it seems to
  /// give better results for the ramping equation too :)
  uint8_t z = 0, i;
  uint16_t x, j;
  uint32_t q = ((uint32_t)(0xFFFFU / a)) << 8;

  for (i = 0x80; i; i >>= 1) {
    uint16_t y;

    z |= i;
    y = (uint16_t)z * z;
    if (y > (q >> 8))
      z ^= i;
  }

  x = z << 4;
  for (j = 0x8; j; j >>= 1) {
    uint32_t y;

    x |= j;
    y = (uint32_t)x * x;
    if (y > q)
      x ^= j;
  }

  return x;
}

// this is an ultra-crude pseudo-logarithm routine, such that:
// 2 ^ msbloc(v) >= v
/*! crude logarithm algorithm
  \param v value to find \f$log_2\f$ of
  \return floor(log(v) / log(2))
*/
const uint8_t msbloc (uint32_t v) {
  uint8_t i;
  uint32_t c;
  for (i = 31, c = 0x80000000; i; i--) {
    if (v & c)
      return i;
    c >>= 1;
  }
  return 0;
}

/*! Acceleration ramp length in steps.
 * \param feedrate Target feedrate of the accelerateion.
 * \param steps_per_m Steps/m of the axis.
 * \return Accelerating steps neccessary to achieve target feedrate.
 *
 * s = 1/2 * a * t^2, v = a * t ==> s = v^2 / (2 * a)
 * 7200000 = 60 * 60 * 1000 * 2 (mm/min -> mm/s, steps/m -> steps/mm, factor 2)
 *
 * Note: this function has shown to be accurate between 10 and 10'000 mm/s2 and
 *       2000 to 4096000 steps/m (and higher). The numbers are a few percent
 *       too high at very low acceleration. Test code see commit message.
 */
uint32_t acc_ramp_len(uint32_t feedrate, uint32_t steps_per_m) {
  return (feedrate * feedrate) /
         (((uint32_t)7200000UL * ACCELERATION) / steps_per_m);
}

/*
	Integer square giving int64 result type
*/
int64_t int_sqr(int32_t a) {
	return a * a;
}

#ifdef SCARA_PRINTER
/*
	Scara-type printers need a different calculation, because the ratio
	between xy-coordinates and xy-steps is not constant.
	The start coordinates are needed here, because the steps between two points 
	depend on the coordinates of start- and endpoint.

	The following calculations are a work of Quentin Harley, the inventor of the 
	RepRap Morgan Scara-printer. We need 64-bit integers here, because we need to
	square micrometer units. Assuming a 200mm by 200mm print area we need to handle
	200,000 x 200,000 = 40,000,000,000 (35 bits needed to represent that).
	Thank you Quentin for your outstanding work.

	For Scara we need more memory and CPU-cycles, compared to a Delta-type printer.

	pos_x, pos_y, distance_x and distance_y are micrometer values. 
*/
void getPhiTheta(int32_t x, int32_t y, double *phi, double *theta) {

	int64_t scara_pos_x = x - SCARA_TOWER_OFFSET_X;
	int64_t scara_pos_y = y - SCARA_TOWER_OFFSET_Y;

	#if (INNER_ARM_LENGTH == OUTER_ARM_LENGTH)
		int64_t scara_c2 = (int_sqr(scara_pos_x)+int_sqr(scara_pos_y)-2*int_sqr(INNER_ARM_LENGTH)) / (2 * int_sqr(INNER_ARM_LENGTH));
	#else
		int64_t scara_c2 = (int_sqr(scara_pos_x)+int_sqr(scara_pos_y)-int_sqr(INNER_ARM_LENGTH)-int_sqr(OUTER_ARM_LENGTH)) / 45000; 
	#endif

	int64_t scara_s2 = sqrt(1-int_sqr(scara_c2));

	int64_t scara_k1 = INNER_ARM_LENGTH + OUTER_ARM_LENGTH * scara_c2;
	int64_t scara_k2 = OUTER_ARM_LENGTH * scara_s2;

	/*
		TODO:
		Replace atan2-calls with an implementation of the Cordic-algorithm or similar.
	*/
	*theta = (atan2(scara_pos_x,scara_pos_y)-atan2(scara_k1, scara_k2))*-1;
	*phi = atan2(scara_s2,scara_c2);
}

void scara_um_to_steps(int32_t pos_x, int32_t pos_y, int32_t distance_x, int32_t distance_y, int32_t *steps_x, int32_t *steps_y) {

	double	phi_start,  //Phi-angle of startpoint
			phi_dest;	//Theta-angle of startpoint
	double	theta_start,	//Phi-angle of destination point
			theta_dest;		//Theta-angle of destination point

	getPhiTheta(pos_x, pos_y, &phi_start, &theta_start);
	getPhiTheta(pos_x + distance_x,pos_y+ distance_y, &phi_dest, &theta_dest);

	double phi_delta= phi_dest + phi_start;
	double theta_delta = theta_dest - theta_start;

	/*
		degrees = (radians * 4068) / 71
		This is more accurate for floating point operations than radians*180/Pi

		For Scara-type printers, a "unit" (STEPS_PER_M_...) is one degree (1/1000th of a degree in Teacup).
		To here we have calculated the required changes in radians, the scara-arms have to move to bring 
		the nozzle from (pos_x,pos_y) to (pos_x+distance_x,pos_y+distance_y).
		Now we have to convert to degerees and calculate the steps.
	*/
	*steps_x = (int32_t) trunc(um_to_steps_x(((theta_delta * 4068) / 71) * 1000000UL));  
	*steps_y = (int32_t) trunc(um_to_steps_y((((theta_delta + phi_delta) * 4068) / 71) * 1000000UL)); 
}
#endif

