uniform mat4 u_viewprojection;
uniform mat4 u_model;

layout(location = 1) $in vec4 a_pos;
layout(location = 2) $in vec4 a_color;

#ifdef INSTANCED
// instanced rendering
layout(location = 3) $in vec3 a_offset;
#endif // INSTANCED

$out vec4 v_color;

void main()
{
	v_color = a_color;
#ifdef INSTANCED
	gl_Position = u_viewprojection * (vec4(a_offset, 0.0) + u_model * a_pos);
#else // INSTANCED
	gl_Position = u_viewprojection * u_model * a_pos;
#endif // INSTANCED
}
