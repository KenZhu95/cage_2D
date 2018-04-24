R"zzz(#version 330 core
out vec4 fragment_color;
in vec2 tex_coord;
uniform float bar_frame_shift;
uniform int number_preview;
void main() {
	if (number_preview <= 3) {
		fragment_color = vec4(0.3, 0.3, 0.3, 1.0);
	} else {
		float total = number_preview * 240;
		float top = (total - bar_frame_shift) / total;
		float bottom = (total - bar_frame_shift - 720) / total;
		if (tex_coord.y >= bottom && tex_coord.y <= top) {
			fragment_color = vec4(0.3, 0.3, 0.3, 1.0);
		} else {
			fragment_color = vec4(0.8, 0.8, 0.8, 1.0);
		}
	}
}
)zzz"