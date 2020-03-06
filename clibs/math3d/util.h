#ifndef math3d_util_h
#define math3d_util_h

extern "C"{
	#include "math3d.h"
};

#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/ext/scalar_relational.hpp"
#include "glm/ext/vector_relational.hpp"

static inline const glm::vec4 &
get_vec_value(lua_State *L, struct lastack *LS, int index) {
	return *(const glm::vec4 *)math3d_get_value(L, LS, index, LINEAR_TYPE_VEC4);
}

static inline const glm::quat &
get_quat_value(lua_State* L, struct lastack* LS, int index) {
	return *(const glm::quat *)math3d_get_value(L, LS, index, LINEAR_TYPE_QUAT);
}

static inline const glm::mat4x4 &
get_mat_value(lua_State* L, struct lastack* LS, int index){
	return *(const glm::mat4x4*)math3d_get_value(L, LS, index, LINEAR_TYPE_MAT);
}

static inline float
get_number_value(lua_State *L, struct lastack * LS, int index) {
	const float *v = math3d_get_value(L, LS, index, LINEAR_TYPE_NUM);
	return v[0];
}

#define tov3(v4)	((const glm::vec3*)(&(v4.x)))

template<typename T>
inline bool
is_zero(const T& a, const T& e = T(glm::epsilon<float>())) {
	return glm::all(glm::equal(a, glm::zero<T>(), e));
}

inline bool
is_zero(const float& a, float e = glm::epsilon<float>()) {
	return glm::equal(a, glm::zero<float>(), e);
}

template<typename T>
inline bool
is_equal(const T& a, const T& b, const T& e = T(glm::epsilon<float>())) {
	return is_zero(a - b, e);
}

#endif //math3d_util_h