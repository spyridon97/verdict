/*=========================================================================

  Module:    V_PyramidMetric.cpp

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 *
 * PyramidMetrics.cpp contains quality calculations for Pyramids
 *
 * This file is part of VERDICT
 *
 */


#include "verdict.h"
#include "VerdictVector.hpp"
#include "verdict_defines.hpp"
#include <memory.h> 

// local methods
void v_make_pyramid_tets(double coordinates[][3], double tet1_coords[][3], double tet2_coords[][3],
                                                  double tet3_coords[][3], double tet4_coords[][3]);
void v_make_pyramid_faces(double coordinates[][3], double base[][3], double tri1[][3],
                          double tri2[][3], double tri3[][3], double tri4[][3]);
void v_make_pyramid_edges( VerdictVector edges[7], double coordinates[][3] );
double v_distance_point_to_pyramid_base( int num_nodes, double coordinates[][3], double &cos_angle);
double v_largest_pyramid_edge( double coordinates[][3] );
/*
  the pyramid element

       5
       ^
       |\ 
      /| \\_
     |  \   \
     |  | \_ \_
     /   \/4\  \
    |   /|    \ \_
    | /  \      \ \
    /     |       \
  1 \_    |      _/3
      \_   \   _/
        \_ | _/
          \_/
          2

    a quadrilateral base and a pointy peak like a pyramid
          
*/


/*!
  the volume of a pyramid

  the volume is calculated by dividing the pyramid into
  2 tets and summing the volumes of the 2 tets.
*/

C_FUNC_DEF double v_pyramid_volume( int num_nodes, double coordinates[][3] )
{
    
  double volume = 0;
  VerdictVector side1, side2, side3;
  
  if (num_nodes == 5)
  {
    // divide the pyramid into 2 tets and calculate each

    side1.set( coordinates[1][0] - coordinates[0][0],
        coordinates[1][1] - coordinates[0][1],
        coordinates[1][2] - coordinates[0][2] );
    
    side2.set( coordinates[3][0] - coordinates[0][0],
        coordinates[3][1] - coordinates[0][1],
        coordinates[3][2] - coordinates[0][2] );
    
    side3.set( coordinates[4][0] - coordinates[0][0],
        coordinates[4][1] - coordinates[0][1], 
        coordinates[4][2] - coordinates[0][2] );
    
    // volume of the first tet
    volume = (side3 % (side1 * side2 ))/6.0;
    
    
    side1.set( coordinates[3][0] - coordinates[2][0],
        coordinates[3][1] - coordinates[2][1],
        coordinates[3][2] - coordinates[2][2] );
    
    side2.set( coordinates[1][0] - coordinates[2][0],
        coordinates[1][1] - coordinates[2][1],
        coordinates[1][2] - coordinates[2][2] );
    
    side3.set( coordinates[4][0] - coordinates[2][0],
        coordinates[4][1] - coordinates[2][1],
        coordinates[4][2] - coordinates[2][2] );
    
    // volume of the second tet
    volume += (side3 % (side1 * side2 ))/6.0;
 
  }   
  return (double)volume;
    
}

C_FUNC_DEF double v_pyramid_jacobian( int num_nodes, double coordinates[][3] )
{
  // break the pyramid into four tets return the minimum scaled jacobian of the two tets
  double tet1_coords[4][3];
  double tet2_coords[4][3];
  double tet3_coords[4][3];
  double tet4_coords[4][3];

  v_make_pyramid_tets(coordinates, tet1_coords, tet2_coords, tet3_coords, tet4_coords);

  double j1 = v_tet_jacobian(4, tet1_coords);
  double j2 = v_tet_jacobian(4, tet2_coords);
  double j3 = v_tet_jacobian(4, tet3_coords);
  double j4 = v_tet_jacobian(4, tet4_coords);

  double p1 = j1 < j2 ? j1 : j2;
  double p2 = j3 < j4 ? j3 : j4;

  return p1 < p2 ? p1 : p2;
}

C_FUNC_DEF double v_pyramid_scaled_jacobian( int num_nodes, double coordinates[][3] )
{
  // break the pyramid into four tets return the minimum scaled jacobian of the two tets
  double tet1_coords[4][3];
  double tet2_coords[4][3];
  double tet3_coords[4][3];
  double tet4_coords[4][3];
  double min_scaled_jac = VERDICT_DBL_MAX;

  v_make_pyramid_tets(coordinates, tet1_coords, tet2_coords, tet3_coords, tet4_coords);

  double j1 = v_tet_jacobian(4, tet1_coords);
  double j2 = v_tet_jacobian(4, tet2_coords);
  double j3 = v_tet_jacobian(4, tet3_coords);
  double j4 = v_tet_jacobian(4, tet4_coords);

  VerdictVector edges[8];
  v_make_pyramid_edges(edges, coordinates);

  double length[8];
  length[0] = edges[0].length();
  length[1] = edges[1].length();
  length[2] = edges[2].length();
  length[3] = edges[3].length();
  length[4] = edges[4].length();
  length[5] = edges[5].length();
  length[6] = edges[6].length();
  length[7] = edges[7].length();

  if (length[0] < VERDICT_DBL_MIN ||
      length[1] < VERDICT_DBL_MIN ||
      length[2] < VERDICT_DBL_MIN ||
      length[3] < VERDICT_DBL_MIN ||
      length[4] < VERDICT_DBL_MIN ||
      length[5] < VERDICT_DBL_MIN ||
      length[6] < VERDICT_DBL_MIN ||
      length[7] < VERDICT_DBL_MIN )
    return 0;

  double factor = sqrt(2.0)/2;
  double scaled_jac = j1/(length[0] * length[1] * length[5] * factor);
  min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac);

  scaled_jac = j2/(length[2] * length[3] * length[7] * factor);
  min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac);

  scaled_jac = j3/(length[0] * length[3] * length[4] * factor);
  min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac);

  scaled_jac = j4/(length[1] * length[2] * length[6] * factor);
  min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac);

  return min_scaled_jac;

#if 0
  // ideally there will be four equilateral triangles and one square.  
  // Test each face
  double base[4][3];
  double tri1[3][3];
  double tri2[3][3];
  double tri3[3][3];
  double tri4[3][3];

  v_make_pyramid_faces(coordinates, base, tri1, tri2, tri3, tri4);

  double s1 = v_quad_scaled_jacobian(4, base);
  double s2 = v_tri_scaled_jacobian(3, tri1);
  double s3 = v_tri_scaled_jacobian(3, tri2);
  double s4 = v_tri_scaled_jacobian(3, tri3);
  double s5 = v_tri_scaled_jacobian(3, tri4);

  return .2*(s1 + s2 + s3 + s4 + s5);
#endif
}


C_FUNC_DEF double v_pyramid_shape( int num_nodes, double coordinates[][3] )
{
  static double SQRT2_HALVES = sqrt(2.0)/2.0;

  // ideally there will be four equilateral triangles and one square.  
  // Test each face
  double base[4][3];
  double tri1[3][3];
  double tri2[3][3];
  double tri3[3][3];
  double tri4[3][3];

  v_make_pyramid_faces(coordinates, base, tri1, tri2, tri3, tri4);

  double s1 = v_quad_shape(4, base);

  if (s1 == 0.0)
    return 0.0;

  double cos_angle;
  double dist_to_base = v_distance_point_to_pyramid_base(num_nodes, coordinates, cos_angle );

  if (dist_to_base <= 0 || cos_angle <= 0.0)
    return 0.0;

  double longest_edge = v_largest_pyramid_edge(coordinates) * SQRT2_HALVES;
  if (dist_to_base < longest_edge)
    dist_to_base = dist_to_base/longest_edge;
  else
    dist_to_base = longest_edge/dist_to_base;

  double shape = s1 * cos_angle * dist_to_base;

  return shape;
}

void v_make_pyramid_tets(double coordinates[][3], double tet1_coords[][3], double tet2_coords[][3],
                                                  double tet3_coords[][3], double tet4_coords[][3])
{
   // tet1
  tet1_coords[0][0] = coordinates[0][0];
  tet1_coords[0][1] = coordinates[0][1];
  tet1_coords[0][2] = coordinates[0][2];

  tet1_coords[1][0] = coordinates[1][0];
  tet1_coords[1][1] = coordinates[1][1];
  tet1_coords[1][2] = coordinates[1][2];

  tet1_coords[2][0] = coordinates[2][0];
  tet1_coords[2][1] = coordinates[2][1];
  tet1_coords[2][2] = coordinates[2][2];

  tet1_coords[3][0] = coordinates[4][0];
  tet1_coords[3][1] = coordinates[4][1];
  tet1_coords[3][2] = coordinates[4][2];

  // tet2
  tet2_coords[0][0] = coordinates[0][0];
  tet2_coords[0][1] = coordinates[0][1];
  tet2_coords[0][2] = coordinates[0][2];

  tet2_coords[1][0] = coordinates[2][0];
  tet2_coords[1][1] = coordinates[2][1];
  tet2_coords[1][2] = coordinates[2][2];

  tet2_coords[2][0] = coordinates[3][0];
  tet2_coords[2][1] = coordinates[3][1];
  tet2_coords[2][2] = coordinates[3][2];

  tet2_coords[3][0] = coordinates[4][0];
  tet2_coords[3][1] = coordinates[4][1];
  tet2_coords[3][2] = coordinates[4][2];

  // tet3
  tet3_coords[0][0] = coordinates[0][0];
  tet3_coords[0][1] = coordinates[0][1];
  tet3_coords[0][2] = coordinates[0][2];

  tet3_coords[1][0] = coordinates[1][0];
  tet3_coords[1][1] = coordinates[1][1];
  tet3_coords[1][2] = coordinates[1][2];

  tet3_coords[2][0] = coordinates[3][0];
  tet3_coords[2][1] = coordinates[3][1];
  tet3_coords[2][2] = coordinates[3][2];

  tet3_coords[3][0] = coordinates[4][0];
  tet3_coords[3][1] = coordinates[4][1];
  tet3_coords[3][2] = coordinates[4][2];

  // tet4
  tet4_coords[0][0] = coordinates[1][0];
  tet4_coords[0][1] = coordinates[1][1];
  tet4_coords[0][2] = coordinates[1][2];

  tet4_coords[1][0] = coordinates[2][0];
  tet4_coords[1][1] = coordinates[2][1];
  tet4_coords[1][2] = coordinates[2][2];

  tet4_coords[2][0] = coordinates[3][0];
  tet4_coords[2][1] = coordinates[3][1];
  tet4_coords[2][2] = coordinates[3][2];

  tet4_coords[3][0] = coordinates[4][0];
  tet4_coords[3][1] = coordinates[4][1];
  tet4_coords[3][2] = coordinates[4][2];
}

void v_make_pyramid_faces(double coordinates[][3], double base[][3], double tri1[][3],
                          double tri2[][3], double tri3[][3], double tri4[][3])
{
   // base
  base[0][0] = coordinates[0][0];
  base[0][1] = coordinates[0][1];
  base[0][2] = coordinates[0][2];

  base[1][0] = coordinates[1][0];
  base[1][1] = coordinates[1][1];
  base[1][2] = coordinates[1][2];

  base[2][0] = coordinates[2][0];
  base[2][1] = coordinates[2][1];
  base[2][2] = coordinates[2][2];

  base[3][0] = coordinates[3][0];
  base[3][1] = coordinates[3][1];
  base[3][2] = coordinates[3][2];

  // tri1
  tri1[0][0] = coordinates[0][0];
  tri1[0][1] = coordinates[0][1];
  tri1[0][2] = coordinates[0][2];

  tri1[1][0] = coordinates[1][0];
  tri1[1][1] = coordinates[1][1];
  tri1[1][2] = coordinates[1][2];

  tri1[2][0] = coordinates[4][0];
  tri1[2][1] = coordinates[4][1];
  tri1[2][2] = coordinates[4][2];

  // tri2
  tri2[0][0] = coordinates[1][0];
  tri2[0][1] = coordinates[1][1];
  tri2[0][2] = coordinates[1][2];

  tri2[1][0] = coordinates[2][0];
  tri2[1][1] = coordinates[2][1];
  tri2[1][2] = coordinates[2][2];

  tri2[2][0] = coordinates[4][0];
  tri2[2][1] = coordinates[4][1];
  tri2[2][2] = coordinates[4][2];

  // tri3
  tri3[0][0] = coordinates[2][0];
  tri3[0][1] = coordinates[2][1];
  tri3[0][2] = coordinates[2][2];

  tri3[1][0] = coordinates[3][0];
  tri3[1][1] = coordinates[3][1];
  tri3[1][2] = coordinates[3][2];

  tri3[2][0] = coordinates[4][0];
  tri3[2][1] = coordinates[4][1];
  tri3[2][2] = coordinates[4][2];

   // tri4
  tri4[0][0] = coordinates[3][0];
  tri4[0][1] = coordinates[3][1];
  tri4[0][2] = coordinates[3][2];

  tri4[1][0] = coordinates[0][0];
  tri4[1][1] = coordinates[0][1];
  tri4[1][2] = coordinates[0][2];

  tri4[2][0] = coordinates[4][0];
  tri4[2][1] = coordinates[4][1];
  tri4[2][2] = coordinates[4][2];
}

void v_make_pyramid_edges( VerdictVector edges[7], double coordinates[][3] )
{

  edges[0].set(
    coordinates[1][0] - coordinates[0][0],
    coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]
  );
  edges[1].set(
    coordinates[2][0] - coordinates[1][0],
    coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]
  );
  edges[2].set(
    coordinates[3][0] - coordinates[2][0],
    coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]
  );
  edges[3].set(
    coordinates[0][0] - coordinates[3][0],
    coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]
  );
  edges[4].set(
    coordinates[4][0] - coordinates[0][0],
    coordinates[4][1] - coordinates[0][1],
    coordinates[4][2] - coordinates[0][2]
  );
  edges[5].set(
    coordinates[4][0] - coordinates[1][0],
    coordinates[4][1] - coordinates[1][1],
    coordinates[4][2] - coordinates[1][2]
  );
  edges[6].set(
    coordinates[4][0] - coordinates[2][0],
    coordinates[4][1] - coordinates[2][1],
    coordinates[4][2] - coordinates[2][2]
  );
  edges[7].set(
    coordinates[4][0] - coordinates[3][0],
    coordinates[4][1] - coordinates[3][1],
    coordinates[4][2] - coordinates[3][2]
  );
}

double v_largest_pyramid_edge( double coordinates[][3] )
{
  VerdictVector edges[8];
  v_make_pyramid_edges(edges, coordinates);
  double l0 = edges[0].length_squared();
  double l1 = edges[1].length_squared();
  double l2 = edges[2].length_squared();
  double l3 = edges[3].length_squared();
  double l4 = edges[4].length_squared();
  double l5 = edges[5].length_squared();
  double l6 = edges[6].length_squared();
  double l7 = edges[7].length_squared();
  
  double max = VERDICT_MIN(l0, l1);
  max = VERDICT_MAX(max, l2);
  max = VERDICT_MAX(max, l3);
  max = VERDICT_MAX(max, l4);
  max = VERDICT_MAX(max, l5);
  max = VERDICT_MAX(max, l6);
  max = VERDICT_MAX(max, l7);

  return sqrt(max);
}

double v_distance_point_to_pyramid_base( int num_nodes, double coordinates[][3], double &cos_angle )
{
  VerdictVector a(coordinates[0]);
  VerdictVector b(coordinates[1]);
  VerdictVector c(coordinates[2]);
  VerdictVector d(coordinates[3]);
  VerdictVector peak(coordinates[4]);

  VerdictVector centroid = (a + b + c + d)/4.0;
  VerdictVector t1 = b - a;
  VerdictVector t2 = d - a;

  VerdictVector normal = t1 * t2;
  double normal_length = normal.length();
  
  VerdictVector pq = peak - centroid;

  double distance =  (pq % normal)/normal_length;
  cos_angle = distance/pq.length();

  return distance;
}


C_FUNC_DEF void v_pyramid_quality( int num_nodes, double coordinates[][3], 
    unsigned int metrics_request_flag, PyramidMetricVals *metric_vals )
{
  memset( metric_vals, 0, sizeof( PyramidMetricVals ) );

  if(metrics_request_flag & V_PYRAMID_VOLUME)
    metric_vals->volume = v_pyramid_volume(num_nodes, coordinates);
  else if(metrics_request_flag & V_PYRAMID_JACOBIAN)
    metric_vals->jacobian = v_pyramid_jacobian(num_nodes, coordinates);
  else if(metrics_request_flag & V_PYRAMID_SCALED_JACOBIAN)
    metric_vals->scaled_jacobian = v_pyramid_scaled_jacobian(num_nodes, coordinates);
  else if(metrics_request_flag & V_PYRAMID_SHAPE)
    metric_vals->shape = v_pyramid_shape(num_nodes, coordinates);
}

