#include "tools.h"

#define MATH_3D_IMPLEMENTATION
/**

Math 3D v1.0 - 2016-02-15
By Stephan Soller <stephan.soller@helionweb.de> and Tobias Malmsheimer
Licensed under the MIT license

Math 3D is a compact C99 library meant to be used with OpenGL. It provides basic
3D vector and 4x4 matrix operations as well as functions to create transformation
and projection matrices. The OpenGL binary layout is used so you can just upload
vectors and matrices into shaders and work with them without any conversions.

It's an stb style single header file library. Define MATH_3D_IMPLEMENTATION
before you include this file in *one* C file to create the implementation.


QUICK NOTES

- If not explicitly stated by a parameter name all angles are in radians.
- The matrices use column-major indices. This is the same as in OpenGL and GLSL.
  The matrix documentation below for details.
- Matrices are passed by value. This is probably a bit inefficient but
  simplifies code quite a bit. Most operations will be inlined by the compiler
  anyway so the difference shouldn't matter that much. A matrix fits into 4 of
  the 16 SSE2 registers anyway. If profiling shows significant slowdowns the
  matrix type might change but ease of use is more important than every last
  percent of performance.
- When combining matrices with multiplication the effects apply right to left.
  This is the convention used in mathematics and OpenGL. Source:
  https://en.wikipedia.org/wiki/Transformation_matrix#Composing_and_inverting_transformations
  Direct3D does it differently.
- The `m4_mul_pos()` and `m4_mul_dir()` functions do a correct perspective
  divide (division by w) when necessary. This is a bit slower but ensures that
  the functions will properly work with projection matrices. If profiling shows
  this is a bottleneck special functions without perspective division can be
  added. But the normal multiplications should avoid any surprises.
- The library consistently uses a right-handed coordinate system. The old
  `glOrtho()` broke that rule and `m4_ortho()` has be slightly modified so you
  can always think of right-handed cubes that are projected into OpenGLs
  normalized device coordinates.
- Special care has been taken to document all complex operations and important
  sources. Most code is covered by test cases that have been manually calculated
  and checked on the whiteboard. Since indices and math code is prone to be
  confusing we used pair programming to avoid mistakes.

**/

#ifndef MATH_3D_HEADER
#define MATH_3D_HEADER

// Define PI directly because we would need to define the _BSD_SOURCE or
// _XOPEN_SOURCE feature test macros to get it from math.h. That would be a
// rather harsh dependency. So we define it directly if necessary.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


//
// 3D vectors
// 
// Use the `vec3()` function to create vectors. All other vector functions start
// with the `v3_` prefix.
// 
// The binary layout is the same as in GLSL and everything else (just 3 floats).
// So you can just upload the vectors into shaders as they are.
//

typedef struct { unsigned int x, y; } uvec2_t;
typedef struct { float x, y; }        vec2_t;
typedef struct { float x, y, z; }     vec3_t;

static inline vec3_t vec3(float x, float y, float z) { return (vec3_t){ x, y, z }; }

static inline vec3_t v3_add   (vec3_t a, vec3_t b) { return (vec3_t){ a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline vec3_t v3_adds  (vec3_t a, float s)  { return (vec3_t){ a.x + s,   a.y + s,   a.z + s   }; }
static inline vec3_t v3_sub   (vec3_t a, vec3_t b) { return (vec3_t){ a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline vec3_t v3_subs  (vec3_t a, float s)  { return (vec3_t){ a.x - s,   a.y - s,   a.z - s   }; }
static inline vec3_t v3_mul   (vec3_t a, vec3_t b) { return (vec3_t){ a.x * b.x, a.y * b.y, a.z * b.z }; }
static inline vec3_t v3_muls  (vec3_t a, float s)  { return (vec3_t){ a.x * s,   a.y * s,   a.z * s   }; }
static inline vec3_t v3_div   (vec3_t a, vec3_t b) { return (vec3_t){ a.x / b.x, a.y / b.y, a.z / b.z }; }
static inline vec3_t v3_divs  (vec3_t a, float s)  { return (vec3_t){ a.x / s,   a.y / s,   a.z / s   }; }
static inline float  v3_length(vec3_t v)           { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
static inline vec3_t v3_norm  (vec3_t v);
static inline float  v3_dot   (vec3_t a, vec3_t b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline vec3_t v3_proj  (vec3_t v, vec3_t onto);
static inline vec3_t v3_cross (vec3_t a, vec3_t b);
static inline float  v3_angle_between(vec3_t a, vec3_t b);

static inline vec2_t v2_add   (vec2_t a, vec2_t b) { return (vec2_t){ a.x + b.x, a.y + b.y}; }
static inline vec2_t v2_mul   (vec2_t a, vec2_t b) { return (vec2_t){ a.x * b.x, a.y * b.y}; }
static inline vec2_t v2_div   (vec2_t a, vec2_t b) { return (vec2_t){ a.x / b.x, a.y / b.y}; }

//
// 4×4 matrices
//
// Use the `mat4()` function to create a matrix. You can write the matrix
// members in the same way as you would write them on paper or on a whiteboard:
// 
// mat4_t m = mat4(
//     1,  0,  0,  7,
//     0,  1,  0,  5,
//     0,  0,  1,  3,
//     0,  0,  0,  1
// )
// 
// This creates a matrix that translates points by vec3(7, 5, 3). All other
// matrix functions start with the `m4_` prefix. Among them functions to create
// identity, translation, rotation, scaling and projection matrices.
// 
// The matrix is stored in column-major order, just as OpenGL expects. Members
// can be accessed by indices or member names. When you write a matrix on paper
// or on the whiteboard the indices and named members correspond to these
// positions:
// 
// | m[0][0]  m[1][0]  m[2][0]  m[3][0] |
// | m[0][1]  m[1][1]  m[2][1]  m[3][1] |
// | m[0][2]  m[1][2]  m[2][2]  m[3][2] |
// | m[0][3]  m[1][3]  m[2][3]  m[3][3] |
// 
// | m00  m10  m20  m30 |
// | m01  m11  m21  m31 |
// | m02  m12  m22  m32 |
// | m03  m13  m23  m33 |
// 
// The first index or number in a name denotes the column, the second the row.
// So m[i][j] denotes the member in the ith column and the jth row. This is the
// same as in GLSL (source: GLSL v1.3 specification, 5.6 Matrix Components).
//

typedef union {
  // The first index is the column index, the second the row index. The memory
  // layout of nested arrays in C matches the memory layout expected by OpenGL.
  float m[4][4];
  // OpenGL expects the first 4 floats to be the first column of the matrix.
  // So we need to define the named members column by column for the names to
  // match the memory locations of the array elements.
  struct {
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
  };
} mat4_t;

static inline mat4_t mat4(
  float m00, float m10, float m20, float m30,
  float m01, float m11, float m21, float m31,
  float m02, float m12, float m22, float m32,
  float m03, float m13, float m23, float m33
);

static inline mat4_t m4_identity     ();
static inline mat4_t m4_translation  (vec3_t offset);
static inline mat4_t m4_scaling      (vec3_t scale);
static inline mat4_t m4_rotation_x   (float angle_in_rad);
static inline mat4_t m4_rotation_y   (float angle_in_rad);
static inline mat4_t m4_rotation_z   (float angle_in_rad);
              mat4_t m4_rotation     (float angle_in_rad, vec3_t axis);

              mat4_t m4_ortho        (float left, float right, float bottom, float top, float back, float front);
              mat4_t m4_perspective  (float vertical_field_of_view_in_deg, float aspect_ratio, float near_view_distance, float far_view_distance);
              mat4_t m4_look_at      (vec3_t from, vec3_t to, vec3_t up);

static inline mat4_t m4_transpose    (mat4_t matrix);
static inline mat4_t m4_mul          (mat4_t a, mat4_t b);
              mat4_t m4_invert_affine(mat4_t matrix);
              vec3_t m4_mul_pos      (mat4_t matrix, vec3_t position);
              vec3_t m4_mul_dir      (mat4_t matrix, vec3_t direction);

              void   m4_print        (mat4_t matrix);
              void   m4_printp       (mat4_t matrix, int width, int precision);
              void   m4_fprint       (FILE* stream, mat4_t matrix);
              void   m4_fprintp      (FILE* stream, mat4_t matrix, int width, int precision);


//
// 3D vector functions header implementation
//

static inline vec3_t v3_norm(vec3_t v) {
  float len = v3_length(v);
  if (len > 0)
    return (vec3_t){ v.x / len, v.y / len, v.z / len };
  else
    return (vec3_t){ 0, 0, 0};
}

static inline vec3_t v3_proj(vec3_t v, vec3_t onto) {
  return v3_muls(onto, v3_dot(v, onto) / v3_dot(onto, onto));
}

static inline vec3_t v3_cross(vec3_t a, vec3_t b) {
  return (vec3_t){
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  };
}

static inline float v3_angle_between(vec3_t a, vec3_t b) {
  return acosf( v3_dot(a, b) / (v3_length(a) * v3_length(b)) );
}


//
// Matrix functions header implementation
//

static inline mat4_t mat4(
  float m00, float m10, float m20, float m30,
  float m01, float m11, float m21, float m31,
  float m02, float m12, float m22, float m32,
  float m03, float m13, float m23, float m33
) {
  return (mat4_t){
    .m[0][0] = m00, .m[1][0] = m10, .m[2][0] = m20, .m[3][0] = m30,
    .m[0][1] = m01, .m[1][1] = m11, .m[2][1] = m21, .m[3][1] = m31,
    .m[0][2] = m02, .m[1][2] = m12, .m[2][2] = m22, .m[3][2] = m32,
    .m[0][3] = m03, .m[1][3] = m13, .m[2][3] = m23, .m[3][3] = m33
  };
}

static inline mat4_t m4_identity() {
  return mat4(
     1,  0,  0,  0,
     0,  1,  0,  0,
     0,  0,  1,  0,
     0,  0,  0,  1
  );
}

static inline mat4_t m4_translation(vec3_t offset) {
  return mat4(
     1,  0,  0,  offset.x,
     0,  1,  0,  offset.y,
     0,  0,  1,  offset.z,
     0,  0,  0,  1
  );
}

static inline mat4_t m4_scaling(vec3_t scale) {
  float x = scale.x, y = scale.y, z = scale.z;
  return mat4(
     x,  0,  0,  0,
     0,  y,  0,  0,
     0,  0,  z,  0,
     0,  0,  0,  1
  );
}

static inline mat4_t m4_rotation_x(float angle_in_rad) {
  float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
  return mat4(
    1,  0,  0,  0,
    0,  c, -s,  0,
    0,  s,  c,  0,
    0,  0,  0,  1
  );
}

static inline mat4_t m4_rotation_y(float angle_in_rad) {
  float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
  return mat4(
     c,  0,  s,  0,
     0,  1,  0,  0,
    -s,  0,  c,  0,
     0,  0,  0,  1
  );
}

static inline mat4_t m4_rotation_z(float angle_in_rad) {
  float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
  return mat4(
     c, -s,  0,  0,
     s,  c,  0,  0,
     0,  0,  1,  0,
     0,  0,  0,  1
  );
}

static inline mat4_t m4_transpose(mat4_t matrix) {
  return mat4(
    matrix.m00, matrix.m01, matrix.m02, matrix.m03,
    matrix.m10, matrix.m11, matrix.m12, matrix.m13,
    matrix.m20, matrix.m21, matrix.m22, matrix.m23,
    matrix.m30, matrix.m31, matrix.m32, matrix.m33
  );
}

/**
 * Multiplication of two 4x4 matrices.
 * 
 * Implemented by following the row times column rule and illustrating it on a
 * whiteboard with the proper indices in mind.
 * 
 * Further reading: https://en.wikipedia.org/wiki/Matrix_multiplication
 * But note that the article use the first index for rows and the second for
 * columns.
 */
static inline mat4_t m4_mul(mat4_t a, mat4_t b) {
  mat4_t result;
  
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      float sum = 0;
      for(int k = 0; k < 4; k++) {
        sum += a.m[k][j] * b.m[i][k];
      }
      result.m[i][j] = sum;
    }
  }
  
  return result;
}

#endif // MATH_3D_HEADER


#ifdef MATH_3D_IMPLEMENTATION

/**
 * Creates a matrix to rotate around an axis by a given angle. The axis doesn't
 * need to be normalized.
 * 
 * Sources:
 * 
 * https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
 */
mat4_t m4_rotation(float angle_in_rad, vec3_t axis) {
  vec3_t normalized_axis = v3_norm(axis);
  float x = normalized_axis.x, y = normalized_axis.y, z = normalized_axis.z;
  float c = cosf(angle_in_rad), s = sinf(angle_in_rad);
  
  return mat4(
    c + x*x*(1-c),            x*y*(1-c) - z*s,      x*z*(1-c) + y*s,  0,
        y*x*(1-c) + z*s,  c + y*y*(1-c),            y*z*(1-c) - x*s,  0,
        z*x*(1-c) - y*s,      z*y*(1-c) + x*s,  c + z*z*(1-c),        0,
        0,                        0,                    0,            1
  );
}


/**
 * Creates an orthographic projection matrix. It maps the right handed cube
 * defined by left, right, bottom, top, back and front onto the screen and
 * z-buffer. You can think of it as a cube you move through world or camera
 * space and everything inside is visible.
 * 
 * This is slightly different from the traditional glOrtho() and from the linked
 * sources. These functions require the user to negate the last two arguments
 * (creating a left-handed coordinate system). We avoid that here so you can
 * think of this function as moving a right-handed cube through world space.
 * 
 * The arguments are ordered in a way that for each axis you specify the minimum
 * followed by the maximum. Thats why it's bottom to top and back to front.
 * 
 * Implementation details:
 * 
 * To be more exact the right-handed cube is mapped into normalized device
 * coordinates, a left-handed cube where (-1 -1) is the lower left corner,
 * (1, 1) the upper right corner and a z-value of -1 is the nearest point and
 * 1 the furthest point. OpenGL takes it from there and puts it on the screen
 * and into the z-buffer.
 * 
 * Sources:
 * 
 * https://msdn.microsoft.com/en-us/library/windows/desktop/dd373965(v=vs.85).aspx
 * https://unspecified.wordpress.com/2012/06/21/calculating-the-gluperspective-matrix-and-other-opengl-matrix-maths/
 */
mat4_t m4_ortho(float left, float right, float bottom, float top, float back, float front) {
  float l = left, r = right, b = bottom, t = top, n = front, f = back;
  float tx = -(r + l) / (r - l);
  float ty = -(t + b) / (t - b);
  float tz = -(f + n) / (f - n);
  return mat4(
     2 / (r - l),  0,            0,            tx,
     0,            2 / (t - b),  0,            ty,
     0,            0,            2 / (f - n),  tz,
     0,            0,            0,            1
  );
}

/**
 * Creates a perspective projection matrix for a camera.
 * 
 * The camera is at the origin and looks in the direction of the negative Z axis.
 * `near_view_distance` and `far_view_distance` have to be positive and > 0.
 * They are distances from the camera eye, not values on an axis.
 * 
 * `near_view_distance` can be small but not 0. 0 breaks the projection and
 * everything ends up at the max value (far end) of the z-buffer. Making the
 * z-buffer useless.
 * 
 * The matrix is the same as `gluPerspective()` builds. The view distance is
 * mapped to the z-buffer with a reciprocal function (1/x). Therefore the z-buffer
 * resolution for near objects is very good while resolution for far objects is
 * limited.
 * 
 * Sources:
 * 
 * https://unspecified.wordpress.com/2012/06/21/calculating-the-gluperspective-matrix-and-other-opengl-matrix-maths/
 */
mat4_t m4_perspective(float vertical_field_of_view_in_deg, float aspect_ratio, float near_view_distance, float far_view_distance) {
  float fovy_in_rad = vertical_field_of_view_in_deg / 180 * M_PI;
  float f = 1.0f / tanf(fovy_in_rad / 2.0f);
  float ar = aspect_ratio;
  float nd = near_view_distance, fd = far_view_distance;
  
  return mat4(
     f / ar,           0,                0,                0,
     0,                f,                0,                0,
     0,                0,               (fd+nd)/(nd-fd),  (2*fd*nd)/(nd-fd),
     0,                0,               -1,                0
  );
}

/**
 * Builds a transformation matrix for a camera that looks from `from` towards
 * `to`. `up` defines the direction that's upwards for the camera. All three
 * vectors are given in world space and `up` doesn't need to be normalized.
 * 
 * Sources: Derived on whiteboard.
 * 
 * Implementation details:
 * 
 * x, y and z are the right-handed base vectors of the cameras subspace.
 * x has to be normalized because the cross product only produces a normalized
 *   output vector if both input vectors are orthogonal to each other. And up
 *   probably isn't orthogonal to z.
 * 
 * These vectors are then used to build a 3x3 rotation matrix. This matrix
 * rotates a vector by the same amount the camera is rotated. But instead we
 * need to rotate all incoming vertices backwards by that amount. That's what a
 * camera matrix is for: To move the world so that the camera is in the origin.
 * So we take the inverse of that rotation matrix and in case of an rotation
 * matrix this is just the transposed matrix. That's why the 3x3 part of the
 * matrix are the x, y and z vectors but written horizontally instead of
 * vertically.
 * 
 * The translation is derived by creating a translation matrix to move the world
 * into the origin (thats translate by minus `from`). The complete lookat matrix
 * is then this translation followed by the rotation. Written as matrix
 * multiplication:
 * 
 *   lookat = rotation * translation
 * 
 * Since we're right-handed this equals to first doing the translation and after
 * that doing the rotation. During that multiplication the rotation 3x3 part
 * doesn't change but the translation vector is multiplied with each rotation
 * axis. The dot product is just a more compact way to write the actual
 * multiplications.
 */
mat4_t m4_look_at(vec3_t from, vec3_t to, vec3_t up) {
  vec3_t z = v3_muls(v3_norm(v3_sub(to, from)), -1);
  vec3_t x = v3_norm(v3_cross(up, z));
  vec3_t y = v3_cross(z, x);
  
  return mat4(
    x.x, x.y, x.z, -v3_dot(from, x),
    y.x, y.y, y.z, -v3_dot(from, y),
    z.x, z.y, z.z, -v3_dot(from, z),
    0,   0,   0,    1
  );
}


/**
 * Inverts an affine transformation matrix. That are translation, scaling,
 * mirroring, reflection, rotation and shearing matrices or any combination of
 * them.
 * 
 * Implementation details:
 * 
 * - Invert the 3x3 part of the 4x4 matrix to handle rotation, scaling, etc.
 *   correctly (see source).
 * - Invert the translation part of the 4x4 matrix by multiplying it with the
 *   inverted rotation matrix and negating it.
 * 
 * When a 3D point is multiplied with a transformation matrix it is first
 * rotated and then translated. The inverted transformation matrix is the
 * inverse translation followed by the inverse rotation. Written as a matrix
 * multiplication (remember, the effect applies right to left):
 * 
 *   inv(matrix) = inv(rotation) * inv(translation)
 * 
 * The inverse translation is a translation into the opposite direction, just
 * the negative translation. The rotation part isn't changed by that
 * multiplication but the translation part is multiplied by the inverse rotation
 * matrix. It's the same situation as with `m4_look_at()`. But since we don't
 * store the rotation matrix as 3D vectors we can't use the dot product and have
 * to write the matrix multiplication operations by hand.
 * 
 * Sources for 3x3 matrix inversion:
 * 
 * https://www.khanacademy.org/math/precalculus/precalc-matrices/determinants-and-inverses-of-large-matrices/v/inverting-3x3-part-2-determinant-and-adjugate-of-a-matrix
 */
mat4_t m4_invert_affine(mat4_t matrix) {
  // Create shorthands to access matrix members
  float m00 = matrix.m00,  m10 = matrix.m10,  m20 = matrix.m20,  m30 = matrix.m30;
  float m01 = matrix.m01,  m11 = matrix.m11,  m21 = matrix.m21,  m31 = matrix.m31;
  float m02 = matrix.m02,  m12 = matrix.m12,  m22 = matrix.m22,  m32 = matrix.m32;
  
  // Invert 3x3 part of the 4x4 matrix that contains the rotation, etc.
  // That part is called R from here on.
    
    // Calculate cofactor matrix of R
    float c00 =   m11*m22 - m12*m21,   c10 = -(m01*m22 - m02*m21),  c20 =   m01*m12 - m02*m11;
    float c01 = -(m10*m22 - m12*m20),  c11 =   m00*m22 - m02*m20,   c21 = -(m00*m12 - m02*m10);
    float c02 =   m10*m21 - m11*m20,   c12 = -(m00*m21 - m01*m20),  c22 =   m00*m11 - m01*m10;
    
    // Caclculate the determinant by using the already calculated determinants
    // in the cofactor matrix.
    // Second sign is already minus from the cofactor matrix.
    float det = m00*c00 + m10*c10 + m20 * c20;
    if (fabsf(det) < 0.00001)
      return m4_identity();
    
    // Calcuate inverse of R by dividing the transposed cofactor matrix by the
    // determinant.
    float i00 = c00 / det,  i10 = c01 / det,  i20 = c02 / det;
    float i01 = c10 / det,  i11 = c11 / det,  i21 = c12 / det;
    float i02 = c20 / det,  i12 = c21 / det,  i22 = c22 / det;
  
  // Combine the inverted R with the inverted translation
  return mat4(
    i00, i10, i20,  -(i00*m30 + i10*m31 + i20*m32),
    i01, i11, i21,  -(i01*m30 + i11*m31 + i21*m32),
    i02, i12, i22,  -(i02*m30 + i12*m31 + i22*m32),
    0,   0,   0,      1
  );
}

/**
 * Multiplies a 4x4 matrix with a 3D vector representing a point in 3D space.
 * 
 * Before the matrix multiplication the vector is first expanded to a 4D vector
 * (x, y, z, 1). After the multiplication the vector is reduced to 3D again by
 * dividing through the 4th component (if it's not 0 or 1).
 */
vec3_t m4_mul_pos(mat4_t matrix, vec3_t position) {
  vec3_t result = vec3(
    matrix.m00 * position.x + matrix.m10 * position.y + matrix.m20 * position.z + matrix.m30,
    matrix.m01 * position.x + matrix.m11 * position.y + matrix.m21 * position.z + matrix.m31,
    matrix.m02 * position.x + matrix.m12 * position.y + matrix.m22 * position.z + matrix.m32
  );
  
  float w = matrix.m03 * position.x + matrix.m13 * position.y + matrix.m23 * position.z + matrix.m33;
  if (w != 0 && w != 1)
    return vec3(result.x / w, result.y / w, result.z / w);
  
  return result;
}

/**
 * Multiplies a 4x4 matrix with a 3D vector representing a direction in 3D space.
 * 
 * Before the matrix multiplication the vector is first expanded to a 4D vector
 * (x, y, z, 0). For directions the 4th component is set to 0 because directions
 * are only rotated, not translated. After the multiplication the vector is
 * reduced to 3D again by dividing through the 4th component (if it's not 0 or
 * 1). This is necessary because the matrix might contains something other than
 * (0, 0, 0, 1) in the bottom row which might set w to something other than 0
 * or 1.
 */
vec3_t m4_mul_dir(mat4_t matrix, vec3_t direction) {
  vec3_t result = vec3(
    matrix.m00 * direction.x + matrix.m10 * direction.y + matrix.m20 * direction.z,
    matrix.m01 * direction.x + matrix.m11 * direction.y + matrix.m21 * direction.z,
    matrix.m02 * direction.x + matrix.m12 * direction.y + matrix.m22 * direction.z
  );
  
  float w = matrix.m03 * direction.x + matrix.m13 * direction.y + matrix.m23 * direction.z;
  if (w != 0 && w != 1)
    return vec3(result.x / w, result.y / w, result.z / w);
  
  return result;
}

void m4_print(mat4_t matrix) {
  m4_fprintp(stdout, matrix, 6, 2);
}

void m4_printp(mat4_t matrix, int width, int precision) {
  m4_fprintp(stdout, matrix, width, precision);
}

void m4_fprint(FILE* stream, mat4_t matrix) {
  m4_fprintp(stream, matrix, 6, 2);
}

void m4_fprintp(FILE* stream, mat4_t matrix, int width, int precision) {
  mat4_t m = matrix;
  int w = width, p = precision;
  for(int r = 0; r < 4; r++) {
    fprintf(stream, "| %*.*f %*.*f %*.*f %*.*f |\n",
      w, p, m.m[0][r], w, p, m.m[1][r], w, p, m.m[2][r], w, p, m.m[3][r]
    );
  }
}

#endif // MATH_3D_IMPLEMENTATION


//
// Les constantes char* xxx_glsl ci-dessous sont les programmes GLSL
// envoyés à la carte graphique permettant de visualiser les éléments
// graphiques de la scène.
//

const char* texture_frag_glsl = "\n					\
#version 410\n								\
\n									\
in  vec2 coord;\n							\
out vec4 color;\n							\
\n									\
// (.x,.y): translation, .z: scale\n					\
uniform vec3 translation_scale;\n					\
uniform vec2 screensize;\n						\
uniform sampler2D tex;\n						\
\n									\
void main(){\n								\
   vec2 center = vec2(0.5,0.5);\n					\
   vec2 texsize  = textureSize(tex, 0);\n				\
   vec2 texscale = screensize/texsize;\n				\
   texscale = texscale/min(texscale.x, texscale.y);\n			\
   vec2 p = translation_scale.xy + (coord-center)/translation_scale.z;\n \
   color = vec4(texture(tex, p*texscale).xyz, 1);\n     		\
}\n									\
";

const char* texture_vert_glsl = "\n                             \
#version 410\n							\
\n								\
out vec2 coord;\n						\
\n								\
// (.x,.y): translation, .z: scale\n				\
uniform vec3 translation_scale;\n				\
\n								\
vec2 positions[6] = vec2[6](\n					\
   vec2(-1, -1),\n						\
   vec2(+1, -1),\n						\
   vec2(+1, +1),\n						\
   vec2(-1, -1),\n						\
   vec2(+1, +1),\n						\
   vec2(-1, +1)\n						\
);\n								\
\n								\
void main(){\n							\
   coord = (positions[gl_VertexID] + vec2(1))/2.0;\n		\
   gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n	\
}\n								\
"; 

const char* geom_vert_glsl = "\n                                        \
#version 410\n								\
\n									\
layout(location = 0) in vec3 pos;\n					\
layout(location = 1) in vec3 color;\n					\
\n									\
out vec3 fpos;\n							\
out vec3 fcolor;\n							\
\n									\
// (.x,.y): translation, .z: scale\n					\
uniform vec3 translation_scale;\n					\
\n									\
void main(){\n								\
   vec2 center = vec2(0.5,0.5);\n					\
   fpos = pos;\n							\
   fpos.xy = center + (pos.xy-translation_scale.xy)*translation_scale.z;\n \
   fcolor = color;\n							\
   vec2 glpos = (fpos.xy-center)*2.0;\n					\
   gl_Position = vec4(glpos, pos.z, 1);\n				\
}\n									\
";

const char* geom_frag_glsl = "\n \
#version 410\n			 \
\n				 \
in vec3 fpos;\n			 \
in vec3 fcolor;\n		 \
out vec4 pcolor;\n		 \
\n				 \
void main(){\n			 \
   pcolor = vec4(fcolor, 1);\n	 \
}\n				 \
";

const char* mapshader_vert_glsl = "\n   \
#version 410\n				\
\n					\
layout(location = 0) in vec3 pos;\n	\
out vec3 fpos;\n			\
\n					\
uniform mat4 MVP;\n			\
uniform vec3 bbox;\n			\
\n					\
void main(){\n				\
   fpos = pos;\n			\
   gl_Position = MVP*vec4(pos, 1.0);\n	\
}\n					\
";

// Shader pour l'affichage des triangles de la grille.
//
// La lumière dépend de la hauteur max (bbox.y) via le coefficient
// hcoeff compris dans [0,1] (et via les variables externes lux et
// contrast). La 2e ligne avec hcoeff permet de définir l'ombre où la
// source de l'éclairage est la caméra (cam_pos).
//
// mix() et clamp() sont des fonctions GLSL prédéfinies:
// - mix(a,b,x) = (1-x)*a + x*b -> interpolation de x entre a et b
// - clamp(x, a, b) = min(max(x, a), b) -> met x dans [a,b]
//
const char* mapshader_frag_glsl = "\n                                   \
#version 410\n								\
\n									\
in vec3 fpos;\n								\
out vec4 color;\n							\
\n									\
uniform float contrast;\n						\
uniform float lux;\n							\
uniform vec3 bbox;\n							\
uniform vec3 cam_pos;\n							\
uniform sampler2D maptex;\n						\
\n									\
void main(){\n								\
    vec2 coord = (fpos/bbox).xz + vec2(0.5);\n				\
    float r = fpos.y/bbox.y;\n						\
    float hcoeff = (bbox.y < 0.01)? 1.0 : clamp(mix(lux, sqrt(r)*1.5, contrast), 0.0, 1.0);\n \
    hcoeff *= max(0.5, dot(normalize(cross(dFdx(fpos)*1000.0, dFdy(fpos)*1000.0)), normalize(cam_pos)));\n \
    float dcam = distance(fpos, cam_pos);\n				\
    float lightcam = min(0.6/(dcam*dcam), 1.0);\n			\
    color = vec4(texture(maptex, coord).xyz*(hcoeff+lightcam), 1.0);\n	\
}\n									\
";

#define MAX_GLBUFFERS_VAO 5

typedef unsigned int uint;

typedef struct {
  uint vertex;
  uint fragment;
  uint program;
} GLShader;

typedef struct {
  uint id;
  uint size; // taille de data[]
  vec3_t* data;
} GLBuffer;

typedef struct {
  vec3_t data[MAX_VERTICES];
  uint size;
} StaticVector;

typedef struct {
  uint id;
  uint size; // taille de data[]
  uint* data;
} GLElementBuffer;

typedef struct {
  uint id;
  GLBuffer* attached[MAX_GLBUFFERS_VAO+1];
  GLElementBuffer* indices;
} VAO;

//
// Structure contenant tous les sommets et les triangles décrivant la
// grille G. Chaque cellule (x,y) de la grille a 4 coins (corners),
// soit (G->X+1)*(G->Y+1) sommets, et un centre, soit G->X*G->Y autres
// sommets. Les centres sont stockés dans GLBuffer après tous les
// corners, c'est-à-dire à partir de l'indice (G->X+1)*(G->Y+1).
//
typedef struct {
  grid* G; // la grille
  GLBuffer* vertices; // les sommets
  GLElementBuffer* indices; // les triangles (indices de sommets)
  vec2_t size; // bounding boxes de la grille (2D)
} GridBuffer;

enum camViewMode { ROTATING=0, MANUAL };


///////////////////////////////////////////////////////////////////////////
//
// Variables et fonctions internes (static) qui ne doivent pas être
// visibles à l'extérieur de ce fichier. À ne pas mettre dans le .h.
//
///////////////////////////////////////////////////////////////////////////

// pour l'affichage de l'aide
static char HELP[] = "\n\
┌──\n\
│ Commandes pour l'interface graphique\n\
│\n\
│  [h] ... affiche l'aide\n\
│  [q] ... passe running à false\n\
│  [p] ... pause de 0\"5, maintenir pour pause plus longue\n\
│  [souris] ... déplacement d'un ou de tous les points, zoom\n\
│\n\
│ Spécifique pour TSP:\n\
│\n\
│  [s] ... change la taille des points\n\
│  [o] ... indique l'orientation de la tournée\n\
│  [r] ... indique le point de départ (racine du MST) de la tournée\n\
│  [t] ... dessine ou pas la tournée et/ou l'arbre MST\n\
│  [g] ... dessine ou pas une grille pour le positionnement des points\n\
│  [u] ... recentrage (modification) du nuage de points dans la fenêtre\n\
│  [w] ... écrit les coordonnées des points dans un fichier\n\
│\n\
│ Spécifique pour A*:\n\
│\n\
│  [c] ..... maintient ou supprime les sommets visités à la fin de A*\n\
│  [-/+] ... ralentit/accélère drawGrid() et l'exécution d'A*\n\
│  [z/a] ... idem que [-/+]\n\
│  [d] ..... bascule la vue en 3D ou 2D\n\
│  [l/m] ... en vue 3D, ralentit/accélère la vitesse de la caméra\n\
│  [k] ..... en vue 3D, stoppe la caméra\n\
│  [1/2] ... en vue 3D, diminue/augmente la lumineusité\n\
│  [3/4] ... en vue 3D, diminue/augmente le constrast\n\
└──\n\
";

// nombres d'appels au dessin de la grille attendus par seconde
static unsigned long call_speed = 1 << 7;

static float mouse_dx = 0; // distance parcourue par la souris en x depuis le dernier calcul de MVP
static float mouse_dy = 0; // distance parcourue par la souris en y depuis le dernier calcul de MVP
static bool mouse_ldown = false; // bouton souris gauche, vrai si enfoncé
static bool mouse_rdown = false; // boutons souris droit, vrai si enfoncé
static bool oriented = false;    // pour afficher l'orientation de la tournée
static bool root = false;        // pour afficher le point de départ de la tournée
static int selectedVertex = -1;  // indice du point sélectionné avec la souris
static point *POINT = NULL;      // tableau de points (interne)
static int nPOINT = 0;           // nombre de points (de POINTS)
static int mst = 3;              // pour drawGraph():
                                 // bit-0: dessin de l'arbre (1=oui/0=non)
                                 // bit-1: dessin de la tournée (1=oui/0=non)
static int quadrillage = 0;      // quadrillage pour drawX(): 0 (=off), 1, 2, 3, 0
static SDL_Window *window;       // la fenêtre d'affichage
static SDL_GLContext glcontext;
static GLvoid *gridImage;        // image de pixels calculée à partir d'une 'grid' G
static int view3D = 0;           // par défaut, vue en 2D (= 1 si 3D)

static GLuint maptexture;

// les shaders
static GLShader mapshader;
static GLShader texshader;
static GLShader geomshader;

// variables permettant de communiquer entre GPU et CPU
static uint MVP_location = 0;
static uint lux_location = 0;
static uint contrast_location = 0;
static uint bbox_location = 0;
static uint cam_pos_location = 0;
static uint transcale_location = 0;
static uint screensize_location = 0;
static uint geom_transcale_location = 0;

static vec3_t transcale = {0.5, 0.5, 1.0}; // (.x,.y): translation, .z: scale
static vec3_t cpuvertices[MAX_VERTICES];
static vec3_t cpucolors[MAX_VERTICES];
static uint cpu_triangles_amount = 0;

static VAO* vao3d   = NULL;
static VAO* vaotex  = NULL;
static VAO* vaogeom = NULL;
static GridBuffer* glgrid  = NULL;
static GLBuffer* gvertices = NULL;
static GLBuffer* gcolors   = NULL;

static vec3_t cam_pos = {0,0,-2};
static vec3_t cam_dir = {0,0,1};
static vec3_t up_head = {0,1,0};
static vec3_t cam_target = {0,0,-1};
static float cam_speed = 0.2f;
static float cam_old_speed = 0.2f;
static float cam_manual_speed = 0.012f;
static float cam_sensi = -0.006f;
static float cam_angle = 0.0f;
static float cam_arrows_speed = 0.1f;
static float cam_tour_radius = 4.0f;
static enum camViewMode cam_view_mode = ROTATING;

static vec3_t bbox; // bounding boxes de la scène
static mat4_t proj;
static mat4_t view;
static mat4_t MVP;

static float  contrast = 0.5; // contrast dans [0,1] 
static float  lux      = 1.2; // lumière dans [0,+∞[ (pas d'effet si constrast=1)

//////////////////////////////////////////////////////////////////////
// Functions principales OpenGL
//////////////////////////////////////////////////////////////////////
static GridBuffer* createGridBuffer(grid*);
static void freeGridBuffer(GridBuffer*);
static void compute_MVP();
static void error(const char*);
static VAO* createVAO(void);
static void bindVAO(const VAO*);
static void unbindVAO(void);
static void freeVAO(VAO*);
static GLBuffer* createEmptyGLBuffer(void);
static GLBuffer* createGLBuffer(vec3_t*, uint);
static GLElementBuffer* createEmptyGLElementBuffer(void);
static GLElementBuffer* createGLElementBuffer(uint*, uint);
static void bindGLBuffer(GLBuffer*);
static void bindGLElementBuffer(GLElementBuffer*);
static void recreateGLBufferDataGPU(GLBuffer*);
static void recreateGLElementBufferGPU(GLElementBuffer*);
static void replaceGLBufferData(GLBuffer*, vec3_t*, uint, uint);
static void freeGLBuffer(GLBuffer*);
static void freeGLElementBuffer(GLElementBuffer*);
static void attachBuffertoVAO(VAO*, GLBuffer*, uint);
static void attachElementBufferVAO(VAO*, GLElementBuffer*);
static void drawVAO(GLShader*, VAO*);
static void draw_stored_triangles();
static void prepareDrawLineGrad(const point, const point, const vec3_t, const vec3_t, float);
static void prepareDrawLine(const point, const point, const vec3_t, double);
static void prepareDrawPoint(const point, const vec3_t, double);
static vec3_t point_to_glcoord(const point);
static void prepareDrawArrow(const point, const point, const vec3_t, double);
static void prepareDrawTriangleGradGLcoord(const vec3_t, const vec3_t, const vec3_t, const vec3_t, const vec3_t, const vec3_t);
//////////////////////////////////////////////////////////////////////

static vec3_t right_dir(void) { return v3_norm(v3_cross(cam_dir, (vec3_t){0,1,0})); }
static vec3_t up_dir(void) { return v3_cross((vec3_t){0,1,0}, v3_norm(v3_cross(cam_dir, (vec3_t){0,1,0}))); }

static void openglerr(void) {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR)
    printf("OpenGL error: %d\n", err);
}

// Donne la hauteur du corner (x,y). Les hauteurs sont décalées de
// G->hmin pour que le point le plus bas soit en y = 0, sinon
// l'ensemble est trop sombre. Renvoie 0 si (x,y) est sur le bord de
// G.
static float get_corner_height(grid *G, uint x, uint y) {
  if(mounts <= 0) return 0;
  if(x >= G->X || y >= G->Y || x <= 0 || y <= 0) return 0;
  return G->height[x][y] - G->hmin;
}

// Interpole (=barycentre) la hauteur du centre à partir des 4 corners
// de la cellule (x,y).
static float get_center_height(grid *G, uint x, uint y) {
  return (get_corner_height(G, x+0, y+0)+
	  get_corner_height(G, x+1, y+0)+
	  get_corner_height(G, x+1, y+1)+
	  get_corner_height(G, x+0, y+1))/4;
}

// Construit dans un GridBuffer, à partir de la grille G, tous les
// sommets et les triangles. Chaque cellule (x,y) de la grille est est
// un carré unité qui est découpée en 4 triangles partageant le centre
// du carré.
//
// 1 cellule   4 corners + 1 center
//   o---o          o---o
//   |   |          |\ /|
//   |   |          | o |
//   |   |          |/ \|
//   o---o          o---o
//
static GridBuffer* createGridBuffer(grid *G) {

  GridBuffer* B = malloc(sizeof(*B));

  B->G = G;
  B->vertices = createEmptyGLBuffer(); // les sommets
  B->indices  = createEmptyGLElementBuffer(); // les triangles

  // raccourcis
  GLBuffer* V = B->vertices;
  GLElementBuffer* I = B->indices;

  // allocation des sommets
  V->size = (G->X+1)*(G->Y+1)+(G->X)*(G->Y); // nb de vertices dans buffer
  V->size += 5; // pour le drapeau (4 sommets)
  V->data = malloc(sizeof(*V->data)*V->size); // 1 vertex = 1 vec3_t

  // allocation des triangles (1 triangle = 3 indices consécutifs)
  I->size = (G->X)*(G->Y)*4*3; // nb d'indices dans buffer
  I->size += 3*3; // pour le drapeau (2 triangles)
  I->data = malloc(sizeof(*I->data)*I->size); // 1 triangle = 3 uint

  // Fixe les coordonnées des corners. NB: Les x et y sont décalés
  // pour que le centre de la grille soit en (0,0,h).

  uint idata = 0; // indices pour V->data[]

  for(uint x=0; x<=G->X; x++)
    for(uint y=0; y<=G->Y; y++){
      float fx = (float)x - (float)G->X / 2.0f; // centrage en x
      float fy = (float)y - (float)G->Y / 2.0f; // centrage en y
      float h = get_corner_height(B->G, x, y); // hauteur de (x,y)
      V->data[idata++] = (vec3_t){ fx, h, fy };
    }

  // indice à partir duquel sont stockés les centers
  uint icenters = idata;

  // Fixe les coordonnées des centres. NB: Les x et y sont décalés
  // pour que le centre de la grille soit en (0,0,h).

  for(uint x=0; x<G->X; x++) // pour chaque cellule (x,y) de G
    for(uint y=0; y<G->Y; y++){
      
      float fx = (float)x - (float)G->X / 2.0f + 0.5f; // centrage en x
      float fy = (float)y - (float)G->Y / 2.0f + 0.5f; // centrage en y
      float h = get_center_height(B->G, x, y); // hauteur de (x,y)
      V->data[idata++] = (vec3_t){ fx, h, fy };
    }

  // Fixe les triangles, 4 triangles par cellule de G, chacun étant un
  // triplet d'indices à mettre dans I->data.

  uint itriangle = 0; // indices pour I->data[] 

  for(uint x=0; x<G->X; x++) // pour chaque cellule (x,y) de G
    for(uint y=0; y<G->Y; y++){
      
      uint corners[4] = { // indices des 4 corners de (x,y)
	(G->Y+1)*(x+0) + (y+0),
	(G->Y+1)*(x+0) + (y+1),
	(G->Y+1)*(x+1) + (y+1),
	(G->Y+1)*(x+1) + (y+0),
      };
      uint center = icenters + G->Y*x + y; // indice du centre de (x,y)

      for(uint i=0; i<4; i++){ // les 4 triangles pour (x,y)
        I->data[itriangle++] = corners[i]; 
        I->data[itriangle++] = corners[(i+1)%4]; 
        I->data[itriangle++] = center;
      }
    }

  // Ajout du drapeau sur la cellule G.end: 5 sommets et 3 triangles
  //
  // D----     A = centre de la cellule
  // |\   \    A-B-D à la verticale
  // B-C---E   B-C-E à l'horizontale
  // | /
  // |/
  // A

  uint iflag = idata; // indice des sommets du drapeau

  float fx = (float)G->end.x - (float)G->X / 2.0f + 0.5f; // Centre de G->end
  float fz = (float)G->end.y - (float)G->Y / 2.0f + 0.5f;
  float fy = get_center_height(G, G->end.x, G->end.y);
  float H = (G->hmax-G->hmin)/15.0f; // hauteur total du drapeau
  
  // Sommets du drapeau (après ceux de la grille)
  V->data[idata++] = (vec3_t){ fx,      fy,        fz };      // A
  V->data[idata++] = (vec3_t){ fx,      fy+0.7f*H, fz };      // B
  V->data[idata++] = (vec3_t){ fx+0.3f, fy+0.7f*H, fz+0.3f }; // C
  V->data[idata++] = (vec3_t){ fx,      fy+H,      fz };      // D
  V->data[idata++] = (vec3_t){ fx+1.5f, fy+0.7f*H, fz+1.5f }; // E

  // Triangles du drapeau (après ceux de la grille)

  // Triangle 1 : A-B-C
  I->data[itriangle++] = iflag + 0;
  I->data[itriangle++] = iflag + 1;
  I->data[itriangle++] = iflag + 2;
  
  // Triangle 2 : B-C-D
  I->data[itriangle++] = iflag + 1;
  I->data[itriangle++] = iflag + 2;
  I->data[itriangle++] = iflag + 3;

  // Triangle 3 : C-D-E
  I->data[itriangle++] = iflag + 2;
  I->data[itriangle++] = iflag + 3;
  I->data[itriangle++] = iflag + 4;

  // Mise à jour du GPU.
  recreateGLBufferDataGPU(V);
  recreateGLElementBufferGPU(I);

  // bounding boxes
  //
  // en x: largeur en x des corners
  // en z: largeur en y des corners
  // en y: différence d'altitude
  bbox = (vec3_t){
    V->data[icenters-1].x - V->data[0].x,
    G->hmax - G->hmin + H,
    V->data[icenters-1].z - V->data[0].z
  };

  B->size = (vec2_t){bbox.x, bbox.z};

  return B;
}

static void attachGridBufferVAO(GridBuffer* g, VAO* svao) {
  attachBuffertoVAO(svao, g->vertices, 0);
  attachElementBufferVAO(svao, g->indices);
}

static void freeGridBuffer(GridBuffer* g) {
  free(g->vertices->data);
  free(g->indices->data);

  freeGLBuffer(g->vertices);
  freeGLElementBuffer(g->indices);
  free(g);
}

static void compute_MVP() {

  if (cam_view_mode == ROTATING) {
    float R = cam_tour_radius*(0.9f+cos(cam_angle)*0.1f);
    cam_pos = (vec3_t){cam_target.x+cos(cam_angle)*R,
                       cam_target.y*(4.0f + (1.0f+cos(cam_angle*3.0f))*1.4f),
                       cam_target.z+sin(cam_angle)*R};
    cam_dir = v3_sub(cam_target, cam_pos);
  }
  
  if (cam_view_mode == MANUAL && mouse_rdown) {
    mat4_t rot = m4_rotation(mouse_dx*cam_sensi, up_head);
    cam_dir = m4_mul_dir(rot, cam_dir);
    vec3_t rightv = v3_norm(v3_cross(cam_dir, up_head));
    rot = m4_rotation(mouse_dy*cam_sensi, rightv);
    cam_dir = m4_mul_dir(rot, cam_dir);
    
    mouse_dx = 0;
    mouse_dy = 0;
  }
  
  view = m4_look_at(cam_pos, v3_add(cam_pos, cam_dir), up_head);
  MVP  = m4_mul(proj, view);
}

static void error(const char* msg) {
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(EXIT_FAILURE);
}

static VAO* createVAO() {
  VAO* ret = malloc(sizeof(VAO));
  glGenVertexArrays(1, &(ret->id));
  for(uint i=0; i<MAX_GLBUFFERS_VAO+1; i++) ret->attached[i] = NULL;
  ret->indices = NULL;
  return ret;
}

static void bindVAO(const VAO* svao) {
  glBindVertexArray(svao->id);
}

static void unbindVAO() {
  glBindVertexArray(0);
}

static void freeVAO(VAO* svao) {
  glDeleteVertexArrays(1, &(svao->id));
  free(svao);
}

static GLBuffer* createEmptyGLBuffer() {
  GLBuffer* B = malloc(sizeof(*B));
  glGenBuffers(1, &(B->id));
  B->size = 0;
  B->data = NULL;
  return B;
}

static GLElementBuffer* createEmptyGLElementBuffer() {
  GLElementBuffer* B = malloc(sizeof(*B));
  glGenBuffers(1, &(B->id));
  B->size = 0;
  B->data = NULL;
  return B;
}

static void bindGLBuffer(GLBuffer* buffer) {
  glBindBuffer(GL_ARRAY_BUFFER, buffer->id);
}

static void bindGLElementBuffer(GLElementBuffer* buffer) {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->id);
}

static void recreateGLBufferDataGPU(GLBuffer* buffer) {
  bindGLBuffer(buffer);
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(buffer->data[0])*buffer->size,
	       buffer->data, GL_DYNAMIC_DRAW);
}

static void replaceGLBufferData(GLBuffer* buffer, vec3_t* data,
				uint buffer_first, uint size) {
  bindGLBuffer(buffer);
  glBufferSubData(GL_ARRAY_BUFFER,
		  sizeof(buffer->data[0])*buffer_first,
		  sizeof(buffer->data[0])*size, data);
}

static void replaceRecreateGLBufferData(GLBuffer* buffer, vec3_t* data,
					uint buffer_first, uint size) {
  bindGLBuffer(buffer);
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(buffer->data[0])*size,
	       data, GL_DYNAMIC_DRAW);
}

static void recreateGLElementBufferGPU(GLElementBuffer* buffer) {
  bindGLElementBuffer(buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       sizeof(uint)*(buffer->size),
	       buffer->data, GL_DYNAMIC_DRAW);
}

static GLBuffer* createGLBuffer(vec3_t* data, uint size) {
  GLBuffer* B = createEmptyGLBuffer();
  B->data = data;
  B->size = size;
  recreateGLBufferDataGPU(B);
  return B;
}

static GLElementBuffer* createGLElementBuffer(uint* data, uint size) {
  GLElementBuffer* B = createEmptyGLElementBuffer();
  B->data = data;
  B->size = size;
  recreateGLElementBufferGPU(B);
  return B;
}

static void freeGLBuffer(GLBuffer* buffer) {
  glDeleteBuffers(1, &(buffer->id)); 
  free(buffer);
}

static void freeGLElementBuffer(GLElementBuffer* buffer) {
  glDeleteBuffers(1, &(buffer->id)); 
  free(buffer);
}

static void attachBuffertoVAO(VAO* svao, GLBuffer* buffer, uint location) {
  if(location >= MAX_GLBUFFERS_VAO)
    error("attachBuffertoVAO(): location out of bounds");

  svao->attached[location] = buffer;

  bindVAO(svao);
  bindGLBuffer(buffer);
  glEnableVertexAttribArray(location);
  glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

static void attachElementBufferVAO(VAO* svao, GLElementBuffer* buffer) {
  svao->indices = buffer;

  bindVAO(svao);
  bindGLElementBuffer(buffer);
}

static void drawVAO(GLShader* shad, VAO* svao) {
  if(svao->attached[0] == NULL)
    error("drawVAO(): svao has no attached buffers");

  glUseProgram(shad->program);

  // Update MVP uniform
  compute_MVP();
  glUniformMatrix4fv(MVP_location, 1, GL_FALSE, (GLfloat*)&MVP);
  glUniform3fv(bbox_location, 1, (GLfloat*)&bbox);
  glUniform3fv(cam_pos_location, 1, (GLfloat*)&cam_pos);
  glUniform1fv(contrast_location, 1, &contrast);
  glUniform1fv(lux_location, 1, &lux);

  // Draw parameters
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  // Bind and drawcall
  bindVAO(svao);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, maptexture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, glgrid->G->X, glgrid->G->Y,
               GL_RGB, GL_UNSIGNED_BYTE, gridImage);

  if(svao->indices) glDrawElements(GL_TRIANGLES, svao->indices->size, GL_UNSIGNED_INT, 0);
  else glDrawArrays(GL_TRIANGLES, 0, svao->attached[0]->size);

  openglerr();
}

// Convertit les coordonnées pixels en coordonnées OpenGL entre 0 et 1
// dans le dessin, cf.
// https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluUnProject.xml
static void pixelToCoord(int pixel_x, int pixel_y, double *x, double *y) {

  float cx = +(((float)pixel_x)/((float)width)  - 0.5f)/ transcale.z;
  float cy = -(((float)pixel_y)/((float)height) - 0.5f)/ transcale.z;

  *x = cx + transcale.x;
  *y = cy + transcale.y;
}

// Convertit les coordonnées pixels en coordonnées dans le dessin.
static point transformPoint(const point p) {

  point r = (point){transcale.x*width, (1.0-transcale.y)*height};
  r.x += (p.x - (((double)(width ))/2.0))/transcale.z;
  r.y += (p.y - (((double)(height))/2.0))/transcale.z;

  return r;
}

// Récupère les coordonnées du centre de la fenêtre.
static void getCenterCoord(double *x, double *y) {

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  pixelToCoord((viewport[0] + viewport[2]) / 2,
               (viewport[1] + viewport[3]) / 2,
               x, y);
}

// Renvoie l'indice imin du point le plus proche de (x,y).
static int getClosestVertex(double x, double y) {
  int imin = 0;
  double dmin = DBL_MAX;
  
  for (int i = 0; i < nPOINT; i++) {
    double d = (x - POINT[i].x) * (x - POINT[i].x) +
      (y - POINT[i].y) * (y - POINT[i].y);
    if (d < dmin) dmin = d, imin = i;
  }
  
  return imin;
}

// Calcule le titre de la fenêtre. Ce titre est mis dans un buffer
// static pour éviter des malloc() sans free().
static char *getTitle(void) {
  static char buffer[MAX_FILE_NAME];
  sprintf(buffer,"Techniques Algorithmiques et Programmation - %d x %d (%.2lf)",
          width,height,scale);
  return buffer;
}

// Zoom d'un facteur s centré en (x,y).
static void zoomAt(double s, double x, double y) {
  transcale = (vec3_t){ x, y, transcale.z * s};
}

// Zoom d'un facteur s centré sur la position de la souris, modifie
// la variable globale scale du même facteur.
static void zoomMouse(double s) {

  int mx, my;
  double x, y;
  SDL_GetMouseState(&mx, &my);
  pixelToCoord(mx, my, &x, &y);
  // Merci Timothé pour la formule de ses morts
  zoomAt(s, transcale.x/s + (1.0f - 1.0f/s)*x, transcale.y/s + (1.0f - 1.0f/s)*y);
  scale *= s;
}

// Vrai ssi (i,j) est sur le bord de la grille G.
static inline int onBorder(grid *G, int i, int j) {
  return (i == 0) || (j == 0) || (i == G->X - 1) || (j == G->Y - 1);
}

// Vrai ssi p est une position de la grille. Attention ! cela ne veut
// pas dire que p est un sommet du graphe, car la case peut contenir
// TX_WALL. Si (inGrid(&G,p) && !onBorder(&G,p)) alors p est à
// l'intérieur de la grille.
static inline int inGrid(grid *G, position p) {
  return (0 <= p.x) && (p.x < G->X) && (0 <= p.y) && (p.y < G->Y);
}

// Vrai ssi deux positions sont égales.
static inline int equalPosition(position p, position q) {
  return (p.x == q.x) && (p.y == q.y);
}

// Distance L2 entre deux positions.
static inline double distL2(position s, position t) {
  return hypot(t.x - s.x, t.y - s.y);
}

typedef struct {
  // l'ordre de la déclaration est important
  GLubyte R;
  GLubyte G;
  GLubyte B;
} RGB;

static RGB color[] = {

  // l'ordre de la déclaration est important, il doit vérifier l'ordre
  // de l'enum dans tools.h. Pour visualiser les couleurs, disons
  // {0xF0,0xFA,0xFF}: https://imagecolorpicker.com/color-code/F0FAFF

  {0xE0, 0xE0, 0xE0}, // TX_FREE
  {0x10, 0x10, 0x30}, // TX_WALL
  {0xF0, 0xD8, 0xA8}, // TX_SAND
  {0x00, 0x6D, 0xBA}, // TX_WATER
  {0x7C, 0x70, 0x56}, // TX_MUD
  {0x23, 0xBA, 0x28}, // TX_GRASS
  {0x70, 0xE0, 0xD0}, // TX_TUNNEL
  {0x8F, 0x82, 0x77}, // TX_GRAVEL
  {0x4F, 0x4A, 0x45}, // TX_ROCK
  {0xF9, 0xFD, 0xFF}, // TX_SNOW
  {0xD4, 0xEF, 0xFC}, // TX_ICE
  {0x00, 0x42, 0xBA}, // TX_LAKE
  {0x0D, 0x04, 0xA2}, // TX_POOL
  {0x00, 0xA0, 0x60}, // TX_MEADOW
  {0x03, 0x69, 0x22}, // TX_FOREST
  {0x9E, 0x9E, 0x8A}, // TX_TRACK

  {0x80, 0x80, 0x80}, // MK_NULL
  {0x12, 0x66, 0x66}, // MK_USED
  {0x08, 0xF0, 0xF0}, // MK_FRONT
  {0x90, 0x68, 0xF8}, // MK_PATH

  {0xFF, 0x00, 0x00}, // C_START
  {0xFF, 0x88, 0x28}, // C_END
  {0x99, 0xAA, 0xCC}, // C_FINAL
  {0xFF, 0xFF, 0x80}, // C_END_WALL

  {0x66, 0x12, 0x66}, // MK_USED2
  {0xC0, 0x4F, 0x16}, // C_FINAL2
  {0xFF, 0xFF, 0x00}, // C_PATH2
};

//
// Construit l'image de pixels (variable globale gridImage) à partir
// de la grille G, en vue de l'afficher. Elle est appellée
// régulièrement par drawGrid(). Permet des effets de couleurs en
// fonction du temps (fading). Le point (0,0) de G correspond au coin
// en haut à gauche.
//
// +--x
// |
// y
//
static void makeImage(grid *G) {

  // Attention! modifie update si fin=true

  static int cpt; // compteur d'étape lorsqu'on reconstruit le chemin

  RGB *I = gridImage, c;
  int k = 0, v, m, f;

  // fin = true ssi le chemin a fini d'être construit, .start et .end
  //       ont été marqués MK_PATH tous les deux
  bool const fin =
    (G->mark[G->start.x][G->start.y] == MK_PATH) &&
    (G->mark[G->end.x][G->end.y] == MK_PATH);

  // debut = vrai ssi le chemin commence à être construit
  bool debut = false;
  for (int j = 0; j < G->Y && !debut; j++)
    for (int i = 0; i < G->X && !debut; i++)
      if(G->mark[i][j]==MK_PATH) debut = true;

  if (fin) update = false;
  if (!debut) cpt = 0;
  if (debut) cpt++;
  if (debut && cpt == 1){
    call_speed = sqrt(call_speed/4);
    if(call_speed < 0) call_speed = 1;
  }

  double t1,t2,dmax = distL2(G->start, G->end);
  if (dmax == 0) dmax = 1E-10; // pour éviter la division par 0

  for (int j = 0; j < G->Y; j++)
    for (int i = 0; i < G->X; i++) {
      m = G->mark[i][j];
      if ((m < 0) || (m >= CARD(color))) m = MK_NULL;
      v = G->texture[i][j];
      if ((v < 0) || (v >= CARD(color))) v = TX_FREE;
      do { // do...while(0) pour permettre des break
        if (m == MK_PATH) {
          c = color[m];
          if ( fin && !erase ) c = color[C_PATH2];
          break;
        }
        if (fin && erase) {
          c = color[v];
          break;
        } // affiche la grille d'origine à la fin
        if (m == MK_NULL) {
          c = color[v];
          break;
        } // si pas de marquage
        if (m == MK_USED || m == MK_USED2) {
          // interpolation de couleur entre les couleurs MK_USED(2) et
          // C_FINAL(2) ou bien MK_USED(2) et v si on est en train de
          // reconstruire le chemin
          position p = {.x = i, .y = j};
          t1 = (m == MK_USED) ? distL2(G->start, p) / dmax : distL2(G->end, p) / dmax;
          t1 = fmax(t1, 0.0), t1 = fmin(t1, 1.0);
          t2 = (debut && erase)? 0.5 * cpt / dmax : 0;
          t2 = fmin(t2, 1.0);
          f = (m == MK_USED) ? C_FINAL : C_FINAL2;
          c.R = t2*color[v].R + (1-t2) * (t1 * color[f].R + (1-t1)*color[m].R);
          c.G = t2*color[v].G + (1-t2) * (t1 * color[f].G + (1-t1)*color[m].G);
          c.B = t2*color[v].B + (1-t2) * (t1 * color[f].B + (1-t1)*color[m].B);
          break;
        }
        c = (m == MK_NULL) ? color[v] : color[m];
        break;
      } while (0);
      I[k++] = c;
    }

  if (inGrid(G, G->start)) {
    k = G->start.y * G->X + G->start.x;
    I[k] = color[C_START];
  }

  if (inGrid(G, G->end)) {
    v = (G->texture[G->end.x][G->end.y] == TX_WALL) ? C_END_WALL : C_END;
    k = G->end.y * G->X + G->end.x;
    I[k] = color[v];
  }
}


//
// Fixe les hauteurs de la grille G pour la surface 3D, en remplissant
// le champs G->height[][] (y compris le bord) avec un ensemble de
// pics (montagnes ou creux) positionnés aléatoirement sur la grille
// et de hauteurs aléatoires <= z (en fait de différence d'altitutes
// entre pics et creux = z). Le nombre de pics, leur raideur, la
// disparité des hauteurs et la fraction des creux par rapport aux
// montagnes sont respectivement contrôlées par les constantes
// globales: 'mounts', 'steepness', 'disparity' et 'holeness'.
//
// Le tableau G->height[][] n'est pas modifié si mounts = 0. En
// sorties, G->hmax et G->hmin contiennent la hauteur maximum et la
// hauteur minimum.
// 
static void generateHeights(grid* G, float z) {

  if( mounts == 0 || z <= 0){
    G->hmin = G->hmax = 0; // rien à faire les hauteurs sont déjà à 0
    return;
  }

  position M[mounts]; // liste des pics, NB: on peut avoir plusieurs pics
                      // sur la même case
  
  // choisit les pics placés aléatoirement dans [0,G->X[×[0,G->Y[
  for(int i=0; i < mounts; i++)
    M[i] = (position){ random()%G->X, random()%G->Y }; 
  
  // Ajuste la hauteur des pics relativement à la positions des autres
  // pics sur la grille, et calcule les hauteurs min et max
  // rencontrées. Après cette étape, les hauteurs sont dans [-1,1].

  float hmin = FLT_MAX; // hauteur min
  float hmax = 0;       // hauteur max
  float dmax = fmin(G->X,G->Y);
  float const fmounts = mounts; // pour éviter un cast
  long const A = 7877, B = 37; // deux nombres premiers
  
  for(int x = 0; x < G->X; x++)
    for(int y = 0; y < G->Y; y++){

      // Modifie la hauteur des pics en tenant compte de celle des
      // autres afin d'avoir des massifs de montagnes plutôt que des
      // pics isolés, en fonction de 'steepness', 'disparity' et
      // 'holeness'.
      //
      // La variable R produit une séquence pseudo-aléatoire qui sert
      // à mettre des creux selon la fréquence 'holeness'. Cette
      // séquence doit être identique pour chaque (x,y).

      position XY = {x,y};
      float h = 0;
      uint R = B; // premier nombre de la séquence pseudo-aléatoire

      for(int i=0; i<mounts; i++) { // tient compte de tous les pics
	float d = distL2(XY,M[i])/dmax; // distance entre (x,y) et le pic i
	d = 1/(1 + steepness*fmounts*d*d); // modifiée selon la raideur
	d *= (1-disparity) + i*disparity/fmounts; // modulée selon la disparité
	R = (uint)(A*R + B); if(R<0) R = -R; // pour avoir R >= 0
	if ( R%mounts < holeness*fmounts ) d = -d; // négatif de temps de temps 
	h += d;
      }
      
      G->height[x][y] = h;
      hmin = fmin(hmin, G->height[x][y]);
      hmax = fmax(hmax, G->height[x][y]);
    }

  // Modifie les hauteurs de sorte que hmax - hmin = z. NB: Ici z>0.
  double c = (hmin == hmax)? z : z / (hmax-hmin);
  for(int x = 0; x < G->X; x++)
    for(int y = 0; y < G->Y; y++)
      G->height[x][y] *= c;

  // NB: Les tests comme (G->height[x][y] == hmax) ou (G->height[x][y]
  // == hmin) vont bien vraies pour une certaine position (x,y), car
  // la formule (... x c) est la même pour G->height et G->hmax,
  // G->hmin.

  G->hmin = c * hmin;
  G->hmax = c * hmax;
}

//
// Alloue et initialise une grille G aux dimensions x,y ainsi que son
// image (tableau de pixels pour visualisation). On force x,y >= 3
// pour avoir au moins un point qui n'est pas sur le bord. La texture
// est initialisée à TX_FREE et les bords à TX_WALL. Les marques
// .mark[][] sont initialisées à MK_NULL. Les hauteurs .height[][]
// sont remplies en fonction des paramètres 3D, en particulier la
// différence de hauteurs max et min des pics/creux est fixées à
// min(x,y)/flatness. Les paramètres 'mounts', 'flatness',
// 'steepness', 'holeness' et 'disparity' sont vérifiés. Attention!
// .start et .end ne sont pas définis pour être dans la grille.
//
static grid allocGrid(int x, int y) {

  grid G;
  position p = {-1, -1};
  G.start = G.end = p;

  if (x < 3) x = 3;
  if (y < 3) y = 3;
  G.X = x;
  G.Y = y;

  mounts    = (int)fmax(mounts,0);       // au moins 0
  flatness  = fmax(flatness,0.1);        // au moins 0.1
  steepness = fmax(steepness,0);         // au moins 0
  disparity = fmin(fmax(disparity,0),1); // dans [0,1]
  holeness  = fmin(fmax(holeness,0),1);  // dans [0,1]
  G.hmin = G.hmax = 0;
 
  G.texture = malloc(x * sizeof(*(G.texture)));
  G.mark    = malloc(x * sizeof(*(G.mark)));
  G.height  = malloc(x * sizeof(*(G.height)));

  for(int i=0; i<x; i++){
    G.texture[i] = malloc(y * sizeof(*(G.texture[i])));
    G.mark[i]    = malloc(y * sizeof(*(G.mark[i])));
    G.height[i]  = malloc(y * sizeof(*(G.height[i])));
    for (int j=0; j<y; j++){
      G.mark[i][j] = MK_NULL; // marques par défaut
      G.height[i][j] = 0;     // hauteurs par défaut
    }
  }

  // met les bords à TX_WALL et remplit l'intérieur à TX_FREE
  for (int i=0; i<x; i++)
    for (int j=0; j<y; j++)
      G.texture[i][j] = onBorder(&G,i,j) ? TX_WALL : TX_FREE;

  // initialise les hauteurs, G->hmax et G->hmin
  generateHeights(&G, fmin(x,y)/flatness);

  // initialise le tableau de pixels
  gridImage = malloc(3 * x*y * sizeof(GLubyte));
  return G;
}

///////////////////////////////////////////////////////////////////////////
//
// Variables et fonctions utilisées depuis l'extérieur (non static).
// À mettre dans le .h.
//
///////////////////////////////////////////////////////////////////////////

// valeurs par défaut
int    width    = 1280; // NB: 1280/720 = 16 x 9ème
int    height   = 720;
bool   update   = true;
bool   running  = true;
bool   hover    = true;
bool   erase    = true;
double scale    = 1;
GLfloat size_pt = 5.0;

int   mounts    = 50;
float flatness  = 2.0;
float steepness = 5.0;
float holeness  = 0.2;
float disparity = 0.7;

bool NextPermutation(int *P, int n) {
  /*
    Génère la prochaine permutation P de taille n dans l'ordre
    lexicographique. On renvoie true si la prochaine permutation a pu
    être déterminée et false si P était la dernière permutation (et
    alors P n'est pas modifiée). Il n'est pas nécessaire que les
    valeurs de P soit dans [0,n[.

    On se base sur l'algorithme classique qui est:

    1. Trouver le plus grand index i tel que P[i] < P[i+1].
       S'il n'existe pas, la dernière permutation est atteinte.
    2. Trouver le plus grand indice j tel que P[i] < P[j].
    3. Echanger P[i] avec P[j].
    4. Renverser la suite de P[i+1] jusqu'au dernier élément.

  */
  int i=-1, j, m=n-1, t;

  /* étape 1: cherche i le plus grand tq P[i]<P[i+1] */
  for (j = 0; j < m; j++)
    if (P[j] < P[j + 1]) i = j; /* on a trouvé un i tq P[i]<P[i+1] */
  if (i < 0) return false; /* le plus grand i tq P[i]<[i+1] n'existe pas */

  /* étape 2: cherche j le plus grand tq P[i]<P[j] */
  for (j = i+1; (j<n) && (P[i]<P[j]) ; j++);
  j--;

  /* étape 3: échange P[i] et P[j] */
  SWAP(P[i], P[j], t);

  /* étape 4: renverse P[i+1]...P[n-1] */
  for (++i; i < m; i++, m--)
    SWAP(P[i], P[m], t);

  return true;
}

char *TopChrono(int const i) {
#define CHRONOMAX 10
  /*
    Met à jour le chronomètre interne numéro i (i=0..CHRNONMAX-1) et
    renvoie sous forme de char* le temps écoulé depuis le dernier
    appel à la fonction pour le même chronomètre. La précision dépend
    du temps mesuré. Elle varie entre la minute et le 1/1000e de
    seconde. Plus précisément le format est le suivant:

    1d00h00'  si le temps est > 24h (précision: 1')
    1h00'00"  si le temps est > 60' (précision: 1s)
    1'00"0    si le temps est > 1'  (précision: 1/10s)
    1"00      si le temps est > 1"  (précision: 1/100s)
    0"000     si le temps est < 1"  (précision: 1/1000s)

    Pour initialiser et mettre à jour tous les chronomètres (dont le
    nombre vaut CHRONOMAX), il suffit d'appeler une fois la fonction,
    par exemple avec TopChrono(0). Si i<0, alors les pointeurs alloués
    par l'initialisation sont désalloués. La durée maximale est
    limitée à 100 jours. Si une erreur se produit (durée supérieure ou
    erreur avec gettimeofday()), alors on renvoie la chaîne
    "--error--".
  */
  if (i >= CHRONOMAX) return "--error--";

  /* variables globales, locale à la fonction */
  static int first = 1; /* =1 ssi c'est la 1ère fois qu'on exécute la fonction */
  static char *str[CHRONOMAX];
  static struct timeval last[CHRONOMAX], tv;
  int j;

  if (i < 0) {  /* libère les pointeurs */
    if (!first) /* on a déjà alloué les chronomètres */
      for (j = 0; j < CHRONOMAX; j++) free(str[j]);
    first = 1;
    return NULL;
  }

  /* tv=temps courant */
  int err = gettimeofday(&tv, NULL);

  if (first) { /* première fois, on alloue puis on renvoie TopChrono(i) */
    first = 0;
    for (j = 0; j < CHRONOMAX; j++) {
      str[j] = malloc(
                      10); // assez grand pour "--error--", "99d99h99'" ou "23h59'59""
      last[j] = tv;
    }
  }

  /* t=temps en 1/1000" écoulé depuis le dernier appel à TopChrono(i) */
  long t = (tv.tv_sec - last[i].tv_sec) * 1000L +
    (tv.tv_usec - last[i].tv_usec) / 1000L;
  last[i] = tv; /* met à jour le chrono interne i */
  if ((t < 0L) || (err))
    t = LONG_MAX; /* temps erroné */

  /* écrit le résultat dans str[i] */
  for (;;) { /* pour faire un break */
    /* ici t est en millième de seconde */
    if (t < 1000L) { /* t<1" */
      sprintf(str[i], "0\"%03li", t);
      break;
    }
    t /= 10L;        /* t en centième de seconde */
    if (t < 6000L) { /* t<60" */
      sprintf(str[i], "%li\"%02li", t / 100L, t % 100L);
      break;
    }
    t /= 10L;         /* t en dixième de seconde */
    if (t < 36000L) { /* t<1h */
      sprintf(str[i], "%li'%02li\"%li", t / 360L, (t / 10L) % 60L, t % 10L);
      break;
    }
    t /= 10L;         /* t en seconde */
    if (t < 86400L) { /* t<24h */
      sprintf(str[i], "%lih%02li'%02li\"", t / 3600L, (t / 60L) % 60L, t % 60L);
      break;
    }
    t /= 60L;         /* t en minute */
    if (t < 144000) { /* t<100 jours */
      sprintf(str[i], "%lid%02lih%02li'", t / 1440L, (t / 60L) % 24L, t % 60L);
      break;
    }
    /* error ... */
    sprintf(str[i], "--error--");
  }

  return str[i];
#undef CHRONOMAX
}

//
// Renvoie une position aléatoire de la grille qui est uniforme parmi
// toutes les cases de la grille du type t (hors les bords de la
// grille). Si aucune case de type t n'est trouvée, la position
// est initialisée aléatoirement à l'intérieure de la grille.
//
position randomPosition(grid G, int t) {
  int i, j, c;
  int n;                      // n=nombre de cases de type t hors le bord
  int r = -1;                 // r=numéro aléatoire dans [0,n[
  position p = {-1, -1};      // position par défaut
  int const stop = G.X * G.Y; // pour sortir des 2 boucles for
  int const X2 = G.X - 2;
  int const Y2 = G.Y - 2;

  // On fait deux parcours: un 1er pour compter le nombre n de cases
  // de type t, et un 2e pour tirer au hasard la position parmi les n
  // possibles. À la fin du premier parcours on connaît le nombre n de
  // cases de type t. (Si ici n=0, alors on s'arrête en renvoyant une
  // position aléatoirement à l'intérieur de la grille.) On tire alors
  // au hasard un numéro r dans [0,n[.  Puis on recommence le comptage
  // (n=0) de cases de type t et on s'arrête dès qu'on arrive à la
  // case numéro r.

  c = 0;
  do {
    n = 0;
    for (i = 1; i <= X2; i++)
      for (j = 1; j <= Y2; j++)
        if (G.texture[i][j] == t) {
          if (n == r){ // toujours faux au 1er parcours (n=0, r<0)
            p = (position){ i, j };
            i = j = stop;
          }
          n++;
        }
    c = 1 - c;
    if (c){
      if(n == 0) return (position){ 1 + random()%X2, 1 + random()%Y2 }; // si texture non trouvée
      r = random()%n; // aléatoire uniforme dans [0,n[
    }
  } while (c); // vrai la 1ère fois, faux la 2e

  return p;
}


//
// Libère les pointeurs alloués par allocGrid().
//
void freeGrid(grid G) {
  for (int i = 0; i < G.X; i++) {
    free(G.texture[i]);
    free(G.mark[i]);
    free(G.height[i]);
  }
  free(G.texture);
  free(G.mark);
  free(G.height);
  free(gridImage);
}

//
// Renvoie une grille de dimensions x,y rempli de points aléatoires de
// type et de densité donnés. Le départ et la destination sont
// initialisées aléatoirement dans une case TX_FREE (si possible).
//
grid initGridPoints(int x, int y, int type, double density) {

  grid G = allocGrid(x, y); // alloue la grille et son image

  // vérifie que le type est correct, MK_NULL par défaut
  if ((type < 0) || (type >= CARD(color))) type = MK_NULL;

  // remplit l'intérieur aléatoirement
  for (int i = 1; i < x-1; i++)
    for (int j = 1; j < y-1; j++)
      if(RAND01 <= density) G.texture[i][j] = type;

  // position start/end aléatoires
  G.start = randomPosition(G, TX_FREE);
  G.end = randomPosition(G, TX_FREE);

  return G;
}

//
// Renvoie une grille aléatoire de dimensions x,y (au moins 3)
// correspondant à un labyrinthe basé sur un arbre couvrant aléatoire
// uniforme. On fixe les points 'start' et 'end' au milieu de la
// cellule en bas à droite et en haut à gauche respectivement. La
// largeur des couloirs est donnée par w>0.
//
// Il s'agit de l'algorithme de Wilson par "marches aléatoires avec
// effacement de boucle" (cf. https://bl.ocks.org/mbostock/11357811)
// qui génère un arbre couvrant uniforme -- un échantillon non biaisé
// de tous les arbres couvrants possibles. La plupart des autres
// algorithmes de génération de labyrinthes, comme ceux de Prim, de
// parcours aléatoire ou de parcours en profondeur aléatoire, ne
// possèdent pas cette propriété remarquable.
//
// L’algorithme initialise le labyrinthe avec une cellule de départ
// arbitraire. Ensuite, une nouvelle cellule est ajoutée au
// labyrinthe, déclenchant une marche aléatoire. Cette marche
// aléatoire se poursuit jusqu’à ce qu’elle se reconnecte au
// labyrinthe existant. Cependant, si la marche aléatoire
// s’auto-intersecte, la boucle résultante est effacée avant que la
// marche aléatoire ne se poursuive.
//
// Au début, l’algorithme peut sembler désespérément lent à observer,
// car les premières marches aléatoires ont peu de chances de se
// reconnecter au petit labyrinthe existant. Cependant, à mesure que
// le labyrinthe grandit, les marches aléatoires ont de plus en plus
// de chances d’entrer en collision avec le labyrinthe, et
// l’algorithme s’accélère de manière spectaculaire.
//
grid initGridLaby(int x, int y, int w) {

  // vérifie les paramètres
  if (x < 3) x = 3;
  if (y < 3) y = 3;
  if (w <= 0) w = 1;

  // initialise la grille et son image
  int const w1 = w+1;
  grid G = allocGrid(x*w1 + 1, y*w1 + 1);
  int const x1 = G.X-1;
  int const y1 = G.Y-1;

  // position par défaut (au mileu du couloir)
  G.start = (position){ .x = x1 - w1/2, .y = y1 - w1/2 };
  G.end   = (position){ .x = w1/2, .y = w1/2 };

  int i,j;
  
  // au début des murs sur les bords de chaque cellules de G, ce qui
  // forme une maillage, les bords de G étant déjà à TX_WALL
  for (i=1; i < x1; i++)
    for (j=1; j < y1; j++)
      if((i%w1 == 0)||(j%w1 == 0)) G.texture[i][j] = TX_WALL;

  // tableau pour la génération du labyrinthe
  int *maze = malloc(x*y*sizeof(*maze));
  memset(maze, -1, x*y*sizeof(*maze)); // vide le tableau (avec des -1)

  int count = 1;
  maze[0] = 0;

  while (count < x*y) {
    // cherche un point i0 non vide
    int i0 = 0;
    while (i0 < x*y && maze[i0] != -1) i0++;
    maze[i0] = i0 + 1;
    while (i0 < x*y) {
      int x0 = i0/y, y0 = i0%y;
      // déplace (x0,y0) selon une direction aléatoire
      // qui doit rester dans le cadre [0,x[ × [0,y[
      while (true) {
        switch (random()&3) { // =0,1,2 ou 3
        case 0: if (x0 <= 0)   continue; x0--; break;
        case 1: if (y0 <= 0)   continue; y0--; break;
        case 2: if (x0 >= x-1) continue; x0++; break;
        case 3: if (y0 >= y-1) continue; y0++; break;
        }
        break;
      }
      if (maze[x0*y + y0] == -1) {
        maze[x0*y + y0] = i0 + 1;
        i0 = x0*y + y0;
      } else {
        if (maze[x0*y + y0] > 0) {
          while (i0 != x0*y + y0 && i0 > 0) {
            int i1 = maze[i0] - 1;
            maze[i0] = -1;
            i0 = i1;
          }
        } else { // trace le couloir en effaçant les murs
          int i1 = i0;
          i0 = x0*y + y0;
          do {
            int const x0 = i0/y, y0 = i0%y;
            int const x1 = i1/y, y1 = i1%y;
	    if( x1 != x0) {
	      int const m = (x0>x1)? x0 : x1;
	      for (i=0; i<w; ++i) G.texture[m*w1][y0*w1 + i+1] = TX_FREE;
	    }
	    if( y1 != y0) {
	      int const m = (y0>y1)? y0 : y1;
              for (i=0; i<w; ++i) G.texture[x1*w1 + i+1][m*w1] = TX_FREE;
	    }
            i0 = i1;
            i1 = maze[i0] - 1;
            maze[i0] = 0;
            count++;
          } while (maze[i1] != 0);
          break;
        }
      }
    }
  }

  free(maze);

  return G;
}

//
// Initialise une grille à partir d'un fichier. Chaque caractère
// correspond à une texture particulière: ' ' -> TX_FREE, '#' ->
// TX_WALL, '~' -> TX_WATER, ... (voir le switch dans le code
// ci-dessous). Tous les types de texture ne sont pas codés.
//
grid initGridFile(char *file) {
  FILE *f = fopen(file, "r");
  if (f == NULL) {
    printf("Cannot open file \"%s\"\n", file);
    exit(1);
  }

  char *L = NULL; // L=buffer pour la ligne de texte à lire
  size_t b = 0;   // b=taille du buffer L utilisé (nulle au départ)
  ssize_t n;      // n=nombre de caractères lus dans L, sans le '\0'

  // Étape 1: on évalue la taille de la grille. On s'arrête si c'est
  // la fin du fichier ou si le 1ère caractère n'est pas un '#'

  int x = 0; // x=nombre de caractères sur une ligne
  int y = 0; // y=nombre de lignes

  while ((n = getline(&L, &b, f)) > 0) {
    if (L[0] != '#') break;
    if (L[n - 1] == '\n')
      n--; // se termine par '\n' sauf si fin de fichier
    if (n > x) x = n;
    y++;
  }

  rewind(f);

  // initialise la grille et son image
  grid G = allocGrid(x, y);

  // Étape 2: on relie le fichier et on remplit la grille

  int v,i,j;
  for (j=0; j<y; j++) {
    n = getline(&L, &b, f);
    if (L[n-1] == '\n') n--; // enlève le '\n' éventuelle
    for (i=0; i<n; i++) { // ici n<=x
      switch (L[i]) {
      case ' ': v = TX_FREE;   break;
      case '#': v = TX_WALL;   break;
      case ';': v = TX_SAND;   break;
      case '~': v = TX_WATER;  break;
      case ',': v = TX_MUD;    break;
      case '.': v = TX_GRASS;  break;
      case '+': v = TX_TUNNEL; break;
      case 's': v = TX_FREE; G.start = (position){ .x=i, .y=j }; break;
      case 't': v = TX_FREE; G.end   = (position){ .x=i, .y=j }; break;
      default:  v = TX_FREE;
      }
      G.texture[i][j] = v;
    }
  }

  free(L);
  fclose(f);
  return G;
}

//
// Initialise une grille 3D de taille x*y dont la texture dépend de
// l'altitude, le champs .height[][]. Positionne .end sur le plus haut
// pic et .start le un point d'altitude >= 0 le plus loin possible de
// .end mais pas à 10% d'un bord.
//
grid initGrid3D(int x, int y) {

  contrast = 0.5; lux = 1; // valeurs qui vont bien

  // Hiéarchie des textures en fonction de l'altitude.

  // du plus bas au plus profond
  int const EAU[]={ // si altitude < 0
    TX_WATER,
    TX_LAKE,
    TX_POOL,
  };

  // du plus bas au plus haut
  int const MONT[]={ // si altitude >= 0
    TX_SAND,
    TX_GRASS,
    TX_MEADOW,
    TX_FOREST,
    TX_GRAVEL,
    TX_ROCK,
    TX_ICE,
    TX_SNOW,
  };

  // initialise la grille et son image
  grid G = allocGrid(x, y); // G->height est rempli

  int i,j,t;

  // Remplissage des creux selon EAU[] et des montagnes selon MONT[],
  // sans toucher aux bords. On ne le fait que si G.hmax et G.hmin
  // sont non nuls (division par 0 sinon).

  if(G.hmax > 0 && G.hmin < 0)
    for(i=1; i<x-1; i++)
      for(j=1; j<y-1; j++){
	float h = G.height[i][j];
	if(h>0){
	  t = (CARD(MONT)*h)/G.hmax;
	  if(t == CARD(MONT)) t--;
	  G.texture[i][j] = MONT[t];
	}else{
	  t = (CARD(EAU)*h)/G.hmin;
	  if(t == CARD(EAU)) t--;
	  G.texture[i][j] = EAU[t];
	}
      }


  // Place la destination au sommet du plus haut pic. S'il n'est pas
  // trouvé, alors il est placé aléatoirement.

  for(i=1; i<x-1; i++)
    for(j=1; j<y-1; j++)
      if(G.height[i][j] == G.hmax) goto break_ij;

 break_ij:
  G.end = (i==x && j==y)? randomPosition(G,TX_FREE) : (position){i,j};
  
  // Place la source comme un point d'altitude >= 0 et le plus loin
  // possible de la destination, mais pas à plus de 10% d'un bord.

  G.start = G.end; // par défaut
  double dmin = 0; // distance entre G.end et G.start

  for(i=1+0.1*x; i<0.9*x-1; i++)
    for(j=1+0.1*y; j<0.9*y-1; j++){
      if(G.height[i][j] < 0) continue;
      double d = distL2(G.end, (position){i,j}); // distance entre G.end et (i,j)
      d -= G.height[i][j]*G.height[i][j]; // pénalité fonction de la hauteur
      if (d>dmin) G.start = (position){i,j}, dmin = d; // on a trouvé plus loin
    }
  
  return G;
}

//
// Ajoute à la grille n régions aux contours aléatoires (blobs) de
// type t donnés, sans toucher aux bords ni à start et end.
//
void addRandomBlob(grid G, int type, int n) {

  int const V[8][2] = {{ 0, -1}, { 1, 0}, {0,  1}, {-1, 0},
		       {-1, -1}, {-1, 1}, {1, -1}, { 1, 1}};

  int const X2 = G.X-2;
  int const Y2 = G.Y-2;

  for (int i = 0; i < n; i++) // met n graines
    G.texture[1 + random()%X2][1 + random()%Y2] = type;

  int m = (G.X+G.Y)/2; // demi-périmètre
  for (int t = 0; t < m; t++) // répète m fois
    for (int i = 1; i <= X2; i++)
      for (int j = 1; j <= Y2; j++) {
        int c = 0; // c = nombre de voisins de (i,j) de texture "type"
        for (int k = 0; k < 8; k++)
          if (G.texture[i + V[k][0]][j + V[k][1]] == type) c++;
        if (c && (random() % ((8 - c) * 20 + 1) == 0)) G.texture[i][j] = type;
      }
}

static inline double angle(double const x, double const y)
/*
  Renvoie l'angle de [0,2𝜋[ en radian du point de coordonnées
  cartésiennes (x,y) par rapport à l'axe des abscisses (1,0). NB:
  atan(y/x) donne un angle [-𝜋/2,+𝜋/2] ce qui n'est pas ce que l'on
  veut. On renvoie 0 si (x,y)=(0,0).

  Pour avoir ces constantes, faire sous xmaple par exemple:
  > evalf(3𝜋/2,39);
*/
{
#define M_2PI   6.28318530717958647692528676655900576839 /* = 2𝜋 */
#define M_3PI_2 4.71238898038468985769396507491925432630 /* = 3𝜋/2 */

  if(x==0){
    if(y>0) return M_2PI;   // =  𝜋/2
    if(y<0) return M_3PI_2; // = 3𝜋/2
    return 0;
  }

  // atan(y/x) renvoie un angle entre -𝜋/2 et +𝜋/2
  // l'angle est correct si x>0 et y>0
  // si x,y de signe contraire alors atan(y/x)=-atan(y/x)

  double const a=atan(y/x);
  if(x>0) return (y>0)? a : a+M_2PI; // = a ou a+2𝜋
  return a+M_PI;                     // = a+𝜋
}

//
// Ajoute à la grille G un ensemble de n segments (arcs) de texture de
// type t entre G.start et G.end, sans toucher aux bords, ni à G.start
// et G.end, sauf si ces derniers sont < 0. Dans ce cas ils sont fixés
// aléatoirement dans une partie libre de G (TX_FREE), si possible.
// 
void addRandomArc(grid G, int type, int n) {
  
  // Algorithme: n fois on crée un arc de cercle entre +𝜋/4 et -𝜋/4
  // (en fait 𝜋/(2c) où c=0.6 autour de l'axe s-t (on tire au hasard
  // si l'arc est depuis s ou depuis t). L'arc est continue selon de
  // 4-voisinage.

  if( !inGrid(&G,G.start) || !inGrid(&G,G.end) ){
    G.start = randomPosition(G, TX_FREE);
    G.end = randomPosition(G, TX_FREE);
  }
  // ici .star et .end sont dans la grille
  if(equalPosition(G.start,G.end)) return; // rien à faire 

  double d,a1,a2,t,a,da;
  position p,q,u,v;
  double const c=0.6;

  for(int i=0;i<n; i++){ // on répète n fois

    a1 = RAND01 * M_PI*c-M_PI*c/2; // angle1 par rapport au segment s-t
    a2 = RAND01 * M_PI*c-M_PI*c/2; // angle2 par rapport au segment s-t

    if(a1<a2) SWAP(a1,a2,t); // ici a1>a2
    if(a1-a2>M_PI/3) a2=(a1+a2)/2; // si arc trop grand

    // choisit au hasard le point de départ: s ou t
    p=G.start, q=G.end;
    if(random()&1) SWAP(p,q,u);

    d=RAND01*distL2(p,q); // d=distance p-q
    t=angle(q.x-p.x,q.y-p.y); // angle entre p->q
    a=a1; // angle courant
    da=(a1-a2)/(1.5*d); // variation d'angle

    for(int j=0;j<(int)(1.5*d); j++){

      // u=position courante à dessiner
      u=(position){ p.x+d*cos(t+a), p.y+d*sin(t+a) };
      if(!inGrid(&G,u) || onBorder(&G,u.x,u.y)) continue;
      G.texture[u.x][u.y]=type; // position sur la grille
      a-=da; // pour la prochaine position

      // teste si la prochaine position v de u va être en diagonale
      // pour la 4-connexité
      v=(position){ p.x+d*cos(t+a), p.y+d*sin(t+a) };
      if(abs(u.x-v.x)>0 && abs(u.y-v.y)>0){ // v en diagonale ?
        if(random()&1) u.x=v.x; else u.y=v.y; // corrige x ou y au hasard
        if(!inGrid(&G,u) || onBorder(&G,u.x,u.y)) continue;
        G.texture[u.x][u.y]=type; // position sur la grille
      }
    }
  }

}

static void init_opengl_drawing() {
  vao3d  = createVAO();
  vaotex = createVAO();

  MVP   = m4_identity(); 
  proj  = m4_perspective(45, (float)width/(float)height, 1, 10000);
  view  = m4_identity();
}

static GLShader build_shader(const char* vertex, const char* fragment) {

  GLShader shad;
  const char* vstring = vertex;
  const char* fstring = fragment;

  shad.vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shad.vertex, 1, &vstring, NULL);
  glCompileShader(shad.vertex);

  shad.fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shad.fragment, 1, &fstring, NULL);
  glCompileShader(shad.fragment);

  int  success;
  char infoLog[512];

  glGetShaderiv(shad.vertex, GL_COMPILE_STATUS, &success);
  if(!success) {
    glGetShaderInfoLog(shad.vertex, 512, NULL, infoLog);
    printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED:\n%s", infoLog);
    exit(EXIT_FAILURE);
  }

  glGetShaderiv(shad.fragment, GL_COMPILE_STATUS, &success);
  if(!success) {
    glGetShaderInfoLog(shad.fragment, 512, NULL, infoLog);
    printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED:\n%s", infoLog);
    exit(EXIT_FAILURE);
  }

  shad.program = glCreateProgram();
  glAttachShader(shad.program, shad.vertex);
  glAttachShader(shad.program, shad.fragment);
  glLinkProgram(shad.program);

  glGetProgramiv(shad.program, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(shad.program, 512, NULL, infoLog);
    printf("ERROR::SHADER::LINKING::COMPILATION_FAILED:\n%s", infoLog);
    exit(EXIT_FAILURE);
  }

  return shad;
}

static void free_shader(GLShader shad) {
  glDeleteShader(shad.vertex);
  glDeleteShader(shad.fragment); 
}

// Initialisation de SDL
void init_SDL_OpenGL(void) {

  SDL_Init(SDL_INIT_VIDEO);

  // Set OpenGL version (2.1 Core Profile in this case)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  
  // Set OpenGL profile to Core
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Multi-sampling (for antialiasing)
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  window = SDL_CreateWindow(getTitle(), SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, width, height,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (window == NULL) { // échec lors de la création de la fenêtre
    printf("Could not create window: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }

  // SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
  SDL_GetWindowSize(window, &width, &height);

  // Contexte OpenGL
  glcontext = SDL_GL_CreateContext(window);

  // Some GL options
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_MULTISAMPLE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &maptexture);
  glBindTexture(GL_TEXTURE_2D, maptexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // Initialisation des shaders et des variables
  mapshader = build_shader(mapshader_vert_glsl, mapshader_frag_glsl);
  MVP_location         = glGetUniformLocation(mapshader.program, "MVP");
  bbox_location        = glGetUniformLocation(mapshader.program, "bbox");
  cam_pos_location     = glGetUniformLocation(mapshader.program, "cam_pos");
  lux_location         = glGetUniformLocation(mapshader.program, "lux");
  contrast_location    = glGetUniformLocation(mapshader.program, "contrast");
  
  texshader = build_shader(texture_vert_glsl, texture_frag_glsl);
  transcale_location = glGetUniformLocation(texshader.program, "translation_scale");
  screensize_location = glGetUniformLocation(texshader.program, "screensize");

  geomshader = build_shader(geom_vert_glsl, geom_frag_glsl);
  geom_transcale_location = glGetUniformLocation(geomshader.program, "translation_scale");

  init_opengl_drawing();
}

// Fermeture de SDL
void cleaning_SDL_OpenGL() {
  freeVAO(vao3d);
  freeVAO(vaotex);
  if(vaogeom) freeVAO(vaogeom);
  if(glgrid) freeGridBuffer(glgrid);
  if(gvertices) freeGLBuffer(gvertices);
  if(gcolors) freeGLBuffer(gcolors);
  free_shader(mapshader); 
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

// Permet de recentrer les points de POINT[] pour qu'ils apparaissent
// au centre de la fenêtre [0,width] × [0,height] en laissant un bord
// vide tout autour (margin). Ici margin est une fraction de width et
// height. Prendre par exemple margin=0.05 pour laisser un bord de 5%
// vide tout autour. Les valeurs de POINT[] sont modifiées.
void centering(double margin){
  point Pmin = POINT[0];
  point Pmax = Pmin;

  for(int i=1; i<nPOINT; i++){ // bounding box
    Pmin.x = fmin(Pmin.x,POINT[i].x);
    Pmin.y = fmin(Pmin.y,POINT[i].y);
    Pmax.x = fmax(Pmax.x,POINT[i].x);
    Pmax.y = fmax(Pmax.y,POINT[i].y);
  }

  double const m = fmin(width,height);
  double s = fmax(Pmax.x-Pmin.x,Pmax.y-Pmin.y);
  if(s==0) s=m; // si tous les points sont confondus

  point const c = { width / 2.0, height / 2.0}; // centre fenêtre
  point const M = { (Pmin.x+Pmax.x)/2, (Pmin.y+Pmax.y)/2 }; // centre points
  double const K = m/s/(1+margin);

  // recentrage (avec décalage) et une marge au bord
  for(int i=0; i<nPOINT; i++){
    POINT[i].x = (POINT[i].x - M.x)*K + c.x;
    POINT[i].y = (POINT[i].y - M.y)*K + c.y;
  }

}

// Compare deux angles entre l'axe des abscisses et les point P et Q.
int fcmp_angle(const void *P, const void *Q){
  double const p = angle(((point*)P)->x,((point*)P)->y);
  double const q = angle(((point*)Q)->x,((point*)Q)->y);
  return (p>q) - (p<q);
}

// Génère n points aléatoires du rectangle [0,width] × [0,height] et
// renvoie le tableau des n points (type double) ainsi générés. Met à
// jour les variables globales POINT[] et nPOINT. Une bande de taille
// 2*size_pt est laissée libre autour du rectangle afin d'éviter
// d'avoir des points trop proche du bord.
point *generatePoints(int n) {

  POINT = malloc(n * sizeof(point));
  double const d = 2*size_pt;
  double const rx = (double)(width-2*d);
  double const ry = (double)(height-2*d);
  for (int i = 0; i < n; i++) {
    POINT[i].x = d + rx * RAND01;
    POINT[i].y = d + ry * RAND01;
  }
  nPOINT = n;
  return POINT;
}

// Génère n points du rectangle [0,width] × [0,height] répartis
// aléatoirement sur k cercles concentriques de rayon r_j =
// j*min{height,width}/(2.2*k) pour j=1..k. Met à jour les variables
// globales POINT[] et nPOINT.
point *generateCircles(int n, int k) {

  POINT = malloc(n * sizeof(point));
  point c = { width / 2.0, height / 2.0 }; // centre
  double const r0 = fmin(width,height)/(2.2*k);

  for (int i = 0; i < n; i++) { // on place les n points
    int j = random()%k; // j=numéro du cercle
    double r = (j+1)*r0;
    double a = 2.0 * M_PI * RAND01; // angle aléatoire
    POINT[i].x = c.x + r * cos(a);
    POINT[i].y = c.y + r * sin(a);
  }

  nPOINT = n;
  return POINT;
}

// Génère n points sur un disque centrée dans le rectangle [0,width] ×
// [0,height] de rayon 98% du min(width,height)/2 avec comme distance
// au centre u^p avec u aléaoire uniforme dans [0,1]. Prendre p=0.5
// pour une distribution uniforme dans le disque, p>0.5 pour une
// concentration des valeurs vers le centre et p<0.5 pour un
// écartement du centre. Les valeurs <0 de p donne des écartements au
// delà du rayon du disque. Met à jour les variables globales POINT[]
// et nPOINT.
point *generatePower(int n, double p){

  POINT = malloc(n * sizeof(point));
  point c = { width / 2.0, height / 2.0 }; // centre
  double const r0 = 0.49*fmin(width,height);

  for (int i = 0; i < n; i++) { // on place les n points
    double a = 2.0 * M_PI * RAND01; // angle aléatoire
    double r = r0 * pow(RAND01,p); // loi puissance
    POINT[i].x = c.x + r * cos(a);
    POINT[i].y = c.y + r * sin(a);
  }

  nPOINT = n;
  return POINT;
}

// Génère n points en position convexe dans le rectangle [0,width] ×
// [0,height]. Le principe de l'algorithme, de complexité O(nlog(n)),
// est le suivant:
//
// On part de points aléatoires dans [0,1[², puis on calcule (pour les
// points finaux) la différence entre deux points consécutifs. La
// somme des n différences est nulle. On trie alors les points obtenus
// selon l'angle, puis on dessine de proche en proche les points de
// l'enveloppe convexe (avec chaque fois d'un angle croissant donc).
//
// Le dessin est ensuite recalibré pour tenir dans la fenêtre. Met à
// jour les variables globales POINT[] et nPOINT.
point *generateConvex(int n){

  POINT = malloc(n * sizeof(point));

  for(int i=0; i<n; i++)
    POINT[i] = (point){ RAND01, RAND01 };

  point p0 =  POINT[0];      // sauvegarde le 1er point
  for(int i=0; i<n-1; i++){ // différences
    POINT[i].x -= POINT[i+1].x;
    POINT[i].y -= POINT[i+1].y;
  }
  POINT[n-1].x -= p0.x;
  POINT[n-1].y -= p0.y;

  qsort(POINT,n,sizeof(point),fcmp_angle); // trie les angles

  for(int i=1; i<n; i++){ // dessin
    POINT[i].x += POINT[i-1].x;
    POINT[i].y += POINT[i-1].y;
  }

  nPOINT = n;
  centering(0.2); // centrage avec 20% de marge
  return POINT;
}

// Génère pq points sur une grille pxq régulière et centrée sur le
// rectangle [0,width] × [0,height]. On suppose p,q>0 et n!=NULL. Le
// point d'indice 0 est en haut à gauche, les autres suivent lignes
// par lignes, les segments horizontaux et verticaux étant le même
// longueur. Ecrit dans n le nombre de points, soit pq. Met à jour les
// variables globales POINT[] et nPOINT.
point *generateGrid(int *n, int p, int q) {

  POINT = malloc(p*q * sizeof(point));

  for (int y = *n = 0; y < p; y++)
    for (int x = 0; x < q; x++, (*n)++ )
      POINT[*n] = (point){ x, y };

  nPOINT = *n;
  centering(0.2); // centrage avec 20% de marge
  return POINT;
}

// Lecture des points à partir d'un fichier. Renvoie NULL et n=0 en
// cas d'erreur. Met à jour les variables globales POINT[] et nPOINT.
point *generateLoad(int* n, char *file){

  // ouvre le fichier
  FILE *f=fopen(file,"r");
  if(f==NULL){
    printf("Cannot open file \"%s\"\n",file);
    nPOINT = *n = 0;
    POINT = NULL;
    return NULL;
  }

// lit les commentaires éventuels, ils doivent commencer par '#' mais
// comprendre au moins un autre caractère
#define READ_COMMENT(f)				\
  do{						\
    char tmp[1024];				\
    while(!feof(f)&&fscanf(f,"#%[^\n] ",tmp));	\
  }while(0)

  // lit le nombre de points
  READ_COMMENT(f);
  fscanf(f,"%i\n",n);
  bool center = (*n<0); // si n<0, alors centrage à la fin
  *n = abs(*n);

  // lit les options
  double factor = 1.0;
  point shift = {0,0};
  READ_COMMENT(f);
  fscanf(f,"factor = %lf\n",&factor);
  READ_COMMENT(f);
  fscanf(f,"shift = %lf %lf\n",&(shift.x),&(shift.y));

  // lit les points
  POINT = malloc((*n)*sizeof(point));

  int i=0;
  for(;;){
    READ_COMMENT(f);
    if(feof(f)) break;
    fscanf(f,"%lf %lf\n",&(POINT[i].x),&(POINT[i].y));
    POINT[i].x = POINT[i].x * factor + shift.x;
    POINT[i].y = POINT[i].y * factor + shift.y;
    i++; // compte le nombre de points lus
  }
  fclose(f);

#undef READ_COMMENT

  if( (i!=(*n)) || ((*n)<1) || (POINT==NULL) ){
    printf("Incorrect number of points.\n");
    *n = 0;
    POINT = NULL;
  }

  nPOINT = *n;
  if(center) centering(0.1); // centrage avec 10% de marge
  return POINT;
}


// couleurs pour drawX(), valeurs RGB dans [0,1]

GLfloat COLOR_GROUND[] = { 0.0, 0.0, 0.0 }; // fond
GLfloat COLOR_POINT[]  = { 1.0, 0.0, 0.0 }; // point
GLfloat COLOR_LINE[]   = { 1.0, 1.0, 1.0 }; // ligne de la tournée
GLfloat COLOR_PATH[]   = { 0.0, 0.0, 1.0 }; // chemin
GLfloat COLOR_ROOT[]   = { 0.9, 0.8, 0.3 }; // racine, point de départ
GLfloat COLOR_TREE[]   = { 0.0, 0.4, 0.0 }; // arête de l'arbre

#define CLR_GROUND  COLOR_GROUND[0], COLOR_GROUND[1], COLOR_GROUND[2]
#define CLR_POINT   COLOR_POINT[0],  COLOR_POINT[1],  COLOR_POINT[2]
#define CLR_LINE    COLOR_LINE[0],   COLOR_LINE[1],   COLOR_LINE[2]
#define CLR_PATH    COLOR_PATH[0],   COLOR_PATH[1],   COLOR_PATH[2]
#define CLR_ROOT    COLOR_ROOT[0],   COLOR_ROOT[1],   COLOR_ROOT[2]
#define CLR_TREE    COLOR_TREE[0],   COLOR_TREE[1],   COLOR_TREE[2]

vec3_t* CLR_GROUND_V3 = (vec3_t*)COLOR_GROUND;
vec3_t* CLR_POINT_V3  = (vec3_t*)COLOR_POINT;
vec3_t* CLR_LINE_V3   = (vec3_t*)COLOR_LINE;
vec3_t* CLR_PATH_V3   = (vec3_t*)COLOR_PATH;
vec3_t* CLR_ROOT_V3   = (vec3_t*)COLOR_ROOT;
vec3_t* CLR_TREE_V3   = (vec3_t*)COLOR_TREE;

// dessine les k premiers sommets d'une tournée; ou dessine une
// tournée complète (si k=n+1); ou dessine un graphe (si G<>NULL) et
// sa tournée complète.

void drawX(point *V, int n, int *P, int k, graph *G) {
  static uint last_tick = 0;
 
  cpu_triangles_amount  = 0;

  if(vaogeom == NULL) {
    vaogeom = createVAO();
    bindVAO(vaogeom);

    gvertices = createGLBuffer(cpuvertices, MAX_VERTICES);
    gcolors   = createGLBuffer(cpucolors  , MAX_VERTICES);

    bindGLBuffer(gvertices);
    attachBuffertoVAO(vaogeom, gvertices, 0);
    
    bindGLBuffer(gcolors);
    attachBuffertoVAO(vaogeom, gcolors  , 1);
   
    unbindVAO();
  }

  // saute le dessin si le précédent a été fait il y a moins de 20ms
  // ou si update est faux
  if ((!update) && (last_tick + 20 > SDL_GetTicks())) return;
  last_tick = SDL_GetTicks();

  // gestion de la file d'event
  handleEvent(false);

  // dessine un quadrillage, si besoin
  if (quadrillage){
    GLfloat C[3]; // couleur du quadrillage = couleur du fond +/- 0.35
    for(int i=0;i<3;i++) C[i]=COLOR_GROUND[i] + 0.35*((COLOR_GROUND[i]<0.5)? +1.0:-1.0);
    double m = 1.2*fmax(width,height); // quadrillage 20% plus large que la fenêtre
    int k = (1<<(quadrillage+1))+1; // nombre de croix qui doit être impair
    double e = m/(k-1); // écart entre les croix
    point z = (point){ (width - e*(k-1))/2, (height-e*(k-1))/2 }; // point zéro
    // on place les k croix sur la diagonale allant de z+(0,0) à z+(m,m)
    for (int i=0; i<k; i++ ){ // k croix, dont une au centre
      prepareDrawLine( (point){ z.x+0,   z.y+i*e }, (point){ z.x+m,   z.y+i*e }, *(vec3_t*)C, 1);
      prepareDrawLine( (point){ z.x+i*e, z.y+0   }, (point){ z.x+i*e, z.y+m   }, *(vec3_t*)C, 1);
    }
  }

  // dessine G, s'il existe
  if (G && V && G->list && (G->deg[0]>=0) && (mst&1))
    for (int i = 0; i < n; i++)
      for (int j = 0; j < G->deg[i]; j++)
        if (i < G->list[i][j])
          prepareDrawLine(V[i], V[G->list[i][j]], *CLR_TREE_V3, 6.0f);

  // dessine le cycle en blanc si k=n+1; ou
  if (V && P && (P[0]>=0) && ((G && (mst&2)) || (!G && (mst&1)))) {
    vec3_t col;
    if(k>n){
      col = *CLR_LINE_V3;
      k=n+1; // k = pas plus que n+1
    }
    else col = *CLR_PATH_V3;
    for (int i=0; i<k-1; i++){
      if(oriented) prepareDrawArrow(V[P[i]], V[P[(i + 1) % n]], col, 1.5f);
      else prepareDrawLine(V[P[i]], V[P[(i + 1) % n]], col, 1.5f);
    }
    if (root) {
      if(oriented && n>0) prepareDrawArrow(V[P[0]], V[P[1%n]], col, 1.5f);
      else prepareDrawLine(V[P[0]], V[P[1%n]], col, 1.5f);
    }
  }
  
  // dessine les points
  if (V) {
    for (int i = 0; i < n; i++)
      prepareDrawPoint(V[i], *CLR_POINT_V3, size_pt);
    if (root && P && (P[0]>=0) ) prepareDrawPoint(V[P[0]], *CLR_ROOT_V3, size_pt);
  }

  glUseProgram(geomshader.program);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  
  glUniform3fv(geom_transcale_location, 1, (GLfloat*)&transcale);

  // efface la fenêtre
  glClearColor(CLR_GROUND, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  

  bindVAO(vaogeom);
  draw_stored_triangles();

  // affiche le dessin
  SDL_GL_SwapWindow(window);
}

static void prepareDrawArrow(const point p, const point q, const vec3_t color, double arrowwidth) {
  float arrowsize = ((float)(width))/100.0f;
  float arrowwidth_coeff = 0.5f;
  
  vec3_t p1 = {p.x, p.y, 0};
  vec3_t p2 = {q.x, q.y, 0};
  vec3_t dir    = v3_norm(v3_sub(p2, p1));
  vec3_t normal = {-dir.y, dir.x, dir.z};
  dir    = v3_muls(dir   , arrowsize);
  normal = v3_muls(normal, arrowsize*arrowwidth_coeff);

  vec3_t interm = v3_sub(p2, dir);

  vec3_t corner1 = v3_add(interm, normal);
  vec3_t corner2 = v3_sub(interm, normal);

  point corner1p = {corner1.x, corner1.y};
  point corner2p = {corner2.x, corner2.y};

  prepareDrawLine(p, q, color, arrowwidth);
  prepareDrawLine(q, corner1p, color, arrowwidth);
  prepareDrawLine(q, corner2p, color, arrowwidth);
}

static vec3_t point_to_glcoord(const point p) {
  return (vec3_t){p.x/((float)(width)),(1.0f-p.y/((float)(height))),0};
}

static vec3_t to_glcoord(const vec3_t p) {
  return (vec3_t){p.x/((float)(width)),(1.0f-p.y/((float)(height))),0};
}

static vec3_t unproj_point(const point p) {
  return (vec3_t){p.x, p.y,0};
}

static void prepareDrawPoint(const point p, const vec3_t color, double width) {
  int precision = 6;
  float w = width/2.0f;

  vec3_t center = point_to_glcoord(p);

  for(uint i=0; i<precision; i++){
    int j = (i+1)%precision;

    float angle1 = (float)(i)/(float)(precision) * M_PI * 2.0f;
    float angle2 = (float)(j)/(float)(precision) * M_PI * 2.0f;

    prepareDrawTriangleGradGLcoord(
      center,
      point_to_glcoord((point){p.x + w*cos(angle1), p.y + w*sin(angle1)}),
      point_to_glcoord((point){p.x + w*cos(angle2), p.y + w*sin(angle2)}),
      color,
      color,
      color
    );
  }
}

/*
  t1-----------------t2
  |                   |
  |   u1         u2   |
  |                   |
  b1-----------------b2
*/

static void prepareDrawLineGrad(const point p1, const point p2,
				const vec3_t color1, const vec3_t color2,
				float width) {

  float w = width/2.0f;
  vec3_t u1 = unproj_point(p1); 
  vec3_t u2 = unproj_point(p2);

  vec3_t dirN    = v3_norm(v3_sub(u2,u1));
  vec3_t normalN = {-dirN.y, dirN.x, 0};

  vec3_t b1 = v3_sub(v3_sub(u1, v3_muls(dirN, w)), v3_muls(normalN, w));
  vec3_t t1 = v3_add(v3_sub(u1, v3_muls(dirN, w)), v3_muls(normalN, w));
  vec3_t b2 = v3_sub(v3_add(u2, v3_muls(dirN, w)), v3_muls(normalN, w));
  vec3_t t2 = v3_add(v3_add(u2, v3_muls(dirN, w)), v3_muls(normalN, w));

  vec3_t b1gl = to_glcoord(b1);
  vec3_t t1gl = to_glcoord(t1);
  vec3_t b2gl = to_glcoord(b2);
  vec3_t t2gl = to_glcoord(t2);

  prepareDrawTriangleGradGLcoord(
    b1gl,
    b2gl,
    t2gl,
    color1,
    color2,
    color2
  );
  prepareDrawTriangleGradGLcoord(
    b1gl,
    t2gl,
    t1gl,
    color1,
    color2,
    color1
  );
}

static void prepareDrawTriangleGradGLcoord(const vec3_t p1, const vec3_t p2, const vec3_t p3,
					   const vec3_t color1, const vec3_t color2, const vec3_t color3) {

  if((cpu_triangles_amount+1)*3 >= MAX_VERTICES) {
    fprintf(stderr, "Too many lines to be drawn (%d), change the macro MAX_VERTICES\n", cpu_triangles_amount);
    exit(EXIT_FAILURE);
  }

  cpuvertices[cpu_triangles_amount*3+0] = p1;
  cpuvertices[cpu_triangles_amount*3+1] = p2;
  cpuvertices[cpu_triangles_amount*3+2] = p3;

  cpucolors[cpu_triangles_amount*3+0] = color1;
  cpucolors[cpu_triangles_amount*3+1] = color2;
  cpucolors[cpu_triangles_amount*3+2] = color3;

  cpu_triangles_amount++;
}

static void prepareDrawLine(const point p, const point q,
			    const vec3_t color, double width) {
  prepareDrawLineGrad(p, q, color, color, width);
}

// si vaogeom est borné
static void draw_stored_triangles(void) {

  if(cpu_triangles_amount > 0) {
    if(cpu_triangles_amount*3 >= MAX_VERTICES) {
      fprintf(stderr, "Too many lines to be drawn (%d), change the macro MAX_VERTICES (%d)\n", cpu_triangles_amount, MAX_VERTICES);
      exit(EXIT_FAILURE);
    }

    replaceGLBufferData(gvertices, cpuvertices, 0, cpu_triangles_amount*3);
    replaceGLBufferData(gcolors  , cpucolors, 0, cpu_triangles_amount*3);

    glDrawArrays(GL_TRIANGLES, 0 , cpu_triangles_amount*3);
    
    cpu_triangles_amount = 0;
  }
}

void drawTour(point *V, int n, int *P) { drawX(V,n,P,n+1,NULL); }
void drawPath(point *V, int n, int *P, int k) { drawX(V,n,P,k,NULL); }
void drawGraph(point *V, int n, int *P, graph G) { drawX(V,n,P,n+1,&G); }

static void drawGridImage3D(grid G) {

  // initialise glgrid et la caméra
  if(glgrid == NULL) { // une seule fois au départ
    glgrid = createGridBuffer(&G);
    attachGridBufferVAO(glgrid, vao3d);
    cam_arrows_speed = (G.X+G.Y)*0.01f;
    cam_tour_radius  = (G.X+G.Y)*0.5f;
    cam_target.y = (G.hmax-G.hmin)/2.0f;
  } else glgrid->G = &G;

  // Met à jour l'angle de la caméra: ajoute à cam_angle la vitesse
  // cam_speed (en radian/seconde) × la différence de temps entre
  // deux appels (en seconde).

  static struct timeval oldtime = {0,0}; // 0 seconde et 0 microseconde
  double dt; // différence de temps en seconde

  if(oldtime.tv_sec == 0 && oldtime.tv_usec == 0){ // une seule fois au départ
    gettimeofday(&oldtime, NULL);
    dt = 0;
  }
  else{
    struct timeval newtime;
    gettimeofday(&newtime, NULL); // temps courant
    dt = (newtime.tv_sec - oldtime.tv_sec); // différence de secondes
    dt += (newtime.tv_usec - oldtime.tv_usec)/1000000.0; // +différence de μs
    oldtime = newtime;
  }

  cam_angle += dt * cam_speed;
  drawVAO(&mapshader, vao3d);
}

static void drawGridImage2D(grid G) {

  glUseProgram(texshader.program);

  bindVAO(vaotex);

  // Draw parameters
  glEnable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, maptexture);

  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, G.X, G.Y,
		  GL_RGB, GL_UNSIGNED_BYTE, gridImage);  

  glUniform3fv(transcale_location, 1, (GLfloat*)&transcale );

  vec2_t screensize = (vec2_t){width, height};  
  glUniform2fv(screensize_location, 1, (GLfloat*)&screensize);  

  glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void update_maptexture(grid G) {

  static bool created = false;

  if(!created) {
    glBindTexture(GL_TEXTURE_2D, maptexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, G.X, G.Y, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, gridImage);
    created = true;
  }
}

static void drawGridImage(grid G) {

  // Efface la fenêtre
  glClearColor(CLR_GROUND, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  update_maptexture(G);
  if(view3D) drawGridImage3D(G);
  else drawGridImage2D(G);

  openglerr();
}

#undef CLR_GROUND
#undef CLR_POINT
#undef CLR_LINE
#undef CLR_PATH
#undef CLR_ROOT
#undef CLR_TREE

void waitGridDelay(grid G, uint delay, uint frame_delay) {
  uint const last_tick = SDL_GetTicks();
  uint current_tick = SDL_GetTicks();

  while(running && current_tick - last_tick < delay) {
    handleEvent(false);
    drawGridImage(G);
    SDL_GL_SwapWindow(window);

    if (delay - (current_tick - last_tick) > frame_delay)
      SDL_Delay(frame_delay);
    else
      SDL_Delay(delay - (current_tick - last_tick));
    current_tick = SDL_GetTicks();
  }
}

// affichage de la grille
void drawGrid(grid G) {

  static uint last_tick = 0;
  static uint last_drawn_call = 0;
  static uint call_count = 0;
  
  uint const current_tick = SDL_GetTicks();

  uint const frame_rate = 50; // 50 fps
  uint const frame_delay = 1000 / frame_rate; // durée en millisecondes

  call_count++;
  uint next_drawn_call = call_count;

  if(!update)
    next_drawn_call = last_drawn_call + call_speed / frame_rate;

  if (next_drawn_call > call_count)
    return;

  uint delay = 0;
  uint elasped_tick = current_tick - last_tick;
  uint elasped_call = call_count - last_drawn_call;

  if (elasped_call*1000 > elasped_tick * call_speed)
    delay = elasped_call*1000 / call_speed - elasped_tick;

  // ceci intervient quand call_speed diminue
  // le choix suivant est raisonnable au vu de la vitesse des entrées
  // utilisateur: on dessine la grille sans attendre
  if (elasped_call > call_speed) delay = 0;

  if (!update) waitGridDelay(G, delay, frame_delay);

  makeImage(&G);
  handleEvent(false);

  drawGridImage(G);

  // affiche le résultat puis attend un certain délai
  SDL_GL_SwapWindow(window);
  last_tick = current_tick;
  last_drawn_call = call_count;
}

bool handleEvent(bool wait_event) {

  bool POINT_has_changed = false;
  SDL_Event e;

  if (wait_event) SDL_WaitEvent(&e); // attendre un événement
  else if (!SDL_PollEvent(&e)) return false;

  do {
    switch (e.type) {

    case SDL_QUIT:
      goto quit;

    case SDL_KEYDOWN:
      if (e.key.keysym.sym == SDLK_q) {
	quit:
        running = false;
        update = false;
	call_speed = ULONG_MAX; // vitesse d'affichage max pour drawGrid()
        break;
      }
      if (e.key.keysym.sym == SDLK_p) { // pause
        SDL_Delay(500);
        SDL_WaitEvent(&e);
        break;
      }
      if (e.key.keysym.sym == SDLK_c) { // affichage des sommets visités pour A*
        erase = !erase;
        break;
      }
      if (e.key.keysym.sym == SDLK_o) { // affichage de l'orientation
        oriented = !oriented;
        break;
      }
      if (e.key.keysym.sym == SDLK_r) { // affichage de la racine de l'arbre
        root = !root;
        break;
      }
      if (e.key.keysym.sym == SDLK_t) { // switch points -> arbre -> tournée -> arbre+tournée
        mst = (mst+1)&3; // +1 modulo 4
        break;
      }
      if (e.key.keysym.sym == SDLK_g) { // affichage un quadrillage
        quadrillage = (quadrillage+1)%5; // +1 modulo 5
        break;
      }
      if (e.key.keysym.sym == SDLK_d) { // switch 3D
        view3D = 1-view3D;
        break;
      }
      if (e.key.keysym.sym == SDLK_s) { // taille des points
        size_pt *= 1.75; // NB: 1.75^5 = 16.41
        if(size_pt > 17) size_pt = 1; // redémarre à 1
        break;
      }
      if (e.key.keysym.sym == SDLK_RIGHT && view3D) {
        cam_target = v3_add(cam_target, v3_muls(right_dir(), cam_arrows_speed));
        cam_pos = v3_add(cam_pos, v3_muls(right_dir(), cam_arrows_speed));
        break;
      }
      if (e.key.keysym.sym == SDLK_LEFT && view3D) {
        cam_target = v3_sub(cam_target, v3_muls(right_dir(), cam_arrows_speed));
        cam_pos = v3_sub(cam_pos, v3_muls(right_dir(), cam_arrows_speed));
        break;
      }
      if (e.key.keysym.sym == SDLK_UP && view3D) {
        cam_target = v3_add(cam_target, v3_muls(up_dir(), cam_arrows_speed));
        cam_pos = v3_add(cam_pos, v3_muls(up_dir(), cam_arrows_speed));
        break;
      }
      if (e.key.keysym.sym == SDLK_DOWN && view3D) {
        cam_target = v3_sub(cam_target, v3_muls(up_dir(), cam_arrows_speed));
        cam_pos = v3_sub(cam_pos, v3_muls(up_dir(), cam_arrows_speed));
        break;
      }
      if (e.key.keysym.sym == SDLK_k && view3D) {
	// stoppe ou redémarre la caméra
	if(cam_speed > 0) {
	  cam_old_speed = cam_speed;
	  cam_speed = 0;
	} else cam_speed = cam_old_speed;
        break;
      }
      if (e.key.keysym.sym == SDLK_m && view3D) {
        if(cam_speed < 10) cam_speed *= 1.5f; // +50%
	if(cam_speed == 0) cam_speed = 0.05f; // redémarrage après arrêt
        break;
      }
      if (e.key.keysym.sym == SDLK_l && view3D) {
        cam_speed /= 1.5f; // -50%
        if(cam_speed < 0.05) cam_speed = 0; // arrêt
        break;
      }
      if (e.key.keysym.sym == SDLK_z || e.key.keysym.sym == SDLK_KP_MINUS) {
	// divise par deux la vitesse d'affichage pour drawGrid()
	// jusqu'à 1
	call_speed >>= 1;
	if (call_speed == 0) call_speed = 1;
        break;
      }
      if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_KP_PLUS) {
	// augmente (x2) la vitesse d'affichage pour drawGrid()
	// jusqu'à ULONG_MAX (sans le dépasser)
	if (call_speed == 0) call_speed = 1;
	if (call_speed >= (ULONG_MAX>>1)) call_speed = ULONG_MAX;
	call_speed <<= 1;
        break;
      }
      if (e.key.keysym.sym == SDLK_1 && view3D) {
	lux -= 0.1;
	if (lux < 0) lux = 0;
        break;
      }
      if (e.key.keysym.sym == SDLK_2 && view3D) {
	lux += 0.1;
	if (lux > 10) lux = 10;
        break;
      }
      if (e.key.keysym.sym == SDLK_3 && view3D) {
	contrast -= 0.1;
	if (contrast < 0) contrast = 0;
        break;
      }
      if (e.key.keysym.sym == SDLK_4 && view3D) {
	contrast += 0.1;
	if (contrast > 1) contrast = 1;
        break;
      }
      if (e.key.keysym.sym == SDLK_u) {
	if( (nPOINT<=0) || (POINT==NULL) ) break; // rien à faire
	centering(0.1); // centrage avec 10% de marge
        POINT_has_changed = true;
	break;
      }
      if (e.key.keysym.sym == SDLK_w) { // sauvegarde des points
	if( (nPOINT<=0) || (POINT==NULL) ) break; // rien à faire
	char file[MAX_FILE_NAME];
	printf("\nWrite points into a file.\n");
	printf("Enter file name ('q' to quit): ");
	scanf("%s",file);
	if(strcmp(file,"q")==0){
	  printf("waiting for a key ...");
	  break;
	}
	FILE *f=fopen(file,"w");
	if(f==NULL){
	  printf("Cannot open file \"%s\"\n",file);
	  break;
	}
	fprintf(f,"%i\n",nPOINT);
	for(int i=0; i<nPOINT; i++)
	  fprintf(f,"%g %g\n",POINT[i].x,POINT[i].y);
	fclose(f);
	printf("%i points saved in \"%s\".\n",nPOINT,file);
	printf("waiting for a key ...");
	break;
      }
      if (e.key.keysym.sym == SDLK_h) {
	printf("%s",HELP);
	fflush(stdout);
	break;
      }
      break;

    case SDL_WINDOWEVENT:
      if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        double x, y;
        getCenterCoord(&x, &y);
        glViewport(0, 0, e.window.data1, e.window.data2);
        zoomAt(scale, x, y);
        SDL_GetWindowSize(window, &width, &height);
        SDL_SetWindowTitle(window, getTitle());
      }
      break;

    case SDL_MOUSEWHEEL:
      if (e.wheel.y > 0 && !view3D) zoomMouse(2.0);
      if (e.wheel.y < 0 && !view3D) zoomMouse(0.5);
      if (e.wheel.y > 0 && view3D) {
        if(cam_view_mode == ROTATING) {
          cam_tour_radius /= 1.06f;
        }
        if(cam_view_mode == MANUAL) {
          cam_pos = v3_add(cam_pos, v3_muls(cam_dir, cam_manual_speed));
        }
      }
      if (e.wheel.y < 0 && view3D) {
        if(cam_view_mode == ROTATING) {
          cam_tour_radius *= 1.06f;
        }
        if(cam_view_mode == MANUAL) {
          cam_pos = v3_sub(cam_pos, v3_muls(cam_dir, cam_manual_speed));
        }
      }
      SDL_GetWindowSize(window, &width, &height);
      SDL_SetWindowTitle(window, getTitle());
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (e.button.button == SDL_BUTTON_LEFT && !view3D) {
        mouse_ldown = true;
        point transformed = transformPoint((point){e.motion.x, e.motion.y});
        double x = transformed.x;
        double y = transformed.y;
        if (hover) {
          int v = getClosestVertex(x, y);
          if ((x - POINT[v].x)*(x - POINT[v].x) +
              (y - POINT[v].y)*(y - POINT[v].y) < (size_pt*size_pt/transcale.z/transcale.z)+2 )
            selectedVertex = v;
        }
      }
      if (e.button.button == SDL_BUTTON_LEFT && view3D) {
        mouse_ldown = true;
	cam_view_mode = ROTATING;
	cam_speed = cam_old_speed;
      }
      if (e.button.button == SDL_BUTTON_RIGHT && view3D) {
        mouse_rdown = true;
	cam_view_mode = MANUAL;
	if(cam_speed > 0) cam_old_speed = cam_speed;
	cam_speed = 0.0f;
      }
      break;

    case SDL_MOUSEBUTTONUP:
      if (e.button.button == SDL_BUTTON_LEFT) {
        mouse_ldown = false;
        selectedVertex = -1;
      }
      if (e.button.button == SDL_BUTTON_RIGHT)
        mouse_rdown = false;
      break;

    case SDL_MOUSEMOTION:
      mouse_dx = (abs(e.motion.xrel) > 0) ? e.motion.xrel : mouse_dx;
      mouse_dy = (abs(e.motion.yrel) > 0) ? e.motion.yrel : mouse_dy;
      if (hover && !mouse_rdown && mouse_ldown && selectedVertex >= 0) {
        POINT[selectedVertex] = transformPoint((point){e.motion.x, e.motion.y});
        POINT_has_changed = true;
      }
      if (mouse_rdown) { 
        transcale.x -= ((float)e.motion.xrel) / ((float)width ) / transcale.z;
        transcale.y += ((float)e.motion.yrel) / ((float)height) / transcale.z;
      }
      break;

    }
  } while (SDL_PollEvent(&e));

  return POINT_has_changed;
}
