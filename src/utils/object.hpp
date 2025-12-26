#pragma once
#include "../vma.hpp"
#include "../texture.hpp"
#include "transform.hpp"
#include "../shader_program.hpp"


namespace sve {
	struct Mesh {
		vma::Buffer vertex_buffer;
		uint32_t index_count;
	};

	struct Material {
		ShaderProgram* shader;
		Texture* texture;
	};

	struct Object {
		Mesh mesh;
		Material material;
		Transform transform;

		uint32_t texture_index{};
		uint32_t instance_count = 1;
	};
}